#ifndef _XMLPARSER_
#define _XMLPARSER_
#include <malloc.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>

namespace TXML 
{

// Сообщения об ошибках:
TCHAR InvChar[];
TCHAR Ueof[];
TCHAR SFileOpenError[];


#ifndef TXML_OVERRIDE
#define _BICONNXML_ ///< Двусвязный список
#define _PARKNWXML_ ///< Запоминать родителя

#define _ALLOW_VIRTUAL_
#define TXML_TESTONCREATE
#define TXML_DYNSTR
#define TXML_TESTONCREATE
/* Режим динамических строк: 
	Все значения FName и FValue создаются с помощью malloc, очищаются с помощью free
   Режим статических строк:
    Копируются указатели на строки, не освобождаются
*/
#endif

#ifdef _ALLOW_VIRTUAL_
	#define xml_virtual virtual
	#define xml_static virtual
#else
	#define xml_virtual inline
	#define xml_static static inline
#endif


/* 
Разрешение использования таблиц виртуальных методов для
1. Всех деструкторов xml_virtual
2. Статических фунций вывода из парсера (xml_static): 
	AppendTextFromParser, 
	ExtractNameFromParser
	ExtractValueFromParser

*/


///////////////// Символьные сущности //////////////////////////////////////////

char Symbols[];

char    * SNames[];
wchar_t * WNames[];

int SSizes[];

///////////////// Функции обработки строк //////////////////////////////////////

inline int slen(const char    * S) { return strlen(S);}
inline int slen(const wchar_t * S) { return wcslen(S);}

inline int scmp(const char    * A, const char    * B) { return strcmp(A, B);}
inline int scmp(const wchar_t * A, const wchar_t * B) { return wcscmp(A, B);}

inline char    * scpy(const char    * A) { int l = (strlen(A) + 1) * sizeof(char)   ; char    * r = (char    *)malloc(l); memcpy(r, A, l); return r;}
inline wchar_t * scpy(const wchar_t * A) { int l = (wcslen(A) + 1) * sizeof(wchar_t); wchar_t * r = (wchar_t *)malloc(l); memcpy(r, A, l); return r;}

inline const char    * spos(const char    * S, const char    * Sub) { return strstr(S, Sub);}
inline const wchar_t * spos(const wchar_t * S, const wchar_t * Sub) { return wcsstr(S, Sub);}

inline const char    * sgetcommentend(const char    * S) { return strstr(S,  "-->");}
inline const wchar_t * sgetcommentend(const wchar_t * S) { return wcsstr(S, L"-->");}

inline void sadd(char    * &S, int n) {memcpy(S, SNames[n], SSizes[n] * sizeof(*S)); S += SSizes[n];};
inline void sadd(wchar_t * &S, int n) {memcpy(S, WNames[n], SSizes[n] * sizeof(*S)); S += SSizes[n];};

inline char sGetSymbolByName(const char * Name)
{
	for(int x = 0; SNames[x]; x++)
		if(!memcmp(SNames[x], Name, SSizes[x] * sizeof(*Name)))
			return Symbols[x];
	return 0;
}

inline wchar_t sGetSymbolByName(const wchar_t * Name)
{
	for(int x = 0; SNames[x]; x++)
		if(!memcmp(WNames[x], Name, SSizes[x] * sizeof(*Name)))
		{
			if(Name[SSizes[x]] != ';') continue;
			return Symbols[x];
		}
	return 0;
}

template <class _char>
bool spcmp(const _char * A, const _char * B, const _char * BStop)
{
	if(A < (const _char *)0xF) return true;
	while((*A == *B) && (B < BStop)) {A++; B++;}
	return (B == BStop) && !*A;
}


template <class _char>
int sToXML(_char * Destination, const _char * Source)
{
	if(!Source) return 0;
	_char * d = Destination;
	for(_char * a = (_char *) Source; *a; a++)
	{
		char * x;
		for(x = Symbols; *x; x++) if(*x == *a)
		{
			*(d++) = '&';
			sadd(d, x - Symbols);
			*(d++) = ';';
			break;
		}
		if(!*x) *(d++) = *a;
	}			
	return d - Destination;
}




//////////////// Классы и шаблоны //////////////////////////////////////////////////////////////

/*typedef enum _TEncoding {
	enNone, enUTF8, enWindows1251, enCP866;
} TEncoding;*/

template <class _char>
class TProcessingInstruction;

//*************************************************************************************************
//! Шаблон базового класса для всех тегов, атрибутов и прочих элементов документа XML.
//*************************************************************************************************
template <class _char>
class TNode {
protected:
// Статические служебные функции 
	static inline int GetNameSizeFromParser(const _char * Name, const _char * Stop) {return (int)Stop - (int)Name + 1;};
	static inline void WriteNameFromParser(_char * Dest, const _char * Name, const _char * Stop)
	{
		int l = (int)Stop - (int)Name;
		memcpy(Dest, Name, l);
		*(_char *)((int)Dest + l) = 0;
	};
	static inline int GetValueSizeFromParser(const _char * Value, const _char * Stop)
	{
		int l = Stop - Value + 1;
		const _char * b = 0;
		for(const _char * x = Value; x < Stop; x++)
			if(*x == '&') 
				b = x;
			else
				if(b && (*x == ';')) 
				{ 
					l -= x - b; 
					b = 0;
				};
		return b ? 0 : l;
	}

