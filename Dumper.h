#ifndef _DUMPER_
#define _DUMPER_
#include <stdio.h>
#include <windows.h>
#define CDumperTimeout 5000

typedef enum
{
        DUMP_SYSTEM_ERROR = 0,
        DUMP_FAILED = 1,
        DUMP_ERROR = 2,
        DUMP_WARNING = 3,
        DUMP_MESSAGE = 4,
        DUMP_INFO = 5,
		/*        
		LOG_STAGE_OPEN = 6,
        LOG_STAGE_CLOSE = 7,
        LOG_ENTER_DEFAULT = 8,
        LOG_RESET = 9,
        LOG_NONE  = 10
		*/
} DumperErrorCodes;


class CDumper
{
private:
#ifdef DUMPER_BY_API
	HANDLE File;
#else
	FILE * File;
	char * FileName;
#endif
	HANDLE Mutex;
public:
	int ThresholdLevel;// ???
	bool Log(int Level, const char * Format, ...);
	bool Clear(void);
	inline bool IsValid(void) {return File && Mutex;};
	CDumper(const char * DumpFileName, int ThresholdLevel = DUMP_INFO);
	~CDumper(void);
};

#ifdef _DEBUG

#define DUMPER_DECLARE(Name) CDumper * Name = 0;
#define DUMPER_CREATE(Name, File, Threshold) Name = new CDumper(File, Threshold);
#define DUMPER_CLEAR(Name) Name->Clear();
#define DUMPER_DESTROY(Name) delete Name;
#define DUMPER_LOG(Name) Name->Log

#else

#define DUMPER_DECLARE(Name)
#define DUMPER_CREATE(Name, File, Threshold)
#define DUMPER_CLEAR(Name)
#define DUMPER_DESTROY(Name)
#define DUMPER_LOG(Name) 1 ? (void)0 : 

#endif
//#define Dump(a, ...) Dumper->Dump(a, ...)





#endif