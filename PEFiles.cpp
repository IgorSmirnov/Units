
#include "stdafx.h"
#include "PERecords.h"
#include <windows.h>
//#include "CErrors.h"
#include <stdio.h>
#include "PEFiles.h"
#define _CRT_SECURE_NO_DEPRECATE

/********************************* TPESource *****************************************************************/

namespace PE{

//! Конструктор должен обнулить все поля
TPESource::TPESource(void)
{				
	MZHeader = 0;
	PEHeader = 0;
	// Объекты
	FSections = 0;
	FSectionCount = 0;
	// Импорт
	FImported = 0;
	FImportedCount = 0;
	FImportedLibs = 0;
	FImportedLibCount = 0;
	// Экспорт
	FExportName = 0;
	FExported = 0;
	FExportedCount = 0;

	FDataBlock = 0;
	Image_base = 0;
}


TPESource::~TPESource(void)
{
	free(FImported);
	free(FImportedLibs);
	free(FExported);
	ReleaseData(FDataBlock);
}

void TPESource::Clear(void)
{
	free(FImported); 
	FImported = 0;
	FImportedCount = 0;
	free(FImportedLibs);
	FImportedLibs = 0;
	FImportedLibCount = 0;
	free(FExported);
	FExported = 0;
	FExportedCount = 0;
	ReleaseData(FDataBlock);
	FDataBlock = 0;
	MZHeader = 0;
	PEHeader = 0;
}

/// Метод извлечения из загруженных секций информации об импортируемых функциях
bool TPESource::ReadImports(void)
{
	if(!PEHeader) return false;
	IMPORTDIRECTORYTABLE * Import = (IMPORTDIRECTORYTABLE *)PEHeader->Import_table_offset;
	if(!Import) return false;
	void * End;
	int TabCtr = 0;

	IMPORTDIRECTORYTABLE * Table = (IMPORTDIRECTORYTABLE *)RVA(Import, &End);
	if(!Table) return false;
// Посчитаем количество импортируемых библиотек
	for(;Table->Library_name_offset; Table++)
	{
		if(Table >= End)
		{
			Table = (IMPORTDIRECTORYTABLE *)RVA(Import + TabCtr, &End);
			if(!Table) return false;
		}
		TabCtr++;
	}
	FImportedLibs = (IMPORTEDLIB *)malloc((TabCtr + 1) * sizeof(IMPORTEDLIB));
	FImportedLibs[TabCtr].Name = 0;
	FImportedLibCount = TabCtr;
	TabCtr = 0;
	int EntCtr = 0;
// Заполним имена библиотек и посчитаем число функций
	for(Table = (IMPORTDIRECTORYTABLE *)RVA(Import, &End); Table->Library_name_offset; Table++)
	{
		if(Table >= End)
			Table = (IMPORTDIRECTORYTABLE *)RVA(Import + TabCtr, &End);
		FImportedLibs[TabCtr].Name = (char *)RVA(Table->Library_name_offset);

		void * * Address = (void * *)RVA(Table->Lookup_offset ? Table->Lookup_offset : Table->Address_table_offset);
		void * * a = Address;
		if(a) for(; *a; a++);
		EntCtr += FImportedLibs[TabCtr].Entries_count = a - Address;
		TabCtr++;
	}
	FImported = (IMPORTEDENTRY *) malloc((EntCtr + 1) * sizeof(IMPORTEDENTRY));
	memset(FImported, 0, (EntCtr + 1) * sizeof(IMPORTEDENTRY));
	FImportedCount = EntCtr;

	TabCtr = 0;
	IMPORTEDENTRY * Imp = FImported;

	for(Table = (IMPORTDIRECTORYTABLE *)RVA(Import, &End); Table->Library_name_offset; Table++)
	{
		if(Table >= End)
			Table = (IMPORTDIRECTORYTABLE *)RVA(Import + TabCtr, &End);
		void * * Address = (void * *) Table->Address_table_offset;
		FImportedLibs[TabCtr].Entries = Imp;
		char * * Names = (char * *)RVA(Table->Lookup_offset);
		if(!Names) Names = (char * *)RVA(Address);
		if(!Names) continue;
		Address = (void * *) ((int)Address + (int)Image_base);
		for( ; *Names; Names++)
		{
			Imp->Address = Address++;
			Imp->Library = FImportedLibs + TabCtr;
			if(char * Name = (char *)RVA(*Names))
			{
				//Hint не нужен// Imp->Ordinal = *(WORD *)Name;
				Imp->Name = Name + 2;
			}
			else
				Imp->Ordinal = (WORD)*Names;
			Imp++;
		}
		TabCtr++;
	}
	return true;
}

/// Метод извлечения из загруженных секций информации об экпортируемых функциях
bool TPESource::ReadExports(void)
{
	free(FExported);
	FExported = 0;
	FExportedCount = 0;
	if(!PEHeader) return false;
	EXPORTDIRECTORYTABLE * Export = (EXPORTDIRECTORYTABLE *) PEHeader->Export_table_offset;
	if(!Export) return false;
	EXPORTDIRECTORYTABLE * Table = (EXPORTDIRECTORYTABLE *)RVA(Export);
	if(!Table) return false;
	char * FExportName = (char *)RVA(Table->Name_RVA);
	if(!FExportName || !*FExportName) return false;
	char * * pFuncNames = (char * *)RVA(Table->Name_pointers_RVA);
	int ExportedCount = Table->Number_of_funcs;
	EXPORTEDENTRY * Exported = (EXPORTEDENTRY *) malloc(sizeof(EXPORTEDENTRY) * (ExportedCount));
// Извлекаем адреса функций
	for(int x = 0; x < ExportedCount; x++)
	{
		Exported[x].Ordinal = x + Table->Ordinal_base;
		void * * Addr = (void * *)RVA((void * *)Table->Address_table_offset + x);
		Exported[x].Address = Addr ? *Addr : 0;
		Exported[x].Name = 0;
	}
// Извлекаем имена функций
	int NameCount = Table->Number_of_names;
	for(int x = 0; x < NameCount; x++)
	{
		WORD * Ord = (WORD *)RVA((WORD *)Table->Ordinal_table_RVA + x);
		if(!Ord || (*Ord >= ExportedCount)) { free(Exported); return false;}
		Exported[*Ord].Name = (char *)RVA(*pFuncNames);
		pFuncNames++;
	}
// Фильтруем от пустых
	EXPORTEDENTRY * Dst = Exported;
	for(int x = 0; x < ExportedCount; x++) 
		if(Exported[x].Address || Exported[x].Name)
		{
			*Dst = Exported[x];
			(Dst++)->Address = (void *)((DWORD)Dst->Address + Image_base);
		}

	FExported = (EXPORTEDENTRY *)realloc(Exported, (int)Dst - (int)Exported);
	FExportedCount = Dst - Exported;
	return true;
}	


/********************************* TPEFile *******************************************************************/


TPEFile::TPEFile(const char * FileName)
{
	FFileSize = 0;
	FFileData = 0;
	FSectData = 0;
	if(FileName) LoadFromFile(FileName);
}

TPEFile::~TPEFile(void)
{
	free(FSectData);
	free(FFileData);
}

void TPEFile::Clear(void)
{
	TPESource::Clear();
	/*if(FSectData)
	{
		for(int x = 0; x < FSectionCount; x++)
			free(FSectData[x]);*/
	free(FSectData);
	FSectData = 0;
	//}
	//free(FSections);
	free(FFileData);
	FFileData = 0;
	FFileSize = 0;

	FExportName = 0;
	FSectionCount = 0;
	FSections = 0;
}

char ErrMessage[256];

#define ADD_MESSAGE { sprintf(ErrMessage, 
#define HALT ); return ErrMessage;}

char * TPEFile::SaveToFile(const char * FileName)
{
	HANDLE f = CreateFile(FileName, FILE_WRITE_DATA, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(f == INVALID_HANDLE_VALUE)
		ADD_MESSAGE "Ошибка открытия %s (Код %d)", FileName, GetLastError() HALT
	DWORD t;
	if(!WriteFile(f, FFileData, FFileSize, &t, 0) || (t != FFileSize))
		ADD_MESSAGE "Ошибка чтения %s (Код %d)", FileName, GetLastError() HALT
	CloseHandle(f);
	return 0;
}

char * TPEFile::LoadFromFile(const char * FileName)
{
	Clear();

//Прочитать файл
	void * f = CreateFile(FileName, FILE_READ_DATA, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(f == INVALID_HANDLE_VALUE)
		ADD_MESSAGE "Ошибка открытия %s (Код %d)", FileName, GetLastError() HALT
	FFileSize = GetFileSize(f, 0);
	FFileData = (char *) malloc(FFileSize);
	DWORD t;
	if(!ReadFile(f, FFileData, FFileSize, &t, 0) || (t != FFileSize))
		ADD_MESSAGE "Ошибка чтения %s (Код %d)", FileName, GetLastError() HALT
	CloseHandle(f);

//Загрузить заголовок MZ

	MZHeader = (MZHEADER *)FFileData;
	//DWORD l;
	//if(!ReadFile(f, &FMZHeader, sizeof(FMZHeader), &t, 0) || (t != sizeof(FMZHeader)))
	//	ADD_MESSAGE "Ошибка чтения MZ из %s (Код %d)", FileName, GetLastError() HALT

	int Signature = *(unsigned short *)&(MZHeader->Signature);
	if((Signature != 'MZ') && (Signature != 'ZM'))
		ADD_MESSAGE "Неверная сигнатура MZ в файле %s", FileName HALT

	//MZHeader = &FMZHeader;
	if(MZHeader->Table_offset < 0x40 || MZHeader->PE_offset + sizeof(PEHEADER) > FFileSize) return "Заголовок PE отсутствует.";

//Загрузить заголовок PE

	PEHeader = (PEHEADER *) (FFileData + MZHeader->PE_offset);
	//if(SetFilePointer(f, FMZHeader.PE_offset, 0, FILE_BEGIN) != FMZHeader.PE_offset) 
	//	{ CloseHandle(f); return "Ошибка установки позиции чтения";}

	//if(!ReadFile(f, &FPEHeader, sizeof(FPEHeader), &t, 0) || (t != sizeof(FPEHeader)))
	//	ADD_MESSAGE "Ошибка чтения PE из %s (Код %d)", FileName, GetLastError() HALT

    if(*(int *)&(PEHeader->Signature) != 0x4550) return "Неверная сигнатура PE.";
	//PEHeader = &FPEHeader;
	Image_base = PEHeader->Image_base;

// Загрузить заголовки секций
	if(!(FSectionCount = PEHeader->Number_of_objects))
		ADD_MESSAGE "Секции отсутствуют в %s.", FileName HALT
	FSections = (OBJECTTABLE *)(PEHeader + 1);// malloc(sizeof(OBJECTTABLE) * FSectionCount);

	//l = FSectionCount * sizeof(OBJECTTABLE);
	//if(!ReadFile(f, FSections, l, &t, 0) || (t != l))
	//	ADD_MESSAGE "Ошибка чтения секций из %s (Код %d)", FileName, GetLastError() HALT

// Нарастить секции по потребности


// Загрузить данные секций
	FSectData = (char * *)malloc(sizeof(char *) * FSectionCount);

	OBJECTTABLE * Section = FSections;
	for(int x = 0; x < FSectionCount; x++)
	{
		if(Section->Phisical_size > Section->Virtual_size)
			Section->Virtual_size = Section->Phisical_size;
		DWORD Size = Section->Virtual_size;

		FSectData[x] = FFileData + Section->Offset;// (char *) malloc(Size);
		DWORD pSize = Section->Phisical_size;
		/*if(Size > pSize)
		{
			memset(FSectData[x] + pSize, 0, Size - pSize);
			Size = pSize;
		}*/
		/*if(SetFilePointer(f, Section->Offset, 0, FILE_BEGIN) != Section->Offset) 
			{ CloseHandle(f); return "Ошибка позиционирования в файле при загрузке секций";}
		if(!ReadFile(f, FSectData[x], pSize, &t, 0) || (t != pSize))
			ADD_MESSAGE "Ошибка чтения секции из %s (Код %d)", FileName, GetLastError() HALT
		*/
		Section++;
	}
	//CloseHandle(f);
	ReadExports();
	ReadImports();

	return 0;
}


/// Метод получения блока данных
void * TPEFile::GetData(void * VA, DWORD * Size, DWORD * Type, void * Seg)
{
	*Type = 0;
	*Size = 0;
	if(!PEHeader) return 0;
	DWORD lSize = (DWORD)-1;
//	void * RVA = (void *)((DWORD)VA - Image_base);
	OBJECTTABLE * s = FSections;
	for(int x = 0; x < FSectionCount; x++)
	{
		if((DWORD)VA < (DWORD)s->RVA + s->Phisical_size + Image_base)
		{
			if((DWORD)s->RVA + Image_base <= (DWORD)VA)
			{
				DWORD Offset = (DWORD)VA - (DWORD)s->RVA - Image_base;
				*Size = s->Phisical_size - Offset;
				*Type = 1;
				return FSectData[x] + Offset;
			}
			else
			{
				DWORD Offset = (DWORD)s->RVA + Image_base - (DWORD)VA;
				if(lSize > Offset) lSize = Offset;
			}
		}
		s++;
	}
	if(lSize < (DWORD)-1) *Size = lSize;
	return 0;
}


/********************************* TPELocalModule *******************************************************************/

TPELocalModule::TPELocalModule(void * ImageBase)
{
	MZHeader = (MZHEADER *) ImageBase;
	int Signature = *(unsigned short *)&(MZHeader->Signature);
	if((Signature != 'MZ') && (Signature != 'ZM')) return;
	if(MZHeader->Table_offset < 0x40) return;
	PEHeader = (PEHEADER *)((DWORD)ImageBase + MZHeader->PE_offset);
    if(*(int *)&(PEHeader->Signature) != 0x4550) return;
	FSectionCount = PEHeader->Number_of_objects;
	FSections = (OBJECTTABLE *)(PEHeader + 1);
	Image_base = (DWORD)ImageBase;
}

/*TPELocalModule::~TPELocalModule(void)
{

}*/


void * TPELocalModule::GetData(void * VA, DWORD * Size, DWORD * Type, void * Seg)
{
	MEMORY_BASIC_INFORMATION MBI;
	if(VirtualQuery(VA, &MBI, sizeof(MBI)) != sizeof(MBI))
	{
		*Size = -1;
		*Type = 0;
		return 0;	
	};
	*Size = MBI.RegionSize - (DWORD)VA + (DWORD)MBI.BaseAddress;
	if(MBI.State == MEM_COMMIT)
	{
		*Type = MBI.Protect;
		return VA;
	}
	*Type = 0;
	return 0;
}


void * TPELocalModule::PatchImport(void * NewAddress, char * Name, char * Library)
{
	IMPORTEDENTRY * Entry = FImported;
	int l = FImportedCount;
	if(Library)
	{
		IMPORTEDLIB * Lib = FImportedLibs;
		int x;
		for(x = FImportedLibCount; x--; Lib++)
		if(!_stricmp(Lib->Name, Library))
		{
			Entry = Lib->Entries;
			l = Lib->Entries_count;
			break;
		}
		if(!x) return 0;
	}
	for(; l--; Entry++)
	if(!strcmp(Entry->Name, Name))
	{
		void * * Addr = (void * *)Entry->Address;
		MEMORY_BASIC_INFORMATION MBI;
		if(VirtualQuery(Addr, &MBI, sizeof(MBI)) != sizeof(MBI)) return 0;	
		if(MBI.State != MEM_COMMIT) return 0;
		DWORD OldProtect;
		if(!VirtualProtect(Addr, sizeof(*Addr), PAGE_READWRITE, &OldProtect)) return 0;
		void * Result = *Addr;
		*Addr = NewAddress;
		VirtualProtect(Addr, sizeof(*Addr), OldProtect, &OldProtect);
		return Result;
	}
	return 0;
}

}