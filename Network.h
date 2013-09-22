#pragma once
#include <complex>
#include <atlbase.h>
#include <map>
#include "device.h"  // объявление cplx

namespace DFW
{
	/************************ Заголовок *************************************************/
	struct R_pdn{
		int ndd, // Число элементов в maj
			nk; // Число элементов в матрице проводимости may

		struct outn
		{
			int n1, // niyp = Original в виде индекса								|
				n2, // nobr = Sorted в виде индекса									|
				n3, // nrp = nhp - индекс узла в матрице проводимости				|
				n4; // nhj - индекс узла в триангулированной матрице
			// от nhj до nhj следующего узла должны храниться смежные узлы в ПОИ
		} * o; // Все узлы.
		struct Rmay
		{
			int nqy; // Индекс смежного узла
			cplx y; // Значение проводимости
		} * may;  // Матрица проводимости (размер nk)

		int *maj; // Граф триангулированной матрицы, со сменой направления на противоположное

		R_pdn() {o = NULL; may = NULL; maj = NULL;}
		~R_pdn(){delete_all();}
		void delete_all(void)
		{	
			free(o);
			free(may);
			free(maj);
		}
	};
	//! Кусочно-плоский массив на основе двусвязного списка
	template<class T> 
	class CPFlat
	{
	public:
		class CPiece 
		{
		public:
			CPiece * Next, * Prev;
			T * End, * Last; // End - конец выделенной памяти, Last - последний заполненный элемент
			inline T * GetFirst(void) { return (T *)(this + 1);};
			inline void Reset(void) { Last = ((T *)(this + 1)) - 1;};
		};
		T * Alloc(int Count); // Выделить заданное количество элементов под результат триангуляции узла
		int Align;
		CPiece * First, * Last;
		inline void Reset(void) { Last = First; if(First) First->Reset();}
		inline void Clear(void);
		inline bool Valid(void) const { return First && (First->Last + 1 != First->GetFirst());};
		int GetUsedCount(void) const;
		int GetMemoryOverhead(void) const;
		inline bool IsFlat(void) const {return First == Last;};
		void MakeFlat(void);
		CPFlat(int Align = 1023) { this->Align = Align; First = 0; Last = 0;}
		~CPFlat(void);
	};

	//typedef std::complex<double> cplx;

	//! Класс сети
	class CNetwork 
	{
		class CYList;
		class CSList;
		typedef struct _CLU;
	public:
		class CNode;
		//! Класс статической характеристики нагрузки
		class CSXN
		{
		private:
			struct CFactors
			{
				cplx Factors[3];
				double Vrmin;
				double Frec;
				CFactors * Next;
			} * Factors;
		public: 
			//void * Apply(cplx * Y, cplx &I, const cplx &V, const double &Vnom, const cplx &Snom);
			void ApplyToY(cplx &Y, CNode * Node);
			bool GetCurrent(cplx &I, CNode * Node);
			void GetSn(cplx &Sn, CNode * Node);
			~CSXN(void);
			CSXN(void);
			void SetMinVoltage(const double &Vrmin);
			double GetMaxBreak(void);
			void AddFactors(const double &Vrmin, const cplx &f0, const cplx &f1, const cplx &f2, const double &Frec = 0);
		};
		//! Класс узла сети
		__declspec(align(16))class CNode 
		{
		private:
			cplx Isxn;
		public:
			cplx y; //!< Проводимость узла (НЕ Сопрягается!!!)
			cplx y1; //!< Ещё одна проводимость узла
			cplx V; //!< Напряжение в узле
			cplx I; //!< Задающий ток узла
			cplx Sn; //!< Номинальная мощность нагрузки в узле
			cplx Sg; //!< Мощность генерации в узле
			cplx yo; //!< Изначальная проводимость узла (из Metakit)
			double ModV;
			double ArgV;
			double SlipU;
			double Vnom; //!< Номинальное напряжение узла
			enum TYPE {DISABLED = 0, CONST_V = 1, USUAL = 2, MISSED = 3} Type;
			CSXN * SXN;
			int N; //!< Номер узла
			int Island; //!< Номер острова
		private:
			friend CNetwork;
			void * AddBranch(CNode * AdjacentNode, CYList * &Destination);
			// Данные
			union
			{
				CYList * YList; //!< Связный список матрицы проводимости
				cplx * pY; // Yii, на которую влияет данный узел
			};
			int BranchCount; //!< Число ветвей узла

			union
			{
				struct
				{
					_CLU * LU;
					cplx * * pLU;
				};
				struct
				{
					CNode * Mark;
					CYList * YMark;
				};
			};
			struct CSorted // Данные узла, которые хранятся в порядке оптимального исключения
			{
				CNode * Original; // niyp
			} SData, *Sorted; // nobr
			void * LastFactors;
		};

