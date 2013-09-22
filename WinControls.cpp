//! \file WinControls.cpp Реализация оконных визуальных компонентов
#include "WinControls.h"
#include <tchar.h>

#define pint(r) ((int *)&r)
#define MAX(a, b) a > b ? a : b
#define MIN(a, b) a < b ? a : b

namespace StdGUI {

inline void TStubManager::InitializeStub32(unsigned char * Stub, void * Object, void * Handler)
{
	Stub[0] = 0x58; // POP EAX
	Stub[1] = 0x68; // PUSH []
	*pint(Stub[2]) = (int) Object;
	Stub[6] = 0x50; // PUSH EAX
	Stub[7] = 0xE9; // JMP []
	*pint(Stub[8]) = (int)Handler - int(Stub) - 12; 
}


TStubManager::~TStubManager(void)
{
	for(unsigned char * p = FPages, * n; p; p = n)
	{
		n = *(unsigned char * *)(p + (FAlign - 1) * 12);
		VirtualFree(p, 0, MEM_RELEASE);
	}
}

void * TStubManager::NewStub(void * Object, void * Handler)
{
	unsigned char * p = FPages;
	if(!p) FPages = p = (unsigned char *)VirtualAlloc(0, 12 * FAlign, MEM_COMMIT, PAGE_READWRITE);
	while(true)
	{
		unsigned char * pp = p;
		for(int x = FAlign - 2; x; x--)
		{
			if(!*p)
			{
				DWORD d;
				BOOL r = VirtualProtect(pp, 12 * FAlign, PAGE_READWRITE, &d);
				InitializeStub32(p, Object, Handler);
				r = VirtualProtect(pp, 12 * FAlign, PAGE_EXECUTE_READ, &d);
				r = GetLastError();
				return p;
			}
			p += 12;
		}
		unsigned char * next = *(unsigned char * *)p;
		if(!next)
		{
			DWORD d;
			VirtualProtect(pp, 12 * FAlign, PAGE_READWRITE, &d);
			p = *(unsigned char * *)p = (unsigned char *)VirtualAlloc(0, 12 * FAlign, MEM_COMMIT, PAGE_READWRITE);
			VirtualProtect(pp, 12 * FAlign, PAGE_EXECUTE_READ | PAGE_EXECUTE | PAGE_READONLY, &d);
		}
	}
}

void Release(void * Stub)
{
	*(int *)Stub = 0;
}


/************************************ TWinControl *************************************************/

INITCOMMONCONTROLSEX TWinControl::Ficc = {sizeof(INITCOMMONCONTROLSEX), 0};
TStubManager TWinControl::FStubManager;

TWinControl::TWinControl(void)
{
	hInstance = 0;
	hWindow = 0;
	FDefWindowProc = 0;
	long (CALLBACK TWinControl::*t)(HWND, UINT, WPARAM, LPARAM);
	t = &TWinControl::WindowProc;
	FStub = FStubManager.NewStub(this, *(void * *)&t);
	//InitializeStub(FStub, this, *(void * *)&t);
}

TWinControl::~TWinControl(void)
{
	if(hWindow) 
	{
		DestroyWindow(hWindow);
		hWindow = 0;
	}
}

bool TWinControl::LoadFromResource(TParentControl * Parent, int Id, int Layout)
{
	HWND pw = Parent->GetWindowHandle();
	hWindow = GetDlgItem(pw, Id);
	if(!hWindow) return false;
	RECT Rect;
	GetWindowRect(hWindow, &Rect);
	ScreenToClient(pw, (POINT *)&Rect.left);
	ScreenToClient(pw, (POINT *)&Rect.right);
	_TWinControl(Parent, &Rect, Layout);
	return true;
}

void TWinControl::_TWinControl(TParentControl * Parent, RECT * Rect, int Layout)
{
	_TControl(Parent, Rect, Layout);
	if(FWinParent) hInstance = FWinParent->GetInstance();
	int cc = FWinParent->FWinChildCount;
	FWinParent->FWinChilds = (TWinControl **) realloc(FWinParent->FWinChilds, (cc + 1) * sizeof(TWinControl *));
	FWinParent->FWinChilds[cc++] = this;
	FWinParent->FWinChildCount = cc;
}

void TWinControl::OnSetRect(RECT * Rect) // Уведомление от родительских компонентов
{
	if(hWindow)	
		MoveWindow(hWindow, Rect->left, Rect->top, Rect->right - Rect->left, Rect->bottom - Rect->top, false);
}

LRESULT CALLBACK TWinControl::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CHAR:
		if(wParam == 13 && OnReturn.Assigned()) if(OnReturn(this)) return 0;
		if(OnChar.Assigned()) if(OnChar(this, &wParam, &lParam)) return 0;
		break;
	}
	return CallWindowProc(FDefWindowProc, hwnd, uMsg, wParam, lParam);
}


/********************************* TWinParentControl **********************************************/

TCHAR MDIClientClassName[] = _T("MDICLIENT");
TStubManager TWinParentControl::FStubManager;


LRESULT CALLBACK TWinParentControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_CREATE)
	{
		CREATESTRUCT * CS = (CREATESTRUCT *) lParam; 
		TWinParentControl * Control = (TWinParentControl *) CS->lpCreateParams;
		if((CS->lpszClass == MDIClientClassName) || (CS->dwExStyle & WS_EX_MDICHILD))
			Control = (TWinParentControl *) (((MDICREATESTRUCT *) Control)->lParam);
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG) Control->FStub);
		GetWindowRect(hwnd, &Control->FRect);
		GetClientRect(hwnd, &Control->FClient);
		Control->FFreeClient = Control->FClient;
		Control->FClientPresent = true;
		Control->hWindow = hwnd;
		Control->WindowProc(hwnd, uMsg, wParam, lParam);
		return 0;
	}
	if(uMsg == WM_INITDIALOG)
	{
		TWinParentControl * Control = (TWinParentControl *) lParam;
		BOOL (CALLBACK TWinParentControl::*Handler)(HWND, UINT, WPARAM, LPARAM);
		Handler = &TWinParentControl::DialogProc;
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG) Control->FStub);
		Control->FDefWindowProc = DefWindowProc;
		GetWindowRect(hwnd, &Control->FRect);
		GetClientRect(hwnd, &Control->FClient);
		Control->FFreeClient = Control->FClient;
		Control->FClientPresent = true;
		Control->hWindow = hwnd;
		Control->DialogProc(hwnd, uMsg, wParam, lParam);
		Control->OnInitDialog();
		return (LRESULT) true;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

TWinParentControl::TWinParentControl(TParentControl * Parent, RECT * Rect, int Layout)
{
	FWinChilds = 0;
	FWinChildCount = 0;
	FCommands = 0;
	FCommandCount = 0;
	hWindow = 0;
	hInstance = 0;
	FFocusedChild = 0;
	_TParentControl(Parent, Rect, Layout);
	FDefWindowProc = DefWindowProc;
	if(FWinParent) hInstance = FWinParent->GetInstance();
	long (CALLBACK TWinParentControl::*t)(HWND, UINT, WPARAM, LPARAM);
	t = &TWinParentControl::WindowProc;
	FStub = FStubManager.NewStub(this, *(void * *)&t);
	//InitializeStub(FStub, this, *(void * *)&t);
}

TWinParentControl::TWinParentControl(void)
{
	FWinChilds = 0;
	FWinChildCount = 0;
	FCommands = 0;
	FCommandCount = 0;
	hWindow = 0;
	hInstance = 0;
	FFocusedChild = 0;
	FDefWindowProc = DefWindowProc;
	long (CALLBACK TWinParentControl::*t)(HWND, UINT, WPARAM, LPARAM);
	t = &TWinParentControl::WindowProc;
	FStub = FStubManager.NewStub(this, *(void * *)&t);
	//InitializeStub(FStub, this, *(void * *)&t);
}

void TWinParentControl::_TWinParentControl(TParentControl * Parent, RECT * Rect, int Layout)
{
	_TParentControl(Parent, Rect, Layout);
	if(FWinParent) hInstance = FWinParent->GetInstance();
}


/*DLGPROC TWinParentControl::UseDialogProc(void)
{
	BOOL (CALLBACK TWinParentControl::*t)(HWND, UINT, WPARAM, LPARAM);
	t = &TWinParentControl::DialogProc;
	InitializeStub(FStub, this, *(void * *)&t);
	void * k = FStub;
	return *(DLGPROC *)&k;
}*/


void TWinParentControl::SetFocusedChild(TControl * Focused) 
{
	if(FFocusedChild == Focused) return;
	if(FFocusedChild) 
	{
		FFocusedChild->OnFocus(false);
		FFocusedChild = Focused;
		if(FFocusedChild) FFocusedChild->OnFocus(true);
	}
	else
	{
		FFocusedChild = Focused;
		SetFocus(hWindow);		
	}	
};

TWinParentControl::~TWinParentControl(void) 
{
	if(OnDestroy.Assigned()) OnDestroy(this);
	if(hWindow)
	{
		SetWindowLong(hWindow, GWL_WNDPROC, (LONG) TWinParentControlProc);
		DestroyWindow(hWindow);
		hWindow = 0;
	}
	free(FCommands);
	free(FWinChilds);
}


void TWinParentControl::GetClientRct(RECT * Rect)
{
	if(hWindow) GetClientRect(hWindow, Rect);
	else TParentControl::GetClientRct(Rect);
}

BOOL CALLBACK TWinParentControl::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = WindowProc(hwnd, uMsg, wParam, lParam);
	if(Result == -1) return false;
	SetWindowLong(hwnd, DWL_MSGRESULT, Result); 
	return true;
}

bool TWinParentControl::ParentSetRect(RECT * Rect) // Уведомление от родительских компонентов
{
	if(hWindow)	MoveWindow(hWindow, Rect->left, Rect->top, Rect->right - Rect->left, Rect->bottom - Rect->top, false);
	return true;
}

