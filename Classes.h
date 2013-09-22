//! \file Classes.h Определения делегатов и вспомогательных классов
#ifndef _Classes_
#define _Classes_
#include <string.h>
#include <windows.h>

template<class T>
inline void CopyString(T * d, const T * s, int Len) { memcpy(d, s, Len * sizeof(T)); }
inline void CopyString(wchar_t * d, const char * s, int Len){MultiByteToWideChar(0, 0, s, Len, d, Len);}
inline void CopyString(char * d, const wchar_t * s, int Len){WideCharToMultiByte(0, 0, s, Len, d, Len, 0, 0);}

inline size_t StrLen(char * Value) {return strlen(Value);};
inline size_t StrLen(wchar_t * Value) {return wcslen(Value);};


/// Класс статической строки.
template <class T>
class TString 
{
protected:
	T * Value;
public:
	inline operator T * (){ return Value;};
	inline TString(T * Value) {this->Value = Value;};
	inline TString(void) {Value = 0;};
	inline TString(TString &Source) {memcpy(this, &Source, sizeof(*this));};
	virtual int get_Len(void);
	virtual ~TString() {};
};

/// Класс динамической строки
template <class T>
class TDynString: public TString<T>
{
private:
	typedef struct _PROP
	{
		int Len;
		int RefCount;
	} PROP;
public:
	virtual int get_Len(void) {return Value ? ((PROP *) Value)[-1].Len : 0;};
	/// Копирующий конструктор.
	template <class sT>
	TDynString(TString<sT> &String)
	{
		int Len = String.get_Len();
		if(!Len) 
		{
			Value = 0;
			return;
		}
		PROP * Prop = (PROP *) malloc(sizeof(PROP) + (Len + 1) * sizeof(T));
		Prop->Len = Len;
		Prop->RefCount = 1;
		Value = (T *)(Prop + 1);
		CopyString(Value, String, Len);
	}
	/// Конструктор создания строки из указателя.
	template <class sT>
	TDynString(sT * String)
	{
		int Len;
		if(!String || !(Len = StrLen(String))) return;
		PROP * Prop = (PROP *) malloc(sizeof(PROP) + (Len + 1) * sizeof(T));
		Prop->Len = Len;
		Prop->RefCount = 1;
		Value = (T *)(Prop + 1);
		CopyString(Value, String, Len);
	}
	/// Конструктор для создания нового объекта с той же строкой
	TDynString(TDynString &String)
	{
		if(Value = String.Value)
			((PROP *) Value)[-1].RefCount++;
	}
	/// Деструктор
	virtual ~TDynString(void)
	{
		if(Value)
		{
			PROP * Prop = ((PROP *) Value) - 1;
			if(!--(Prop->RefCount)) free(Prop);
		}
	}
};

/// Абстрактный класс, базовый для всех классов, содержащих список строк типа char*.
class TStrings
{
public:
	virtual int Add(const TCHAR * S, void * Object = 0) = 0; /// Добавляет строку в список.
	virtual void Delete(int Index /*!Номер строки*/) = 0; /// Удаляет строку из списка.
	virtual void Clear(void) = 0; /// Очищает список.
	virtual int GetCount(void) = 0; /// Возвращает число строк в списке.
	virtual int IndexOf(const TCHAR * S); /// Ищет строку в списке, возвращает номер строки.
	virtual void Insert(int Index, const TCHAR * S) = 0; /// Вставляет строку в список.
	virtual TCHAR * get_Strings(int Index) = 0; /// Получает строку по номеру.
	virtual void set_Strings(int Index, TCHAR * String) = 0; /// Присваивает строку по номеру
};

/// Абстрактный класс, базовый для всех классов, содержащих список строк типа char* и состояние каждой строки.
class TStateStrings: public TStrings
{
public:
	virtual int Add(char * S, int State, void * Object = 0) = 0;
	virtual int Add(char * S, void * Object = 0) {return Add(S, false, Object);};
	virtual int get_State(int Index) = 0;
	virtual int get_State(void * Object) = 0;
};

//////////////////////////////////// Реализация делегатов ///////////////////////////////////////////////

//#define __delfast

#ifdef __delfast
#define __delcall __fastcall
#else
#define __delcall __cdecl
#endif

//typedef void (TEmptyClass::*TNotifyMethod) (void * Sender, ...);
//typedef void (*TNotifyProc) (void * Sender, ...);

class TNotifyEvent {
public:
	void * Handler;
	void * Object;  // Объект метода или 0.
	inline TNotifyEvent(void) {Handler = 0; Object = 0;};
	inline void * operator = (void * a)
	{
		Handler = a;
		Object = 0;
		return a;
	};
	template <class T>
	inline T operator = (T &a)
	{
		Handler = a;
		Object = 0;
		return a;
	};
	template <class T>
	inline void Assign(T a, void * Obj = 0)
	{
		Handler = *(void * *)(&a);
		Object = Obj;
	};

	/*template <class T>
	TNotifyEvent(T a, void * Obj)
	{
		Handler = *(void * *)(&a);
		Object = Obj;
	};
	template <class T>
	TNotifyEvent(T a)
	{
		Handler = *(void * *)(&a);
		Object = 0;
	};*/
	inline void * Assigned(void) {return Handler;};
	inline operator void *(void) {return Handler;};
	int __cdecl operator () (void * Sender, ...);
};

TNotifyEvent * AddHandler(TNotifyEvent * * Events);

#endif