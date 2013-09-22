#ifndef _PEFiles_
#define _PEFiles_
#include "PERecords.h"
#include "binary.h"

namespace PE{

/// ����������� ����� ������ Portable Executable
/// ����� ������������� ������������� ������ ������ Portable Executable
class TPESource: public TDumpSource
{
private:
	void * FDataBlock;
protected:
	inline void * RVA(void * Pointer, void * * End = 0)
	{
		ReleaseData(FDataBlock);
		DWORD Size = 0x1000, Type = 0;
		FDataBlock = GetData((void *)((DWORD)Pointer + Image_base), &Size, &Type);
		if(End) *End = (void *)((DWORD)FDataBlock + (Size > 0x1000 ? 0x1000 : Size));
		return FDataBlock;
	};
	DWORD Image_base;
public:
	// ���������
	MZHEADER * MZHeader;			//!< ��������� MZ
	PEHEADER * PEHeader;			//!< ��������� PE
	// �������
	OBJECTTABLE * FSections;		//!< ������� ��������
	int FSectionCount;				//!< ���������� ��������
	// ������
	IMPORTEDENTRY * FImported;		//!< ������ ������������� �������
	int FImportedCount;				//!< ����� ������������� �������
	IMPORTEDLIB * FImportedLibs;	//!< ������ ������������� ���������
	int FImportedLibCount;			//!< ����� ������������� ���������
	bool ReadImports(void);			//!< ����� ��� ���������� ����� �������
	// �������
	char * FExportName;				//!< �������������� ��� ����������
	EXPORTEDENTRY * FExported;		//!< ������ �������������� �������
	int FExportedCount;				//!< ���������� �������������� �������
	bool ReadExports(void);			//!< ����� ���������� ����� ��������

	TPESource(void);				//!< ����������� ������ �������� ��� ����
	virtual void Clear(void);
	virtual ~TPESource(void);		//!< ���������� ������ ���������� ���� ������� � ��������
};

/// ����� ��� ������ � ������� Portable Executable, � ��������� �� �����
class TPEFile: public TPESource
{
private:
	char * LoadFromFile(const char * FileName);
	char * * FSectData;
	char * FFileData; // ��������� �����
	DWORD FFileSize;
public:
	virtual void * GetData(void * VA, DWORD * Size, DWORD * Type, void * Seg);//!< ����� ��������� ����� ������
	virtual void Clear(void);
	char * SaveToFile(const char * FileName);
	TPEFile(const char * FileName);
	virtual ~TPEFile(void);
};

/// ����� ��� ������ � ������� PortableExecutable, ������� ��� �������� � ������ �������� ������������
class TPELocalModule: public TPESource
{
public:
	virtual void * GetData(void * VA, DWORD * Size, DWORD * Type, void * Seg);//!< ����� ��������� ����� ������
	TPELocalModule(void * ImageBase);
	void * PatchImport(void * NewAddress, char * Name, char * Library = 0);
	//virtual ~TPELocalModule(void);
};


	/// �����, ������������ ����� � ����������� ������ �� ��� RVA.
	/*inline void * RVA(void * Pointer)
	{
		if(OBJECTTABLE * Table = GetSection(Pointer))
			return (DWORD)Pointer - (DWORD)Table->RVA + FSectData[Table - FSections];
		return 0;
	}

	/// �����, ������������ ����� � ����������� ������ �� ��� RVA, ����� ���������� ��������� �� ����� ������
	inline void * RVA(void * Pointer, void * &End)
	{
		if(OBJECTTABLE * Table = GetSection(Pointer))
		{
			End = FSectData[Table - FSections] + Table->Virtual_size;
			return (DWORD)Pointer - (DWORD)Table->RVA + FSectData[Table - FSections];
		}
		return 0;
	}*/


	/*inline OBJECTTABLE * GetSection(void * Pointer)
	{
		for(int x = 0; x < FSectionCount; x++)
			if((FSections[x].RVA <= Pointer) && ((DWORD)Pointer < (DWORD)FSections[x].RVA + FSections[x].Phisical_size))
				return FSections + x;
		return 0;
	}*/
}

#endif