LRESULT CALLBACK TWinParentControl::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	/*case WM_NCPAINT:
		HDC dc;
		//if(wParam != 1) dc = GetDCEx(hwnd, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
		//else 
			dc = GetWindowDC(hwnd);
		//DWORD e; e = GetLastError();
		RECT Rec;
		Rec.left = 0;
		Rec.top = 0;
		Rec.right = 500;
		Rec.bottom = 500;
		//IntersectClipRect(dc, Rec.left, Rec.top, Rec.right, Rec.bottom);
		int e; e = FillRect(dc, &Rec, (HBRUSH) (COLOR_BTNFACE + 1));
		DrawText(dc, "Text", 4, &Rec, 0);
		// Paint into this DC
	    ReleaseDC(hwnd, dc);
		return 0;*/
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
	case WM_MOUSEWHEEL:
		{
			POINT p = {(short)(lParam), (short)(lParam >> 16)};
			ScreenToClient(hWindow, &p);
			lParam = p.x + (p.y << 16);
		}
#endif
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		if((uMsg == WM_LBUTTONDOWN) | (uMsg == WM_MBUTTONDOWN) | (uMsg == WM_RBUTTONDOWN))
			SetCapture(hwnd);
		MouseEvent((short)(lParam), (short)(lParam >> 16), wParam, uMsg);
		if((uMsg == WM_LBUTTONUP) | (uMsg == WM_MBUTTONUP) | (uMsg == WM_RBUTTONUP))
			ReleaseCapture();
		return 0;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
		if(FFocusedChild) FFocusedChild->OnKeyEvent(uMsg, wParam, lParam);
		return 0;
	case WM_SETFOCUS:
		if(FFocusedChild) FFocusedChild->OnFocus(true); 
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	case WM_KILLFOCUS: 
		if(FFocusedChild) 
		{
			FFocusedChild->OnFocus(false);
			FFocusedChild = 0;
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	case WM_SIZING:
		{
			RECT & r = *(RECT *)lParam;
			RECT c;
			get_Constraints(&c);
			int w = r.right - r.left;
			int h = r.bottom - r.top;
			bool Left = (wParam == WMSZ_LEFT || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_TOPLEFT);
			bool Right = (wParam == WMSZ_RIGHT || wParam == WMSZ_BOTTOMRIGHT || wParam == WMSZ_TOPRIGHT);
			bool Top = (wParam == WMSZ_TOP || wParam == WMSZ_TOPRIGHT || wParam == WMSZ_TOPLEFT);
			bool Bottom = (wParam == WMSZ_BOTTOM || wParam == WMSZ_BOTTOMRIGHT || wParam == WMSZ_BOTTOMLEFT);
			if(c.left && c.left > w)
			{
				if(Left) r.left -= c.left - w;
				if(Right) r.right += c.left - w;
			}
			if(c.right && c.right < w)
			{
				if(Left) r.left -= c.right - w;
				if(Right) r.right += c.right - w;
			}
			if(c.top && c.top > h)
			{
				if(Top) r.top -= c.top - h;
				if(Bottom) r.bottom += c.top - h;
			}
			if(c.bottom && c.bottom < h)
			{
				if(Top) r.top -= c.bottom - h;
				if(Bottom) r.bottom += c.bottom - h;
			}
			return TRUE;
		}
//	case WM_MOVE:
	case WM_SIZE:
		RECT Rect;
		GetWindowRect(hwnd, &Rect);
		HWND hw;
		if(FWinParent && (hw = FWinParent->GetWindowHandle())) 
		{
			ScreenToClient(hw, (POINT *)&Rect);
			ScreenToClient(hw, ((POINT *)&Rect) + 1);
		}
		if(Rect.left != -32000 || Rect.top != -32000)
			TParentControl::ParentSetRect(&Rect);
		return 0;
	case WM_PAINT:
		PAINTSTRUCT PaintStruct;
		if(BeginPaint(hWindow, &PaintStruct))
		{
			PaintChilds(&PaintStruct);
			OnPaint(&PaintStruct);
			EndPaint(hWindow, &PaintStruct);
		}
		return 0;
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORDLG:
		SetBkMode((HDC) wParam, TRANSPARENT);
		if(FWinChilds && lParam)
			for(int x = 0; x < FWinChildCount; x++)
				if((HWND) lParam == FWinChilds[x]->hWindow)
					return (LRESULT) FWinChilds[x]->FBackBrush;
		return (LRESULT) FBackBrush;
	case WM_COMMAND:
		if(FWinChilds && lParam)
			for(int x = 0; x < FWinChildCount; x++)
				if((HWND) lParam == FWinChilds[x]->hWindow)
					return FWinChilds[x]->OnCommand(wParam, lParam);
		wParam &= 0xFFFF;
		if(FCommands && wParam)
			for(int x = 0; x < FCommandCount; x++)
				if(wParam == FCommands[x].Identifier)
				{
					FCommands[x].Event(this, wParam);
					return 0;
				}
		return 0;
	case WM_DESTROY:
		if(FParent) FParent->RemoveChild(this, true);
		else 
		{
			if(OnDestroy) OnDestroy(this);
			hWindow = 0;
		}
		return 0;
	case WM_NOTIFY:
		if(FWinChilds && lParam)
		{
			NMHDR * NM = (NMHDR *)lParam;
			for(int x = 0; x < FWinChildCount; x++)
				if(NM->hwndFrom == FWinChilds[x]->hWindow)
					return FWinChilds[x]->OnNotify((NMHDR *)lParam);
		}
	case WM_CLOSE:
		if(OnClose && OnClose(this)) return 0;
	default:
		return FDefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

void TWinParentControl::AddChild(TControl * Child)
{
	Child->FRect.left -= FClient.left; // Относительные координаты
	Child->FRect.right -= FClient.left;
	Child->FRect.top -= FClient.top;
	Child->FRect.bottom -= FClient.top;
	TParentControl::AddChild(Child);
}

TNotifyEvent * TWinParentControl::Commands(UINT Identifier)
{
	for(int x = 0; x < FCommandCount; x++)
		if(FCommands[x].Identifier == Identifier) return &FCommands[x].Event;
	FCommands = (COMMANDREC *) realloc(FCommands, (FCommandCount + 1) * sizeof(*FCommands));
	FCommands[FCommandCount].Identifier = Identifier;
	FCommands[FCommandCount].Event.Assign(0);
	return &FCommands[FCommandCount++].Event;
}

void TWinParentControl::SetCommandsTable(COMMANDREC * Commands, int Count)
{
	FCommands = Commands;
	FCommandCount = Count;
}

/*********************************** TAppWindow ***********************************************/

TCHAR TAppWindow::FClassName[] = _T("TAppWindow");
ATOM TAppWindow::FClassAtom = 0;						// style, Proc, 
WNDCLASSEX TAppWindow::FWindowClass = {sizeof(WNDCLASSEX), 0, TWinParentControlProc, 0, 0, 0, 0, 0, /*(HBRUSH)(COLOR_BTNFACE + 1)*/0, 0, FClassName, 0};
int TAppWindow::FAppWinCount = 0;
bool TAppWindow::PMWorking = false;

LRESULT CALLBACK TAppWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_CLOSE)
	{
		if(!OnClose.Assigned() || (OnClose.Assigned() && OnClose(this)))
		{
			if(PMWorking)
			{
				PMWorking = false;
				PostMessage(hwnd, uMsg, wParam, lParam);
			}
			else
				DestroyWindow(hwnd);
		}
		return 0;
	}
	if(uMsg == WM_DESTROY) // Так как у окна приложения нет родителя, объект удаляет сам себя
	{
		if(PMWorking)
		{
			PMWorking = false;
			PostMessage(hwnd, uMsg, wParam, lParam);
			return 0;
		}
		//SetWindowLong(hwnd, GWL_WNDPROC, (LONG) DefWindowProc);
		delete this;
		return 0;
	}
	return TWinParentControl::WindowProc(hwnd, uMsg, wParam, lParam);
}

TAppWindow::TAppWindow(HINSTANCE hInstance, const TCHAR * Caption, HMENU hMenu, int Left, int Top, int Width, int Height):TWinParentControl(0, 0, 0)
{
	this->hInstance = hInstance;
	if(!FClassAtom)
	{
		FWindowClass.hInstance = hInstance;
		FWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		FClassAtom = RegisterClassEx(&FWindowClass);
	}
	FAppWinCount++;
	hWindow = CreateWindowEx(WS_EX_WINDOWEDGE|WS_EX_APPWINDOW, FClassName, Caption, /*WS_CLIPCHILDREN|*/WS_OVERLAPPEDWINDOW, Left, Top, Width, Height, 0, hMenu, hInstance, this);
}

TAppWindow::~TAppWindow()
{
	if(!--FAppWinCount)
	{
		if(FClassAtom) UnregisterClass((TCHAR *)FClassAtom, hInstance);
		FClassAtom = 0;
		PostQuitMessage(0);
	}
}


void TAppWindow::Show(int nCmdShow)
{
	ShowWindow(hWindow, nCmdShow);
	UpdateWindow(hWindow);
}

/************************************* TPopupWindow ***********************************************/

TCHAR TPopupWindow::FClassName[] = _T("TPopupWindow");
ATOM TPopupWindow::FClassAtom = 0;						// style, Proc, 
WNDCLASSEX TPopupWindow::FWindowClass = {sizeof(WNDCLASSEX), 0, TWinParentControlProc, 0, 0, 0, 0, 0, /*(HBRUSH)(COLOR_BTNFACE + 1)*/0, 0, FClassName, 0};

LRESULT CALLBACK TDlgDefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return -1;
}

TPopupWindow::TPopupWindow(TControl * Parent, int Left, int Top, int Width, int Height):TWinParentControl(0, 0, 0)
{
	HWND ParentWin = Parent ? Parent->GetWindowHandle() : 0;
	hInstance = GetModuleHandle(0);
	if(!FClassAtom)
	{
		FWindowClass.hInstance = hInstance;
		FWindowClass.hIconSm = 0;
		FWindowClass.hIcon = 0;
		FWindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		FClassAtom = RegisterClassEx(&FWindowClass);
	}
	hWindow = CreateWindowEx(WS_EX_WINDOWEDGE, FClassName, _T(""), WS_POPUP | WS_BORDER, Left, Top, Width, Height, ParentWin, 0, hInstance, this);
}

TPopupWindow::TPopupWindow(void):TWinParentControl(0, 0, 0)
{
	hWindow = 0;
}

void TPopupWindow::_TPopupWindow(TControl * Parent, LPCTSTR Template)
{
	FDefWindowProc = TDlgDefWindowProc;//DLGPROC
	hWindow = CreateDialogParam(GetModuleHandle(0), Template, Parent ? Parent->GetWindowHandle() : 0, (DLGPROC) TWinParentControlProc, (LPARAM) this);
}

