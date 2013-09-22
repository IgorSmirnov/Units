#ifndef _D3DUtils_
#define _D3DUtils_
#include "Controls.h"
#include <d3d9.h>

class TD3DImage: public TImage
{
private:
	IDirect3DDevice9 * FD3DDevice;
//	virtual void Invalidate(RECT * OldRect);
protected:
	virtual void OnPaint(PAINTSTRUCT * PaintStruct); // Процедура отрисовки компонента
	//virtual bool ParentSetRect(RECT * Rect); // Уведомление от родительских компонентов
	//virtual void Invalidate(RECT * OldRect);
public:
	TD3DImage(TParentControl * Parent, RECT * Rect, int Layout, IDirect3D9 * Direct3D = 0, UINT Adapter = D3DADAPTER_DEFAULT);
	void CreateDevice(IDirect3D9 * Direct3D, UINT Adapter = D3DADAPTER_DEFAULT);
	IDirect3DDevice9 * StartRender(D3DCOLOR Background);
	void EndRender(void);
	~TD3DImage(void) {if(FD3DDevice) FD3DDevice->Release();};
};

#endif