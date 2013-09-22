//! Описания базовых визуальных компонентов
/** \file
 	Файл содержит определения классов визуальных компонентов, не имеющих
	своего окна и определённой специализации.
*/
#ifndef _Controls_
#define _Controls_
#include <Windows.h>
#include "Classes.h"
#include <crtdbg.h>


namespace StdGUI {
/*************************************** Классы *************************************/

class TControl;///< любой графический компонент, имеющий геометрические размеры
	class TParentControl;///< графический компонент, имеющий размеры и дочерние компоненты
		class TSimplePanel;///< Безоконная панель
		class TWinParentControl;///< графический компонент, имеющий дочерние компоненты и окно
			class TAppWindow;///< Главное окно приложения.
	class TWinControl;///<  графический компонент, имеющий окно

/************************************ Поля и методы ***********************************
	RECT FRect;	- Координаты компонента в клинтской области родительского компонента-окна
		(FWinParent или GWL_HWNDPARENT)


	virtual bool ParentSetRect(RECT * Rect); - Уведомление от родительского компонента о перемещении.
		Компонент должен обновить свой FRect и вызвать соответствующие InvalidateRect.


******************************************************************************/

#define LT_ALLEFT	0x010 //!< Прижать к левому краю
#define LT_ALTOP	0x011 //!< Прижать к верхнему краю
#define LT_ALRIGHT	0x012 //!< Прижать к правому краю
#define LT_ALBOTTOM	0x013 //!< Прижать к нижнему краю

#define LT_SIZEIN	0x800 //!< Только на вход конструктору: Вместо RECT - int размер.

#define LT_SALLEFT		0x010|LT_SIZEIN //!< Прижать к левому краю
#define LT_SALTOP		0x011|LT_SIZEIN //!< Прижать к верхнему краю
#define LT_SALRIGHT		0x012|LT_SIZEIN //!< Прижать к правому краю
#define LT_SALBOTTOM	0x013|LT_SIZEIN //!< Прижать к нижнему краю

#define LT_ALVERT	0x001 //!< Возвращает: прижато к верху или к низу?
#define LT_ALOFFSET	0x003 //!< Возвращает смещение в структуре RECT стороны, к которой прижат компонент

#define LT_ANLEFT	0x021 //!< Якорь слева
#define LT_ANTOP	0x022 //!< Якорь сверху
#define LT_ANRIGHT	0x024 //!< Якорь справа
#define LT_ANBOTTOM	0x028 //!< Якорь снизу

#define LT_ALIGN	0x010 //!< Режим: Прижать к краю
#define LT_ANCHORS	0x020 //!< Режим: Якоря
#define LT_ALCLIENT	0x030 //!< Режим: Использовать всё доступное пространство
#define LT_MODE		0x030 //!< Возвращает режим

#define LT_SCALE	0x040 //!< Автоматическое сохранение соотношения размеров при изменении предоставленного пространства
#define LT_BSCALE	0x080 //!< Возможность LT_SCALE временно заблокирована
#define LT_CWIDTH	0x100 //!< Сохранять ширину компонента
#define LT_LAYOUT   0x03F //!< Получить положение компонента без прочих параметров

/// Базовый класс для всех графических компонентов.
class TControl
{
public:
	TNotifyEvent OnMouse;
protected:
	int FLayout;  ///< Положение элемента на клиентской области родителя
	int FSegment; ///< Отношение размера компонента к размеру FFreeClient родителя в момент расчёта позиции << 16
	RECT FRect;	///< Координаты компонента в клиентской области FWinParent
	RECT FOldRect; ///< Координаты до вызова ParentSetRect
	RECT FConstraints;
	TControl * FNext; ///< Следующий в связном списке родителя
	TControl * FPrev; ///< Предыдущий в связном списке родителя
	TParentControl * FParent;	///< Родительский компонент
	TWinParentControl * FWinParent; ///< Ближайший родительский компонент, являющийся окном

	virtual bool ParentSetRect(RECT * Rect); /*! Уведомление от родительских компонентов об изменеии размера.
	При вызове этого метода, компонент должен изменить FRect, и вызвать Invalidate */
	void ChildSetSize(RECT * Rect); // Контрол хочет изменить свои размеры
	virtual void OnSetRect(RECT * Rect) {}; ///< Уведомление для наследующих классов