void TPopupWindow::Show(int nCmdShow)
{
	ShowWindow(hWindow, nCmdShow);
	UpdateWindow(hWindow);
}

/************************************* TDialog ***********************************************/

TCHAR TDialog::FClassName[] = _T("TDialog");
ATOM TDialog::FClassAtom = 0;						// style, Proc, 
WNDCLASSEX TDialog::FWindowClass = {sizeof(WNDCLASSEX), 0, TWinParentControlProc, 0, 0, 0, 0, 0, /*(HBRUSH)(COLOR_BTNFACE + 1)*/0, 0, FClassName, 0};

/*TDialogWindow::TDialogWindow(TControl * Parent, int Left, int Top, int Width, int Height):TWinParentControl(0, 0, 0)
{
	HWND ParentWin = Parent ? Parent->GetWindowHandle() : 0;
	hInstance = GetModuleHandle(0);
	if(!TPWClass)
	{
		TPWClassEx.hInstance = hInstance;
		TPWClassEx.hIconSm = 0;
		TPWClassEx.hIcon = 0;
		TPWClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
		TPWClass = RegisterClassEx(&TPWClassEx);
	}
	hWindow = CreateWindowEx(WS_EX_WINDOWEDGE, TPWName, _T(""), WS_POPUP | WS_BORDER, Left, Top, Width, Height, ParentWin, 0, hInstance, this);
}*/

TDialog::TDialog(void):TWinParentControl(0, 0, 0)
{
	hWindow = 0;
}

void TDialog::_TDialog(TControl * Parent, LPCTSTR Template)
{
	FDefWindowProc = TDlgDefWindowProc;//DLGPROC
	hWindow = CreateDialogParam(GetModuleHandle(0), Template, Parent ? Parent->GetWindowHandle() : 0, (DLGPROC) TWinParentControlProc, (LPARAM) this);
}

void TDialog::Show(int nCmdShow)
{
	ShowWindow(hWindow, nCmdShow);
	UpdateWindow(hWindow);
}

int TDialog::ShowModal(int nCmdShow)
{
	ShowWindow(hWindow, nCmdShow);
	UpdateWindow(hWindow);

	BOOL r;
	MSG msg;
	while(r = GetMessage(&msg, hWindow, 0, 0))
	{
		if(r == -1) 
		{
			_ASSERTE(!"GetMessage error!");
			return -1;
		}
		else//if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}

/********************************** TEmbeddedDialog ***********************************************/

TEmbeddedDialog::TEmbeddedDialog(TParentControl * Parent, RECT * Rect, int Layout, LPCTSTR Template)
{
	FDefWindowProc = TDlgDefWindowProc;//DLGPROC
	HWND hp = Parent ? Parent->GetWindowHandle() : 0;
	hWindow = CreateDialogParam(GetModuleHandle(0), Template, hp, (DLGPROC) TWinParentControlProc, (LPARAM) this);
	RECT Rct;
	GetWindowRect(hWindow, &Rct);
	if(hp)
	{
		ScreenToClient(hp, (POINT *)&Rct);
		ScreenToClient(hp, ((POINT *)&Rct) + 1);
	}
	_TWinParentControl(Parent, &Rct, Layout);
}

void TEmbeddedDialog::Invalidate(RECT * OldRect)
{
	TParentControl::Invalidate(OldRect);
	/*if(!hWindow) return;
	int dx = FRect.right - FRect.left - OldRect->right + OldRect->left;
	int dy = FRect.bottom - FRect.top - OldRect->bottom + OldRect->top;
	RECT Rect;
	GetClientRect(hWindow, &Rect);
	Rect.left = Rect.right - dx - 1;
	Rect.right += dx;
	InvalidateRect(hWindow, &Rect, false);*/

}


void TEmbeddedDialog::Show(int nCmdShow)
{
	ShowWindow(hWindow, nCmdShow);
	UpdateWindow(hWindow);
}

/************************************ TModalDialog ***********************************************/

INT_PTR TModalDialog::Show(int nCmdShow)
{
	return DialogBoxParam(hInstance, FTemplate, FParentHandle, (DLGPROC) TWinParentControlProc, (LPARAM) this);
	//ShowWindow(hWindow, nCmdShow);
	//UpdateWindow(hWindow);
}

/*********************************** TMDIChildWindow ***********************************************/

TCHAR TMCName[] = _T("TMDIChildWindow");
ATOM TMCClass = 0;						// style, Proc, 
WNDCLASSEX TMCClassEx = {sizeof(WNDCLASSEX), 0, TWinParentControlProc, 0, 0, 0, 0, 0,/*(HBRUSH)(COLOR_BTNFACE + 1)*/0, 0, TMCName, 0};


TMDIChildWindow::TMDIChildWindow(TMDIClient * MDIClient, TCHAR * Name, int Left, int Top, int Width, int Height):TWinParentControl(0, 0, 0)
{
	FDefWindowProc = DefMDIChildProc;
	hInstance = MDIClient->GetInstance();
	if(!TMCClass)
	{
		TMCClassEx.hInstance = hInstance;
		TMCClassEx.hIconSm = 0;//LoadIcon(hInstance, (LPCTSTR)IDI_SMALL);
		TMCClassEx.hIcon = 0;//LoadIcon(hInstance, (LPCTSTR)IDI_APILOG2);
		TMCClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
		TMCClass = RegisterClassEx(&TMCClassEx);
	}

	hWindow = CreateWindowEx(WS_EX_WINDOWEDGE|WS_EX_MDICHILD, TMCName, Name, /*WS_CLIPCHILDREN|*/WS_OVERLAPPEDWINDOW, Left, Top, Width, Height, MDIClient->GetWindowHandle(), 0, hInstance, this);
	set_Font(MDIClient->get_Font());
	FBackBrush = MDIClient->FBackBrush;
	FTextColor = MDIClient->FTextColor;
	FPen = MDIClient->FPen;
}

void TMDIChildWindow::Show(int nCmdShow)
{
	ShowWindow(hWindow, nCmdShow);
	UpdateWindow(hWindow);
}

/************************************* TMDIClient *************************************************/

TMDIClient::TMDIClient(TParentControl * Parent, RECT * Rect, int Layout)
{
	_TWinControl(Parent, Rect, Layout);
	CLIENTCREATESTRUCT CCS = {0, 1000};
	hWindow = CreateWindowEx(0, MDIClientClassName, 0, WS_VISIBLE | WS_CLIPCHILDREN | WS_CHILD | WS_CLIPSIBLINGS, FRect.left, FRect.top, GetWidth(), GetHeight(), FWinParent->GetWindowHandle(), 0, hInstance, &CCS);
}

void TMDIClient::OnSetRect(RECT * Rect) // Уведомление от родительских компонентов
{
	if(hWindow)	
		//SetWindowPos(hWindow, 0, Rect->left, Rect->top, Rect->right - Rect->left, Rect->bottom - Rect->top, SWP_NOZORDER | SWP_NOREDRAW);
		MoveWindow(hWindow, Rect->left, Rect->top, Rect->right - Rect->left, Rect->bottom - Rect->top, false);
}

/************************************* TReBar *************************************************/

REBARINFO ReBarInfo = {sizeof(REBARINFO), 0, 0};

TReBar::TReBar(TParentControl * Parent)
{
	hWindow = CreateWindowEx(WS_EX_TOOLWINDOW,
                           REBARCLASSNAME,
                           NULL,
                           /*WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|
                           WS_CLIPCHILDREN|RBS_VARHEIGHT| RBS_AUTOSIZE | RBS_BANDBORDERS |
                           CCS_NODIVIDER*/
						   WS_VISIBLE|WS_BORDER|WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS
						   |CCS_NODIVIDER|CCS_NOPARENTALIGN|RBS_VARHEIGHT|RBS_BANDBORDERS,
                           0,0,400,200,
                           Parent->GetWindowHandle(),
                           NULL,
                           hInstance,
                           NULL);
	if(hWindow)
	{
		RECT Rect;
		GetWindowRect(hWindow, &Rect);
		_TWinControl(Parent, &Rect, LT_ALTOP);
	};
   if(hWindow) SendMessage(hWindow, RB_SETBARINFO, 0, (LPARAM)&ReBarInfo);
}

void TReBar::Invalidate(RECT * OldRect)
{
	if(!hWindow) return;
	RECT Rect;
	Rect.top = 0;
	Rect.left = 0;
	Rect.right = GetWidth();
	Rect.bottom = GetHeight();
	InvalidateRect(hWindow, &Rect, true);
	if(FWinParent)
	InvalidateRect(FWinParent->GetWindowHandle(), OldRect, true);
}

LRESULT TReBar::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if((HIWORD(wParam) == BN_CLICKED) && OnClick.Assigned())
		OnClick(this);
	return 0;
}

LRESULT TReBar::OnNotify(NMHDR * Header) 
{
	if(Header->code == RBN_HEIGHTCHANGE) //&& ((NMRBAUTOSIZE *) Header)->fChanged)
	{
		GetWindowRect(hWindow, &FRect);
		ChildSetSize(&FRect);
	}
	return 0;
};


/************************************* TButton *************************************************/

TButton::TButton(TParentControl * Parent, RECT * Rect, int Layout, TCHAR * Caption, int ButtonStyle)
{
	_TWinControl(Parent, Rect, Layout);
	hWindow = CreateWindowEx(0, _T("BUTTON"), Caption, WS_CHILD | WS_VISIBLE | ButtonStyle, FRect.left, FRect.top, GetWidth(), GetHeight(), FWinParent->GetWindowHandle(), 0, hInstance, 0);
	if(FFont) SendMessage(hWindow, WM_SETFONT, (WPARAM)FFont, 0);
}

void TButton::Invalidate(RECT * OldRect)
{
	if(!hWindow) return;
	RECT Rect;
	Rect.top = 0;
	Rect.left = 0;
	Rect.right = GetWidth();
	Rect.bottom = GetHeight();
	InvalidateRect(hWindow, &Rect, true);
	if(FWinParent)
	InvalidateRect(FWinParent->GetWindowHandle(), OldRect, true);
}

LRESULT TButton::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if((HIWORD(wParam) == BN_CLICKED) && OnClick.Assigned())
		OnClick(this);
	return 0;
}

/************************************* TUpDown *************************************************/

