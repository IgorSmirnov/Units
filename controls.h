//! �������� ������� ���������� �����������
/** \file
 	���� �������� ����������� ������� ���������� �����������, �� �������
	������ ���� � ����������� �������������.
*/
#ifndef _Controls_
#define _Controls_
#include <Windows.h>
#include "Classes.h"
#include <crtdbg.h>


namespace StdGUI {
/*************************************** ������ *************************************/

class TControl;///< ����� ����������� ���������, ������� �������������� �������
	class TParentControl;///< ����������� ���������, ������� ������� � �������� ����������
		class TSimplePanel;///< ���������� ������
		class TWinParentControl;///< ����������� ���������, ������� �������� ���������� � ����
			class TAppWindow;///< ������� ���� ����������.
	class TWinControl;///<  ����������� ���������, ������� ����

/************************************ ���� � ������ ***********************************
	RECT FRect;	- ���������� ���������� � ��������� ������� ������������� ����������-����
		(FWinParent ��� GWL_HWNDPARENT)


	virtual bool ParentSetRect(RECT * Rect); - ����������� �� ������������� ���������� � �����������.
		��������� ������ �������� ���� FRect � ������� ��������������� InvalidateRect.


******************************************************************************/

#define LT_ALLEFT	0x010 //!< ������� � ������ ����
#define LT_ALTOP	0x011 //!< ������� � �������� ����
#define LT_ALRIGHT	0x012 //!< ������� � ������� ����
#define LT_ALBOTTOM	0x013 //!< ������� � ������� ����

#define LT_SIZEIN	0x800 //!< ������ �� ���� ������������: ������ RECT - int ������.

#define LT_SALLEFT		0x010|LT_SIZEIN //!< ������� � ������ ����
#define LT_SALTOP		0x011|LT_SIZEIN //!< ������� � �������� ����
#define LT_SALRIGHT		0x012|LT_SIZEIN //!< ������� � ������� ����
#define LT_SALBOTTOM	0x013|LT_SIZEIN //!< ������� � ������� ����

#define LT_ALVERT	0x001 //!< ����������: ������� � ����� ��� � ����?
#define LT_ALOFFSET	0x003 //!< ���������� �������� � ��������� RECT �������, � ������� ������ ���������

#define LT_ANLEFT	0x021 //!< ����� �����
#define LT_ANTOP	0x022 //!< ����� ������
#define LT_ANRIGHT	0x024 //!< ����� ������
#define LT_ANBOTTOM	0x028 //!< ����� �����

#define LT_ALIGN	0x010 //!< �����: ������� � ����
#define LT_ANCHORS	0x020 //!< �����: �����
#define LT_ALCLIENT	0x030 //!< �����: ������������ �� ��������� ������������
#define LT_MODE		0x030 //!< ���������� �����

#define LT_SCALE	0x040 //!< �������������� ���������� ����������� �������� ��� ��������� ���������������� ������������
#define LT_BSCALE	0x080 //!< ����������� LT_SCALE �������� �������������
#define LT_CWIDTH	0x100 //!< ��������� ������ ����������
#define LT_LAYOUT   0x03F //!< �������� ��������� ���������� ��� ������ ����������

/// ������� ����� ��� ���� ����������� �����������.
class TControl
{
public:
	TNotifyEvent OnMouse;
protected:
	int FLayout;  ///< ��������� �������� �� ���������� ������� ��������
	int FSegment; ///< ��������� ������� ���������� � ������� FFreeClient �������� � ������ ������� ������� << 16
	RECT FRect;	///< ���������� ���������� � ���������� ������� FWinParent
	RECT FOldRect; ///< ���������� �� ������ ParentSetRect
	RECT FConstraints;
	TControl * FNext; ///< ��������� � ������� ������ ��������
	TControl * FPrev; ///< ���������� � ������� ������ ��������
	TParentControl * FParent;	///< ������������ ���������
	TWinParentControl * FWinParent; ///< ��������� ������������ ���������, ���������� �����

	virtual bool ParentSetRect(RECT * Rect); /*! ����������� �� ������������ ����������� �� �������� �������.
	��� ������ ����� ������, ��������� ������ �������� FRect, � ������� Invalidate */
	void ChildSetSize(RECT * Rect); // ������� ����� �������� ���� �������
	virtual void OnSetRect(RECT * Rect) {}; ///< ����������� ��� ����������� �������

