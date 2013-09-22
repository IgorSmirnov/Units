

#include "Dumper.h"
#include <conio.h>
//#include "d:\progs\vss\astra\stdafx.h"

CDumper::CDumper(const char * DumpFileName, int ThresholdLevel)
{
	memset(this, 0, sizeof(*this));
	if(DumpFileName)
	{
		int l = strlen(DumpFileName) + 1;
		FileName = (char *)malloc(l);
		memcpy(FileName, DumpFileName, l);
	}
	this->ThresholdLevel = ThresholdLevel;

#ifdef DUMPER_BY_API
	File = CreateFile(DumpFileName, GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(File != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(File, 0, 0, FILE_END);
		Mutex = CreateMutex(0, false, 0);
	}
#else
	if(File = fopen(DumpFileName, "ba"))
	{
		fseek(File, 0, SEEK_END);
		Mutex = CreateMutex(0, false, "AstraDump");
	}
#endif
}

CDumper::~CDumper(void)
{
	free(FileName);
	if(Mutex)
	{
		if(WaitForSingleObject(Mutex, CDumperTimeout) == WAIT_OBJECT_0)
		{
			if(File) 
#ifdef DUMPER_BY_API
				CloseHandle(File);
#else
				fclose(File);
#endif
			File = 0;
			ReleaseMutex(Mutex);
		}
		CloseHandle(Mutex);
		return;
	}
	if(File) 
#ifdef DUMPER_BY_API
		CloseHandle(File);
#else
		fclose(File);
#endif
}

bool CDumper::Clear(void)
{
	if(File && Mutex && (WaitForSingleObject(Mutex, CDumperTimeout) == WAIT_OBJECT_0))
	{
		bool Result = false;
		__try
		{
			//fflush(File);
			//HANDLE f = (void *)_fileno(File);
#ifdef DUMPER_BY_API
			SetFilePointer(File, 0, 0, FILE_BEGIN);
#else
			fclose(File);
			File = fopen(FileName, "bw");
#endif
			Result = true;
		} 
		__finally
		{
			ReleaseMutex(Mutex);
			return Result;
		}
	}
	return false;
}

bool CDumper::Log(int Level, const char * Format, ...)
{
	if(Level > ThresholdLevel) return false;
	
	if(File && Mutex && (WaitForSingleObject(Mutex, CDumperTimeout) == WAIT_OBJECT_0))
	{
		bool Result = false;
		__try
		{
			va_list argptr;
			va_start(argptr, Format);


#ifdef DUMPER_BY_API
			char b[1024];
			if(int l = _vsnprintf(b, 1024, Format, argptr))
				WriteFile(File, b, (l > 0) ? l : 1024, (DWORD *)&l, 0);
#else
			vfprintf(File, Format, argptr);
			fflush(File);
#endif
			
			/*HANDLE t = GetStdHandle(STD_ERROR_HANDLE);
			SetStdHandle(STD_ERROR_HANDLE, File);
			_cprintf(Format, 0, 0, argptr);
			SetStdHandle(STD_ERROR_HANDLE, t);*/
			/*char * b = new char[l + 1];
			vsprintf(b, Format, argptr);
			WriteFile(File, b, l, &l, 0);
			delete b;*/

			va_end(argptr);
			Result = true;
		}
		__finally
		{
			ReleaseMutex(Mutex);
			return Result;
		}
	}
	return false;
}