TUpDown::TUpDown(TParentControl * Parent, RECT * Rect, int Layout)
{
	_TWinControl(Parent, Rect, Layout);
	hWindow = 0;//CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE, FRect.left, FRect.top, GetWidth(), GetHeight(), FWinParent->GetWindowHandle(), 0, hInstance, 0);
}

void TUpDown::Invalidate(RECT * OldRect)
{
	if(!hWindow) return;
	RECT Rect;
	Rect.top = 0;
	Rect.left = 0;
	Rect.right = GetWidth();
	Rect.bottom = GetHeight();
	InvalidateRect(hWindow, &Rect, true);
	if(FWinParent)
	InvalidateRect(FWinParent->GetWindowHandle(), OldRect, true);
}

LRESULT TUpDown::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if((HIWORD(wParam) == UDN_DELTAPOS) && OnChange.Assigned())
		OnChange(this, ((LPNMUPDOWN) lParam)->iPos);
	return 0;
}

/************************************* TEdit *************************************************/

TEdit::TEdit(TParentControl * Parent, RECT * Rect, int Layout, DWORD EditStyle)
{
	_TWinControl(Parent, Rect, Layout);
	hWindow = CreateWindowEx(0, _T("EDIT"), _T(""), EditStyle | WS_CHILD | WS_VISIBLE, FRect.left, FRect.top, GetWidth(), GetHeight(), FWinParent->GetWindowHandle(), 0, hInstance, 0);
	if(FFont) SendMessage(hWindow, WM_SETFONT, (WPARAM)FFont, 0);
}

void TEdit::Invalidate(RECT * OldRect)
{
	if(!hWindow) return;
	RECT Rect;
	Rect.top = 0;
	Rect.left = 0;
	Rect.right = GetWidth();
	Rect.bottom = GetHeight();
	InvalidateRect(hWindow, &Rect, true);
	if(FWinParent)
	InvalidateRect(FWinParent->GetWindowHandle(), OldRect, true);
}

LRESULT TEdit::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if((HIWORD(wParam) == EN_CHANGE) && OnChange.Assigned())
		OnChange(this);
	return 0;
}

/************************************* TStatusBar *************************************************/

TStatusBar::TStatusBar(TWinParentControl * Parent, TCHAR * Caption)
{
	HWND hp = Parent->GetWindowHandle();
	InitCC(ICC_BAR_CLASSES);
	hWindow = CreateWindowEx(0, STATUSCLASSNAME, Caption, WS_CHILDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hp, 0, hInstance, 0);
	_ASSERTE(hWindow);
	RECT Rect;
	GetWindowRect(hWindow, &Rect);
	ScreenToClient(hp, (POINT *)&Rect);
	ScreenToClient(hp, ((POINT *)&Rect) + 1);
	_TWinControl(Parent, &Rect, LT_ALBOTTOM);
}

void TStatusBar::Invalidate(RECT * OldRect)
{
	if(!hWindow) return;
	RECT Rect;
	Rect.top = 0;
	Rect.left = 0;
	Rect.right = GetWidth();
	Rect.bottom = GetHeight();
	InvalidateRect(hWindow, &Rect, true);
}

/*LRESULT TStatusBar::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if((HIWORD(wParam) == BN_CLICKED) && OnClick)
		OnClick(this);
	return 0;
}*/

/************************************* TListBox *************************************************/

TListBox::TListBox(TParentControl * Parent, RECT * Rect, int Layout, int Style)
{
	FOCW = 0;
	FItemHeight = 0;
	_TWinControl(Parent, Rect, Layout);
	hWindow = CreateWindowEx(0/*WS_EX_CLIENTEDGE*/, _T("LISTBOX"), _T("ListBox"), WS_VSCROLL | WS_CHILD | WS_VISIBLE | LBS_NOTIFY | Style, FRect.left, FRect.top, GetWidth(), GetHeight(), FWinParent->GetWindowHandle(), 0, hInstance, 0);
	if(FFont)
		SendMessage(hWindow, WM_SETFONT, (WPARAM)FFont, 0);
	FItemHeight = SendMessage(hWindow, LB_GETITEMHEIGHT, 0, 0);
	Items.hWindow = hWindow;
}

TStrings * TListBox::get_Items(void)
{
	Items.hWindow = hWindow;
	return &Items;
}

void TListBox::Invalidate(RECT * OldRect)
{
	if(!hWindow) return;
	///RECT ORect = {0, 0, FOCW, OldRect->bottom - OldRect->top};
	//if(FItemHeight) ORect.bottom -= (ORect.bottom % FItemHeight);

	RECT Rect;
	GetClientRect(hWindow, &Rect);
	/*FOCW = Rect.right;
	if((ORect.right >= Rect.right) && (ORect.bottom >= Rect.bottom)) return;
	if(ORect.right >= Rect.right) Rect.top = ORect.bottom;
	if(ORect.bottom >= Rect.bottom) Rect.left = ORect.right;*/
	InvalidateRect(hWindow, &Rect, true);
	InvalidateRect(FParent->GetWindowHandle(), OldRect, false);
}

LRESULT TListBox::OnCommand(WPARAM wParam, LPARAM lParam)// WM_COMMAND
{
	if(HIWORD(wParam) == LBN_SELCHANGE)
		if(OnChange.Handler) OnChange(this);
	return 0;
}

int TListBox::get_ItemIndex(void)
{
	return SendMessage(hWindow, LB_GETCURSEL, 0, 0);
}

void TListBox::set_ItemIndex(int Index)
{
	SendMessage(hWindow, LB_SETCURSEL, Index, 0);
}

void TListBox::Paint(PAINTSTRUCT * PaintStruct)
{
	RECT Rect;
	GetWindowRect(hWindow, &Rect);
	int h = Rect.bottom - Rect.top;
	Rect = FRect;
	Rect.top += h;
	if(Rect.bottom - Rect.top)
		FillRect(PaintStruct->hdc, &Rect, FParent->BackBrush);
}

/************************************* TListBoxStrings *********************************************/

int TListBoxStrings::Add(const TCHAR * S, void * Object)
{
	return SendMessage(hWindow, LB_ADDSTRING, 0, (long) S);
}

void TListBoxStrings::Delete(int Index)
{
	SendMessage(hWindow, LB_DELETESTRING, Index, 0);
}

void TListBoxStrings::Clear(void)
{
	SendMessage(hWindow, LB_RESETCONTENT, 0, 0);
}

int TListBoxStrings::GetCount(void)
{
	return SendMessage(hWindow, LB_GETCOUNT, 0, 0);
}

int TListBoxStrings::IndexOf(char * S)
{
	return SendMessage(hWindow, LB_FINDSTRINGEXACT, (WPARAM) -1, (long) S);
}

void TListBoxStrings::Insert(int Index, const TCHAR * S)
{
	SendMessage(hWindow, LB_INSERTSTRING, Index, (long) S);
}

TCHAR * TListBoxStrings::get_Strings(int Index)
{
	if(int l = SendMessage(hWindow, LB_GETTEXTLEN, Index, 0))
	{
		TCHAR * r = (TCHAR *) malloc((l + 1) * sizeof(TCHAR));
		SendMessage(hWindow, LB_GETTEXT, Index, (int) r);
		return r;
	}
	else return 0;
}

void TListBoxStrings::set_Strings(int Index, TCHAR * String)
{
	SendMessage(hWindow, LB_DELETESTRING, Index, 0);
	SendMessage(hWindow, LB_INSERTSTRING, Index, (long) String);
}

/************************************* TComboBox *************************************************/

TComboBox::TComboBox(TParentControl * Parent, RECT * Rect, int Layout)
{
	_TWinControl(Parent, Rect, Layout);
	hWindow = CreateWindowEx(0, _T("COMBOBOX"), _T(""), WS_VSCROLL | WS_CHILD | CBS_DROPDOWNLIST | WS_VISIBLE, FRect.left, FRect.top, GetWidth(), GetHeight(), FWinParent->GetWindowHandle(), 0, hInstance, 0);
	Items.hWindow = hWindow;
	GetWindowRect(hWindow, &FRect);
	ScreenToClient(FWinParent->GetWindowHandle(), (POINT *) &FRect);
	ScreenToClient(FWinParent->GetWindowHandle(), ((POINT *) &FRect) + 1);
	ChildSetSize(&FRect);
}

/*bool TComboBox::LoadFromResource(TParentControl * Parent, int Id, int Layout)
{
	hWindow = GetDlgItem(Parent->GetWindowHandle(), Id);
	if(!hWindow) return false;
	Items.hWindow = hWindow;
	RECT Rect;
	GetWindowRect(hWindow, &Rect);
	_TWinControl(Parent, &Rect, Layout);
	return true;
}*/

TStrings * TComboBox::get_Items(void)
{
	Items.hWindow = hWindow;
	return &Items;
}

LRESULT TComboBox::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if((HIWORD(wParam) == CBN_SELCHANGE) && OnChange.Assigned())
		OnChange(this);
	return 0;
}

/*bool TComboBox::ParentSetRect(RECT * Rect) // Уведомление от родительских компонентов
{
	bool Result = TControl::ParentSetRect(Rect);
	int Width = GetWidth();
	int Height = GetHeight();
	SetWindowPos(hWindow, 0, FRect.left, FRect.top, Width, Height + 200, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);
	return Result;
}*/

void TComboBox::Invalidate(RECT * OldRect)
{
	/*if(!hWindow) return;
	RECT Rect;
	Rect.top = 0;
	Rect.left = 0;
	Rect.right = GetWidth();
	Rect.bottom = GetHeight();
	InvalidateRect(hWindow, &Rect, true);
	if(FWinParent)
	InvalidateRect(FWinParent->GetWindowHandle(), OldRect, true);*/

	if(FWinParent)
	InvalidateRect(FWinParent->GetWindowHandle(), OldRect, true);
}

/*void TComboBox::Paint(PAINTSTRUCT * PaintStruct)
{
	RECT Rect;
	GetWindowRect(hWindow, &Rect);
	int h = Rect.bottom - Rect.top;
	Rect = FRect;
	Rect.top += h;
	if(Rect.bottom - Rect.top)
		FillRect(PaintStruct->hdc, &Rect, FParent->BackBrush);
}*/

int TComboBox::get_ItemIndex(void)
{
	return SendMessage(hWindow, CB_GETCURSEL, 0, 0);
}

void TComboBox::set_ItemIndex(int Index)
{
	SendMessage(hWindow, CB_SETCURSEL, Index, 0);
}