	static inline bool WriteValueFromParser(_char * Dest, const _char * Value, const _char * Stop)
	{
		_char * r = Dest;
		const _char * b = 0;
		for(const _char * x = Value; x < Stop; x++)
		{
			if(*x == '&') b = x + 1;
			else
			if(!b) *(r++) = *x;
			else
			if(*x == ';') 
			{
				if(!(*(r++) = sGetSymbolByName(b))) return false;
				b = 0;
			}
		}
		*r = 0;
		return !b;
	}

/// Вычисляет размер строки при сохранении её в XML
	template <class _char>
	static int sXMLlen(_char * S) 
	{
		if(!S) return 0;
		int Len = 0;
		_char * a;
		for(a = S; *a; a++)
			for(char * x = Symbols; *x; x++)
				if(*x == *a){ Len += SSizes[x - Symbols] + 1; break;}
		return Len + a - S;
	};

public:
	_char * FName; ///< Имя узла
	_char * FValue; ///< Значение узла

#ifdef TXML_DYNSTR
	inline void put_Value(const _char * Value)
	{
		free(FValue);
		if(!Value) FValue = 0;
		int l = slen(Value) + 1;
		FValue = (_char *) malloc(l);
		memcpy(FValue, Value, l);
	}
	inline const _char * get_Value(void) const {return this ? FValue : 0;};
	inline double get_ValueAsDouble(void) const {return (this && FValue) ? atof(FValue) : 0.0;};
	inline void put_ValueAsDouble(const double & Value)
	{
		free(FValue);
		int l = _scprintf("%f", Value) + 1;
		FValue = (_char *)malloc(l * sizeof(_char));
		if(sizeof(_char) > sizeof(_char))
			swprintf_s((wchar_t *)FValue, l, L"%f", Value);
		else
			sprintf_s((char *)FValue, l, "%f", Value);

	}
	inline void put_f(const _char * format, ...)
	{
		va_list args;
		va_start(args, format);
		free(FValue);
		if(sizeof(_char) > sizeof(_char))
		{
			int l = _vscwprintf((const wchar_t *)format, args) + 1;
			FValue = (_char *)malloc(l * sizeof(_char));
			vswprintf_s((wchar_t *)FValue, l, (const wchar_t *)format, args); 
		}
		else
		{
			int l = _vscprintf((const char *)format, args) + 1;
			FValue = (_char *)malloc(l * sizeof(_char));
			vsprintf_s((char *)FValue, l, (const char *)format, args); 
		}
	}
	inline void put_Name(const _char * Name)
	{
		free(FName);
		if(!Name) FName = 0;
		int l = slen(Name) + 1;
		FName = (_char *) malloc(l);
		memcpy(FName, Name, l);
	}
	inline const _char * get_Name(void) const {return FName;};
	__declspec(property(get = get_Name, put = put_Name)) const _char * Name;
	__declspec(property(get = get_Value, put = put_Value)) const _char * Value;
	__declspec(property(get = get_ValueAsDouble, put = put_ValueAsDouble)) double ValueAsDouble;
#else
	__declspec(property(get = FName, put = FName)) _char * Name;
	__declspec(property(get = FValue, put = FValue)) _char * Value;
	__declspec(property(get = get_ValueAsDouble)) double ValueAsDouble;
#endif