		//! Класс ветви сети
		class CBranch 
		{
		private:
			friend CNetwork;
			template <bool Add> inline void Apply(cplx * Yp, cplx * Yq, cplx * Ypq, cplx * Yqp);
			union {cplx * pYpq; int iYpq; CBranch * NextBranch;};
			union {cplx * pYqp; int iYqp;}; // Проводимости, на которые влияет данная ветвь
		//private:
		public:
			int ip, iq; //!< Индексы узлов
			cplx yp, yq, y; //!< Проводимости П-образной схемы замещения
			cplx kt; //!< Коэффициент трансформации - включён между проводимостями y и yq, означает Vq = kt * Vp
			enum STATE {
				CONN = 0,	//!< Ветвь включена
				P_DISC = 2, //!< Ветвь отключена в начале
				Q_DISC = 3, //!< Ветвь отключена в конце
				DISC = 1	//!< Ветвь отключена
			} State; // Состояние ветви
			int Island; //!< Номер острова
			int NP; //!< Номер параллельной ветви
		};
		//! Класс короткого замыкания
		class CShortCurcuit
		{
		public:
			double Position;
			cplx Shunt;
			int Node, Branch; // Узел и ветвь, созданые замыканием
			cplx yc, gl; // Параметры линии с распределёнными параметрами
		};
	private:
		//! Элемент матрицы проводимостей
		typedef struct _CYArray
		{
			cplx Yij;
			cplx Yji;
		} CYArray;

		//! Класс элемента матрицы проводимости в виде связного списка
		class CYList 
		{
		public:
			cplx y;                    // y - проводимость
			CNode * Node;   // nqy - соответствующий узел
			CYList * Next;  // nky - следующая ветвь
			cplx * * Sources; // Источники данной проводимости
			cplx * * Branches; // Проводимости ветвей - источники данной проводимости
			inline void AddSource(cplx * &Source);
			inline void FillSources(int Value);
			inline void AddBranch(cplx * &Source);
			inline void FillBranches(cplx * Value);
		};

		//! Элемент триангулированной матрицы
		typedef struct _CLU 
		{
			cplx L, U; // L = Yij, Yii для V; U = Yji, 0 для I
			CNode * Node;
			CYArray * Y;
		} CLU;


		// Данные класса
		int NodeCount, BranchCount; // Исходные данные
		int UsualNodeCount;
		CNode * Nodes, * _Nodes;				//
		CBranch * Branches;			//

		CYList * YList, * FreeY;	// Матрица проводимости в виде связного списка
		CYArray * Y;				// Матрица проводимости в виде массива
		int YSize;					//

		CNode * Base;

		CPFlat<CLU> LU;	//!< Триангулированная матрица
		int LUIndex;
		CPFlat<cplx *> pLU; 

		std::map<int, CNode *> NodeMap;
		std::map<int, CShortCurcuit> ShortCurcuits;
		std::map<int, CSXN *> SXNMap;

		std::map<int, cplx> Shunts;
		// Методы класса

		void * ExcludeNode(CNode * Node);
		void PrepareYList(void); // prep_steady_state::formy - инициализирует Y и YSize
		bool TriangulateYList(void);
		void IdxToPointers(void);
		void PrepareY(void);
		void ResetLU(CLU * First = 0);
		bool TriangulateLU(void);
		//template<bool Fast, bool Y> void PrepareSteadyState(void); //!< Метод подготовки задающих токов и шунтов из СХН и генераторов
		bool NeedPrepare;
		bool YisValid;
		bool Prepare(void); //!< Метод триангуляции.
	public:
		bool UseFastTriangulation; //!< Разрешает быструю триангуляцию
		bool UseBestDiagonal; //!< Разрешает поиск узла с максимальной диагональю и минимальным числом ветвей.
			   //Иначе поиск ведётся до первого узла с минимальным числом ветвей и удовлетворительной диагональю.
		double MinimalDiagonalNorm; //!< Минимально допустимая норма диагонали
		bool UseSXN; //!< Разрешает использование СХН
		bool UseSSE2; //!< Разрешает использование SSE2
		//bool YChanged;
		enum STATE {} State; //Состояние сети
		// Методы управления контентом
		void UpdateNodeMap(void);
		inline CSXN * GetSXN(int Number, bool Create = false) 
		{
			std::map<int, CSXN *>::iterator i = SXNMap.find(Number);
			if(i != SXNMap.end()) return i->second;
			CSXN * SXN = 0;
			if(Create)
			{
				SXN = new CSXN();
				SXNMap.insert(std::make_pair(Number, SXN));
			}
			return SXN;
		};
		void SetMinSXNV(const double &V) { for(std::map<int, CSXN *>::iterator i = SXNMap.begin(); i != SXNMap.end(); i++) i->second->SetMinVoltage(V);};
		double GetMaxSXNBreak(int * N) //!< Возвращает максимальный разрыв между кусками СХН
		{
			double r = 0;
			int n = 0;
			for(std::map<int, CSXN *>::iterator i = SXNMap.begin(); i != SXNMap.end(); i++)
			{
				double b = i->second->GetMaxBreak();
				if(b > r)
				{
					r = b;
					n = i->first;
				}
			}
			if(N) *N = n;
			return r;
		};