/************************************* TComboBoxStrings *********************************************/

int TComboBoxStrings::Add(const TCHAR * S, void * Object)
{
	return SendMessage(hWindow, CB_ADDSTRING, 0, (long) S);
}

void TComboBoxStrings::Delete(int Index)
{
	SendMessage(hWindow, CB_DELETESTRING, Index, 0);
}

void TComboBoxStrings::Clear(void)
{
	SendMessage(hWindow, CB_RESETCONTENT, 0, 0);
}

int TComboBoxStrings::GetCount(void)
{
	return SendMessage(hWindow, CB_GETCOUNT, 0, 0);
}

int TComboBoxStrings::IndexOf(char * S)
{
	return SendMessage(hWindow, CB_FINDSTRINGEXACT, (WPARAM) -1, (long) S);
}

void TComboBoxStrings::Insert(int Index, const TCHAR * S)
{
	SendMessage(hWindow, CB_INSERTSTRING, Index, (long) S);
}

TCHAR * TComboBoxStrings::get_Strings(int Index)
{
	if(int l = SendMessage(hWindow, CB_GETLBTEXTLEN, Index, 0))
	{
		TCHAR * r = (TCHAR *) malloc((l + 1) * sizeof(TCHAR));
		SendMessage(hWindow, CB_GETLBTEXT, Index, (int) r);
		return r;
	}
	else return 0;
}

void TComboBoxStrings::set_Strings(int Index, TCHAR * String)
{
	SendMessage(hWindow, CB_DELETESTRING, Index, 0);
	SendMessage(hWindow, CB_INSERTSTRING, Index, (long) String);
}

/********************************* TScrollingWinControl *************************************************/

TCHAR TScrollingWinControl::TSWName[] = _T("TScrollingWinControl");
ATOM TScrollingWinControl::TSWClass = 0;						// style, Proc, 
TStubManager TScrollingWinControl::FStubManager;


LRESULT CALLBACK TScrollingWinControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_CREATE)
	{
		CREATESTRUCT * CS = (CREATESTRUCT *) lParam; 
		TScrollingWinControl * Control = (TScrollingWinControl *) CS->lpCreateParams;
		if((CS->lpszClass == MDIClientClassName) || (CS->dwExStyle & WS_EX_MDICHILD))
			Control = (TScrollingWinControl *) (((MDICREATESTRUCT *) Control)->lParam);
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG) Control->FStub);//Control->GetWindowProc());
		GetWindowRect(hwnd, &Control->FRect);
		GetClientRect(hwnd, &Control->FClient);
		Control->FOldRect = Control->FRect;
		Control->hWindow = hwnd;
		Control->WindowProc(hwnd, uMsg, wParam, lParam);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

WNDCLASSEX TScrollingWinControl::TSWClassEx = {sizeof(WNDCLASSEX), CS_DBLCLKS, TScrollingWinControlProc, 0, 0, 0, 0, 0,0/*(HBRUSH)(COLOR_WINDOW + 1)*/, 0, TSWName, 0};

TScrollingWinControl::TScrollingWinControl(void)
{
	long (CALLBACK TScrollingWinControl::*t)(HWND, UINT, WPARAM, LPARAM);
	t = &TScrollingWinControl::WindowProc;
	FStub = FStubManager.NewStub(this, *(void * *)&t);
	//InitializeStub(FStub, this, *(void * *)&t);

	FScrolls[0].cbSize = sizeof(SCROLLINFO);
	FScrolls[0].fMask = 0;//SIF_DISABLENOSCROLL;
	FScrolls[0].nMin = 0;
	FScrolls[0].nMax = 0;
	FScrolls[0].nPage = 10;
	FScrolls[0].nPos = 0;

	FScrolls[1].cbSize = sizeof(SCROLLINFO);
	FScrolls[1].fMask = 0;//SIF_DISABLENOSCROLL;
	FScrolls[1].nMin = 0;
	FScrolls[1].nMax = 0;
	FScrolls[1].nPage = 10;
	FScrolls[1].nPos = 0;
	FScrollUnits[0] = 1;
	FScrollUnits[1] = 1;

	FFixed.left = 0;
	FFixed.top = 0;
	FFixed.right = 0;
	FFixed.bottom = 0;

	FFocused = false;
}

TScrollingWinControl::~TScrollingWinControl(void)
{
	if(hWindow) SetWindowLong(hWindow, GWL_WNDPROC, (LONG) TScrollingWinControlProc);
}

/*void TScrollingWinControl::Invalidate(RECT * OldRect)
{
	RECT ORect;
	ORect.left = 0;
	ORect.top = 0;
	ORect.right = OldRect->right - OldRect->left;
	ORect.bottom = OldRect->bottom - OldRect->top;
	Rect CRect

	InvalidateRect(hWindow, &ORect, false);
	InvalidateRect(hWindow, &FRect, false);
	RECT Valid;
	Valid.left = MAX(OldRect->left, FRect.left);
	if(OldRect->left != FRect.left) Valid.left += FBorderWidth + FLeftWidth;
	Valid.top = MAX(OldRect->top, FRect.top);
	if(OldRect->top != FRect.top) Valid.top += FBorderWidth + FTopWidth;

	Valid.right = MIN(OldRect->right, FRect.right);
	if(OldRect->right != FRect.right) Valid.right -= FBorderWidth;
	Valid.bottom = MIN(OldRect->bottom, FRect.bottom);
	if(OldRect->bottom != FRect.bottom) Valid.bottom -= FBorderWidth;

	ValidateRect(Handle, &Valid);
}*/

void TScrollingWinControl::SetRange(int Bar, int Max, int Min)
{
	FScrolls[Bar].nMax = Max;
	FScrolls[Bar].nMin = Min;
	FScrolls[Bar].fMask = SIF_RANGE | SIF_PAGE;// | SIF_POS;
	SetScrollInfo(hWindow, Bar, FScrolls + Bar, true);

	if(FScrolls[Bar].nPos && (FScrolls[Bar].nPos + (int)FScrolls[Bar].nPage > FScrolls[Bar].nMax))
		OnScroll(Bar, SB_THUMBTRACK);
}

void TScrollingWinControl::ClientToLocal(int * x, int * y)
{
	if(x) *x += FScrollUnits[0] * FScrolls[0].nPos;
	if(y) *y += FScrollUnits[1] * FScrolls[1].nPos;
}

void TScrollingWinControl::LocalToClient(int * x, int * y)
{
	if(x) *x -= FScrollUnits[0] * FScrolls[0].nPos;
	if(y) *y -= FScrollUnits[1] * FScrolls[1].nPos;
}

void TScrollingWinControl::_TScrollingWinControl(TParentControl * Parent, RECT * Rect, int Layout, UINT WS_EX)
{
	_TWinControl(Parent, Rect, Layout);
	memset(&FClient, 0, sizeof(FClient));
	if(!TSWClass)
	{
		TSWClassEx.hInstance = hInstance;
		TSWClassEx.hIconSm = 0;//LoadIcon(hInstance, (LPCTSTR)IDI_SMALL);
		TSWClassEx.hIcon = 0;//LoadIcon(hInstance, (LPCTSTR)IDI_APILOG2);
		TSWClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
		TSWClass = RegisterClassEx(&TSWClassEx);
	}
	hWindow = CreateWindowEx(WS_EX, TSWName, 0, WS_CHILD | WS_VISIBLE, FRect.left, FRect.top, GetWidth(), GetHeight(), FWinParent->GetWindowHandle(), 0, hInstance, this);

	for(int Bar = 0; Bar < 2; Bar++) if(FScrollUnits[Bar])
	{
		FScrolls[Bar].nPage = (pint(FClient)[Bar + 2] - pint(FFixed)[Bar] - pint(FFixed)[Bar + 2]) / FScrollUnits[Bar];
		FScrolls[Bar].fMask = SIF_PAGE;
		SetScrollInfo(hWindow, Bar, FScrolls + Bar, true);
	}
}

void TScrollingWinControl::OnSetRect(RECT * Rect) // Уведомление от родительских компонентов
{
	if(hWindow)
	{
		MoveWindow(hWindow, Rect->left, Rect->top, Rect->right - Rect->left, Rect->bottom - Rect->top, true);
		for(int Bar = 0; Bar < 2; Bar++) if(FScrollUnits[Bar])
		{
			int Page = (pint(FClient)[Bar + 2] - pint(FFixed)[Bar] - pint(FFixed)[Bar + 2]) / FScrollUnits[Bar];
			FScrolls[Bar].nPage = Page;
			FScrolls[Bar].fMask = SIF_PAGE;// | SIF_RANGE;
			SetScrollInfo(hWindow, Bar, FScrolls + Bar, true);

			if(FScrolls[Bar].nPos && (FScrolls[Bar].nPos + Page > FScrolls[Bar].nMax))
				OnScroll(Bar, SB_THUMBTRACK);
		}
	}
}

/// Метод устанавливает FScrolls[Bar].nPos и возвращает смещение изображения в пикселах.
/// false означает отмену смещения. Если true, но смещение нулевое - Invalidate
bool TScrollingWinControl::ScrollQuery(int Bar, int Req, POINT * Offset)
{
	long * d = (long *) Offset;
	d[Bar] = FScrolls[Bar].nPos;
	d[Bar ^ 1] = 0;
	switch(Req)
	{
	case SB_BOTTOM:			// Scrolls to the lower right.
		FScrolls[Bar].nPos = FScrolls[Bar].nMax;
		break;
	case SB_LINEDOWN:		// Scrolls one line down.
		if(FScrolls[Bar].nPos < FScrolls[Bar].nMax - int(FScrolls[Bar].nPage) + 1)
			FScrolls[Bar].nPos++;
		break;
	case SB_LINEUP:			// Scrolls one line up.
		if(FScrolls[Bar].nPos > FScrolls[Bar].nMin)
			FScrolls[Bar].nPos--;
		break;
	case SB_PAGEDOWN:		// Scrolls one page down.
		FScrolls[Bar].nPos += FScrolls[Bar].nPage;
		if(FScrolls[Bar].nPos > FScrolls[Bar].nMax - int(FScrolls[Bar].nPage))
			FScrolls[Bar].nPos = FScrolls[Bar].nMax - int(FScrolls[Bar].nPage) + 1;
		break;
	case SB_PAGEUP:			// Scrolls one page up.
		FScrolls[Bar].nPos -= FScrolls[Bar].nPage;
		if(FScrolls[Bar].nPos < FScrolls[Bar].nMin)
			FScrolls[Bar].nPos = FScrolls[Bar].nMin;
		break;
	case SB_ENDSCROLL:		// Ends scroll.
	case SB_THUMBPOSITION:	// The user has dragged the scroll box (thumb) and released the
							//mouse button. The high-order word indicates the position of the
							//scroll box at the end of the drag operation.
		return false;

	case SB_THUMBTRACK:		// The user is dragging the scroll box. This message is sent
							//repeatedly until the user releases the mouse button. The
							//high-order word indicates the position that the scroll box has
							//been dragged to.
		FScrolls[Bar].fMask = SIF_TRACKPOS;
		GetScrollInfo(hWindow, Bar, FScrolls + Bar);
		FScrolls[Bar].nPos = FScrolls[Bar].nTrackPos;
		break;
	case SB_TOP:			// Scrolls to the upper left.
		FScrolls[Bar].nPos = 0;
		break;
	}
	if(!(d[Bar] -= FScrolls[Bar].nPos)) return false;
	if(FScrollUnits[Bar] && (abs(d[Bar]) > 16384)) d[Bar] = 0;
	else d[Bar] *= FScrollUnits[Bar];
	return true;
}