	inline operator const _char *(void) {return this ? FValue : 0;};
	inline const _char * operator = (const _char * &a) { _ASSERTE(this); put_Value(a); return a;};
	TNode * Next; ///< Следующий узел
	#ifdef _BICONNXML_
	TNode * Prev; ///< Предыдущий узел
	#endif
	#ifdef _PARKNWXML_
	TNode * Parent; ///< Родительский узел
	#endif
	//inline void set_Name(const _char * Name) { free((void *)this->Name); this->Name = Name; };
	//inline void set_Value(const _char * Value) { free((void *)this->Value); this->Value = Value; };
	xml_virtual ~TNode(void) {free((void *)FName); free((void *)FValue);}; //!< Деструктор освобождает строки.
};

//*************************************************************************************************
//! Шаблон класса атрибута элемента документа XML.
//*************************************************************************************************
template <class _char>
class TAttribute: public TNode<_char> 
{
public:
	inline TAttribute(void) 
	{
	#ifdef _ALLOW_VIRTUAL_
		FValue = 0;
		Next = 0;
		Prev = 0;
		Parent = 0;
		FName = 0;
	#else
		memset(this, 0, sizeof(*this)); 
	#endif
	};
	xml_virtual ~TAttribute(void) //<! Деструктор извлекает атрибут из родителя
	{
		if(Next) Next->Prev = Prev;
		else if(Parent) ((TAttributedNode<_char> *)Parent)->LastAttr = (TAttribute<_char> *)Prev;
		if(Prev) Prev->Next = Next;
		else if(Parent) ((TAttributedNode<_char> *)Parent)->FirstAttr = (TAttribute<_char> *)Next;
	}
};

//*************************************************************************************************
//!  Шаблон базового класса узла с атрибутами документа XML.
//*************************************************************************************************
template <class _char>
class TAttributedNode: public TNode<_char> 
{
protected:
	xml_static _char * ExtractNameFromParser(const _char * Name, const _char * Stop)
	{
#ifdef TXML_DYNSTR
		int l = (int)Stop - (int)Name;
		_char * Result = (_char *) malloc(l + sizeof(_char));
		memcpy(Result, Name, l);
		*(_char *)((int)Result + l) = 0;
		return Result;
#else
		*(char *)Stop = 0;
		return (char *)Name;
#endif
	};
	xml_static _char * ExtractValueFromParser(const _char * Value, const _char * Stop)
	{
		int l = Stop - Value + 1;
		const _char * b = 0;
		const _char * x;
		for(x = Value; x < Stop; x++)
			if(*x == '&') b = x;
			else
			if(b && (*x == ';')) { l -= x - b; b = 0;};

		if(b)
		{
	/*#ifdef _DEBUG
			printf("ExtractValueFromParser(): '&' without ';'.");
	#endif*/
		return 0;
		}

		_char * Result = (_char *) malloc(l * sizeof(_char));
		_char * r = Result;
		for(x = Value; x < Stop; x++)
		{
			if(*x == '&') b = x + 1;
			else
			if(!b) *(r++) = *x;
			else
			if(*x == ';') 
			{
				if(!(*(r++) = sGetSymbolByName(b)))
				{
/*#ifdef _DEBUG
					*x = 0;
					printf("ExtractValueFromParser(): Unknown entity &%s;", b);
#endif*/
					free((void *)Result);
					return 0;
				}
				b = 0;
			}
		}
		*r = 0;
/*#ifdef _DEBUG
		if(int x = (r - Result) - l + 1) printf("ExtractValueFromParser(): Wrong buffer size: %d\n", x);
#endif*/
		return Result;
	};

public:
	TAttribute<_char> * FirstAttr; ///< Первый атрибут
	#ifdef _BICONNXML_
	TAttribute<_char> * LastAttr;  ///< Последний атрибут
	#endif

/// Метод поиска атрибута узла по имени.
	inline TAttribute<_char> * get_Attribute(const _char * Name)
	{
		for(TNode<_char> * x = FirstAttr; x; x = x->Next)
			if(!scmp(x->Name, Name)) return (TAttribute<_char> *)x;
		return 0;
	}

	inline const TAttribute<_char> * get_Attribute(const _char * Name) const
	{
		for(TNode<_char> * x = FirstAttr; x; x = x->Next)
			if(!scmp(x->Name, Name)) return (TAttribute<_char> *)x;
		return 0;
	}

/// Метод добавления атрибута к узлу с указанием имени.
	xml_virtual TAttribute<_char> * AppendAttribute(const _char * Name)
	{
	#ifdef TXML_TESTONCREATE
		if(TAttribute<_char> * t = get_Attribute(Name)) return t;
	#endif

		TAttribute<_char> * Result = new TAttribute<_char>;
		Result->Name = Name;
	#ifdef _PARKNWXML_
		Result->Parent = this;
	#endif

	#ifdef _BICONNXML_
		if(LastAttr) 
		{
			LastAttr->Next = Result;
			Result->Prev = LastAttr;
			LastAttr = Result;
		}
		else LastAttr = FirstAttr = Result;
	#else
		if(FirstAttr)
		{
			Result->Next = FirstAttr;
			FirstAttr = Result;
		} else FirstAttr = Result;
	#endif
		return Result;
	}
	xml_virtual ~TAttributedNode(void) //!< Деструктор уничтожает все атрибуты
	{
		for(TAttribute<_char> * n, * x = FirstAttr; x; x = n)
		{
			n = (TAttribute<_char> *)x->Next;
			x->Parent = 0;
			x->Next = 0;
			x->Prev = 0;
			delete x;
		}
	}
};

template <class _char>
class TSchemeElement;

//*************************************************************************************************
//! Шаблон класса атрибута элемента документа XML
//*************************************************************************************************
template <class _char>
class TElement: public TAttributedNode<_char> 
{
protected:
	/// Уведомление о завершении чтения атрибутов
	xml_virtual TCHAR * AttributesCompleted(TElement<_char> * Element) { return 0;};