		//void CreateStandardSXN(void);
		inline CBranch &Branch(int Index) { _ASSERTE((Index >= 0) && (Index < BranchCount)); return Branches[Index];};
		inline CNode &Node(int Index) { _ASSERT((Index >= 0) && (Index < NodeCount)); return Nodes[Index];};
		inline CBranch * GetBranches(void) { return Branches;};
		inline CNode * GetNodes(void) { return Nodes;};
		double GetMaxSXNDisc(void); //!< Возвращает невязку тока СХН
		// Уведомления
		void SetSize(int NodeCount, int BranchCount);
		void SetBranchState(int Branch, CBranch::STATE State);
		void SetBranchParams(int Branch, const cplx * y, const cplx * yp = 0, const cplx * yq = 0, const cplx * kt = 0);
		void SetNodeType(int Node, CNode::TYPE Type);
		void SetNodeParams(int Node, const cplx * y, const cplx * y1 = 0);//!< Устанавивает шунты в узле. y - шунт нагрузки (сети), y1 - шунт генератора.
		void SetShunt(int Node, const cplx &Y); //!< Устанавивает дополнительный шунт в узле. Используется для установки КЗ.
		void RemoveShunt(int Node);
		const cplx& GetShunt(int Node); //!< Возвращает дополнительный шунт в узле.
		inline cplx GetTotalShunt(int Node) // Возвращает полный шунт узла (не считая шунта генератора)
		{
			_ASSERTE((Node >= 0) && (Node < NodeCount));
			CNode & n = Nodes[Node];
			return n.y /*+ n.y1*/ + GetShunt(Node);					
		};
		inline cplx GetNodeY(int Node) // Возвращает изначальную проводимость узла (из Metakit)
		{
			_ASSERTE((Node >= 0) && (Node < NodeCount));
			CNode & n = Nodes[Node];
			return n.yo;
		};
		inline cplx GetNodeSn(int Node)
		{
			_ASSERTE((Node >= 0) && (Node < NodeCount));
			CNode & n = Nodes[Node];
			cplx Sn = norm(n.V) * conj(n.y - n.yo);
			if(UseSXN && n.SXN)
			{
				cplx Ssxn;
				n.SXN->GetSn(Ssxn, &n);
				Sn += Ssxn;
			}
			return Sn;
		}
		int SetShortCircuit(int Branch, const cplx * Shunt, double Position = 0.5); // Позиция от 0 до 1.
		void inline DiscardY(bool Reorder = false) 
		{
			YisValid = false; 
			NeedPrepare = true; 
			if(Reorder) { LU.Reset(); pLU.Reset();}
		};
		// Справочные методы
		inline int GetNodeCount(void) const {return NodeCount;};
		inline int GetBranchCount(void) const {return BranchCount;};
		void GetBranchCurrent(int Branch, bool End, cplx &Current);//!< Ток, _в_текающий в линию.
		void GetBranchPower(int Branch, bool End, cplx &Power);//!< Мощность, _в_текающая в линию.
		void GetBranchResistance(int Branch, bool End, cplx &Resistance);//!< Мощность, _в_текающая в линию.
		inline CNode * GetNodeByNumber(int Number) 
		{
			if(NodeMap.empty()) UpdateNodeMap();
			std::map<int, CNode *>::iterator i = NodeMap.find(Number);
			return (i == NodeMap.end()) ? 0 : i->second;
		};
		inline int GetNodeIndexByNumber(int Number) 
		{
			if(NodeMap.empty()) UpdateNodeMap();
			std::map<int, CNode *>::iterator i = NodeMap.find(Number);
			return (i == NodeMap.end()) ? -1 : i->second - Nodes;
		};
		inline const CPFlat<CLU> &GetLU(void) const {return LU;};	//!< Триангулированная матрица
		inline const CPFlat<cplx *> &GetpLU(void) const {return pLU;};
		inline bool GetNeedPrepare() const { return NeedPrepare ; }
		// Методы расчёта
		bool Solve(void);
		void CalcPolarV();
		//void SolveSteadyState(void);
		// Прочее
		int MarkIslands(void);
		void Clean(); //!< Метод освобождения памяти
		double Test(void);
		bool ExportToR_pd(R_pdn * pd);
		bool ImportFromR_pd(R_pdn * pd);
		CNetwork(void);
		~CNetwork(void);
	};