	virtual void Invalidate(RECT * OldRect);
	virtual void OnPaint(PAINTSTRUCT * PaintStruct) = 0; ///< Процедура отрисовки компонента.
	virtual void OnMouseEvent(int x, int y, int mk, UINT uMsg) { if(OnMouse.Assigned()) OnMouse(this, x, y, mk, uMsg);}; ///< Процедура обработки событий мыши
	virtual void OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) {};
	virtual void OnFocus(bool Focused) {};
	virtual void MakeFocused(void);
	virtual void MouseEvent(int x, int y, int mk, UINT uMsg); // Уведомление о движении
	virtual void Paint(PAINTSTRUCT * PaintStruct){ OnPaint(PaintStruct);}; // Уведомление об отрисовке
	friend TParentControl;
	friend TWinParentControl;
	void _TControl(TParentControl * Parent, RECT * Rect, int Layout);
	TControl(void);
	HFONT FFont;
	bool MoveInvalidate(RECT * OldRect, int Flags);
public:
	static HBRUSH FDefaultBackBrush; //< При отсутствии родителя, все новые компоненты будут пользоваться этой кистью
	inline int get_Layout(void) {return FLayout;};
	HBRUSH FBackBrush;
	DWORD FTextColor;
	static DWORD FDefaultTextColor;
	static HPEN FDefaultPen;
	HPEN FPen;
	virtual HWND GetWindowHandle(void); ///< Возвращает хендл окна компонента, либо окна, на котором он находится
	__declspec(property(get=GetWindowHandle)) HWND WindowHandle;
	__declspec(property(get=get_Layout)) int Layout;
	bool SetSize(int Width, int Height);
	int GetWidth(void);
	int GetHeight(void);
	void inline GetRect(RECT * Rect) {*Rect = FRect;};
	virtual void set_Font(HFONT Font) {FFont = Font;};
	inline HFONT get_Font(void) {return FFont;};
	__declspec(property(get = get_Font, put = set_Font)) HFONT Font;
	HBRUSH inline get_BackBrush(void) {return FBackBrush;};
	__declspec(property(get = get_BackBrush)) HBRUSH BackBrush;
	virtual ~TControl(void);
	virtual void get_Constraints(RECT * c){*c = FConstraints;};
	virtual void set_Constraints(const RECT &Constraints) {FConstraints = Constraints;};
	inline void SetConstraints(int MinWidth, int MinHeight, int MaxWidth = 0, int MaxHeight = 0) { RECT r = {MinWidth, MinHeight, MaxWidth, MaxHeight}; set_Constraints(r);};
};

/// Базовый класс для всех графических компонентов, позволяющих содержать внутри себя другие компоненты.
class TParentControl: public TControl
{
private:
	TControl * FMouseOwner;
	void RestoreFC(int Layout, RECT * Rect);
protected:
	TControl * FFirstChild;
	TControl * FLastChild;	
	RECT FChildConstraints, FPartConstraints;
	RECT FClient; ///< Область, предоставляемая дочерним компонентам.
	RECT FBorders; ///< Толщина собственных границ компонента.
	RECT FFreeClient; ///< Часть области, предоставляемой дочерним компонентам, не занятая в данный момент.
	bool FClientPresent; ///< Указывает - существует ли FFreeClient.
	virtual bool ParentSetRect(RECT * Rect); ///< Уведомление от родительских компонентов
	virtual void ChildSetSize(TControl * Child, RECT * Rect); // Уведомление от дочерних компонентов
	virtual void PaintChilds(PAINTSTRUCT * PaintStruct); // Уведомления об отрисовке детям
	virtual void OnPaint(PAINTSTRUCT * PaintStruct);
	virtual void Paint(PAINTSTRUCT * PaintStruct) {PaintChilds(PaintStruct); OnPaint(PaintStruct);}; // Уведомление об отрисовке
	virtual void GetClientRct(RECT * Rect);
	virtual void MouseEvent(int x, int y, int mk, UINT uMsg);
	friend TControl;
	virtual void AddChild(TControl * Child);
	virtual void Invalidate(RECT * OldRect);
	virtual TWinParentControl * GetWinParent(void) {return FWinParent;}; // Получить FWinParent для детей данного компонента
	bool Align(TControl * Child, RECT * Old, bool Move);
	void Align(TControl * Child, int Layout, bool MoveChilds, RECT * ChildRect); // Прижимает Child к краю, отсекает от FFreeClient
	void ReAlign(TControl * First);
	void _TParentControl(TParentControl * Parent, RECT * Rect, int Layout); ///< Псевдоконструктор. Выполняется потомками класса в конструкторе для завершения инициализации объекта.
	TParentControl(void); ///< Первичный конструктор. Для полной инициализации объекта необходимо выполнить _TParentControl
public:
	/// Метод удаления дочернего компонента из клиентской области. 
	/*!Дочерние компоненты должны уведомлять родителя о своём уничтожении с помощью этого метода, с Destroy = true,
	если деструктор ещё не вызван.*/
	virtual void RemoveChild(TControl * Child/*!Устраняемый дочерний компонент*/, bool Destroy/*!<Вызвать деструктор и освободить объект*/); 
	virtual ~TParentControl(void);
	inline bool GetFreeClient(RECT * Rect) {if(!FClientPresent) return false; *Rect = FFreeClient; return true;}
	virtual void get_Constraints(RECT * c);
};