	/// Метод записи объекта в предоставленный буфер в формате XML
	void WriteXML(_char * &Destination, int Level = 0, const _char * Tab = 0)
	{
		if(!Name) { Destination += sToXML<_char>(Destination, Value); return; } // Просто текст
	// Заголовок
		_char * d = Destination;
		int tl;
		if(Tab)
		{
			*(d++) = 13;
			*(d++) = 10;
			tl = slen(Tab);
			for(int x = Level; x; x--)
			{
				memcpy(d, Tab, tl * sizeof(_char));
				d += tl;
			}
		}
		*(d++) = '<';
		int nl = slen(Name);
		memcpy(d, Name, nl * sizeof(_char)); d += nl;
	// Заливаем атрибуты
		for(TAttribute<_char> * a = FirstAttr; a; a = (TAttribute<_char> *) a->Next) 
		{
			*(d++) = ' ';
			int l = slen(a->Name);
			memcpy(d, a->Name, l * sizeof(_char)); d += l;
			*(d++) = '='; *(d++) = '"';
			d += sToXML(d, a->Value);
			*(d++) = '"';
		}

		if(FirstChild || Value)
		{
			*(d++) = '>';
			if(Value) d += sToXML(d, Value);
			// Рекурсивный алгоритм записи узлов
			for(TElement * x = FirstChild; x; x = (TElement<_char> *) x->Next) 
				x->WriteXML(d, Level + 1, Tab);
			if(Tab && FirstChild)
			{
				*(d++) = 13;
				*(d++) = 10;
				for(int x = Level; x; x--)
				{
					memcpy(d, Tab, tl * sizeof(_char));
					d += tl;
				}
			}
			*(d++) = '<';
			*(d++) = '/';
			memcpy(d, Name, nl * sizeof(_char)); d += nl;
			*(d++) = '>';
		}
		else
		{
			*(d++) = '/';
			*(d++) = '>';
		}
		Destination = d;
	};

/// Метод добавления текста в элемент. Дополнительно осуществяет замену сущностей и исключение комментариев
/// Возвращает элемент, который стал содержать добавленный текст.
	xml_virtual TElement<_char> * AppendTextFromParser(const _char * Text, const _char * Stop)
	{
	// Расчёт длины текста
		int l = Stop - Text + 1;
		const _char * b = 0;
		const _char * x;
		for(x = Text; x < Stop; x++)
			if(*x == '<') {const _char * t = x; x = sgetcommentend(x + 4) + 2; l -= x - t + 1; }
			else
			if(*x == '&') b = x;
			else
			if(b && (*x == ';')) { l -= x - b; b = 0;}
		if(b)
		{
/*	#ifdef _DEBUG
			printf("AppendTextFromParser(): '&' without ';'.");
	#endif*/
			return 0;
		}
	// Определение принимающего элемента
		TElement<_char> * TextElement = this;
		if(LastChild) 
		{
			if(LastChild->FName) TextElement = AppendChild(0);
			else TextElement = LastChild;
		} 
	// Создание строки для текста
		int v = TextElement->FValue ? slen(TextElement->FValue) : 0;
		TextElement->FValue = (_char *) realloc(TextElement->FValue, (l + v) * sizeof(_char));
		_char * r = (_char *)TextElement->FValue + v;
	// Заполнение строки
		for(x = Text; x < Stop; x++)
		{
			if(*x == '<') x = sgetcommentend(x + 4) + 2;
			else
			if(*x == '&') b = x + 1;
			else
			if(!b) *(r++) = *x;
			else
			if(*x == ';') 
			{
				if(!(*(r++) = sGetSymbolByName(b)))
				{
/*	#ifdef _DEBUG
					*x = 0;
					printf("AppendTextFromParser(): Unknown entity &%s;", b);
	#endif*/
					*r = 0;
					return 0;
				}
				b = 0;
			}
		}
		*r = 0;
		// Проверка и завершение
/*	#ifdef _DEBUG
		if(int x = (r - TextElement->Value) - v - l + 1) printf("AppendTextFromParser(): Wrong buffer size: %d\n", x);
	#endif*/
		return TextElement;
	};

public:
	TElement<_char> * FirstChild; ///< Первый дочерний элемент
	#ifdef _BICONNXML_
	TElement<_char> * LastChild;  ///< Последний дочерний элемент
	#endif

