//! Описания оконных визуальных компонентов
/** \file
 	Файл содержит определения классов оконных визуальных компонентов
	без определённой специализации.
*/
#ifndef _WinControls_
#define _WinControls_
#include <Windows.h>
#include <commctrl.h>
#include "Controls.h"
#include "Classes.h"
#include <stdlib.h>
#include <tchar.h>

namespace StdGUI {

class TStubManager
{
private:
	inline void InitializeStub32(unsigned char * Stub, void * Object, void * Handler);
	unsigned char * FPages;
	int FAlign;
public:
	TStubManager(int Align = 128): FAlign(Align), FPages(0){};
	~TStubManager(void);
	void * NewStub(void * Object, void * Handler);
	void Release(void * Stub);
};

//! Компонент-окно без детей и собственного оконного класса
class TWinControl: public TControl
{
private:
	static TStubManager FStubManager;
	void * FStub;
protected:
	WNDPROC FDefWindowProc;
	LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INITCOMMONCONTROLSEX Ficc;
	static inline BOOL InitCC(DWORD c) {if(Ficc.dwICC & c) return TRUE; Ficc.dwICC |= c; return InitCommonControlsEx(&Ficc);};
	HWND hWindow;
	friend TWinParentControl;
	HINSTANCE hInstance;
	virtual void OnSetRect(RECT * Rect); // Уведомление от родительских компонентов
	virtual void Paint(PAINTSTRUCT * PaintStruct){}; // Уведомление об отрисовке
	void _TWinControl(TParentControl * Parent, RECT * Rect, int Layout);
	TWinControl(void);
	virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam) {return 0;};//!< WM_COMMAND
	virtual LRESULT OnNotify(NMHDR * Header) //!< WM_NOTIFY
	{
		if(Header->code == NM_RETURN && OnReturn.Assigned()) return OnReturn(this);
		return 0;
	};
public:
	inline void ReplaceWindowProc(void) 
	{ 
		if(!FDefWindowProc && hWindow)
		{
			FDefWindowProc = (WNDPROC) GetWindowLongPtr(hWindow, GWLP_WNDPROC);
			SetWindowLong(hWindow, GWLP_WNDPROC, (LONG)FStub);
		}
	}
	TNotifyEvent OnReturn;//!< При нажатии Enter. Прототип: bool __delcall OnType(TControl * Sender)
	TNotifyEvent OnChar;//!< При WM_CHAR. Прототип: bool __delcall OnChar(TControl * Sender, WPARAM &wParam, LPARAM &lParam)
	virtual bool LoadFromResource(TParentControl * Parent, int Id, int Layout = LT_ANLEFT | LT_ANTOP);
	virtual HWND GetWindowHandle(void) {return hWindow;};
	inline HINSTANCE GetInstance(void) {return hInstance;};
	inline void set_Text(const TCHAR * Text) {SetWindowText(hWindow, Text);};
	inline int get_TextLength(void) {return GetWindowTextLength(hWindow);};
	inline TCHAR * get_Text(void) // Строки следует уничтожать
	{
		int l = GetWindowTextLength(hWindow);
		if(!l++) return 0;
		TCHAR * r = (TCHAR *) malloc(l * sizeof(TCHAR));
		GetWindowText(hWindow, r, l);
		return r;
	}
	inline void AppendText(TCHAR * Text, int Size = 0)
	{
		if(!Size) Size = _tcslen(Text);
		int l = GetWindowTextLength(hWindow);
		TCHAR * r = (TCHAR *) malloc((l + Size + 1) * sizeof(TCHAR));
		GetWindowText(hWindow, r, l + 1);
		memcpy(r + l, Text, Size);
		r[l + Size] = 0;
		SetWindowText(hWindow, r);
		free(r);
	}
	virtual void set_Font(HFONT Font) {FFont = Font; if(hWindow) SendMessage(hWindow, WM_SETFONT, (WPARAM)Font, 0);};
	__declspec(property(get = get_Text, put = set_Text)) TCHAR * Text;
	inline void set_Enabled(BOOL Enabled) {EnableWindow(hWindow, Enabled);};
	inline BOOL get_Enabled(void) { return IsWindowEnabled(hWindow);};
	__declspec(property(get = get_Enabled, put = set_Enabled)) BOOL Enabled;
	virtual ~TWinControl(void);
};

typedef struct _COMMANDREC
{
	TNotifyEvent Event; 
	UINT Identifier;
} COMMANDREC;