/// Простая панель, отрисовываемая вызовом DrawEdge, позволяющая создавать внутри себя другие компоненты.
class TSimplePanel: public TParentControl
{
protected:
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // Процедура отрисовки компонента
public:
	UINT FEdgeType;///< Операнд процедуры DrawEdge, указывающий вид краёв панели.
	TSimplePanel(TParentControl * Parent, RECT * Rect, int Layout);
};

class TGroupBox: public TParentControl
{
private:
	virtual void Invalidate(RECT * OldRect);
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // Процедура отрисовки компонента
	TCHAR * FCaption;
public:
	static int FDefaultStyle;
	int FStyle;
	virtual void set_Font(HFONT Font);
	TGroupBox(TParentControl * Parent, RECT * Rect, int Layout, TCHAR * Caption);
};

class TLabel: public TControl
{
protected:
	TCHAR * FCaption;
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // Процедура отрисовки компонента
	virtual void Invalidate(RECT * OldRect);
public:
	TCHAR * get_Caption(void) {return FCaption;};
	void set_Caption(TCHAR * Caption, bool Copy = true);
	TLabel(TParentControl * Parent, RECT * Rect, int Layout, TCHAR * Caption, bool Copy = false);
};

class TSplitter: public TControl
{
private:
	bool FVertical;
	int FPos;
protected:
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // Процедура отрисовки компонента
	virtual void OnMouseEvent(int x, int y, int mk, UINT uMsg); // Процедура реакции на движение
	virtual void Invalidate(RECT * OldRect);
public:
	int FStyle;
	TSplitter(TParentControl * Parent, int Width, int Layout, int Style = 0);
};

class TImage: public TControl
{
private:
	void DeleteBitMap(void);
	HDC FBMPDC;
	HGDIOBJ FDefBitMap;
	HBITMAP	FBitMap;
	char FBitCount;
protected:
	int FWidth, FHeight;
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // Процедура отрисовки компонента
	virtual bool ParentSetRect(RECT * Rect); // Уведомление от родительских компонентов
	virtual void Invalidate(RECT * OldRect);
	template<class T>
	inline T * GetPixel(T * Buffer, int x, int y)
	{
		_ASSERTE(x >= 0 && y >= 0 && x < FWidth && y < FHeight);
		return Buffer + x + (FHeight - y - 1) * FWidth;
	}
public:
	void * GetPixel(int x, int y);
	inline HDC GetBMP(void) {return FBMPDC;};
	char AutoResize; // Способы масштабирования изображения: 0 - NoResize, 1 - NoCopy, 2 - Copy, 3 - Stretch
	void * Bits;
	void * TImage::SetBitMapSize(int Width, int Height, int BitCount, int Mode = 0);
	inline int GetImageWidth(void) {return FWidth;};
	inline int GetImageHeight(void) {return FHeight;};
	inline void Fill(int Color) {memset(Bits, Color, (FBitCount >> 3) * FWidth * FHeight);}; //!< Залить буфер
	TImage(TParentControl * Parent, RECT * Rect, int Layout);
	~TImage(void) {DeleteBitMap();};
	bool LoadFromBMP(TCHAR * FileName);
	void Update(void); //!< Обновить из буфера
};

}
#endif
