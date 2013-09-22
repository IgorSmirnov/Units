//! \file StdGUI.h Подключает общие визуальные компоненты и DefMessageLoop
#ifndef _StdWinGUI_
#define _StdWinGUI_
#include "Classes.h"
#include "Controls.h"
#include "WinControls.h"

namespace StdGUI{

int DefMessageLoop(HACCEL hAccelTable);
int ProcessMessages(void);
extern TNotifyEvent OnWork;//!< "Квант" работы

template <class T>
inline void InitializeStub(unsigned char * Stub, void * Object, T Handler)// char Stub[12]
{
	Stub[0] = 0x58; // POP EAX
	Stub[1] = 0x68; // PUSH []
	*(int *)(Stub + 2) = (int) Object;
	Stub[6] = 0x50; // PUSH EAX
	Stub[7] = 0xE9; // JMP []
	*(int *)(Stub + 8) = *(int *)(&Handler) - int(Stub) - 12; 
}

}

#endif