/// Метод определяет, в зависимости от действий пользователя, изменение FScrolls и смещает изображение
void TScrollingWinControl::OnScroll(int Bar, int Req)
{
	POINT d;
	if(!ScrollQuery(Bar, Req, &d)) return;

	FScrolls[Bar].fMask = SIF_POS;
	SetScrollInfo(hWindow, Bar, FScrolls + Bar, true);
	RECT Rect = FClient;
	Rect.top += FFixed.top;
	Rect.left += FFixed.left;
	Rect.right -= FFixed.right;
	Rect.bottom -= FFixed.bottom;
	if(d.x || d.y)
		ScrollWindowEx(hWindow, d.x, d.y, &Rect, &Rect, 0, 0, SW_INVALIDATE);
	else
		InvalidateRect(hWindow, &Rect, false);
}

void TScrollingWinControl::SetScroll(int Bar, int Pos)
{
	FScrolls[Bar].fMask = SIF_POS;
	int p = FScrolls[Bar].nPos;
	FScrolls[Bar].nPos = FScrolls[Bar].nTrackPos = Pos;
	SetScrollInfo(hWindow, Bar, FScrolls + Bar, false);
	FScrolls[Bar].nPos = p;
	OnScroll(Bar, SB_THUMBTRACK);
}

LRESULT CALLBACK TScrollingWinControl::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	/*case WM_NCPAINT:
		HDC dc;
		//if(wParam == 1)
			dc = GetWindowDC(hwnd);
		//else
		//	dc = GetDCEx(hwnd, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
		//DWORD e; e = GetLastError();
		RECT Rec;
		Rec.top = 0;
		Rec.left = 0;
		Rec.right = 500;
		Rec.bottom = 500;
		IntersectClipRect(dc, Rec.left, Rec.top, Rec.right, Rec.bottom);
		FillRect(dc, &Rec, (HBRUSH) (COLOR_BTNFACE + 1));
		// Paint into this DC
	    ReleaseDC(hwnd, dc);
		return 0;*/
	case WM_CREATE:
		int Bar, Page;
		Bar = SB_VERT;
		Page = (FClient.bottom - FFixed.top - FFixed.bottom) / FScrollUnits[Bar];
		FScrolls[Bar].nPage = Page;
		FScrolls[Bar].fMask = SIF_PAGE;
		SetScrollInfo(hWindow, Bar, FScrolls + Bar, true);
		return 0;
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
	/*case WM_MOUSEWHEEL: 
		int k; k = ((signed short)HIWORD(wParam)) / WHEEL_DELTA;
		SetScroll(SB_VERT, FScrolls[SB_VERT].nPos - k);
		//SendMessage(hWindow, WM_VSCROLL, SB_THUMBTRACK + (HIWORD(wParam) + FScrolls[SB_VERT].nPage) << 16, 0);
		return 0;*/
	case WM_MOUSEWHEEL:
#endif
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSELEAVE:
		if((uMsg == WM_LBUTTONDOWN) | (uMsg == WM_MBUTTONDOWN) | (uMsg == WM_RBUTTONDOWN))
		{
			if(!FFocused) SetFocus(hWindow);
			SetCapture(hwnd);
		}
		MouseEvent((short)(lParam), (short)(lParam >> 16), wParam, uMsg);
		if((uMsg == WM_LBUTTONUP) | (uMsg == WM_MBUTTONUP) | (uMsg == WM_RBUTTONUP))
			ReleaseCapture();
		return 0;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
		OnKeyEvent(uMsg, wParam, lParam);
		return 0;
	case WM_SETFOCUS: OnFocus(FFocused = true); return DefWindowProc(hwnd, uMsg, wParam, lParam);
	case WM_KILLFOCUS: OnFocus(FFocused = false); return DefWindowProc(hwnd, uMsg, wParam, lParam);
	case WM_SIZE:
		RECT Rect;
		//int x, y;

		Rect = FClient;

		GetClientRect(hWindow, &FClient);
		//Rect = FClient;
		if(Rect.right != FClient.right)
		{
			if(Rect.right < FClient.right)
			{
				Rect.left = Rect.right - 2;
				Rect.right = FClient.right;
			} 
			else
			{
				//Rect.left = Rect.right - 2;
				Rect.left = FClient.right - 2;
			}
			InvalidateRect(hWindow, &Rect, true);
		}
		return 0;

	case WM_MOVE:
		GetWindowRect(hwnd, &Rect);
		if(FWinParent) MapWindowPoints(0, FWinParent->GetWindowHandle(), (POINT *) &Rect, 2);
		TControl::ParentSetRect(&Rect);
		return 0;
	case WM_COMMAND:
		OnCommand(wParam, lParam);
		return 0;
	case WM_VSCROLL:
		OnScroll(SB_VERT, LOWORD(wParam)); return 0;
	case WM_HSCROLL:
		OnScroll(SB_HORZ, LOWORD(wParam)); return 0;
	case WM_PAINT:
		PAINTSTRUCT PaintStruct;
		if(BeginPaint(hWindow, &PaintStruct))
		{
			OnPaint(&PaintStruct);
			EndPaint(hWindow, &PaintStruct);
		}
		return 0;
	case WM_TIMER:
		return OnTimer(wParam, lParam);
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

/********************************* TCustomGrid *************************************************/

DWORD TCustomGrid::FDefaultTxtColor = GetSysColor(COLOR_WINDOWTEXT);
DWORD TCustomGrid::FDefaultSelTxtColor = GetSysColor(COLOR_HIGHLIGHTTEXT);
HBRUSH TCustomGrid::FDefaultCellsBrush = (HBRUSH) (COLOR_WINDOW + 1);
HBRUSH TCustomGrid::FDefaultSelBrush = (HBRUSH) (COLOR_HIGHLIGHT + 1);
HBRUSH TCustomGrid::FDefaultBlurBrush = (HBRUSH) (COLOR_INACTIVEBORDER + 1);

TCustomGrid::TCustomGrid(TParentControl * Parent, RECT * Rect, int Layout)
{
	FUseOnDrawCell = false;
	FVLineWidth = 1;
	FHLineWidth = 1;
	FSelCol = -1;
	FSelRow = -1;
	FSelMode = _SM(0);
	hEditor = 0;
	FRows = 0;
	FCols = 1;
	FColOffsets = 0;
	FFixedRows = 1;
	FFixedCols = 0;
	FFields = 0;
	FDefRowHeight = 18;
	FDefColWidth = 150;
	FScrollUnits[1] = FDefRowHeight;
	FScrollUnits[0] = 0;//20;
	FFixed.top = FDefRowHeight;
	FCharSize.cx = 10;
	FCharSize.cy = FDefRowHeight - 1;
	FColPos = 0;
	FColNum = 0;
	FCellsBrush = FDefaultCellsBrush;
	FSelBrush = FDefaultSelBrush;
	FBlurBrush = FDefaultBlurBrush;
	FTxtCol = FDefaultTxtColor;
	FSelTxtCol = FDefaultSelTxtColor;
	_TScrollingWinControl(Parent, Rect, Layout);
}

void TCustomGrid::set_Font(HFONT Font)
{
	TScrollingWinControl::set_Font(Font);
	if(FFont)
	{
		HDC dc = GetDC(0);
		HGDIOBJ OldFont = SelectObject(dc, FFont); 
		GetTextExtentPoint32(dc, _T("0"), 1, &FCharSize);
		SelectObject(dc, OldFont);
		ReleaseDC(0, dc);
		FDefRowHeight = FCharSize.cy + 1;
	} else FDefRowHeight = 18;
	FScrollUnits[1] = FDefRowHeight;
	FFixed.top = FDefRowHeight;
}


TCustomGrid::~TCustomGrid(void)
{
	free(FColOffsets);
}

/// Метод установки числа колонок в таблице
void TCustomGrid::SetCols(DWORD Cols)
{
	if(Cols < 1) Cols = 1;
	FColOffsets = (int *) realloc(FColOffsets, (Cols - 1) * sizeof(int));
	if(Cols > FCols) for(DWORD x = FCols - 1; x < Cols - 1; x++)
		FColOffsets[x] = FDefColWidth + (x ? FColOffsets[x - 1] : 0);
	FCols = Cols;
}

/// Метод установки числа строк в таблице
void TCustomGrid::SetRows(DWORD Rows)
{
	FRows = Rows;
	SetRange(SB_VERT, (Rows > 0) ? (Rows - 1) : 0);
}

