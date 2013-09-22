#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "Logs.h"
#include <psapi.h>

char OutString[500];
char * LogMessage = OutString + 26;
char LogName[MAX_PATH + 1] = "";
HANDLE LogHandle = 0;
char * ApplicationName = "Application";

void OpenLog(void)
{
	unsigned long l;
	HANDLE LocHandle = CreateFile(LogName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(LocHandle == INVALID_HANDLE_VALUE)
	{
		int e = GetLastError();
		char * tlm = new char[strlen(LogMessage) + 1];
		strcpy(tlm, LogMessage);
		sprintf(LogMessage, "Error creating log file '%s' (Error No: %d).\n", LogName, e);
		if(!LogHandle)
		{
			LogName[0] = 0;
			AddLogName();
			LogHandle = CreateFile(LogName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
			if(LogHandle != INVALID_HANDLE_VALUE)
			{
				SetFilePointer(LogHandle, 0, 0, FILE_END);
				WriteFile(LogHandle, "\n", 1, &l, 0);
			}
		}
		AddMessage();
		strcpy(LogMessage, tlm);
		delete tlm;
		return;
	}
	if(LogHandle && (LogHandle != INVALID_HANDLE_VALUE)) CloseHandle(LogHandle); // Если лог был уже открыт, то закрыть.

	LogHandle = LocHandle;

	SetFilePointer(LogHandle, 0, 0, FILE_END);
	WriteFile(LogHandle, "\n", 1, &l, 0);
}


void AddMessage(void)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(OutString, "[%.2d.%.2d.%.4d %.2d:%.2d:%.2d.%.3d]", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	LogMessage[-1] = 32;
	//printf(OutString);
	if(LogHandle == 0) OpenLog(); // Если лог не открывался, то открыть.

	if(LogHandle && (LogHandle != INVALID_HANDLE_VALUE))
	{
		unsigned long l = strlen(OutString);
		WriteFile(LogHandle, OutString, l, &l, 0);
	}
}

void AddBSTR(wchar_t * String)
{
	if(LogHandle == 0) OpenLog(); // Если лог не открывался, то открыть.
	unsigned long l = wcslen(String) + 1;
	char * s = (char *) malloc(l);
	WideCharToMultiByte(CP_ACP, 0, String, l, s, l, 0, 0);
	WriteFile(LogHandle, s, l - 1, &l, 0);
	free(s);
}

void AddChars(char * String)
{
	if(LogHandle == 0) OpenLog(); // Если лог не открывался, то открыть.
	unsigned long l = strlen(String);
	WriteFile(LogHandle, String, l, &l, 0);
}

void AddLogName(void)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(LogName + strlen(LogName), "%s_%.4d%.2d%.2d.log", ApplicationName, st.wYear, st.wMonth, st.wDay);
}

void AddTimeExtToLogName(char * Last)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(Last, "_%.4d%.2d%.2d.log", st.wYear, st.wMonth, st.wDay);
}

HMODULE GetModuleByAddress(void * Address) // Psapi.lib
{
#define mq 32
	HMODULE * Modules = (HMODULE *) malloc(mq * sizeof(HMODULE));
	unsigned long Needed;
	HANDLE Process = GetCurrentProcess();
	if(!EnumProcessModules(Process, Modules, mq * sizeof(HMODULE), &Needed)) {free(Modules); return 0;}
	{
		if(Needed > mq * sizeof(HMODULE))
		{
			Modules = (HMODULE *) realloc(Modules, Needed);
			if(!EnumProcessModules(Process, Modules, Needed, &Needed)) {free(Modules); return 0;}
		}
		HMODULE * Stop = (HMODULE *)((int)Modules + Needed);
		for(HMODULE * Module = Modules; Module < Stop; Module++)
		{
			MODULEINFO Info;
			GetModuleInformation(Process, *Module, &Info, sizeof(Info));
			if((Info.lpBaseOfDll <= Address) && ((unsigned long)Info.lpBaseOfDll + Info.SizeOfImage > (unsigned long)Address))
			{
				HMODULE Module = *Modules;
				free(Modules);
				return Module;
			}
		}
	}
	free(Modules);
	return 0;
#undef mq
}


int GetModuleNameByAddr(void * Address, char * FileName, int Length) // Psapi.lib
{
#define mq 32
	HMODULE * Modules = (HMODULE *) malloc(mq * sizeof(HMODULE));
	unsigned long Needed;
	HANDLE Process = GetCurrentProcess();
	if(!EnumProcessModules(Process, Modules, mq * sizeof(HMODULE), &Needed)) {free(Modules); return 0;}
	{
		if(Needed > mq * sizeof(HMODULE))
		{
			Modules = (HMODULE *) realloc(Modules, Needed);
			if(!EnumProcessModules(Process, Modules, Needed, &Needed)) {free(Modules); return 0;}
		}
		HMODULE * Stop = (HMODULE *)((int)Modules + Needed);
		for(HMODULE * Module = Modules; Module < Stop; Module++)
		{
			MODULEINFO Info;
			GetModuleInformation(Process, *Module, &Info, sizeof(Info));
			if((Info.lpBaseOfDll <= Address) && ((unsigned long)Info.lpBaseOfDll + Info.SizeOfImage > (unsigned long)Address))
			{
				int l = GetModuleFileName(*Module, FileName, Length);
				free(Modules);
				return l;
			}
		}
	}
	free(Modules);
	return 0;
#undef mq
}

bool CreateDefLogName(void)
{
#define mq 32
	HMODULE * Modules = (HMODULE *) malloc(mq * sizeof(HMODULE));
	unsigned long Needed;
	HANDLE Process = GetCurrentProcess();
	if(!EnumProcessModules(Process, Modules, mq * sizeof(HMODULE), &Needed)) {free(Modules); return false;}
	{
		if(Needed > mq * sizeof(HMODULE))
		{
			Modules = (HMODULE *) realloc(Modules, Needed);
			if(!EnumProcessModules(Process, Modules, Needed, &Needed)) {free(Modules); return false;}
		}
		HMODULE * Stop = (HMODULE *)((int)Modules + Needed);
		for(HMODULE * Module = Modules; Module < Stop; Module++)
		{
			MODULEINFO Info;
			GetModuleInformation(Process, *Module, &Info, sizeof(Info));
			if((Info.lpBaseOfDll <= CreateDefLogName) && ((unsigned long)Info.lpBaseOfDll + Info.SizeOfImage > (unsigned long)CreateDefLogName))
			{
				int l = GetModuleFileName(*Module, LogName, sizeof(LogName));
				free(Modules);
				if(!l) return false;
				SYSTEMTIME st;
				GetLocalTime(&st);
				for(char * x = LogName + l - 1; (x > LogName) && (*x != '.') && (*x != '\\'); x--);
				if(*x == '\\') x = LogName + l;
				sprintf(x, "_%.4d%.2d%.2d.log", st.wYear, st.wMonth, st.wDay);
				return true;
			}
		}
	}
	free(Modules);
	return false;
}