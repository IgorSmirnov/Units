#include "D3DUtils.h"

/************************************** TD3DImage *********************************************/

void TD3DImage::OnPaint(PAINTSTRUCT * PaintStruct) // Процедура отрисовки компонента
{
	if(FD3DDevice) FD3DDevice->Present(0, &FRect, 0, 0);
	else TImage::OnPaint(PaintStruct);
}

TD3DImage::TD3DImage(TParentControl * Parent, RECT * Rect, int Layout, IDirect3D9 * Direct3D, UINT Adapter):TImage(Parent, Rect, Layout)
{
	FD3DDevice = 0;
	if(Direct3D) CreateDevice(Direct3D, Adapter);
}

void TD3DImage::CreateDevice(IDirect3D9 * Direct3D, UINT Adapter)
{
	D3DPRESENT_PARAMETERS Parameters;
	memset(&Parameters,0, sizeof(Parameters));
	Parameters.BackBufferWidth = GetWidth();
	Parameters.BackBufferHeight = GetHeight();
	Parameters.Windowed = true;
	Parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	Direct3D->CreateDevice(Adapter, D3DDEVTYPE_HAL, GetWindowHandle(), D3DCREATE_HARDWARE_VERTEXPROCESSING, &Parameters, &FD3DDevice);
}

IDirect3DDevice9 * TD3DImage::StartRender(D3DCOLOR Background)
{
	FD3DDevice->Clear(0, 0, D3DCLEAR_TARGET, Background, 1.0f, 0);
	FD3DDevice->BeginScene();
	return FD3DDevice;
}

void TD3DImage::EndRender(void)
{
	FD3DDevice->EndScene();
	FD3DDevice->Present(0, &FRect, 0, 0);
}