	virtual void Invalidate(RECT * OldRect);
	virtual void OnPaint(PAINTSTRUCT * PaintStruct) = 0; ///< ��������� ��������� ����������.
	virtual void OnMouseEvent(int x, int y, int mk, UINT uMsg) { if(OnMouse.Assigned()) OnMouse(this, x, y, mk, uMsg);}; ///< ��������� ��������� ������� ����
	virtual void OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) {};
	virtual void OnFocus(bool Focused) {};
	virtual void MakeFocused(void);
	virtual void MouseEvent(int x, int y, int mk, UINT uMsg); // ����������� � ��������
	virtual void Paint(PAINTSTRUCT * PaintStruct){ OnPaint(PaintStruct);}; // ����������� �� ���������
	friend TParentControl;
	friend TWinParentControl;
	void _TControl(TParentControl * Parent, RECT * Rect, int Layout);
	TControl(void);
	HFONT FFont;
	bool MoveInvalidate(RECT * OldRect, int Flags);
public:
	static HBRUSH FDefaultBackBrush; //< ��� ���������� ��������, ��� ����� ���������� ����� ������������ ���� ������
	inline int get_Layout(void) {return FLayout;};
	HBRUSH FBackBrush;
	DWORD FTextColor;
	static DWORD FDefaultTextColor;
	static HPEN FDefaultPen;
	HPEN FPen;
	virtual HWND GetWindowHandle(void); ///< ���������� ����� ���� ����������, ���� ����, �� ������� �� ���������
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

/// ������� ����� ��� ���� ����������� �����������, ����������� ��������� ������ ���� ������ ����������.
class TParentControl: public TControl
{
private:
	TControl * FMouseOwner;
	void RestoreFC(int Layout, RECT * Rect);
protected:
	TControl * FFirstChild;
	TControl * FLastChild;	
	RECT FChildConstraints, FPartConstraints;
	RECT FClient; ///< �������, ��������������� �������� �����������.
	RECT FBorders; ///< ������� ����������� ������ ����������.
	RECT FFreeClient; ///< ����� �������, ��������������� �������� �����������, �� ������� � ������ ������.
	bool FClientPresent; ///< ��������� - ���������� �� FFreeClient.
	virtual bool ParentSetRect(RECT * Rect); ///< ����������� �� ������������ �����������
	virtual void ChildSetSize(TControl * Child, RECT * Rect); // ����������� �� �������� �����������
	virtual void PaintChilds(PAINTSTRUCT * PaintStruct); // ����������� �� ��������� �����
	virtual void OnPaint(PAINTSTRUCT * PaintStruct);
	virtual void Paint(PAINTSTRUCT * PaintStruct) {PaintChilds(PaintStruct); OnPaint(PaintStruct);}; // ����������� �� ���������
	virtual void GetClientRct(RECT * Rect);
	virtual void MouseEvent(int x, int y, int mk, UINT uMsg);
	friend TControl;
	virtual void AddChild(TControl * Child);
	virtual void Invalidate(RECT * OldRect);
	virtual TWinParentControl * GetWinParent(void) {return FWinParent;}; // �������� FWinParent ��� ����� ������� ����������
	bool Align(TControl * Child, RECT * Old, bool Move);
	void Align(TControl * Child, int Layout, bool MoveChilds, RECT * ChildRect); // ��������� Child � ����, �������� �� FFreeClient
	void ReAlign(TControl * First);
	void _TParentControl(TParentControl * Parent, RECT * Rect, int Layout); ///< �����������������. ����������� ��������� ������ � ������������ ��� ���������� ������������� �������.
	TParentControl(void); ///< ��������� �����������. ��� ������ ������������� ������� ���������� ��������� _TParentControl
public:
	/// ����� �������� ��������� ���������� �� ���������� �������. 
	/*!�������� ���������� ������ ���������� �������� � ���� ����������� � ������� ����� ������, � Destroy = true,
	���� ���������� ��� �� ������.*/
	virtual void RemoveChild(TControl * Child/*!����������� �������� ���������*/, bool Destroy/*!<������� ���������� � ���������� ������*/); 
	virtual ~TParentControl(void);
	inline bool GetFreeClient(RECT * Rect) {if(!FClientPresent) return false; *Rect = FFreeClient; return true;}
	virtual void get_Constraints(RECT * c);
};

/// ������� ������, �������������� ������� DrawEdge, ����������� ��������� ������ ���� ������ ����������.
class TSimplePanel: public TParentControl
{
protected:
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // ��������� ��������� ����������
public:
	UINT FEdgeType;///< ������� ��������� DrawEdge, ����������� ��� ���� ������.
	TSimplePanel(TParentControl * Parent, RECT * Rect, int Layout);
};

class TGroupBox: public TParentControl
{
private:
	virtual void Invalidate(RECT * OldRect);
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // ��������� ��������� ����������
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
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // ��������� ��������� ����������
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
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // ��������� ��������� ����������
	virtual void OnMouseEvent(int x, int y, int mk, UINT uMsg); // ��������� ������� �� ��������
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
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // ��������� ��������� ����������
	virtual bool ParentSetRect(RECT * Rect); // ����������� �� ������������ �����������
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
	char AutoResize; // ������� ��������������� �����������: 0 - NoResize, 1 - NoCopy, 2 - Copy, 3 - Stretch
	void * Bits;
	void * TImage::SetBitMapSize(int Width, int Height, int BitCount, int Mode = 0);
	inline int GetImageWidth(void) {return FWidth;};
	inline int GetImageHeight(void) {return FHeight;};
	inline void Fill(int Color) {memset(Bits, Color, (FBitCount >> 3) * FWidth * FHeight);}; //!< ������ �����
	TImage(TParentControl * Parent, RECT * Rect, int Layout);
	~TImage(void) {DeleteBitMap();};
	bool LoadFromBMP(TCHAR * FileName);
	void Update(void); //!< �������� �� ������
};

}
#endif
