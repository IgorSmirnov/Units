#ifndef _PEFiles_
#define _PEFiles_
#include "PERecords.h"
#include "binary.h"

namespace PE{

/// Абстрактный класс модуля Portable Executable
/// Класс предоставляет специфические данные модуля Portable Executable
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
	// Заголовки
	MZHEADER * MZHeader;			//!< Заголовок MZ
	PEHEADER * PEHeader;			//!< Заголовок PE
	// Объекты
	OBJECTTABLE * FSections;		//!< Таблица объектов
	int FSectionCount;				//!< Количество объектов
	// Импорт
	IMPORTEDENTRY * FImported;		//!< Список импортируемых функций
	int FImportedCount;				//!< Число импортируемых функций
	IMPORTEDLIB * FImportedLibs;	//!< Список импортируемых библиотек
	int FImportedLibCount;			//!< Число импортируемых библиотек
	bool ReadImports(void);			//!< Метод для заполнения полей импорта
	// Экспорт
	char * FExportName;				//!< Экспортируемое имя библиотеки
	EXPORTEDENTRY * FExported;		//!< Список экспортируемых функций
	int FExportedCount;				//!< Количество экспортируемых функций
	bool ReadExports(void);			//!< Метод заполнения полей экспорта

	TPESource(void);				//!< Конструктор должен обнулить все поля
	virtual void Clear(void);
	virtual ~TPESource(void);		//!< Деструктор должен уничтожить поля импорта и экспорта
};

/// Класс для работы с модулем Portable Executable, с загрузкой из файла
class TPEFile: public TPESource
{
private:
	char * LoadFromFile(const char * FileName);
	char * * FSectData;
	char * FFileData; // Контейнер файла
	DWORD FFileSize;
public:
	virtual void * GetData(void * VA, DWORD * Size, DWORD * Type, void * Seg);//!< Метод получения блока данных
	virtual void Clear(void);
	char * SaveToFile(const char * FileName);
	TPEFile(const char * FileName);
	virtual ~TPEFile(void);
};

/// Класс для работы с модулем PortableExecutable, который уже загружен в данное адресное пространство
class TPELocalModule: public TPESource
{
public:
	virtual void * GetData(void * VA, DWORD * Size, DWORD * Type, void * Seg);//!< Метод получения блока данных
	TPELocalModule(void * ImageBase);
	void * PatchImport(void * NewAddress, char * Name, char * Library = 0);
	//virtual ~TPELocalModule(void);
};


	/// Метод, возвращающий адрес в загруженной секции по его RVA.
	/*inline void * RVA(void * Pointer)
	{
		if(OBJECTTABLE * Table = GetSection(Pointer))
			return (DWORD)Pointer - (DWORD)Table->RVA + FSectData[Table - FSections];
		return 0;
	}

	/// Метод, возвращающий адрес в загруженной секции по его RVA, также возвращает указатель на конец секции
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