//! Компонент-окно с детьми
class TWinParentControl: public TParentControl
{
private:
	friend TWinControl;
	friend TControl;
	void * FStub;
	static TStubManager FStubManager;
	TWinControl * * FWinChilds;
	TControl * FFocusedChild;
	int FWinChildCount;
	COMMANDREC * FCommands;
	int FCommandCount;
	friend LRESULT CALLBACK TWinParentControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void SetFocusedChild(TControl * Focused);
protected:
	virtual bool ParentSetRect(RECT * Rect); // Уведомление от родительских компонентов
	virtual LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void Paint(PAINTSTRUCT * PaintStruct){}; // Уведомление об отрисовке
	virtual void GetClientRct(RECT * Rect);
	long (CALLBACK * FDefWindowProc)(HWND, UINT, WPARAM, LPARAM);
	HWND hWindow;
	HINSTANCE hInstance;
	virtual TWinParentControl * GetWinParent(void) {return this;}; // Получить FWinParent для детей данного компонента
	virtual void AddChild(TControl * Child);
	virtual void OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) {if(FFocusedChild) FFocusedChild->OnKeyEvent(uMsg, wParam, lParam);};
	virtual void OnInitDialog(void){};
	TWinParentControl(void);
	void _TWinParentControl(TParentControl * Parent, RECT * Rect, int Layout);
public:
	TNotifyEvent OnDestroy; 
	TNotifyEvent OnClose; // По WM_CLOSE, если возвращает 0, то оставить по умолчанию.
	TNotifyEvent * Commands(UINT Identifier);
	void SetCommandsTable(COMMANDREC * Commands, int Count);
	inline HINSTANCE GetInstance(void) {return hInstance;};
	virtual HWND GetWindowHandle(void) {return hWindow;};
	TWinParentControl(TParentControl * Parent, RECT * Rect, int Layout);
	inline LRESULT SendMsg(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0) {return SendMessage(hWindow, Msg, wParam, lParam);};
	inline void set_Text(const TCHAR * Text) {SetWindowText(hWindow, Text);};
	virtual ~TWinParentControl(void);
};

//! Компонент - окно приложения с детьми.
/*! При создании экземпляра этого класса, увеличивается счётчик FAppWinCount. При уничтожении - уменьшается. 
    Если при этом FAppWinCount достиг 0, выполняется PosQuitMessage(0).*/
class TAppWindow: public TWinParentControl
{
private:
	static TCHAR FClassName[];
	static ATOM FClassAtom;
	static int FAppWinCount;
	static bool PMWorking;
	friend int ProcessMessages(void);
	virtual LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
	static WNDCLASSEX FWindowClass;
	TAppWindow(HINSTANCE hInstance, const TCHAR * Caption, HMENU hMenu = 0, int Left = CW_USEDEFAULT, int Top = CW_USEDEFAULT, int Width = CW_USEDEFAULT, int Height = CW_USEDEFAULT);
	void Show(int nCmdShow);
	TNotifyEvent OnClose;
	virtual ~TAppWindow(void);
};

//! Компонент-всплывающее окно с детьми
class TPopupWindow: public TWinParentControl
{
private:
	static TCHAR FClassName[];
	static ATOM FClassAtom;
	static WNDCLASSEX FWindowClass;
public:
	TPopupWindow(TControl * Parent, int Left, int Top, int Width, int Height);
	void _TPopupWindow(TControl * Parent, LPCTSTR Template);
	TPopupWindow(void);
	void Show(int nCmdShow);
};

//! Окно диалога с детьми
class TDialog: public TWinParentControl
{
private:
	static TCHAR FClassName[];
	static ATOM FClassAtom;
	static WNDCLASSEX FWindowClass;
public:
	//TDialogWindow(TControl * Parent, int Left, int Top, int Width, int Height);
	void _TDialog(TControl * Parent, LPCTSTR Template);
	TDialog(void);
	void Show(int nCmdShow);
	int ShowModal(int nCmdShow);
};

//! Окно встраиваемого диалога с детьми
class TEmbeddedDialog: public TWinParentControl
{
private:
	static TCHAR FClassName[];
	static ATOM FClassAtom;
	static WNDCLASSEX FWindowClass;
protected:
	virtual void Invalidate(RECT * OldRect);
	//virtual void OnSetRect(RECT * Rect); // Уведомление от родительских компонентов
public:
	//TDialogWindow(TControl * Parent, int Left, int Top, int Width, int Height);
	//void _TEmbeddedDialog(TControl * Parent, LPCTSTR Template);
	TEmbeddedDialog(TParentControl * Parent, RECT * Rect, int Layout, LPCTSTR Template);
	void Show(int nCmdShow);
};