	/************************ Реализация функций работы со связными списками *************************************************/

	//! Удалить связный список
	template<class T> inline void DeleteList(T * List)
	{
		for(T * Next; List; List = Next)
		{
			Next = List->Next;
			free(List);
		}
	}

	//! Выделить новый элемент списка
	template<class T> inline T * NewElement(T * &List, T * &Free)
	{
		if(Free)
		{
			T * fs = Free;
			Free = Free->Next;
			return fs;
		}
		int Size = 511;
		T * Old = List;
		List = (T *) malloc((1 + Size) * sizeof(T));
		T * s = List;
		(s++)->Next = Old;
		Free = s + 1;
		for(Size--; Size; Size--) 
		{
			s->Next = s + 1;
			s++;
		}
		s->Next = 0;
		return List + 1;
	};

	//! Удалить из связного списка следующий элемент, добавить его в список свободных
	template<class T> inline void DeleteNextElement(T * Element, T * &FreeList)
	{ 
		T * t = FreeList;
		FreeList = Element->Next;
		Element->Next = FreeList->Next;
		FreeList->Next = t;
	};

	template<class T> CPFlat<T>::~CPFlat(void)
	{
		DeleteList(First);
	}

	template<class T> inline void CPFlat<T>::Clear(void)
	{
		DeleteList(First);
		First = Last = 0;
	}

	//! Выделить заданное количество элементов под результат триангуляции узла
	template<class T> T * CPFlat<T>::Alloc(int Count) 
	{
		CPiece * f, * next;
		T * nLast;
		for(f = Last; f; f = next)
		{
			nLast = f->Last + Count;
			if(nLast < f->End) break;
			next = f->Next;
			if(f != Last) free(f);
			if(next) next->Reset();
		}
		if(!f) 
		{
			int s = sizeof(CPiece) + (Count | Align + 1) * sizeof(T);
			f = (CPiece *)malloc(s);
			_ASSERTE(f);
			f->End = (T *)((char *)f + s);
			f->Next = 0;
			f->Reset();
			nLast = f->Last + Count;
		}
		if(f != Last)
		{
			f->Prev = Last;
			if(Last) 
				Last->Next = f;
			else 
				First = f;
			Last = f;
		}
		T * Result = f->Last + 1;
		f->Last = nLast;
		return Result;
	}

	template<class T> void CPFlat<T>::MakeFlat(void)
	{
		if(First == Last) return;
		int Size = sizeof(CPiece);
		for(CPiece * p = First; p; p = p->Next)
		{
			Size += int((char *)(p->Last + 1) - (char *)p->GetFirst());
			if(p == Last) break;
		}
		int e = int((char *)First->Last - (char *)First);
		First = (CPiece *)realloc(First, Size);
		First->End = (T *)((char *)First + e) + 1;
		CPiece * Next;
		for(CPiece * p = First->Next; p; p = Next)
		{
			Size = int((char *)(p->Last + 1) - (char *)p->GetFirst());
			memcpy(First->End, p->GetFirst(), Size);
			*(char * *)&First->End += Size;
			Next = p->Next;
			if(p == Last)
			{
				First->Next = Next;
				if(Next) Next->Prev = First;
				Last = First;
				free(p);
				break;
			}
			free(p);
		}
		First->Last = First->End - 1;
	}


	inline void CNetwork::CYList::AddSource(cplx * &Source)
	{
		Source = (cplx *)Sources;
		Sources = (cplx **)&Source;
	}

	inline void CNetwork::CYList::FillSources(int Value)
	{
		void * Next;
		for(void * List = Sources; List; List = Next)
		{
			Next = *(void * *)List;
			*(int *)List = Value;
		}
	}

	inline void CNetwork::CYList::AddBranch(cplx * &Source)
	{
		Source = (cplx *)Branches;
		Branches = (cplx **)&Source;
	}

	inline void CNetwork::CYList::FillBranches(cplx * Value)
	{
		void * Next;
		for(void * List = Branches; List; List = Next)
		{
			Next = *(void * *)List;
			*(cplx * *)List = Value;
		}
	}

	template<class T> int CPFlat<T>::GetUsedCount(void) const
	{
		int Result = 0;
		for(CPiece * p = First; p; p = p->Next)
		{
			Result += int(p->Last - p->GetFirst() + 1);
			if(p == Last) break;
		}
		return Result;
	}

	template<class T> int CPFlat<T>::GetMemoryOverhead(void) const
	{
		int Result = 0;
		bool Used = true;
		for(CPiece * p = First; p; p = p->Next)
		{
			Result += int((char *)p->End - (char *)p);
			if(Used)
				Result -= int((char *)p->Last - (char *)p->GetFirst() + sizeof(T));
			if(p == Last) Used = false;
		}
		return Result;
	}
}