void TCustomGrid::OnPaint(PAINTSTRUCT * PaintStruct) // Процедура отрисовки компонента
{
	HGDIOBJ OldFont, OldPen;
	if(FFont) OldFont = SelectObject(PaintStruct->hdc, FFont);
	if(FPen) OldPen = SelectObject(PaintStruct->hdc, FPen);
	RECT Rect = FClient;
	Rect.bottom = Rect.top + FFixedRows * FDefRowHeight;
	FillRect(PaintStruct->hdc, &Rect, FBackBrush);			// Заливка и обводка шапки
	MoveToEx(PaintStruct->hdc, Rect.left, Rect.top, 0);
	LineTo(PaintStruct->hdc, Rect.left, Rect.bottom - 1);
	LineTo(PaintStruct->hdc, Rect.right - 1, Rect.bottom - 1);
	LineTo(PaintStruct->hdc, Rect.right - 1, Rect.top);
	LineTo(PaintStruct->hdc, Rect.left, Rect.top);
	//DrawEdge(PaintStruct->hdc, &Rect, EDGE_ETCHED, BF_RECT);
	if(FFields) // Подписывание столбцов
	{
		GetClientRect(hWindow, &Rect);
		Rect.top = 0;
		Rect.left = 5;
		SetBkMode(PaintStruct->hdc, TRANSPARENT);
		SetTextColor(PaintStruct->hdc, FTextColor);
		DrawText(PaintStruct->hdc, FFields[0], _tcslen(FFields[0]), &Rect, DT_LEFT | DT_NOCLIP);
		for(DWORD x = 1; x < FCols; x++)
		{
			Rect.left = FColOffsets[x - 1] + 5;
			DrawText(PaintStruct->hdc, FFields[x], _tcslen(FFields[x]), &Rect, DT_LEFT | DT_NOCLIP);
		}
	}

	if(FUseOnDrawCell)
	{
		RECT Range;
		if(RectToRange(&PaintStruct->rcPaint, &Range))
		for(DWORD Col = Range.left ; Col <= (DWORD)Range.right; Col++)
		for(DWORD Row = Range.top; Row <= (DWORD)Range.bottom; Row++)
		if(GetCellRect(Col, Row, &Rect))
		{
			HBRUSH Brush = FCellsBrush;
			bool Sel = false;
			if((FSelMode & SM_SHOW) && (FSelMode & (SM_SROW | SM_SCOL)))
				if(((FSelRow == Row) || !(FSelMode & SM_SROW)) && ((FSelCol == Col) || !(FSelMode & SM_SCOL)))
				{
					Sel = true;

					Brush = FFocused ? FSelBrush : FBlurBrush;
				}
			if(Brush) FillRect(PaintStruct->hdc, &Rect, Brush);
			SetTextColor(PaintStruct->hdc, Sel ? FSelTxtCol : FTxtCol);
			if(TCHAR * Text = OnDrawCell(Col, Row, &Rect, PaintStruct->hdc))
			{
				DrawText(PaintStruct->hdc, Text, _tcslen(Text), &Rect, DT_LEFT | DT_NOCLIP);
				//if(Sel)
				//	SetTextColor(PaintStruct->hdc, TxtCol);
			}
		}
		if(FRows <= (DWORD)FScrolls[SB_VERT].nPos + FScrolls[SB_VERT].nPage) // Не хватает рядов
		if(FCellsBrush)
		{
			Rect = FClient;
			Rect.top = (FRows - FScrolls[SB_VERT].nPos + FFixedRows) * FDefRowHeight;
			FillRect(PaintStruct->hdc, &Rect, FCellsBrush);
		}
	}
	else
	{
		Rect = FClient;									// Заливка полей данных
		Rect.top += FFixedRows * FDefRowHeight;
		if(FCellsBrush)	FillRect(PaintStruct->hdc, &Rect, FCellsBrush);

		if(FSelMode & SM_SHOW)					// Заливка выделенного поля
			if(GetCellRect(FSelCol, FSelRow, &Rect))
				FillRect(PaintStruct->hdc, &Rect, FSelBrush);

	}

	if(FVLineWidth)
	for(DWORD x = 0; x < FCols - 1; x++)						// Расчерчивание колонок
	{
		int o = FColOffsets[x];
		MoveToEx(PaintStruct->hdc, o, 0, 0);
		LineTo(PaintStruct->hdc, o, FClient.bottom);
	}
	if(FFont) SelectObject(PaintStruct->hdc, OldFont);
	if(FPen) SelectObject(PaintStruct->hdc, OldPen);
}


bool TCustomGrid::GetCellRect(DWORD Col, DWORD Row, RECT * Rect, _SM Mode)
// FScrolls[SB_VERT].nPos - Самый верхний видимый ряд
// FScrolls[SB_VERT].nPos + nPage - Самый нижний видимый ряд
{
	if(FCols <= Col) return false;
	if(FRows <= Row) return false;
	if(Mode & SM_AROW)
	{
		int VOffset = Row - FScrolls[SB_VERT].nPos;
		if(VOffset < 0) return false;
		if(VOffset > (int)FScrolls[SB_VERT].nPage) return false;
		Rect->top = FDefRowHeight * (VOffset + FFixedRows);
		Rect->bottom = Rect->top + FDefRowHeight;
	} else {
		*Rect = FClient;
		Rect->top += FFixedRows * FDefRowHeight;
	}

	// Обработка колонки
	Rect->left = (Mode & SM_ACOL) && Col ? FColOffsets[Col - 1] + FVLineWidth : 0;
	if((Col == FCols - 1) || !(Mode & SM_ACOL))
	{
		RECT Client;
		GetClientRect(hWindow, &Client);
		Rect->right = Client.right;
	} else Rect->right = FColOffsets[Col];
	return true;
}

bool TCustomGrid::RectToRange(const RECT * Rect, RECT * Range)
{
	if((DWORD)Rect->bottom < FDefRowHeight * FFixedRows) return false;
	//*Range = *Rect;
	Range->top = FScrolls[SB_VERT].nPos;
	Range->left = 0;
	CoordsToCell(Rect->left, Rect->top, (DWORD *)&Range->left, (DWORD *)&Range->top);
	Range->bottom = FScrolls[SB_VERT].nPos + FScrolls[SB_VERT].nPage;
	Range->right = FCols - 1;
	CoordsToCell(Rect->right, Rect->bottom, (DWORD *)&Range->right, (DWORD *)&Range->bottom);
	return true;
}


bool TCustomGrid::CoordsToCell(int x, int y, DWORD * Col, DWORD * Row)
{
	bool Result = true;
	if(Row)
	{
		if((y >= int(FDefRowHeight * FFixedRows)) && (y < FClient.bottom))
		{
			DWORD r = y / FDefRowHeight - FFixedRows + FScrolls[SB_VERT].nPos;
			if(r >= FRows) Result = false;
			else *Row = r;
		} else Result = false;
	}
	if(Col)
	{
		if((x < 0) || (x > FClient.right)) return false;
		for(DWORD i = 0; i < FCols - 1; i++)
		if(x < FColOffsets[i])
		{
			*Col = i;
			return Result;
		}
		*Col = FCols - 1;
	}
	return Result;
}

void TCustomGrid::ShowEditor(int Col, int Row, TCHAR * Data)
{
	RECT Rect;
	if(!GetCellRect(Col, Row, &Rect)) return;
	if(!hEditor)
	{
		hEditor = CreateWindowEx(0, _T("EDIT"), Data, ES_LEFT | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top, hWindow, 0, hInstance, 0);
		if(FFont) SendMessage(hEditor, WM_SETFONT, (WPARAM)FFont, 0);
	}
	else
	{
		SetWindowPos(hEditor, 0,  Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOACTIVATE);
		SetWindowText(hEditor, Data);
	}
	ShowWindow(hEditor, SW_SHOWNORMAL);
	SetFocus(hEditor);
}

void TCustomGrid::OnFocus(bool Focused)
{
	RECT Rect;
	if(GetCellRect(FSelCol, FSelRow, &Rect, FSelMode)) InvalidateRect(hWindow, &Rect, false);
}


void TCustomGrid::SetSelection(DWORD Col, DWORD Row)
{
	if(Row >= FRows) Row = FRows - 1;
	if(Col >= FCols) Col = FCols - 1;
	if(FSelMode & SM_AROW) FSelMode = _SM(FSelMode | SM_SROW);
	if(FSelMode & SM_ACOL) FSelMode = _SM(FSelMode | SM_SCOL);
	if(((FSelMode & SM_AROW) && (Row != FSelRow)) || ((FSelMode & SM_ACOL) && (Col != FSelCol)))
	{
		RECT Rect;
		if(GetCellRect(FSelCol, FSelRow, &Rect, FSelMode)) InvalidateRect(hWindow, &Rect, false);
		if(GetCellRect(Col, Row, &Rect, FSelMode)) InvalidateRect(hWindow, &Rect, false);

		FSelRow = Row;
		FSelCol = Col;
	}
	if(FSelRow < (DWORD)FScrolls[SB_VERT].nPos) SetScroll(SB_VERT, FSelRow);
	else
	if(FSelRow - FScrolls[SB_VERT].nPos >= (int)FScrolls[SB_VERT].nPage) SetScroll(SB_VERT, FSelRow - FScrolls[SB_VERT].nPage + 1);
}

void TCustomGrid::OnMouseEvent(int x, int y, int mk, UINT uMsg) // Процедура реакции на движение
{
	DWORD i;
	int d;
	RECT Rect;
	switch(uMsg)
	{
	case WM_MOUSEMOVE:
		if(FColNum)
		{
			GetClientRect(hWindow, &Rect);
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			d = x - FColPos;
			if(!d) return;
			Rect.left = FColOffsets[FColNum - 1];
			for(i = FColNum - 1; i < FCols - 1; i++) FColOffsets[i] += d;
			ScrollWindowEx(hWindow, d, 0, &Rect, 0, 0, 0, SW_INVALIDATE);
			if(d > 0)
			{
				int r = Rect.right;
				Rect.right = Rect.left;
				Rect.left = Rect.right - d;
				InvalidateRect(hWindow, &Rect, false);
				Rect.right = r;
			}

			Rect.left = Rect.right + (d > 0 ? -1 : d - 1);
			Rect.bottom = Rect.top + FDefRowHeight;
			InvalidateRect(hWindow, &Rect, false);
			FColPos = x;
			return;
		}

		if(y >= (int)(FDefRowHeight * FFixedRows)) return;
		for(i = 0; i < FCols - 1; i++)
		if(abs(x - FColOffsets[i]) < 3)
		{
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			return;
		}
		return;
	case WM_LBUTTONDOWN:
		if(y >= (int)(FDefRowHeight * FFixedRows))
		{
			if(!FSelMode) return;
			DWORD cx, cy;
			if(CoordsToCell(x, y, &cx, &cy)) SetSelection(cx, cy); 
			return;
		}
		for(i = 0; i < FCols - 1; i++)
		if(abs(x - FColOffsets[i]) < 2)
		{
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
			FColPos = x;
			FColNum = i + 1;
			return;
		}
	case WM_LBUTTONUP:
		FColNum = 0;
		return;
	}
}