//! Окно модального диалога с детьми
class TModalDialog: public TWinParentControl
{
protected:
	LPCTSTR FTemplate;
	virtual void OnInitDialog(void){};
	HWND FParentHandle;
public:
	//void _TModalDialog(TControl * Parent, LPCTSTR Template);
	TModalDialog(TControl * Parent, LPCTSTR Template):TWinParentControl(0, 0, 0) {FTemplate = Template; FParentHandle = Parent ? Parent->GetWindowHandle() : 0;};
	INT_PTR Show(int nCmdShow);
};

//! Окно, предоставляющее область для дочерних окон пользовательского интерфейса MDI
class TMDIClient: public TWinControl
{
protected:
//	virtual void AddChild(TControl * Child) {};
	virtual void OnSetRect(RECT * Rect); // Уведомление от родительских компонентов
	virtual void OnPaint(PAINTSTRUCT * PaintStruct) {}; ///< Процедура отрисовки компонента
	virtual TWinParentControl * GetWinParent(void) {return 0;}; // Получить FWinParent для детей данного компонента
public:
	TMDIClient(TParentControl * Parent, RECT * Rect, int Layout);
};

//! Дочернее окно документа в MDI с детьми.
class TMDIChildWindow: public TWinParentControl
{
public:
	TMDIChildWindow(TMDIClient * MDIClient, TCHAR * Name, int Left = CW_USEDEFAULT, int Top = CW_USEDEFAULT, int Width = CW_USEDEFAULT, int Height = CW_USEDEFAULT);
	void Show(int nCmdShow);
};

//! Стандартная кнопка с оконным классом 'BUTTON'
class TButton: public TWinControl
{
protected:
	virtual void Invalidate(RECT * OldRect);
	virtual void OnPaint(PAINTSTRUCT * PaintStruct) {}; ///< Процедура отрисовки компонента
	virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);// WM_COMMAND
public:
	TButton(TParentControl * Parent, RECT * Rect, int Layout, TCHAR * Caption, int ButtonStyle = BS_PUSHBUTTON);
	TButton(void) {};
	TNotifyEvent OnClick;
	int get_State(void) {return (int)SendMessage(hWindow, BM_GETSTATE, 0, 0);};
	void set_State(int State) {SendMessage(hWindow, BM_SETCHECK, State, 0);};
	__declspec(property(get = get_State, put = set_State)) int State;
};

//! Стандартная панель с кнопками
class TReBar: public TWinControl
{
protected:
	virtual void Invalidate(RECT * OldRect);
	virtual void OnPaint(PAINTSTRUCT * PaintStruct) {}; ///< Процедура отрисовки компонента
	virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);// WM_COMMAND
	virtual LRESULT OnNotify(NMHDR * Header);//!< WM_NOTIFY
public:
	TReBar(TParentControl * Parent);
	TReBar(void) {};
	TNotifyEvent OnClick;
};

class TUpDown: public TWinControl
{
protected:
	virtual void Invalidate(RECT * OldRect);
	virtual void OnPaint(PAINTSTRUCT * PaintStruct) {}; // Процедура отрисовки компонента
	virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);// WM_COMMAND
public:
	TUpDown(TParentControl * Parent, RECT * Rect, int Layout);
	TUpDown(void) {};
	TNotifyEvent OnChange;
};


class TEdit: public TWinControl
{
protected:
	virtual void Invalidate(RECT * OldRect);
	virtual void OnPaint(PAINTSTRUCT * PaintStruct) {}; // Процедура отрисовки компонента
	virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);// WM_COMMAND
public:
	inline BOOL get_ReadOnly(void){	return GetWindowLong(hWindow, GWL_STYLE) & ES_READONLY;};
	inline void set_ReadOnly(BOOL ro) { SendMessage(hWindow, EM_SETREADONLY, ro, 0);};
	__declspec(property(get = get_ReadOnly, put = set_ReadOnly)) BOOL ReadOnly;
	TEdit(TParentControl * Parent, RECT * Rect, int Layout, DWORD EditStyle = 0);
	TEdit(void) {};
	TNotifyEvent OnChange;
};

