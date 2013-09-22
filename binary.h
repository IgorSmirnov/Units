//! \file Binary.h Определения для доступа к двоичным данным
#ifndef _BINARY_
#define _BINARY_
#include <windows.h>
#include "..\units\classes.h"

/*! Функция преобразования целого числа любого размера в 
   шестнадцатеричную систему с добавлением завершающего нуля.*/
void IntToHex(const void * Source, int Digits, char * Destination);

/// Абстрактный класс источника двоичных данных.
class TDumpSource
{
public:
	//! Метод получения блока данных
	virtual void * GetData(
		void * VA,     //!< Запрашиваемый виртуальный адрес
		DWORD * Size,    /*!< Получает запрашиваемый размер блока, возвращает доступный размер
							блока однородных данных. При этом возвращаемый блок должен быть 
							не меньше наименьшего из запрашиваемого и доступного. */
		DWORD * Type,	   //!< Возвращает тип блока данных. Если 0 - то блок не доступен для обращения.
		void * Seg = 0 /*!< Дополнительный параметр, уточняющий виртуальный адрес: сегмент памяти,
							способ обращения (Offset, VA, RVA) или другое. */
	) = 0;
	virtual void ReleaseData(void * Data) {}; //!< Метод освобождения блока
	virtual ~TDumpSource() {};
};

//#define DT_NULL 	0x000	
#define DT_HEX  	0x001
#define DT_CHAR		0x002
#define DT_SIGNED	0x004	
#define DT_UNSIGNED 0x008	
#define DT_FLOAT 	0x010	
#define DT_FLAGS	0x020	
#define DT_ENUM		0x040	
#define DT_PRECORD	0x080	
#define DT_COUNTER	0x400


/// Базовый класс, описывающий одно поле данных, содержит методы для работы с этим данными.
class TFieldDesc
{
public:
	/// Имя поля
	char * Name;
	/// Размер поля в байтах
	int Size;
	/// Тип
	int Type;
	/// Указатель на описание структуры, если есть
	TFieldDesc * Record;
public:
//	virtual void set_String(char * String, void * Value){};

	/// Функция для преобразования значения поля в строку.
	/// Результат освобождать через free().
	virtual char * get_String(void * Value);

	/// Функция вывода значения на экран. 
	virtual void Draw(HDC dc, RECT * Rect, void * Value);

	/// Конструктор для создания поля - простого целого числа HEX
	inline TFieldDesc(char * Name, int Size, int Type = DT_HEX) {this->Name = Name; this->Size = Size; this->Type = Type;};

	/// Конструктор для создания поля - указателя на структуру
	inline TFieldDesc(char * Name, TFieldDesc * Record, int Type = DT_PRECORD) 
	{
		this->Name = Name; this->Record = Record; Size = sizeof(TFieldDesc *); this->Type = Type;
		if(Type & DT_COUNTER) Size += sizeof(int);
	};

	/// Конструктор для создания из класса-наследника. 
	/// Полное копирование необходимо для использования таблицы виртуальных функций наследника
	inline TFieldDesc(TFieldDesc &Desc) {memcpy(this, &Desc, sizeof(*this));};

	/// Конструктор для создания пустого(зарезервированниого) описания поля
	inline TFieldDesc(int Size) {Name = 0; this->Size = Size; Type = 0; Record = 0;};

	/// Конструктор для создания пустого(завершающего) описания поля.
	inline TFieldDesc(void)	{Name = 0; Size = 0; Type = 0; Record = 0;};
};

/// Класс, описывающий поле данных - массив
template<class T>
class TArrFieldDesc: public TFieldDesc
{
private:
	inline int slen(const T * s);
public:
//	virtual void set_String(char * String, void * Value);
	inline TArrFieldDesc(char * Name, int Size, int Type = DT_CHAR) {this->Name = Name; this->Size = Size; this->Type = Type;};
	/// Метод преобразования в hex или текст.
	virtual char * get_String(void * Value)
	{// <<50 45 00 00, 'PE'>>, <<50 45 00 00>> <<'PE'>>
		if(!Type) return 0;
		T * V = (T *) Value;
		T * End = (T *)((int)V + Size);
		int Len = 0;	// Полная длина результата
		int l = 0;		// Длина строки в символах
		if(Type & DT_CHAR) 
		{
			T * x;
			for(x = V; (x < End) && *x; x++);
			l = x - V;
			Len += 2 + l; // "'PE'"
		}
		if(Type & DT_HEX) 
		{
			if(Len) Len += 2; // ", " 
			Len += (1 + 2 * sizeof(T)) * Size - 1; // "50 45 00 00"
		}

		char * Result = (char *)malloc(Len + 1);
		char * r = Result;
		if(Type & DT_HEX)
		{
			for(T * x = V; x < End; x++)
			{
				IntToHex(x, 2 * sizeof(T), r);
				r += 2 * sizeof(T);
				*(r++) = ' ';
			}
			if(Type & DT_CHAR) { r[-1] = ','; *(r++) = ' ';}
		}
		if(Type & DT_CHAR)
		{
			*(r++) = '\'';
			if(sizeof(T) == 2) WideCharToMultiByte(0, 0, (wchar_t *)V, l, r, l, 0, 0);
			else memcpy(r, V, l);
			r += l;
			*(r++) = '\'';
		}
		*r = 0;
		return Result;
	}
};

/// Класс, описывающий поле данных - указатель на строку с завершающим нулём
template<class T>
class TPCharFieldDesc: public TFieldDesc
{
public:
//	virtual void set_String(char * String, void * Value);
	inline TPCharFieldDesc(char * Name, int Type = DT_CHAR) {this->Name = Name; this->Size = sizeof(T *); this->Type = Type;};
	/// Метод преобразования в текст.
	virtual char * get_String(void * Value)
	{
		if(!Type) return 0;
		T * V = *(T * *) Value;
		if(!V) return 0;
#ifdef _DEBUG
		if(int(V) == 0xCDCDCDCD)
		{
			char * Result = (char *)malloc(20);
			strcpy(Result, "CDCDCDCD error!");
			return Result;
		}
#endif // _DEBUG
		int l = StrLen(V);

		char * Result = (char *)malloc(l + 1);
		CopyString(Result, V, l);
		Result[l] = 0;
		return Result;
	}
};










#endif