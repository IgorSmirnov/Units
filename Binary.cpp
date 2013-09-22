#define _CRT_SECURE_NO_DEPRECATE
#include "stdafx.h"
#include "binary.h"
#include <stdio.h>

/// Преобразование числа в шестнадцатеричную цифру
#define HexDigit(k) (((k) > 9) ? ((k) + 'A' - 10) : ((k) + '0'))

void IntToHex(const void * Source, int Digits, char * Destination)
{// HHLL
	Destination += Digits;
	for(*Destination = 0; Digits--; Source = (const void *) (int(Source) + 1))
	{
		unsigned char b = *(unsigned char *)Source;
		unsigned char k = b & 0xF;
		*(--Destination) = HexDigit(k);
		if(!Digits--) return;
		k = b >> 4;
		*(--Destination) = HexDigit(k);
	}
}

/************************************ TFieldDesc ****************************************/

/// Базовая функция - преобразует в hex.
char * TFieldDesc::get_String(void * Value)
{
	char Buf[20];
	if(Type & DT_HEX)
	{
		int l = Size * 2;
		char * Result = (char *)malloc(l + 1);
		IntToHex(Value, l, Result);
		return Result;
	}
	if(Type & DT_UNSIGNED)
	{
		DWORD V = 0;
		memcpy(&V, Value, Size > sizeof(V) ? sizeof(V) : Size);
		int l = 1 + sprintf(Buf, "%u", V);
		char * Result = (char *)malloc(l);
		memcpy(Result, Buf, l);
		return Result;
	}
	return 0;
}

void TFieldDesc::Draw(HDC dc, RECT * Rect, void * Value)
{
	if(char * Res = get_String(Value))
	{
		DrawText(dc, Res, strlen(Res), Rect, DT_LEFT | DT_NOCLIP);
		free(Res);
	}
}