class TStatusBar: public TWinControl
{
protected:
	virtual void Invalidate(RECT * OldRect);
	virtual void OnPaint(PAINTSTRUCT * PaintStruct) {}; // Процедура отрисовки компонента
public:
	inline void set_Simple(BOOL s) { SendMessage(hWindow, SB_SIMPLE, s, 0);};
	inline BOOL get_Simple(void) { return SendMessage(hWindow, SB_ISSIMPLE, 0, 0);};
	__declspec(property(get = get_Simple, put = set_Simple)) BOOL Simple;
	TStatusBar(TWinParentControl * Parent, TCHAR * Caption);
	inline BOOL SetParts(int Count, const int * Coords) { return SendMessage(hWindow, SB_SETPARTS, Count, (LPARAM)Coords);};
	inline TCHAR * get_Texts(int i)
	{
		int l = SendMessage(hWindow, SB_GETTEXTLENGTH, i, 0);
		if(!l++) return 0;
		TCHAR * r = (TCHAR *) malloc(l * sizeof(TCHAR));
		SendMessage(hWindow, SB_GETTEXT, i, (LPARAM)r);
		return r;
	}
	inline void set_Texts(int i, const TCHAR * t) { SendMessage(hWindow, SB_SETTEXT, i, (LPARAM)t);};
	__declspec(property(get = get_Texts, put = set_Texts)) TCHAR * Texts[];
};

class TListBoxStrings: public TStrings
{
private:
	HWND hWindow;
	friend class TListBox;
public:
	virtual int Add(const TCHAR * S, void * Object = 0);
	virtual void Delete(int Index);
	virtual void Clear(void);
	virtual int GetCount(void);
	virtual int IndexOf(char * S);
	virtual void Insert(int Index, const TCHAR * S);
	virtual TCHAR * get_Strings(int Index);
	virtual void set_Strings(int Index, TCHAR * String);
};

class TListBox: public TWinControl
{
private:
	int FItemHeight, FOCW;
protected:
	virtual void Paint(PAINTSTRUCT * PaintStruct); // Уведомление об отрисовке
	virtual void OnPaint(PAINTSTRUCT * PaintStruct) {}; // Процедура отрисовки компонента
	virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);// WM_COMMAND
	virtual void Invalidate(RECT * OldRect);
public:
	TListBoxStrings Items;
	TStrings * get_Items(void);
	int get_ItemIndex(void);
	void set_ItemIndex(int Index);
	__declspec(property(get = get_ItemIndex, put = set_ItemIndex)) int ItemIndex;
	TListBox(TParentControl * Parent, RECT * Rect, int Layout, int Style = 0);
	TListBox(void){ FOCW = 0; FItemHeight = 0;}
	TNotifyEvent OnChange;
};

class TComboBoxStrings: public TStrings
{
private:
	HWND hWindow;
	friend class TComboBox;
public:
	virtual int Add(const TCHAR * S, void * Object = 0);
	virtual void Delete(int Index);
	virtual void Clear(void);
	virtual int GetCount(void);
	virtual int IndexOf(char * S);
	virtual void Insert(int Index, const TCHAR * S);
	virtual TCHAR * get_Strings(int Index);
	virtual void set_Strings(int Index, TCHAR * String);
};

class TComboBox: public TWinControl
{
protected:
	//virtual void Paint(PAINTSTRUCT * PaintStruct); // Уведомление об отрисовке
	virtual void OnPaint(PAINTSTRUCT * PaintStruct) {}; // Процедура отрисовки компонента
	virtual void Invalidate(RECT * OldRect);
	virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);// WM_COMMAND
	//virtual bool ParentSetRect(RECT * Rect); // Уведомление от родительских компонентов
public:
	TComboBoxStrings Items;
	//virtual bool LoadFromResource(TParentControl * Parent, int Id, int Layout = 0);
	TStrings * get_Items(void);
	int get_ItemIndex(void);
	void set_ItemIndex(int Index);
	__declspec(property(get = get_ItemIndex, put = set_ItemIndex)) int ItemIndex;
	TComboBox(void) {};
	TComboBox(TParentControl * Parent, RECT * Rect, int Layout);
	TNotifyEvent OnChange;
};

//! Компонент-окно с полосами прокрутки и автоматическим сдвигом клиентской области по ним.
class TScrollingWinControl: public TWinControl
{
private:
	static TCHAR TSWName[];
	static ATOM TSWClass;
	static WNDCLASSEX TSWClassEx;	
	void * FStub;
	static TStubManager FStubManager;
	friend LRESULT CALLBACK TScrollingWinControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
protected:
	bool FFocused;
	void SetRange(int Bar, int Max, int Min = 0);
	void ClientToLocal(int * x, int * y);
	void LocalToClient(int * x, int * y);
	int FScrollUnits[2];
	SCROLLINFO FScrolls[2];// Горизонтально, вертикально
	virtual int OnTimer(WPARAM wParam, LPARAM lParam) {return 0;};
	virtual bool ScrollQuery(int Bar, int Req, POINT * Offset);
	void OnScroll(int Bar, int Req);
	virtual void OnSetRect(RECT * Rect); // Уведомление от родительских компонентов
	RECT FFixed; // Фиксированные границы
	RECT FClient;
	void _TScrollingWinControl(TParentControl * Parent, RECT * Rect, int Layout, UINT WS_EX = 0);
	TScrollingWinControl(void);
	virtual void OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) {};
	void SetScroll(int Bar, int Pos);
	virtual ~TScrollingWinControl(void);
};


