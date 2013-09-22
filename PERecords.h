#ifndef _PERecords_
#define _PERecords_
#include <windows.h>
#include "binary.h"

#define PreDefRec(r) typedef struct _##r r;
#define StartRec(r) TFieldDesc FD_##r[]; typedef struct _##r {
#define EndRec(r) } r;
#define f(t, n) t n;		// Поле
#define fd(t, n, d) t n;	// Поле с описанием
#define fdt(t, n, d, o) t n;// Поле с описанием
#define p(n) void * n;		// Указатель
#define pd(n, d) void * n;	// Указатель с описанием
#define a(t, n, c) t n[c];	// Массив
#define ah a
#define r(n, c) char n[c];	// Reserved
#define pc(t, n) t * n;
#define pcd(t, n, d) pc(t, n)
#define ps(s, n) _##s * n;		// Указатель на структуру
#define psd(s, n, d) _##s * n;	// Указатель на структуру с описанием
#define psc(s, n) _##s * n; int n##_count; // Указатель и счётчик
#define pscd(s, n, d) _##s * n; int n##_count; // Указатель и счётчик

#include "PERecords.inc"

#undef PreDefRec
#undef StartRec
#undef EndRec
#undef f
#undef fd
#undef fdt
#undef p
#undef pd
#undef a
#undef ah
#undef r
#undef ps
#undef psd
#undef psc
#undef pscd
#undef pc
#undef pcd





















/*




RECFIELD RF_MZTITLE[]; typedef struct _MZTITLE {
	char Signature[2];
	unsigned short PartPag;
    unsigned short Page_count;
    unsigned short Relo_count;
    unsigned short Header_size;
    unsigned short Minimum_memory;
    unsigned short Maximum_memory;
	unsigned short ReloSS;
    unsigned short ExeSP;
    unsigned short Check_sum;
    unsigned short ExeIP;
    unsigned short ReloCS;
    unsigned short Table_offset;
    unsigned short Overlay;
    unsigned int Reserved[8];
	unsigned int PE_offset;
} MZTITLE;

RECFIELD RF_PETITLE[]; typedef struct _PETITLE {
    char Signature[4];
    unsigned short CPU_type;
    unsigned short Number_of_objects;
    unsigned int Date_time;
    unsigned int Reserved1[2];
    unsigned short NT_length;
    unsigned short Flags;
    unsigned char Reserved2[2];
    unsigned short Linker_version;
    unsigned int Reserved3[3];
    unsigned int Entrypoint_RVA;
    unsigned int Reserved4[2];
    unsigned int Image_base;
    unsigned int Object_align;
    unsigned int File_align;
    unsigned int OS_version;
    unsigned int User_version;
    unsigned int NT_version;
    unsigned int Reserved5[1];
    unsigned int Program_size;
    unsigned int Full_title_size;
    unsigned int Check_sum;
    unsigned short NT_sub_system;
    unsigned short DLL_flags;
    unsigned int Stack_size;
    unsigned int Minimal_stack_size;
    unsigned int Heap_size;
    unsigned int Minimal_heap_size;
    unsigned int Reserved6[1];
    unsigned int Array_size;
    char * Export_table_offset;
    unsigned int Export_table_size;
    char * Import_table_offset;
    unsigned int Import_table_size;
    char * Resource_table_offset;
    unsigned int Resource_table_size;
    unsigned int Exception_table_offset;
    unsigned int Exception_table_size;
    unsigned int WarrTabOffset;
    unsigned int WarrTabSize;
    char * AddrTabOffset;
    unsigned int AddrTabSize;
    char * Debug_table_offset;
    unsigned int Debug_table_size;
    unsigned int DescrTabOffset;
    unsigned int DescrTabSize;
    unsigned int MachSpecOffset;
    unsigned int MachSpecSize;
    unsigned int TLSOffset;
    unsigned int TLSSize;
    unsigned int LoadCFGOffset;
    unsigned int LoadCFGSize;
    unsigned int Reserved7[2];
    unsigned int IATOffset;
    unsigned int IATSize;
	unsigned int Reserved8[6];
} PETITLE;

RECFIELD RF_OBJECTTABLE[]; typedef struct _OBJECTTABLE {
    char Name[8];
    unsigned int Virtual_size;
    char * RVA;
    unsigned int Phisical_size;
    unsigned int Offset;
    unsigned int Reserved[3];
    unsigned int Flags;
} OBJECTTABLE;

RECFIELD RF_IMPORTDIRECTORYTABLE[]; typedef struct _IMPORTDIRECTORYTABLE {
    char * Lookup_offset;
    unsigned int Time_date_stamp;
    unsigned int Forward_chain;
    char * Library_name_offset;
    char * Address_table_offset;
} IMPORTDIRECTORYTABLE;

RECFIELD RF_EXPORTDIRECTORYTABLE[]; typedef struct _EXPORTDIRECTORYTABLE {
    unsigned int Flags;
    unsigned int Time_date_stamp;
    unsigned int Version;
    char * Name_RVA;
    unsigned int Ordinal_base;
    unsigned int Number_of_funcs;
    unsigned int Number_of_names;
    char * Address_table_offset;
    char * Name_pointers_RVA;
    char * Ordinal_table_RVA;
} EXPORTDIRECTORYTABLE;

RECFIELD RF_EXPORTEDENTRY[]; typedef struct _EXPORTEDENTRY {
	char * Name;
	unsigned int Ordinal;
	char * Address;
} EXPORTEDENTRY;

typedef struct _IMPORTEDLIB;

RECFIELD RF_IMPORTEDENTRY[]; typedef struct _IMPORTEDENTRY {
	char * Name;
	unsigned int Ordinal;
	char * Address;
	_IMPORTEDLIB * Library;
} IMPORTEDENTRY;

RECFIELD RF_IMPORTEDLIB[]; typedef struct _IMPORTEDLIB {
	char * Name;
	_IMPORTEDENTRY * Entries; int Entries_count;
} IMPORTEDLIB;








*/










#endif