	inline TElement<_char> * get_Child(const _char * Name)
	{
		_ASSERTE(Name);
		for(TNode<_char> * x = FirstChild; x; x = x->Next) if(x->FName && !scmp(x->FName, Name)) return (TElement<_char> *)x;
		return 0;
	};

/// Метод добавления дочернего элемента с заданным именем
	xml_virtual TElement<_char> * AppendChild(const _char * Name)
	{
		TElement<_char> * Result = new TElement<_char>();
	#ifdef TXML_DYNSTR
		Result->FName = scpy(Name);
	#else
		Result->FName = (_char *)Name;
	#endif
	#ifdef _PARKNWXML_
		Result->Parent = this;
	#endif

	#ifdef _BICONNXML_
		if(LastChild) 
		{
			LastChild->Next = Result;
			Result->Prev = LastChild;
			LastChild = Result;
		}
		else LastChild = FirstChild = Result;
	#else
		if(FirstChild)
		{
			Result->Next = FirstChild;
			FirstChild = Result;
		} else FirstChild = Result;
	#endif
		return Result;
	};

/// Метод добавления текста в элемент. Возвращает элемент, который стал содержать добавленный текст.
	TElement<_char> * AppendText(const _char * Text)
	{
	// Определение принимающего элемента
		TElement<_char> * TextElement = this;
		if(LastChild) 
		{
			if(LastChild->Name) TextElement = AppendChild(0);
			else TextElement = LastChild;
		} 
	// Создание строки для текста
		int l = slen(Text) + 1;
		int v = TextElement->FValue ? slen(TextElement->FValue) : 0;
		TextElement->FValue = (_char *) realloc(TextElement->FValue, (l + v) * sizeof(_char));
		memcpy(TextElement->FValue + v, l);
		return TextElement;
	};

/// Метод возвращает объект в формате XML в динамическом буфере
	_char * get_XML(void)
	{
		int Size = get_XMLSize() + 1;
		_char * Result = (_char *)malloc(Size * sizeof(_char));
		_char * r = Result;
		WriteXML(r);
		*r = 0;
		return Result;
	};

/// Расчёт размера элемента в символах
	int get_XMLSize(int Level = 0, int TabLen = 0)
	{
		if(!Name) return sXMLlen(Value); // Просто текст
		int Size = slen(Name);
		if(FirstChild || FValue) Size = Size * 2 + 5 + sXMLlen(Value); // <Name>Value</Name>
		else Size += 3; // <Name/>
		for(TNode<_char> * a = FirstAttr; a; a = a->Next)
			Size += slen(a->Name) + sXMLlen(a->Value) + 4; // Кавычки, =, пробел
	// Рекурсивный алгоритм
		for(TElement<_char> * x = FirstChild; x; x = (TElement<_char> *) x->Next) 
			Size += x->get_XMLSize(Level + TabLen, TabLen);
		if(TabLen)
		{
			if(FirstChild && FirstChild->FName) Size += Level + 2;
			Size += Level + 2;
		}
		return Size;
	};

/// Конструктор.
	TElement(TElement<_char> * Parent, char * Name, char * Value = 0) {
	#ifdef _ALLOW_VIRTUAL_
		FirstAttr = 0;
		LastAttr = 0;
		Next = 0;
		Prev = 0;
		this->Value = Value;
		FirstChild = 0;
		LastChild = 0;
		this->Parent = 0;
		this->FName = Name;
	#else
		memset(this, 0, sizeof(*this));
	#endif
		SetParent(Parent);
	};	
	TElement(void) {
	#ifdef _ALLOW_VIRTUAL_
		FirstAttr = 0;
		LastAttr = 0;
		Next = 0;
		Prev = 0;
		FValue = 0;
		FirstChild = 0;
		LastChild = 0;
		Parent = 0;
		FName = 0;
	#else
		memset(this, 0, sizeof(*this));
	#endif
	};

/// Метод смены родителя
	void SetParent(TElement<_char> * Parent = 0)
	{
		if(this->Parent == Parent) return;
		if(this->Parent)
		{ // Разрываем список
			if(Next) Next->Prev = Prev;
			else ((TElement<_char> *)this->Parent)->LastChild = (TElement<_char> *)Prev;
			if(Prev) Prev->Next = Next;
			else ((TElement<_char> *)this->Parent)->FirstChild = (TElement<_char> *)Next;
		}
		Next = 0;
		Prev = 0;
		if(Parent)
		{
			if(TElement<_char> * Last = Parent->LastChild)
			{
				Last->Next = this;
				Prev = Last;
			}
			else
			{
				Parent->FirstChild = this;
			}
			Parent->LastChild = this;
		}
		this->Parent = Parent;
	}
/// Метод извлечения значения по пути вида "\элемент\элемент!атрибут"
	TNode<_char> * get_ByPath(const _char * Path)
	{
		if(!Path || !*Path) return this;
		for(TElement<_char> * x = this; x; )
		switch(*Path)
		{
		case '!':
			return x->get_Attribute(Path + 1);
		case '/':
		case '\\':
			Path++;
			for(x = x->FirstChild; x; x = (TElement<_char> *)x->Next)
				if(const _char * y = x->Name)
				{
					const _char * p;
					for(p = Path; *y && (*p == *y); p++, y++);
					if(!*y && ((*p == '/') || (*p == '\\') || (*p == '!') || !*p))
					{
						Path = p;
						break;
					}
				}
			continue;
		case 0:
			return x;
		default: 
			return 0;
		}
		return 0;
	}

/// Метод разбора XML
/// Возвращает 0 или сообщение об ошибке. В LastPos возвращается последнее положение указателя разбора
	TCHAR * loadXML(const _char * XML, const _char * * LastPos = 0, TSchemeElement<_char> * Scheme = 0)
	//  <Node Param="Value"> Text </Node> <
	// | |    |    ||      ||      ||
	// | |    |    |value  ||	   |cltag
	// | |    |    equal   |text   tag
	// | |    param        |   
	// | tag			   param
	// text				   
	{
	#define IsLetter(a) ((((a) >= 'a') && ((a) <= 'z')) || (((a) >= 'A') && ((a) <= 'Z')) || ((a) == '_') || ((a) == '.') || ((a) == '-'))
	#define IsDigit(a) (((a) >= '0') && ((a) <= '9'))
	#define IsLetterOrDigit(a) (IsLetter(a) || IsDigit(a))

		enum {text, tag, cltag, param, equal, value} Mode = text;
		const _char * b = 0;
		_char q = 0;
		free(FName);
		FName = 0;
		TElement<_char> * Current = 0;
		TAttribute<_char> * Param = 0;
	#define error(m) {if(LastPos) *LastPos = s; return m;}
		const _char * s;
		for(s = XML; *s; s++)
		{
			switch(Mode)
			{
			case text:
				if(*s == '<')
				{
					Mode = tag;
					if(b && (b < s)) 
						if(!Current->AppendTextFromParser(b, s))
							error(_T("Invalid text value."));
					b = 0;
				}
				else 
					if(unsigned(*s) > ' ')
					{
						if(!Current) error(_T("Text not allowed here."));
						if(!b) b = s; // Левый trim текста
					}
				continue;
			case cltag:
			case tag:
				if(b)
				{
					if((unsigned(*s) <= ' ') || (*s == '>') || (*s == '/')) 
					{
						if(Mode == tag) // Открывающий тег
						{
							b = ExtractNameFromParser(b, s);
							if(!b) error(_T("Invalid tag name"));
							if(Current) 
							{
								Current = Current->AppendChild(b);
								if(Scheme && !(Scheme = Scheme->GetChildByName(b))) 
									error(_T("Scheme: Wrong tag name."))
							}
							else
							{
								if(Scheme && !Scheme->IsName(b)) error(_T("Scheme: Wrong root tag name."));
								Current = this;
								if(FName) error(_T("Unexpected second root tag."));
								free(FName);
								FName = (_char *)b;
							}
							Mode = param;
							b = 0;
							// Отсюда идём на case param:
							goto readpar;
						}
						else // Закрывающий тег
						{
							if(!Current) error(_T("Unexpected end tag."));
							if(!spcmp(Current->FName, b, s))
								error(_T("End tag does not match start tag."));
							while(*s && (unsigned(*s) <= ' ')) s++;
							if(*s != '>')error(_T("Expected '>'."));
							Current = (TElement<_char> *) Current->Parent;
							if(Scheme) Scheme = Scheme->GetParent();
							Mode = text;
							b = 0;//s + 1;
							continue;
						}
					} 
					else 
					{
						if(!IsLetterOrDigit(*s)) error(_T("Invalid character in name."));
						continue;
					}
				} 
				else 
				{
					if(*s == '!') 
					{
						if((s[1] != '-') || (s[2] != '-'))error(_T("Invalid name or comment."));
						if(b = sgetcommentend(s + 3))
						{
							s = b + 2;
							b = 0;
							Mode = text;
							continue;
						} else error(_T("Comment not closed."));
					}
					if(unsigned(*s) <= ' ') error(_T("White spaces not allowed here."));
					if((*s == '/') && (Mode == tag)) { Mode = cltag; continue;}
					if(!IsLetter(*s)) error(_T("Invalid character in name."));
					b =	s;
					continue;
				}
			case param:
				if(b)
				{
					if(IsLetterOrDigit(*s)) continue;
					b = ExtractNameFromParser(b, s);
					if(!b) error(_T("Invalid attribute name."));
					Param = Current->AppendAttribute(b);
					b = 0;
					Mode = equal;
				}
				else
				{
			readpar:
					if(unsigned(*s) <= ' ') continue;
					if(*s == '/')
					{
						if(*(++s) != '>') error(_T("Expected '>' after '/'."));
						if(TCHAR * e = AttributesCompleted(Current))
							error(e);
						Current = (TElement<_char> *) Current->Parent;
						if(Scheme) Scheme = Scheme->GetParent();
						goto closepar;
					}
					if(*s == '>')
					{
						if(TCHAR * e = AttributesCompleted(Current))
							error(e);
				closepar:
						Mode = text;
						b = 0;//s + 1;
						continue;
					}
					if(IsLetter(*s)) b = s;
					else 
						error(_T("Invalid character in name."));
					continue;
				}
			case equal:
				if(*s == '=') Mode = value;
				else
				if(unsigned(*s) > ' ') error(_T("'=' expected."));
				continue;
			case value:
				if((*s == '"') || (*s == '\''))
				{
					if(b) 
					{
						if(q != *s) continue;
						Param->FValue = ExtractValueFromParser((_char *)b, (_char *)s);
						if(!Param->FValue && (s > b)) error(_T("Invalid attribute value."));
						b = 0;
						Mode = param;
					}
					else 
					{
						q = *s;
						b = s + 1;
					}
					continue;
				}
				if(!b && (unsigned(*s) > ' ')) error(_T("Quote expected."));
			}
		}
		if((Mode != text) || Current) error(_T("Unexpected end of file."));
		//if(!FirstChild) error(_T("File is empty."));
		#undef error
		return 0;
	};
	xml_virtual ~TElement(void)//! Деструктор уничтожает вложенные элементы и отделяет от родителя.
	{
		TElement<_char> * n;
		for(TElement<_char> * x = FirstChild; x; x = n)
		{
			n = (TElement<_char> *)x->Next;
			x->Parent = 0;
			x->Next = 0;
			x->Prev = 0;
			delete x;
		}
		SetParent();
	}
};


//*************************************************************************************************
//! Шаблон класса документа XML
//*************************************************************************************************
template <class _char>
class TDocument: public TElement<_char> {
public:
	int ErrorOffset;
	TProcessingInstruction<_char> * ProcessingInstruction;
	TDocument(void) {ProcessingInstruction = 0;};
/// Метод возвращает документ XML в виде текста.
	_char * get_XML(const _char * Tab = 0)
	{
		int Size = get_XMLSize(0, Tab ? slen(Tab) : 0) + 1;
		if(ProcessingInstruction) Size += ProcessingInstruction->get_XMLSize();
		_char * Result = (_char *)malloc(Size * sizeof(_char));
		_char * r = Result;
		if(ProcessingInstruction) ProcessingInstruction->WriteXML(r);
		WriteXML(r, 0, Tab);
		*r = 0;
	#ifdef _DEBUG
		if(int x = (r - Result) - Size + 1)
		{
			Result = (_char *)realloc(Result, 50);
			sprintf(Result, "get_XML(): Wrong buffer size: %d\n", x);
			_ASSERTE(!"Wrong buffer size.");
		}
	#endif
		return Result;
	};
/// Метод осуществляет разбор документа XML из текста.
	xml_virtual TCHAR * loadXML(const _char * XML, const _char * * LastPos = 0, TSchemeElement<_char> * Scheme = 0)
	{
		if(ProcessingInstruction) delete ProcessingInstruction;
		ProcessingInstruction = 0;
		const _char * s;
		for(s = XML; (*s <= ' ') && *s; s++);
		if((*s == '<') && (s[1] == '?')) 
		{
			ProcessingInstruction = new TProcessingInstruction<_char>(this);
			if(TCHAR * e = ProcessingInstruction->loadXML(s))return e;
		}
		if(ProcessingInstruction)
		if(const _char * Encoding = *(ProcessingInstruction->get_Attribute("encoding")))
		{
			if(!scmp(Encoding, "UTF-8"))
			{
				_char * R = (_char *)malloc((slen(s) + 1) * sizeof(_char));
				_char * r = R;
				for(const _char * x = s; *x; r++)
				{
					int Res;
					if((unsigned)*x < 0x80) 
						Res = (unsigned)*(x++);
					else
					if(*x & 0x20) 
					{ 
						Res = (unsigned short)((x[2] & 0x3F) | ((x[1] & 0x3F) << 6) | ((*x & 0x3F) << 12)); 
						x += 3;
					}
					else 
					{
						Res = (unsigned short)((x[1] & 0x3F) | ((*x & 0x3F) << 6)); 
						x += 2;
					}
					if(sizeof(_char) == 1)
					{
						if(Res < 0xC0) *r = Res;
						else
						if((Res >= 0x410) && (Res < 0x450)) *r = Res - 0x350;
						else *r = '?';
					} else *r = Res;
				}
				*r = 0;
				TCHAR * Result = TElement<_char>::loadXML(R, LastPos, Scheme);
				if(Result && LastPos && *LastPos) *LastPos += s - R;
				free(R);
				return Result;
			}
		}
		return TElement<_char>::loadXML(s, LastPos, Scheme);
	};
/// Метод осуществляет разбор документа XML из файла
	TCHAR * LoadFromFile(const TCHAR * FileName, TSchemeElement<_char> * Scheme = 0)
	{
		HANDLE f = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if(f == INVALID_HANDLE_VALUE) return SFileOpenError;
		DWORD l = GetFileSize(f, 0);
		char * s = (char *)malloc(l + 1);
		if(!ReadFile(f, s, l, &l, 0))
		{
			CloseHandle(f);
			free(s);
			return _T("TXML::TDocument: File read error.");
		}
		CloseHandle(f);
		s[l] = 0;

		const _char * lp = 0;
		TCHAR * Result = loadXML(s, &lp, Scheme);
		if(Result && lp) ErrorOffset = lp - s;
		free(s);
		return Result;
	};
/// Метод осуществляет вывод документа XML в файл
	TCHAR * SaveToFile(const TCHAR * FileName, const _char * Tab = 0)
	{
		int Size = get_XMLSize(0, Tab ? slen(Tab) : 0);
		if(ProcessingInstruction) Size += ProcessingInstruction->get_XMLSize();
		_char * s = (_char *)malloc(Size * sizeof(_char));
		_char * r = s;
		if(ProcessingInstruction) ProcessingInstruction->WriteXML(r);
		WriteXML(r, 0, Tab);

		_ASSERTE(!((r - s) - Size));

		HANDLE f = CreateFile(FileName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if(f == INVALID_HANDLE_VALUE) return _T("TXML::TDocument: File create error.");

		DWORD l;// = strlen(f, 0);
		if(!WriteFile(f, s, Size, &l, 0))
		{
			CloseHandle(f);
			free(s);
			return _T("TXML::TDocument: File write error.");
		}
		CloseHandle(f);
		free(s);
		return 0;
	};
	xml_virtual ~TDocument(void)//!< Деструктор уничтожает инструкцию.
	{
		if(ProcessingInstruction) 
		{
			ProcessingInstruction->Parent = 0;
			delete ProcessingInstruction;
		}
	}
	void CreatePI(const _char * Encoding = 0, const _char * Version = 0, const _char * Name = 0)
	{
		if(!Name) Name = "xml";
		if(!Version) Version = "1.0";
		ProcessingInstruction = new TProcessingInstruction<_char>(this, scpy(Name));
		ProcessingInstruction->AppendAttribute("version")->Value = Version;
		if(Encoding) 
			ProcessingInstruction->AppendAttribute("encoding")->Value = Encoding;
	}
};


//*************************************************************************************************
//! Шаблон класса инструкции обработки документа XML
//*************************************************************************************************
template <class _char>
class TProcessingInstruction: public TAttributedNode<_char> 
{
protected:
	friend TDocument<_char>;
/// Метод осуществляет вывод текста инструкции в предварительно выделенную строку.
	void WriteXML(_char * &Destination)
	{
		if(!Name) return;
	// Заголовок
		_char * d = Destination;
		*(d++) = '<';
		*(d++) = '?';
		int l = slen(Name);
		memcpy(d, Name, l * sizeof(_char)); d += l;
	// Заливаем атрибуты
		for(TNode<_char> * a = FirstAttr; a; a = a->Next) 
		{
			*(d++) = ' ';
			l = slen(a->Name);
			memcpy(d, a->Name, l * sizeof(_char)); d += l;
			*(d++) = '='; *(d++) = '"';
			d += sToXML(d, a->Value);
			*(d++) = '"';
		}
		*(d++) = '?';
		*(d++) = '>';
		Destination = d;
	};
public:
	TProcessingInstruction(TDocument<_char> * Parent, _char * Name = 0) 
	{
	#ifdef _ALLOW_VIRTUAL_
		FirstAttr = 0;
		LastAttr = 0;
		Next = 0;
		Prev = 0;
		FValue = 0;
	#else
		memset(this, 0, sizeof(*this)); 
	#endif
		this->FName = Name; 
		this->Parent = Parent;
	};
/// Метод осуществляет расчёт размера текста инструкции в символах
	int get_XMLSize(void)
	{
		if(!FName) return 0;
		int Size = slen(FName) + 4; // <?name?>nt
		for(TNode<_char> * a = FirstAttr; a; a = a->Next) Size += slen(a->FName) + sXMLlen(a->FValue) + 4; // Кавычки, =, пробел
		return Size;
	};
/// Метод разбора инструкции. 
/// В случае успеха возвращает 0 и устанавливает XML на конец инструкции
	TCHAR * loadXML(const _char * &XML)
	{
#define error(e) return e
		const _char * s;
		for(s = XML; (*s <= ' ') && *s; s++);
		if((*s != '<') || (s[1] != '?')) error(_T("Missing '<?' on processing instruction."));
		s += 2;
		const _char * b = s;
		if(IsLetter(*s)) for(; IsLetterOrDigit(*s); s++);
		if((s == b) || (*s > ' ')) error(InvChar);
		
		free(FName);
		FName = ExtractNameFromParser(b, s);
		TAttribute<_char> * Param = 0;
		for(s++; *s; s++) if((unsigned)*s > ' ')
		{
			if(Param && (*s == '?') && (s[1] == '>'))
			{
				XML = s + 2;
				return 0;
			}
			b = s;
			if(IsLetter(*s)) for(; IsLetterOrDigit(*s); s++);
			if((s == b) || ((unsigned)*s > ' ') && (*s != '=')) error(InvChar);
			Param = AppendAttribute(ExtractNameFromParser(b, s));
			while((*s <= ' ') && (unsigned)*s) s++;
			if(*(s++) != '=') error(_T("Missing '=' between attribute and attribute value."));
			while((*s <= ' ') && (unsigned)*s) s++;
			_char Q = *(s++);
			if((Q != '\'') && (Q != '"')) error(_T("Quote expected."));
			for(b = s; (*s != Q) && *s; s++);
			if(!*s) error(Ueof);
			free(Param->FValue);\
			Param->FValue = ExtractValueFromParser(b, s);
		}
		error(Ueof);
#undef error
	}
	xml_virtual ~TProcessingInstruction(void) //!< Деструктор отвязывает от родителя.
	{
		if(Parent) ((TDocument<_char> *) Parent)->ProcessingInstruction = 0;
	}
};

template <class _char>
class TSchemeElement: protected TElement<_char> 
{
protected:
	bool StaticNames;

public:
	TSchemeElement(TSchemeElement<_char> * Parent, _char * Name, bool StaticNames = true)
	{
		this->StaticNames = StaticNames;
		if(!StaticNames)
		{
			int l = slen(Name) + 1;
			this->Name = (const _char *) malloc(l * sizeof(_char));
			memcpy((void *)this->Name, Name, l * sizeof(_char));
		} else this->Name = Name;
		SetParent(Parent);
	};
	xml_virtual ~TSchemeElement(void)
	{
		if(StaticNames)
		{
			FName = 0;
			FValue = 0;
		}
	};
	inline TSchemeElement<_char> * GetChildByName(const _char * Name)
	{
		if(StaticNames)
		{
			for(TNode<_char> * x = FirstChild; x; x = x->Next) 
				if(x->FName == Name)
					return (TSchemeElement<_char> *)x;
			return 0;
		}
		else return (TSchemeElement<_char> *)TElement<_char>::get_Child(Name);
	}
	inline bool IsName(const _char * Name)
	{
		if(StaticNames)
			return this->FName == Name;
		else 
			return !scmp(this->FName, Name);
	}
	inline TSchemeElement<_char> * GetParent(void) {return (TSchemeElement<_char> *) Parent;}
};

#undef error
#undef IsLetter
#undef IsDigit
#undef IsLetterOrDigit




#ifdef xml_virtual
#undef xml_virtual
#endif

#ifdef xml_virtual
#undef xml_virtual
#endif

#ifdef xml_static
#undef xml_static
#endif

}

#endif // _XMLPARSER_

