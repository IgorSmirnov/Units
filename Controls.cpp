//! Реализация базовых визуальных компонентов
/** \file
 	Файл содержит реализацию классов визуальных компонентов, не имеющих
	своего окна и определённой специализации.
*/
#include "Controls.h"
#include "WinControls.h"
#include <Limits.h>
#include <tchar.h>

#define pint(r) ((int *)&r)

namespace StdGUI {
/*********************************** TControl *****************************************/

HBRUSH TControl::FDefaultBackBrush = (HBRUSH) (COLOR_BTNFACE + 1);
DWORD TControl::FDefaultTextColor = GetSysColor(COLOR_WINDOWTEXT);
HPEN TControl::FDefaultPen = 0;

TControl::TControl(void)
{
	memset(&FConstraints, 0, sizeof(FConstraints));
	FWinParent = 0;
	FParent = 0;
	FBackBrush = FDefaultBackBrush;
	FTextColor = FDefaultTextColor;
	FPen = FDefaultPen;
	FFont = 0;
	FPrev = 0;
	FNext = 0;
	FLayout = 0;
}

void TControl::_TControl(TParentControl * Parent, RECT * Rect, int Layout)
{
	if(Layout & LT_SIZEIN) 
	{
		Layout &= ~LT_SIZEIN;
		FRect.left = 0;
		FRect.top = 0;
		FRect.right = (long)Rect;
		FRect.bottom = (long)Rect;
	}
	else
	if(Rect) FRect = *Rect;
	else 
	{
		FRect.left = 0;
		FRect.top = 0;
		FRect.right = 100;
		FRect.bottom = 100;
	}
	FLayout = Layout;
	if(FParent = Parent)
	{
		FWinParent = Parent->GetWinParent();
		FBackBrush = Parent->FBackBrush;
		FTextColor = Parent->FTextColor;
		FPen = Parent->FPen;
		set_Font(Parent->FFont);
		Parent->AddChild(this);
	}
}

//! Деструктор визуального компонента.
//! Уведомляет родителя об уничтожении. Извлекает компонент из родительского связного списка.
TControl::~TControl(void)
{
	if(FParent) FParent->RemoveChild(this, false);
}

void TControl::MouseEvent(int x, int y, int mk, UINT uMsg)  // Уведомление о движении
{
	OnMouseEvent(x, y, mk, uMsg);
}

void TControl::ChildSetSize(RECT * Rect)
{
	FParent->ChildSetSize(this, Rect);
}

int TControl::GetWidth(void)
{
	return FRect.right - FRect.left;
}

int TControl::GetHeight(void)
{
	return FRect.bottom - FRect.top;
}

void TControl::MakeFocused(void) 
{
	if(FWinParent) FWinParent->SetFocusedChild(this);
};

bool TControl::ParentSetRect(RECT * Rect) // Уведомление от родительских компонентов
{
	FOldRect = FRect;
	FRect = *Rect;
	int w = FRect.right - FRect.left;
	int h = FRect.bottom - FRect.top;
	bool Result = true;
	/*RECT Cn;
	get_Constraints(&Cn);
	if(w < Cn.left)	{Result = false; FRect.right += Cn.left - w;}
	if(h < Cn.top)	{Result = false; FRect.bottom += Cn.top - h;}
	if(Cn.right && (w > Cn.right))	{Result = false; FRect.right += Cn.right - w;}
	if(Cn.bottom && (h > Cn.bottom))	{Result = false; FRect.bottom += Cn.bottom - h;}*/
	OnSetRect(&FRect);
	Invalidate(&FOldRect);
	return Result;
}

HWND TControl::GetWindowHandle(void)
{
	if(FWinParent) return FWinParent->GetWindowHandle();
	return 0;
}

#define MAX(a, b) a > b ? a : b
#define MIN(a, b) a < b ? a : b

bool TControl::MoveInvalidate(RECT * OldRect, int Flags)
{
	int dx, dy;
	if(!Flags) 
	{
		dx = FRect.left - OldRect->left;
		dy = FRect.top - OldRect->top;
		if(dx != FRect.right - OldRect->right) return false;
		if(dy != FRect.bottom - OldRect->bottom) return false;
	}

	if(!(dx || dy)) return true;
	HWND Handle = FWinParent->GetWindowHandle();
	HRGN RectRgn = CreateRectRgnIndirect(&FRect);
	HRGN OldRgn = CreateRectRgnIndirect(OldRect);
	CombineRgn(OldRgn, OldRgn, RectRgn, RGN_DIFF);
	DeleteObject(RectRgn);
	ScrollWindowEx(Handle, dx, dy, OldRect, 0, OldRgn, 0, SW_INVALIDATE);
	DeleteObject(OldRgn);
	return true;
}

void TControl::Invalidate(RECT * OldRect)
{
	if(!FWinParent) return;
	HWND Handle = FWinParent->GetWindowHandle();
//	InvalidateRect(Handle, OldRect, false);
//	InvalidateRect(Handle, &FRect, false);
	HRGN RectRgn = CreateRectRgnIndirect(&FRect);
	HRGN OldRgn = CreateRectRgnIndirect(OldRect);
	CombineRgn(RectRgn, RectRgn, OldRgn, RGN_OR);
	DeleteObject(OldRgn);

	RECT Valid;
	Valid.left = MAX(OldRect->left, FRect.left);
	Valid.top = MAX(OldRect->top, FRect.top);

	Valid.right = MIN(OldRect->right, FRect.right);
	Valid.bottom = MIN(OldRect->bottom, FRect.bottom);

	HRGN ValidRgn = CreateRectRgnIndirect(&Valid);
	CombineRgn(RectRgn, RectRgn, ValidRgn, RGN_DIFF);
	DeleteObject(ValidRgn);

	InvalidateRgn(Handle, RectRgn, false);
	DeleteObject(RectRgn);

	//ValidateRect(Handle, &Valid);
}

bool TControl::SetSize(int Width, int Height)
{
	RECT Rect = FRect;
	Rect.right = FRect.left + Width;
	Rect.bottom = FRect.top + Height;
	if(FParent) 
	{
		FParent->ChildSetSize(this, &Rect);
		return true;
	} else
	return ParentSetRect(&Rect);
}

/*********************************** TParentControl *****************************************/

TParentControl::TParentControl(void)
{
	memset(&FChildConstraints, 0, sizeof(FChildConstraints));
	memset(&FPartConstraints, 0, sizeof(FPartConstraints));
	memset(&FBorders, 0, sizeof(FBorders));
	FMouseOwner = 0;
	FFirstChild = 0;
	FLastChild = 0;
	FClientPresent = false;
}

/* Принципы уничтожения визуальных компонентов 

     Алгоритм работы деструктора визуального компонента:
	1. Попытаться уничтожить(delete) всех детей (вызывающий код вправе запретить это 
	путём очистки FFirstChild и FLastChild), с запретом уведомления.
	2. Если есть - уничтожить окно и т.п.
	3. Уведомить родителя об уничтожении: RemoveChild(this, false)
	(вызывающий код вправе запретить это путём очистки FParent)
	4?. Уведомить прочих.

     Алгоритм метода, уведомляющего об уничтожении дочерних компонентов(RemoveChild):
	1. Извлечь уничтожаемый компонент из связного списка.
	2. Запустить деструктор дочернего компонента, если Destroy (с запретом уведомления).
	3. Обновить себя?

     Алгоритм обработки сообщений WM_DESTROY и подобных им:
	1. Избавиться от окна, принявшего сообщение.
	2. Уведомить родителя об необходимости уничтожения: RemoveChild(this, true)
	(вызывающий код вправе запретить это путём очистки FParent)

	 Алгоритм работы деструктора формы со статически включёнными компонентами:
	1. Выполнить RemoveChild(&Child, false) для всех включённых детей и запустить их деструкторы.
	3. Запустить унаследованный деструктор.
  */
//! Деструктор родительского компонента.
TParentControl::~TParentControl(void)
{
	for(TControl * x = FFirstChild; x; )
	{
		TControl * y = x;
		x = x->FNext;
		y->FParent = 0;
		delete(y);
	}
}

void TParentControl::RemoveChild(TControl * Child, bool Destroy)
{
	TControl * Next;
	// Распутываем
	if(Next = Child->FNext) Next->FPrev = Child->FPrev;
	else FLastChild = Child->FPrev;
	if(TControl * Prev = Child->FPrev) Prev->FNext = Child->FNext;
	else FFirstChild = Child->FNext;
	Child->FParent = 0;
	// Уничтожаем
	RECT CleanRect = Child->FRect;
	if(Destroy) delete Child;
	// Обновляем
	if(HWND hWindow = GetWindowHandle())
	{
		InvalidateRect(hWindow, &CleanRect, false);
		if(Next && (Next->FLayout & LT_ALIGN)) ReAlign(Next);
	}
}

#define InRect(x, y, Rect) (x > Rect.left)&&(x < Rect.right)&&(y > Rect.top)&&(y < Rect.bottom)

void TParentControl::MouseEvent(int x, int y, int mk, UINT uMsg)
{
	if(FMouseOwner)
	{
		FMouseOwner->MouseEvent(x, y, mk, uMsg);
		if((uMsg == WM_LBUTTONUP) | (uMsg == WM_MBUTTONUP) | (uMsg == WM_RBUTTONUP))
			FMouseOwner = 0;
		return;		
	}

	for(TControl * Child = FFirstChild; Child; Child = Child->FNext)
	if(InRect(x, y, Child->FRect))
	{
		Child->MouseEvent(x, y, mk, uMsg);
		if((uMsg == WM_LBUTTONDOWN) | (uMsg == WM_MBUTTONDOWN) | (uMsg == WM_RBUTTONDOWN))
			FMouseOwner = Child;
		return;
	}
	OnMouseEvent(x, y, mk, uMsg);
}


void TParentControl::_TParentControl(TParentControl * Parent, RECT * Rect, int Layout)
{
	_TControl(Parent, Rect, Layout);
	GetClientRct(&FClient);
	FFreeClient = FClient;
	FClientPresent = (FFreeClient.right - FFreeClient.left) && (FFreeClient.bottom - FFreeClient.top);
}

/*void TParentControl::EraseBorder(PAINTSTRUCT * PaintStruct)
{
	RECT Rect = FRect;
	int Left = Rect.left + FBorderWidth + FLeftWidth;
	Rect.right = Left;
	FillRect(PaintStruct->hdc, &Rect, FBackBrush);
	Rect.right = FRect.right;
	int Right = Rect.right - FBorderWidth;
	Rect.left = Right;
	FillRect(PaintStruct->hdc, &Rect, FBackBrush);
	Rect.right = Right;
	Rect.left = Left;
	Rect.top = Rect.bottom - FBorderWidth;
	FillRect(PaintStruct->hdc, &Rect, FBackBrush);
	Rect.top = FRect.top;
	Rect.bottom = Rect.top + FBorderWidth + FTopWidth;
	FillRect(PaintStruct->hdc, &Rect, FBackBrush);
}*/

void TParentControl::Invalidate(RECT * OldRect)
{
	if(!FWinParent) return;
	HWND Handle = FWinParent->GetWindowHandle();
	//InvalidateRect(Handle, OldRect, true);
	//InvalidateRect(Handle, &FRect, true);
	HRGN RectRgn = CreateRectRgnIndirect(&FRect);
	HRGN OldRgn = CreateRectRgnIndirect(OldRect);
	CombineRgn(RectRgn, RectRgn, OldRgn, RGN_OR);
	DeleteObject(OldRgn);

	RECT Valid;
	Valid.left = MAX(OldRect->left, FRect.left);
	if(OldRect->left != FRect.left) Valid.left += FBorders.left;
	Valid.top = MAX(OldRect->top, FRect.top);
	if(OldRect->top != FRect.top) Valid.top += FBorders.top;

	Valid.right = MIN(OldRect->right, FRect.right);
	if(OldRect->right != FRect.right) Valid.right -= FBorders.right;
	Valid.bottom = MIN(OldRect->bottom, FRect.bottom);
	if(OldRect->bottom != FRect.bottom) Valid.bottom -= FBorders.bottom;

	HRGN ValidRgn = CreateRectRgnIndirect(&Valid);
	CombineRgn(RectRgn, RectRgn, ValidRgn, RGN_DIFF);
	DeleteObject(ValidRgn);

	InvalidateRgn(Handle, RectRgn, false);
	DeleteObject(RectRgn);

//	ValidateRect(Handle, &Valid);
}

void TParentControl::GetClientRct(RECT * Rect)
{
	Rect->left = FRect.left + FBorders.left;
	Rect->top = FRect.top + FBorders.top;
	Rect->right = FRect.right - FBorders.right;
	Rect->bottom = FRect.bottom - FBorders.bottom;
}

inline int GetSegment(RECT * FreeClient, RECT * Rect, int Off)
{
	Off &= 1;
	int w = ((int *)Rect)[Off + 2] - ((int *)Rect)[Off];
	int W = ((int *)FreeClient)[Off + 2] - ((int *)FreeClient)[Off];
	return W ? (w << 16) / W : 32768;
}

void TParentControl::AddChild(TControl * Child)
{
	Child->FPrev = 0;
	Child->FNext = 0;
	if(FLastChild)
	{
		FLastChild->FNext = Child;
		Child->FPrev = FLastChild;
	}
	FLastChild = Child;
	if(!FFirstChild) FFirstChild = Child;


	int Layout = Child->FLayout;
	if((Layout & (LT_ALIGN | LT_SCALE)) == (LT_ALIGN | LT_SCALE)) // Вычисление доли
		Child->FSegment = GetSegment(&FFreeClient, &Child->FRect, Layout);
	if(Layout & LT_ALIGN)
	{
		/*if((Layout & LT_MODE) == LT_ALIGN)
		{// Расчёт полных ограничений:
		 //	По направлению прижатия: Part += Child; FChild = min/max(Part)
		 //	Перпендикулярно: Part

			int c = Layout & 1;
			int p = c ^ 1;
			RECT cr;
			Child->get_Constraints(&cr);
			if(pint(FChildConstraints)[p] < pint(cr)[p])
				pint(FChildConstraints)[p] = pint(cr)[p];
			p += 2;
			if(pint(cr)[p] && pint(FChildConstraints)[p] > pint(cr)[p])
				pint(FChildConstraints)[p] = pint(cr)[p];

			pint(FPartConstraints)[c] += pint(cr)[c];

			if(pint(FChildConstraints)[c] < pint(cr)[c])
				pint(FChildConstraints)[c] = pint(cr)[c];
			c += 2;
			if(pint(cr)[c] && pint(FChildConstraints)[c] > pint(cr)[c])
				pint(FChildConstraints)[c] = pint(cr)[c];

		}*/
		Align(Child, &Child->FRect, true);
		//Align(Child, Layout, true, 0);
	}
	else
	{
		RECT r = Child->FRect;
		r.left += FClient.left; // Относительные координаты
		r.right += FClient.left;
		r.top += FClient.top;
		r.bottom += FClient.top;
		Child->ParentSetRect(&r);
	}
}

void TParentControl::get_Constraints(RECT * c)
{
	c->top = max(FChildConstraints.top, FConstraints.top);
	c->left = max(FChildConstraints.left, FConstraints.left);
	c->right = FConstraints.right;
	if(FChildConstraints.right && FChildConstraints.right < c->right) c->right = FChildConstraints.right;
	c->bottom = FConstraints.bottom;
	if(FChildConstraints.bottom && FChildConstraints.bottom < c->bottom) c->bottom = FChildConstraints.bottom;
};



void TParentControl::ChildSetSize(TControl * Child, RECT * Rect) // Уведомление от дочерних компонентов
{
	TControl * LastChild = 0;
	int Layout = Child->FLayout & LT_LAYOUT;
	for(TControl * x = Child->FPrev; x; x = x->FPrev) // Ищем ближайшего соседа перед данным
	if((x->FLayout & LT_LAYOUT) == Layout)
	{
		LastChild = x;
		break;
	}

	// Рассчитать суммарные ограничения последующих компонентов
	RECT Sum = {0};
	for(TControl * x = FLastChild; x; x = x->FPrev)
	{
		int l = x->FLayout;
		if(!(l & LT_ALIGN)) continue;
		RECT cc;
		x->get_Constraints(&cc);
		if(l & LT_ANCHORS) // LT_ALCLIENT
		{
			if(Sum.left < cc.left) Sum.left = cc.left;
			if(Sum.top < cc.top) Sum.top = cc.top;
			if(cc.right && (!Sum.right || cc.right < Sum.right)) Sum.right = cc.right;
			if(cc.bottom && (!Sum.bottom || cc.bottom < Sum.bottom)) Sum.bottom = cc.bottom;
			continue;
		}
		int p = 1 & ~l;
		if(pint(Sum)[p] < pint(cc)[p]) pint(Sum)[p] = pint(cc)[p];
		p |= 2;
		if(pint(cc)[p] && (!pint(Sum)[p] || pint(cc)[p] < pint(Sum)[p])) pint(Sum)[p] = pint(cc)[p];
		p ^= 3;
		pint(Sum)[p] += pint(cc)[p];
		p |= 2;
		if(pint(cc)[p]) pint(Sum)[p] += pint(cc)[p];
		else pint(Sum)[p] = 0;
		if(x == Child) break;
	}

	FFreeClient = FClient;

	FClientPresent = true;
	bool b = false;
	for(TControl * x = FFirstChild; x; x = x->FNext)
	{
		int Layout = x->FLayout;
		if(x == LastChild) // Если нашли ближайшего соседа перед данным, то изменяем его размер.
		{
			b = true;
			int Cur = Child->FLayout & LT_ALOFFSET;

			RECT CRect = x->FRect; // Наложение ограничений предыдущего соседа
			pint(CRect)[Cur ^ 2] = pint(*Rect)[Cur];
			RECT Cnt;
			x->get_Constraints(&Cnt);
			int cn = pint(Cnt)[Cur & 1];
			if(pint(CRect)[Cur | 2] - pint(CRect)[Cur & 1] < cn)
				pint(CRect)[Cur ^ 2] = pint(CRect)[Cur] + (Cur & 2 ? -cn : cn);
			cn = pint(Cnt)[Cur | 2];
			if(cn && (pint(CRect)[Cur | 2] - pint(CRect)[Cur & 1] > cn))
				pint(CRect)[Cur ^ 2] = pint(CRect)[Cur] + (Cur & 2 ? -cn : cn);

			// Наложение ограничений последующих соседей
			int fc = pint(FFreeClient)[Cur | 2] - pint(FFreeClient)[Cur & 1];
			int cr = pint(CRect)[Cur | 2] - pint(CRect)[Cur & 1];
			fc -= cr; // Размер, остающийся последующим соседям
			if(pint(Sum)[Cur & 1] > fc) 
			{
				fc -= pint(Sum)[Cur & 1];
				pint(CRect)[Cur ^ 2] += Cur & 2 ? -fc : fc;
			}

			x->FSegment = GetSegment(&FFreeClient, &CRect, x->FLayout);
			Align(x, Layout, true, &CRect);
			continue;
		}
		if(Layout & LT_ALIGN)
			Align(x, Layout, b, 0);
	}
}

void TParentControl::ReAlign(TControl * First)
{
	FFreeClient = FClient;
	FClientPresent = true;
	bool b = false;
	for(TControl * x = FFirstChild; x; x = x->FNext)
	{
		if(x == First) b = true;
		if(x->FLayout & LT_ALIGN) Align(x, x->FLayout, b, 0);
	}
}

void TParentControl::RestoreFC(int Layout, RECT * Rect)
{
	if((Layout & LT_MODE) == LT_ALCLIENT)
	{
		FClientPresent = true;
		FFreeClient = *Rect;
		return;
	}
	if((Layout & LT_MODE) == LT_ALIGN)
	{
		int o = Layout & LT_ALOFFSET;
		pint(FFreeClient)[o] += ((int *)Rect)[o] - ((int *)Rect)[o ^ 2];
	}
}

//! Рассчитывает положение дочерних компонентов, начиная от Child. 
//! Результат пишется в Child->FOldRect. Если Move, после окончания вызывается ParentSetRect
bool TParentControl::Align(TControl * Child, RECT * Old, bool Move)
{
	for(TControl * x = FFirstChild; x; x = x->FNext)
	{
		x->FLayout &= ~LT_BSCALE;
		x->FOldRect = x->FRect;
	}
	TControl * First;
	for(First = Child; Child; Child = Child->FNext)
	{
		// Вписываем Child в соответствии с FLayout:
		_ASSERTE(FClientPresent);
		RECT c;
		Child->get_Constraints(&c);
		int Layout = Child->FLayout;
		switch(Layout & LT_MODE)
		{
		case LT_ALIGN:
			{
				int d, Cur = Layout & LT_ALOFFSET, Opp = Cur ^ 2;
				int * cr = pint(Child->FOldRect);

				if(LT_SCALE & Layout && !(LT_BSCALE & Layout))
				{
					d = (((pint(FFreeClient)[Opp] - pint(FFreeClient)[Cur]) * Child->FSegment) + 32768) >> 16;
					int cc = Cur & 1;
					if(pint(c)[cc] > d) d = pint(c)[cc];
					cc = pint(c)[cc | 2];
					if(cc && cc < d) d = cc;
				}
				else d = cr[Opp] - cr[Cur];

				Child->FOldRect = FFreeClient;
				pint(Child->FOldRect)[Opp] = pint(Child->FOldRect)[Cur] + d;

				pint(FFreeClient)[Cur] += d;
				break;
			}
		case LT_ALCLIENT:
			Child->FOldRect = FFreeClient;
			FClientPresent = false;
			break;
		case LT_ANCHORS:
			{
				Child->FOldRect = Child->FRect;
				int * cr = pint(Child->FOldRect);
				for(int y = 0; y < 4; y++)
				{
					int d = (((~Layout) & 1) << 1) ^ y; 
					cr[y] += pint(FRect)[d] - ((int *)Old)[d];
					Layout >>= 1;
				}
				break;
			}
		}

		// Проверяем ограничения:
		// Если ограничение нарушено, необходимо скорректировать размер предыдущего соседа,
		// который прижат в ту сторону, которая перпендикулярна стороне нарушившей ограничение
		int dd[2] = {0}; // Коррекция размеров предыдущего соседа. Положительное значение - увеличить.
		RECT &r = Child->FOldRect;
		int w = r.right - r.left, h = r.bottom - r.top;
		if(c.left > w) dd[0] = w - c.left;
		if(c.top > h) dd[1] = h - c.top;
		if(c.right && w < c.right) dd[0] = c.right - w;
		if(c.bottom && h < c.bottom) dd[1] = c.bottom - h;
		if(dd[0] | dd[1])
		{
			RestoreFC(Child->Layout, &r);
			r.right -= dd[0];
			r.bottom -= dd[1];
			while(Child)
			{
				if(Child == First) First = First->FPrev;
				Child = Child->FPrev;
				_ASSERTE(Child);
				int l = Child->Layout;
				RestoreFC(l, &Child->FOldRect);

				if(l & LT_CWIDTH) continue;
				l &= 0x11;
				if(dd[0] && l == 0x10) 
				{ 
					Child->FLayout |= LT_BSCALE; 
					Child->FOldRect.right += dd[0]; 
					dd[0] = 0;
				}
				if(dd[1] && l == 0x11) 
				{ 
					Child->FLayout |= LT_BSCALE; 
					Child->FOldRect.bottom += dd[1];  
					dd[1] = 0;
				}
				if(!(dd[0] | dd[1]))
				{
					Child = Child->FPrev;
					_ASSERTE(Child);
					break;
				}
			}
			_ASSERTE(!(dd[0] | dd[1]));
		}
	}
	if(Move) for(Child = First; Child; Child = Child->FNext)
		if(memcmp(&Child->FRect, &Child->FOldRect, sizeof(RECT)))
		{
			RECT r = Child->FOldRect;
			Child->ParentSetRect(&r);
		}
	return true;
}

//! Прижимает Child к краю, отсекает от FFreeClient кусок, занятый им

//! Если MoveChild = false, то меняет только FFreeClient
void TParentControl::Align(TControl * Child, int Layout, bool MoveChilds, RECT * ChildRect)
{
	if((Layout & LT_MODE) == LT_ALIGN)
	{
		int d;
		int Cur = Layout & LT_ALOFFSET;
		int Opp = Cur ^ 2;
		int * cr = ChildRect ? (int *) ChildRect : pint(Child->FRect);
		
		if(LT_SCALE & Layout)
			d = (((pint(FFreeClient)[Opp] - pint(FFreeClient)[Cur]) * Child->FSegment) + 32768) >> 16;
		else 
			d = cr[Opp] - cr[Cur];
		if(MoveChilds)
		{
			RECT ChildRect = FFreeClient;
			pint(ChildRect)[Opp] = pint(ChildRect)[Cur] + d;
			if(memcmp(&ChildRect, &Child->FRect, sizeof(RECT)))
				Child->ParentSetRect(&ChildRect);
		}
		pint(FFreeClient)[Cur] += d;
	}
	else
	{
		if(MoveChilds)
			Child->ParentSetRect(&FFreeClient);
		FClientPresent = false;
	}
}

bool TParentControl::ParentSetRect(RECT * Rect) // Уведомление от родительских компонентов
{
	RECT Old = FRect;
	bool Result = TControl::ParentSetRect(Rect);

	GetClientRct(&FFreeClient);
	FClientPresent = true;
	
	Align(FFirstChild, &Old, true);
	GetClientRct(&FClient);
	return Result;
}

void TParentControl::PaintChilds(PAINTSTRUCT * PaintStruct) // Уведомление об отрисовке детям
{
	for(TControl * x = FFirstChild; x; x = x->FNext)
	{
		x->Paint(PaintStruct);
		RECT & Rect = x->FRect;
		ExcludeClipRect(PaintStruct->hdc, Rect.left, Rect.top, Rect.right, Rect.bottom);
	}
}

void TParentControl::OnPaint(PAINTSTRUCT * PaintStruct)
{
	if(FClientPresent)
		FillRect(PaintStruct->hdc, &FFreeClient, FBackBrush);
}

/*********************************** TSimplePanel ***********************************************/

TSimplePanel::TSimplePanel(TParentControl * Parent, RECT * Rect, int Layout)
{
	FBorders.left = 2;
	FBorders.right = 2;
	FBorders.top = 2;
	FBorders.bottom = 2;
	FEdgeType = EDGE_RAISED;
	_TParentControl(Parent, Rect, Layout);
}

void TSimplePanel::OnPaint(PAINTSTRUCT * PaintStruct)
{
	//DrawFocusRect(PaintStruct->hdc, &FRect);
	//DrawFrameControl(PaintStruct->hdc, &FRect, DFC_BUTTON, DFCS_BUTTONPUSH);
	FillRect(PaintStruct->hdc, &FRect, FBackBrush);
	DrawEdge(PaintStruct->hdc, &FRect, FEdgeType, BF_RECT);
}

/*********************************** TImage ***************************************************/

TImage::TImage(TParentControl * Parent, RECT * Rect, int Layout)
{
	FBitMap = 0;
	FBMPDC = 0;
	FBitCount = 32;
	AutoResize = 0;
	_TControl(Parent, Rect, Layout);
}

void TImage::OnPaint(PAINTSTRUCT * PaintStruct)
{
	if(!FBitMap)
	{
		FillRect(PaintStruct->hdc, &FRect, FBackBrush);
		return;
	}
	if(!FBMPDC)
	{
		FBMPDC = CreateCompatibleDC(0);
		FDefBitMap = SelectObject(FBMPDC, FBitMap);
	}
	BitBlt(PaintStruct->hdc, FRect.left, FRect.top, FRect.right - FRect.left, FRect.bottom - FRect.top, FBMPDC, 0, 0, SRCCOPY);
}

void TImage::Update(void)
{
	HWND Win = GetWindowHandle();
	if(HDC dc = GetDC(Win))
	{
		BitBlt(dc, FRect.left, FRect.top, FRect.right - FRect.left, FRect.bottom - FRect.top, FBMPDC, 0, 0, SRCCOPY);
		ReleaseDC(Win, dc);
	}
}

bool TImage::LoadFromBMP(TCHAR * FileName)
{
	void * f = CreateFile(FileName, FILE_READ_DATA, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(f == INVALID_HANDLE_VALUE) return false;
	DWORD l = GetFileSize(f, 0);
	DWORD t;
	struct _HEAD
	{
		BITMAPFILEHEADER Header;
		BITMAPINFO Info;
		RGBQUAD	bmiColors[256];
	} Head;
	if(!ReadFile(f, &Head.Header, sizeof(Head.Header), &t, 0) || (t != sizeof(Head.Header)) || (Head.Header.bfType != 'MB'))
		{ CloseHandle(f); return false; }
	if(!ReadFile(f, &Head.Info, sizeof(Head.Info.bmiHeader), &t, 0) || (t != sizeof(Head.Info.bmiHeader)) || (Head.Info.bmiHeader.biCompression != BI_RGB))
	{ CloseHandle(f); return false; }
	FBitCount = (char) Head.Info.bmiHeader.biBitCount;
	if(int Offset = Head.Info.bmiHeader.biSize - sizeof(Head.Info.bmiHeader))
		SetFilePointer(f, Offset, 0, FILE_CURRENT);
	if(FBitCount <= 8) if(!ReadFile(f, &Head.Info.bmiColors, sizeof(RGBQUAD) << FBitCount, &t, 0))
		{
			CloseHandle(f);
			return false;
		}

	SetFilePointer(f, Head.Header.bfOffBits, 0, FILE_BEGIN);

	DeleteBitMap();

	HBITMAP BitMap = CreateDIBSection(0, &Head.Info, /*bc <= 8 ? DIB_PAL_COLORS :*/ DIB_RGB_COLORS, &Bits, 0, 0);
	HDC BMPDC = CreateCompatibleDC(0);
	HGDIOBJ DefBitMap = SelectObject(BMPDC, BitMap);

	ReadFile(f, Bits, l - Head.Header.bfOffBits, &t, 0);

	FBMPDC = BMPDC;
	FBitMap = BitMap;
	FDefBitMap = DefBitMap;
	FWidth = Head.Info.bmiHeader.biWidth;
	FHeight = Head.Info.bmiHeader.biHeight;
	CloseHandle(f);

	Invalidate(&FRect);
	return true;
}

void * TImage::GetPixel(int x, int y)
{
	_ASSERTE(x >= 0 && y >= 0 && x < FWidth && y < FHeight);
	return (char *)Bits + (x + (FHeight - y - 1) * FWidth) * (FBitCount >> 3);
}

void TImage::DeleteBitMap(void)
{
	if(FBMPDC)
	{
		SelectObject(FBMPDC, FDefBitMap);
		DeleteDC(FBMPDC);
		FBMPDC = 0;
	}
	if(FBitMap)
	{
		DeleteObject(FBitMap);
		FBitMap = 0;
	}
}

void * TImage::SetBitMapSize(int Width, int Height, int BitCount, int Mode)
{
	if(!Mode) DeleteBitMap();
	if(!Width) Width = GetWidth();
	if(!Height) Height = GetHeight();
	if(BitCount) FBitCount = (char) BitCount;
	BITMAPINFO bmpInfo;
	bmpInfo.bmiHeader.biBitCount = FBitCount;
	bmpInfo.bmiHeader.biClrUsed = 0;
	bmpInfo.bmiHeader.biHeight = Height;
	bmpInfo.bmiHeader.biWidth = Width;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biClrImportant = 0;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biSizeImage = 0;
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biXPelsPerMeter = 0;
	bmpInfo.bmiHeader.biYPelsPerMeter = 0;
	HBITMAP BitMap = CreateDIBSection(0, &bmpInfo, DIB_RGB_COLORS, &Bits, 0, 0);
	HDC BMPDC = CreateCompatibleDC(0);
	HGDIOBJ DefBitMap = SelectObject(BMPDC, BitMap);
	if(Mode)
	{
		if(FBMPDC)
		{
			if(Mode == 1) BitBlt(BMPDC, 0, 0, Width, Height, FBMPDC, 0, 0, SRCCOPY);
			if(Mode == 2) StretchBlt(BMPDC, 0, 0, Width, Height, FBMPDC, 0, 0, FWidth, FHeight, SRCCOPY);
		}
		DeleteBitMap();
	}
	FBMPDC = BMPDC;
	FBitMap = BitMap;
	FDefBitMap = DefBitMap;
	FWidth = Width;
	FHeight = Height;
	return Bits;
}

bool TImage::ParentSetRect(RECT * Rect) // Уведомление от родительских компонентов
{
	bool Result = TControl::ParentSetRect(Rect);
	if(AutoResize) SetBitMapSize(GetWidth(), GetHeight(), 0, AutoResize - 1);
	return Result;
}

void TImage::Invalidate(RECT * OldRect)
{
	InvalidateRect(GetWindowHandle(), &FRect, true);

}

/*********************************** TGroupBox ***********************************************/

int TGroupBox::FDefaultStyle = EDGE_ETCHED;

TGroupBox::TGroupBox(TParentControl * Parent, RECT * Rect, int Layout, TCHAR * Caption)
{
	FFont = 0;
	FBorders.left = FBorders.right = FBorders.bottom = 5;
	FBorders.top = 20;
	FStyle = FDefaultStyle;
	FCaption = Caption;
	_TParentControl(Parent, Rect, Layout);
}

void TGroupBox::set_Font(HFONT Font)
{
	FFont = Font;
	if(HWND hWindow = GetWindowHandle())
	{
		if(HDC dc = GetDC(hWindow))
		{
			HGDIOBJ OldFont;
			if(Font) OldFont = SelectObject(dc, Font);
			SIZE Size;
			if(GetTextExtentPoint32(dc, FCaption, _tcslen(FCaption), &Size))
				FBorders.top = Size.cy + 2;
			if(Font) SelectObject(dc, OldFont);
			ReleaseDC(hWindow, dc);
		}		
	}
}

void TGroupBox::OnPaint(PAINTSTRUCT * PaintStruct)
{
#define GBOff 2
	HDC dc = PaintStruct->hdc;
	RECT Edge = {FRect.left + GBOff, FRect.top + (FBorders.top >> 1) + GBOff, FRect.right - GBOff, FRect.bottom - GBOff};
	RECT Text = {Edge.left + 10, FRect.top};
	/*t.top += ;
	t.left += GBOff;
	t.bottom -= GBOff;
	t.right -= GBOff;*/

	FillRect(dc, &FRect, FBackBrush);
/*	t.left += 10;
	t.right -= 10;
	t.top = FRect.top;// + 1;
	t.bottom = FRect.top + 20;*/
	
	SetBkMode(dc, TRANSPARENT);
    //SetBkColor(PaintStruct->hdc, GetSysColor(COLOR_BTNFACE)); 
	HGDIOBJ OldFont = 0;
	if(FFont) OldFont = SelectObject(dc, FFont);
	SetTextColor(dc, FTextColor);
	int Len = _tcslen(FCaption);
	SIZE Size;
	if(GetTextExtentPoint32(dc, FCaption, Len, &Size))
	{
		Text.right = Text.left + Size.cx;
		Text.bottom = Text.top + Size.cy;
		DrawText(dc, FCaption, Len, &Text, DT_LEFT | DT_NOCLIP);
		ExcludeClipRect(dc, Text.left, Text.top, Text.right, Text.bottom);
	}
	if(FFont) SelectObject(dc, OldFont);
	if(FStyle)
		DrawEdge(dc, &Edge, FStyle, BF_RECT);
	else
	{
		HGDIOBJ OldPen;
		if(FPen) OldPen = SelectObject(dc, FPen);
		MoveToEx(dc, Edge.left, Edge.top, 0);
		LineTo(dc, Edge.right, Edge.top);
		LineTo(dc, Edge.right, Edge.bottom);
		LineTo(dc, Edge.left, Edge.bottom);
		LineTo(dc, Edge.left, Edge.top);
		if(FPen) SelectObject(dc, OldPen);
	}
}

void TGroupBox::Invalidate(RECT * OldRect)
{
	TParentControl::Invalidate(OldRect);
	int x = FRect.left - OldRect->left;
	int y = FRect.top - OldRect->top;
	RECT Rect = *OldRect;
	if(Rect.right > FRect.right) Rect.right = FRect.right;
	Rect.right -= FBorders.right;
	if(x > 0) Rect.right -= x;
	Rect.bottom = Rect.top + FBorders.top;
	ScrollWindowEx(GetWindowHandle(), x, y, &Rect, 0, 0, 0, 0);
	if(!x) return;
	if(x > 0)
	{
		Rect.left = FRect.left + FBorders.left;
		Rect.right = Rect.left + x;
	}
	else
	{
		Rect.right = FRect.right - FBorders.right;
		Rect.left = Rect.right + x;
	}
	InvalidateRect(GetWindowHandle(), &Rect, false);
}

/*********************************** TLabel *************************************************/

void TLabel::OnPaint(PAINTSTRUCT * PaintStruct) // Процедура отрисовки компонента
{
	if(!FCaption) return;
	//SetBkColor(PaintStruct->hdc, FBackBrush);
	FillRect(PaintStruct->hdc, &FRect, FBackBrush);
	SetBkMode(PaintStruct->hdc, TRANSPARENT);
	DrawText(PaintStruct->hdc, FCaption, _tcslen(FCaption), &FRect, DT_LEFT | DT_NOCLIP);
}

void TLabel::set_Caption(TCHAR * Caption, bool Copy)
{
	if(Copy && Caption)
	{
		int l = _tcslen(Caption) + 1;
		FCaption = (TCHAR *) malloc(l * sizeof(TCHAR));
		memcpy(FCaption, Caption, l * sizeof(TCHAR));
	}
	else FCaption = Caption;
	InvalidateRect(GetWindowHandle(), &FRect, false);
}

TLabel::TLabel(TParentControl * Parent, RECT * Rect, int Layout, TCHAR * Caption, bool Copy)
{
	if(Copy && Caption)
	{
		int l = _tcslen(Caption) + 1;
		FCaption = (TCHAR *) malloc(l * sizeof(TCHAR));
		memcpy(FCaption, Caption, l * sizeof(TCHAR));
	}
	else FCaption = Caption;
	_TControl(Parent, Rect, Layout);
}

void TLabel::Invalidate(RECT * OldRect)
{
	if(!FWinParent) return;
	/*if(OldRect->left == FRect.left && OldRect->top == FRect.top && OldRect->right == FRect.right && OldRect->bottom == FRect.bottom) return;
	HWND Handle = FWinParent->GetWindowHandle();
	InvalidateRect(Handle, OldRect, false);
	InvalidateRect(Handle, &FRect, false);*/
	MoveInvalidate(OldRect, 0);
}

/*********************************** TSplitter ***********************************************/

TSplitter::TSplitter(TParentControl * Parent, int Width, int Layout, int Style)
{
	FStyle = Style;
	RECT Rect = {0, 0, Width, Width};
	FVertical = Layout & LT_ALVERT;
	FPos = INT_MAX;
	_TControl(Parent, &Rect, Layout | LT_CWIDTH);
}

void TSplitter::OnPaint(PAINTSTRUCT * PaintStruct) // Процедура отрисовки компонента
{
	FillRect(PaintStruct->hdc, &FRect, FBackBrush);
	if(FStyle) 
		DrawEdge(PaintStruct->hdc, &FRect, FStyle, FVertical ? (BF_TOP | BF_BOTTOM) : (BF_RIGHT | BF_LEFT));// BF_RECT);
}

void TSplitter::OnMouseEvent(int x, int y, int mk, UINT uMsg) // Процедура реакции на движение
{
	RECT ORect;
	int d;
	static int o = 0;
	switch(uMsg)
	{
	case WM_MOUSEMOVE:
		SetCursor(LoadCursor(NULL, FVertical ? IDC_SIZENS : IDC_SIZEWE));
		if((mk & MK_LBUTTON) && (FPos != INT_MAX))
		{
			ORect = FRect;
			if(FVertical)
			{
				d = y - FPos - FRect.top;
				if(!d) return;
				ORect.top += d;
				ORect.bottom += d;
			}
			else
			{
				d = x - FPos - FRect.left;
				if(!d) return;
				ORect.left += d;
				ORect.right += d;
			}
			if(d != o)
			{
				ChildSetSize(&ORect);
				o = d;
			}
		}
		return;
	case WM_LBUTTONDOWN:
		o = 0;
		FPos = FVertical ? y - FRect.top : x - FRect.left;
		return;
	}
}

void TSplitter::Invalidate(RECT * OldRect)
{
	if(!FWinParent) return;
	HWND Handle = FWinParent->GetWindowHandle();
	InvalidateRect(Handle, OldRect, false);
	InvalidateRect(Handle, &FRect, false);
}

}