#include "Windows.h"
#include "WinControls.h"
#include <crtdbg.h>

namespace StdGUI {

bool WM_QUITReceived = false;
HACCEL hAT = 0;

TNotifyEvent OnWork;//!< "Квант" работы

int DefMessageLoop(HACCEL hAccelTable)
{
	MSG msg;
	hAT = hAccelTable;
	while(!WM_QUITReceived) 
	{
		if(OnWork && OnWork(0))
			while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				if(msg.message == WM_QUIT) break;
				if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		else
		{
			BOOL r = GetMessage(&msg, NULL, 0, 0);
			if(!r) break;
			if(r == -1) return -1;
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	return msg.wParam;
}


int ProcessMessages(void)
{
	_ASSERTE(!TAppWindow::PMWorking);
	TAppWindow::PMWorking = true;
	MSG msg;
	int r = 0;
	
	while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		r++;
		if(msg.message == WM_QUIT)
		{
			WM_QUITReceived = true;
			continue;
		}
		if(!TranslateAccelerator(msg.hwnd, hAT, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if(!TAppWindow::PMWorking) break;
	} 
	TAppWindow::PMWorking = false;
	return r;
}

}