class TCustomGrid: public TScrollingWinControl
{
private:
	int FColPos;
	int FColNum;
protected:
	enum _SM {
		SM_AROW		= 0x01, ///< Разрешено выделять ряд
		SM_ACOL		= 0x02, ///< Разрешено выделять колонку
		SM_ACOLROW	= 0x03,
		SM_SROW		= 0x04, ///< Ряд выделен
		SM_SCOL		= 0x08, ///< Колонка выделена

		SM_SHOW		= 0x10 ///< Показывать выделение
	} FSelMode;
	bool FUseOnDrawCell;
	HWND hEditor;
	SIZE FCharSize;
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // Процедура отрисовки компонента
	virtual void OnMouseEvent(int x, int y, int mk, UINT uMsg); // Процедура реакции на действия мыши
	virtual TCHAR * OnDrawCell(int Col, int Row, RECT * Rect, HDC dc) {return 0;};
	bool GetCellRect(DWORD Col, DWORD Row, RECT * Rect, _SM Mode = SM_ACOLROW);
	bool CoordsToCell(int x, int y, DWORD * Col, DWORD * Row);
	//void CoordsToCell(int * x, int * y);
	bool RectToRange(const RECT * Rect, RECT * Range);
	DWORD FSelRow, FSelCol;
	void ShowEditor(int Col, int Row, TCHAR * Data);
	virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void OnSetText(char * Data) {free(Data);}; // Процедура должна уничтожать значение
	virtual void OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void SetSelection(DWORD Col, DWORD Row);
	virtual void OnFocus(bool Focused);
public:
	DWORD FRows, FCols;
	DWORD FTxtCol, FSelTxtCol;
	virtual void set_Font(HFONT Font);
	__declspec(property(get = FFont, put = set_Font)) HFONT Font;
	static DWORD FDefaultTxtColor, FDefaultSelTxtColor;
	void SetCols(DWORD Cols);
	void SetRows(DWORD Rows);
	TCHAR * * FFields;
	HBRUSH FCellsBrush, FSelBrush, FBlurBrush;
	static HBRUSH FDefaultCellsBrush, FDefaultSelBrush, FDefaultBlurBrush;
	DWORD FFixedRows, FFixedCols;
	int FVLineWidth, FHLineWidth;
	int * FColOffsets; // Первая колонка начинается с 0, вторая с FColOffsets[0] + FVLineWidth
	DWORD FDefRowHeight, FDefColWidth;
	TCustomGrid(TParentControl * Parent, RECT * Rect, int Layout);
	~TCustomGrid(void);
};

class THintWindow
{
private:
	HFONT FFont;
	RECT FScreen;
	void * FStub;
	static TStubManager FStubManager;
	HWND hWindow;
	friend LRESULT CALLBACK THintWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	TCHAR * FText;
	static TCHAR FClassName[];
	static ATOM FClassAtom;
	static LOGFONT FHintFont;
	static WNDCLASSEX FClass;
public:
	void Show(int nCmdShow) {ShowWindow(hWindow, nCmdShow);	UpdateWindow(hWindow);};
	void ShowText(int x, int y, TCHAR * Text);
	int Showed(void) {return (int) FText;};
	void Hide(void);
	inline HWND GetWindowHandle(void) {return hWindow;};
	TCHAR * Text;
	THintWindow(TWinParentControl * Parent);
	~THintWindow(void);
};

class TMenuStrings: public TStateStrings
{
private:
	HMENU hMenu;
public:
	TMenuStrings(void) {hMenu = CreatePopupMenu();};
	TMenuStrings(HMENU Menu) {hMenu = Menu;};
	~TMenuStrings(void) {DestroyMenu(hMenu);};
	virtual int Add(TCHAR * S, void * Object = 0);
	virtual int Add(TCHAR * S, int State, void * Object = 0);
	virtual void Delete(int Index);
	virtual void Clear(void);
	virtual int GetCount(void);
	virtual void Insert(int Index, const TCHAR * S);
	virtual TCHAR * get_Strings(int Index);
	virtual void set_Strings(int Index, TCHAR * String);
	void * __cdecl ShowPopupMenu(int x, int y, HWND hWnd = 0);
	virtual int get_State(int Index);
	virtual int get_State(void * Object);
};

}
#endif