LRESULT TCustomGrid::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if((HIWORD(wParam) == EN_KILLFOCUS) && ((HWND)lParam == hEditor))
	{
		ShowWindow(hEditor, SW_HIDE);
		int l = SendMessage(hEditor, WM_GETTEXTLENGTH, 0, 0);
		if(l)
		{
			char * s = (char *) malloc((l + 1) * sizeof(TCHAR));
			SendMessage(hEditor, WM_GETTEXT, l + 1, (WPARAM) s);
			OnSetText(s);
		} else
			OnSetText(0);
	}
	return 0;
}

void TCustomGrid::OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_KEYDOWN) switch(wParam)
	{
		int t;
		case 0x24:/*Home*/if(FSelMode & SM_SROW)SetSelection(FSelCol, 0); return;
		case 0x23:/*End*/ if(FSelMode & SM_SROW)SetSelection(FSelCol, FRows - 1); return;
		case 0x21:/*PageUp*/t = FSelRow - FScrolls[SB_VERT].nPage; if(FSelMode & SM_SROW)SetSelection(FSelCol, t >= 0 ? t : 0); return;
		case 0x22:/*PageDn*/if(FSelMode & SM_SROW)SetSelection(FSelCol, FSelRow + FScrolls[SB_VERT].nPage); return;
		case 0x26: if(FSelRow > 0) SetSelection(FSelCol, FSelRow - 1); return;
		case 0x28: if(FSelMode & SM_SROW) SetSelection(FSelCol, FSelRow + 1); return;
		case 0x25: if(FSelCol > 0) SetSelection(FSelCol - 1, FSelRow); return;
		case 0x27: if(FSelMode & SM_SROW) SetSelection(FSelCol + 1, FSelRow); return;
	}
}

/////////////////////////////// THintWindow /////////////////////////////////////

TCHAR THintWindow::FClassName[] = _T("THintWindow");
ATOM THintWindow::FClassAtom = 0;						// style, Proc, 
TStubManager THintWindow::FStubManager;

#define THWLeft 3
#define THWRight 4
#define THWBottom 3
#define THWMaxLine 130

LOGFONT THintWindow::FHintFont = {
-11, //      lfHeight;
0, //      lfWidth;
0, //      lfEscapement;
0, //      lfOrientation;
400, //    lfWeight;
false, //  lfItalic;
false, //  lfUnderline;
false, //  lfStrikeOut;
DEFAULT_CHARSET,//lfCharSet;
OUT_DEFAULT_PRECIS, //lfOutPrecision;
CLIP_DEFAULT_PRECIS, // lfClipPrecision;
ANTIALIASED_QUALITY, // lfQuality;
VARIABLE_PITCH, // lfPitchAndFamily;
_T("Tahoma")
};

LRESULT CALLBACK THintWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_CREATE)
	{
		CREATESTRUCT * CS = (CREATESTRUCT *) lParam; 
		THintWindow * Control = (THintWindow *) CS->lpCreateParams;
		if((CS->lpszClass == MDIClientClassName) || (CS->dwExStyle & WS_EX_MDICHILD))
			Control = (THintWindow *) (((MDICREATESTRUCT *) Control)->lParam);
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG) Control->FStub);
		Control->hWindow = hwnd;
		return Control->WindowProc(hwnd, uMsg, wParam, lParam);
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#define CS_DROPSHADOW 0x00020000

WNDCLASSEX THintWindow::FClass = {sizeof(WNDCLASSEX), CS_SAVEBITS | CS_DROPSHADOW, THintWindowProc, 0, 0, 0, 0, 0, (HBRUSH)(COLOR_INFOBK + 1), 0, FClassName, 0};

LRESULT CALLBACK THintWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		PAINTSTRUCT PaintStruct;
		if(BeginPaint(hWindow, &PaintStruct))
		{
			RECT Rect;
			GetClientRect(hWindow, &Rect);
			Rect.left += THWLeft;
			SetBkMode(PaintStruct.hdc, TRANSPARENT);
			HGDIOBJ OldFont = SelectObject(PaintStruct.hdc, FFont);
			DrawText(PaintStruct.hdc, FText, _tcslen(FText), &Rect, DT_LEFT | DT_NOCLIP | DT_WORDBREAK);
			SelectObject(PaintStruct.hdc, OldFont);
			EndPaint(hWindow, &PaintStruct);
		}
		return 0;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}


THintWindow::THintWindow(TWinParentControl * Parent)
{
	FFont = 0;
	FText = 0;
	GetClientRect(GetDesktopWindow(), &FScreen);
	long (CALLBACK THintWindow::*t)(HWND, UINT, WPARAM, LPARAM);
	t = &THintWindow::WindowProc;
	FStub = FStubManager.NewStub(this, *(void * *)&t);
	//InitializeStub(FStub, this, *(void * *)&t);
	if(!FClassAtom)
	{
		FClass.hInstance = Parent->GetInstance();
		FClass.hIconSm = 0;//LoadIcon(hInstance, (LPCTSTR)IDI_SMALL);
		FClass.hIcon = 0;//LoadIcon(hInstance, (LPCTSTR)IDI_APILOG2);
		FClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		FClassAtom = RegisterClassEx(&FClass);
	}

	hWindow = CreateWindowEx(WS_EX_LTRREADING|WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LEFT, FClassName, 0, ES_READONLY|WS_POPUP|WS_BORDER|WS_OVERLAPPED, 100, 100, 200, 200, Parent->GetWindowHandle(), 0, Parent->GetInstance(), this);

	FFont = CreateFontIndirect(&FHintFont);
}

THintWindow::~THintWindow(void)
{
	if(FFont) DeleteObject(FFont);
}

void THintWindow::ShowText(int x, int y, TCHAR * Text)
{
	FText = Text;
	if(!Text)
	{
		ShowWindow(hWindow, SW_HIDE);
		return;
	}
	HDC dc = GetDC(hWindow);
	if(!dc) return;
	HGDIOBJ OldFont = SelectObject(dc, FFont);
	int cx = 0, cy = THWBottom;
	SIZE s;
	TCHAR * t = Text;
	for(TCHAR * c = Text; ; c++)
	if((*c == 10) || !*c || (*c == ' ' && c - t > THWMaxLine)) if(GetTextExtentPoint32(dc, t, c - t, &s))
	{
		cy += s.cy;
		if(cx < s.cx) cx = s.cx;
		if(!*c) break;
		t = c + 1;
	}
	cx += THWLeft + THWRight;
	if(x + cx + 5 > FScreen.right) x = FScreen.right - cx - 5;
	SetWindowPos(hWindow, HWND_TOP, x, y, cx, cy, SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_NOACTIVATE);

	SelectObject(dc, OldFont);
	ReleaseDC(hWindow, dc);
}

void THintWindow::Hide(void)
{
	if(FText) 
	{
	//	Sleep(1000);
		ShowWindow(hWindow, SW_HIDE);
		FText = 0;
	}
}

/************************************* TMenuStrings *********************************************/

int TMenuStrings::Add(TCHAR * S, void * Object)
{
	UINT n = GetMenuItemCount(hMenu);
	if(AppendMenu(hMenu, MF_ENABLED | MF_STRING, Object ? (UINT) Object : n, S)) return  n;
	else return -1;
}

int TMenuStrings::Add(TCHAR * S, int State, void * Object)
{
	UINT n = GetMenuItemCount(hMenu);
	if(S) State |= MF_STRING;
	if(AppendMenu(hMenu, State, Object ? (UINT) Object : n, S)) return n;
	else return -1;
}

void TMenuStrings::Delete(int Index)
{
	DeleteMenu(hMenu, MF_BYPOSITION, Index);
}

void TMenuStrings::Clear(void)
{
	for(int x = GetMenuItemCount(hMenu); x >= 0; x--) DeleteMenu(hMenu, x, MF_BYPOSITION);
}

int TMenuStrings::GetCount(void)
{
	return GetMenuItemCount(hMenu);
}

MENUITEMINFO MII = {
sizeof(MENUITEMINFO),//    UINT    cbSize;
MIIM_TYPE,// | MIIM_STRING, //    UINT    fMask;
MFT_STRING, //    UINT    fType;          // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
MFS_ENABLED,//    UINT    fState;         // used if MIIM_STATE
0,//    UINT    wID;            // used if MIIM_ID
0,//    HMENU   hSubMenu;       // used if MIIM_SUBMENU
0,//    HBITMAP hbmpChecked;    // used if MIIM_CHECKMARKS
0,//    HBITMAP hbmpUnchecked;  // used if MIIM_CHECKMARKS
0,//    DWORD   dwItemData;     // used if MIIM_DATA
0,//    LPSTR   dwTypeData;     // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
0//    UINT    cch;            // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
//0//    HBITMAP hbmpItem;       // used if MIIM_BITMAP
};

void TMenuStrings::Insert(int Index, const TCHAR * S)
{
	MII.dwTypeData = (TCHAR *) S;
	MII.cch = _tcslen(S);
	InsertMenuItem(hMenu, Index, true, &MII);
}

TCHAR * TMenuStrings::get_Strings(int Index)
{
	MII.dwTypeData = 0;
	if(GetMenuItemInfo(hMenu, Index, true, &MII))
	if(int l = MII.cch)
	{
		TCHAR * r = (TCHAR *) malloc((l + 1) * sizeof(TCHAR));
		MII.dwTypeData = r;
		if(GetMenuItemInfo(hMenu, Index, true, &MII)) return r;
		else free(r);
	}
	return 0;
}

void TMenuStrings::set_Strings(int Index, TCHAR * String)
{
	MII.dwTypeData = String;
	MII.cch = _tcslen(String);
	SetMenuItemInfo(hMenu, Index, true, &MII);
}

int TMenuStrings::get_State(int Index)
{
	return GetMenuState(hMenu, Index, MF_BYPOSITION);
}

int TMenuStrings::get_State(void * Object)
{
	return GetMenuState(hMenu, (UINT) Object, MF_BYCOMMAND);
}

void * __cdecl TMenuStrings::ShowPopupMenu(int x, int y, HWND hWnd)
{
	if(hWnd) ClientToScreen(hWnd, (POINT *) &x);
	return (void *) TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, x, y, 0, hWnd, 0);
}

}