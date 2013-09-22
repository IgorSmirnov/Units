#include "Classes.h"
#include <malloc.h>
#include <string.h>
#include <tchar.h>

/*************************** TString и TDynString ***********************************/

int TString<char>::get_Len(void) {return strlen(Value);};
int TString<wchar_t>::get_Len(void) {return wcslen(Value);};

/******************************* TStrings ******************************************/

int TStrings::IndexOf(const TCHAR * S)
{
	for(int x = GetCount() - 1; x >= 0; x--)
	{
		TCHAR * c = get_Strings(x);
		if(!_tcscmp(S, c))
		{
			free(c);
			return x;
		}
		free(c);
	}
	return -1;
}

/**************************** Делегаты ********************************************/

int Buf[128];
int * B = Buf;
int A;

__declspec(naked) int __cdecl TNotifyEvent::operator () (void * Sender, ...)
{
#ifdef __delfast
	__asm {
		MOV		EAX, [ESP + 4]
		MOV		ECX, [EAX + 4]
		MOV		EAX, [EAX]
		TEST	ECX, ECX
		JZ		NoMethod

		MOV		A, EAX
		MOV		EAX, [B]
		POP		dword ptr [EAX]
		ADD		EAX, 4
		MOV		[B], EAX

		ADD		ESP, 4
		POP		EDX
		CALL	[A]
		SUB		ESP, 8

		MOV		EAX, B
		SUB		EAX, 4
		MOV		B, EAX
		JMP		dword ptr [EAX]
//////////////////////////////////////
NoMethod:
		MOV		A, EAX
		MOV		EAX, [B]
		POP		dword ptr [EAX]
		ADD		EAX, 4
		MOV		[B], EAX

		ADD		ESP, 4
		POP		ECX
		POP		EDX
		CALL	[A]
		SUB		ESP, 12
		MOV		EAX, B
		SUB		EAX, 4
		MOV		B, EAX
		JMP		dword ptr [EAX]
	}
#else
	__asm {
		MOV		EAX, [ESP + 4]
		MOV		ECX, [EAX + 4]
		MOV		EAX, [EAX]
		TEST	ECX, ECX
		JZ		NoMethod
		MOV		[ESP + 4], ECX
		JMP		EAX
//////////////////////////////////////
NoMethod:
		MOV		A, EAX
		MOV		EAX, [B]
		POP		dword ptr [EAX]
		ADD		EAX, 4
		MOV		[B], EAX

		ADD		ESP, 4
		CALL	[A]
		SUB		ESP, 4
		MOV		EDX, B
		SUB		EDX, 4
		MOV		B, EDX
		JMP		dword ptr [EDX]
	}
#endif
}

TNotifyEvent * AddHandler(TNotifyEvent * * Events)
{
	int x = 0;
	if(*Events)
	for( ; (*Events)[x].Handler; x++);
	*Events = (TNotifyEvent *) realloc(*Events, (x + 2) * sizeof(TNotifyEvent));
	(*Events)[x + 1].Handler = 0;
	return *Events + x;
}

/*__declspec(naked) void __delcall TNotifyEvent::operator () (void * Sender, ...)
{
	__asm {
#ifdef __delfast
		MOV		EAX, ECX
#else
		MOV		EAX, [ESP + 4]
#endif
		MOV		ECX, [EAX + 4]
		TEST	ECX, ECX
		JZ		NoMethod
#ifndef __delfast
		MOV		[ESP + 4], ECX
#endif
		JMP		dword ptr [EAX]
//////////////////////////////////////
NoMethod:
		MOV		ECX, [ESP]
		MOV		Buf, ECX
#ifdef __delfast
		MOV		ECX, EDX
		MOV		EDX, [ESP + 4]
#endif
		ADD		ESP, 8
		CALL	[EAX]
		SUB		ESP, 4
		JMP		dword ptr [Buf]
	}
}*/