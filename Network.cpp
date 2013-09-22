#include "..\stdafx.h"
#include "network.h"

using namespace DFW;

//#define PERFCTR

#ifdef PERFCTR
class PCTR
{
public:
	int Count;
	__int64 Clock;
	PCTR(void) {Count = 0; Clock = 0;};
};

PCTR Solve_, Solve1_, Solve1a_, Solve2_, Solve3_;
PCTR Prepare_;
PCTR PolarV_, PolarV1_, PolarV2_, PolarV3_;

PCTR PrepareY_;
PCTR ResetLU_;
PCTR TriangulateLU_;
PCTR PTYList_;

PCTR MarkIslands_;

__declspec(naked) __int64 ReadTSC(void)
{
	__asm
	{
		RDTSC
		RET
	}
}
#define PCTRSTART(proc) __int64 proc##pfctr = ReadTSC()
#define PCTRSTOP(proc) proc##_.Clock += ReadTSC() - proc##pfctr; proc##_.Count++
#else
#define PCTRSTART(proc)
#define PCTRSTOP(proc)
#endif

/************************ Реализация конструктора и деструктора *************************************************/

__declspec(naked) bool GetSSE2Present(void)
{
	__asm
	{
		PUSH		EBX
		PUSH		ECX
		XOR			EAX, EAX
		CPUID
		TEST		EAX, EAX
		JZ			Rt
		XOR			EAX, EAX
		INC			EAX
		CPUID
		SHR			EDX, 26
		AND			DL, 1
		MOV			EAX, EDX
Rt:
		POP			ECX
		POP			EBX
		RET
	}
}

CNetwork::CNetwork(void) 
{
	_Nodes = Nodes = 0;
	Branches = 0;
	NodeCount = 0;
	BranchCount = 0;
	Y = 0;
	YList = 0;
	YSize = 0;
	UseBestDiagonal = false;
	UseFastTriangulation = true;
	UseSXN = false;
	YisValid = false;
	NeedPrepare = true;
	MinimalDiagonalNorm = 0.0000001;
	pLU.Align = 4095;
	UseSSE2 = GetSSE2Present();
};

CNetwork::~CNetwork(void) 
{
	free(_Nodes);
	free(Branches);
	free(Y);
	DeleteList(YList);

	for(std::map<int, CSXN *>::iterator i = SXNMap.begin(); i != SXNMap.end(); i++)
		delete i->second;
};

void CNetwork::UpdateNodeMap(void)
{
	NodeMap.clear();
	for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--)
		NodeMap.insert(std::make_pair(n->N, n));
}

void CNetwork::SetSize(int NodeCount, int BranchCount)
{
	Clean();
	Shunts.clear();
	this->NodeCount = NodeCount;
	free(_Nodes);
	if(NodeCount)
	{
		_Nodes = Nodes = (CNode *) malloc(NodeCount * sizeof(CNode) + 0x10);
		Nodes = (CNode *)((0xF + (DWORD)Nodes) & 0xFFFFFFF0); // Для SSE
		memset(Nodes, 0, NodeCount * sizeof(CNode));
	}
	else
		_Nodes = Nodes = 0;
	this->BranchCount = BranchCount;
	free(Branches);
	if(BranchCount)
	{
		Branches = (CBranch *) malloc(BranchCount * sizeof(CBranch));
		memset(Branches, 0, BranchCount * sizeof(CBranch));
		for(CBranch * b = Branches + BranchCount - 1; b >= Branches; b--) b->kt = 1.0;
	}
	else
		Branches = 0;
}

void CNetwork::Clean(void)
{
	pLU.Clear();
	LU.Clear();
	DeleteList(YList);
	free(Y);
	Y = 0;
	YList = 0;
	NodeMap.clear();
}
/*************** Реализация методов, формирующих связный список матрицы проводимости ************************************/

//! Метод создания проводимости между узлами
void * CNetwork::CNode::AddBranch(CNode * AdjacentNode, CYList * &Destination)
{
	CYList * Last = YList;
	for(CYList * List = Last; List; List = List->Next) 
		if(List->Node == AdjacentNode)
			return List;
		else
			Last = List;
	Last->Next = Destination;
	BranchCount++;
	Destination->Next = 0;
	Destination->Node = AdjacentNode;
	Destination->y = 0;
	Destination->Sources = 0;
	return Destination++;
}

//! Метод добавления ветви к связному списку Y
template <bool Add> inline void CNetwork::CBranch::Apply(cplx * Yp, cplx * Yq, cplx * Ypq, cplx * Yqp)
//inline void CNetwork::CBranch::AddToY(CNode * Nodes, CYList * * Destination)
{
	double kt2 = norm(kt);
	switch(State) // Обработка состояния ветви
	{
	case CONN:
		{
			cplx ypq = y /      kt;
			cplx yqp = y / conj(kt);
			if(Add)
			{
				if(Yp) *Yp -= y + yp;
				if(Yq) *Yq -= y / kt2 + yq;
				if(Ypq) *Ypq += ypq;
				if(Yqp) *Yqp += yqp;
			}
			else
			{
				if(Yp) *Yp += y + yp;
				if(Yq) *Yq += y / kt2 + yq;
				if(Ypq) *Ypq -= ypq;
				if(Yqp) *Yqp -= yqp;
			}
			return;
		}
	case Q_DISC:
		{
			cplx y = yp + kt2 * y * yq / (y + yq * kt2);
			_ASSERTE(Yp);
			if(Add) *Yp -= y;
			else *Yp += y;
			return;
		}
	case P_DISC:
		{
			cplx y = yq + (y * yp) / ((y + yp) * kt2);
			_ASSERTE(Yq);
			if(Add) *Yq -= y;
			else *Yq += y;
			return;
		}
	}
}

/*template<bool Fast, bool Y> void CNetwork::PrepareSteadyState(void)
{
	for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--)
	{
		n->I = -conj(n->Sg / n->V);
		if(n->SXN)
		{
			void * nf = n->SXN->Apply(Y ? (Fast ? n->pY : &n->YList->y) : 0, n->I, n->V, n->Vnom, n->Sn);
			if(!Y && (nf != n->LastFactors)) YChanged = true;
			n->LastFactors = nf;
		}
		else
			n->I += conj(n->Sn / n->V);
	}
}*/


//! Метод формирования матрицы проводимостей Y в виде связного списка Y
void CNetwork::PrepareYList(void)
{
	free(Y);
	DeleteList(YList);
	int Size = NodeCount + BranchCount * 2;
	YList = (CYList *)malloc((Size + 1) * sizeof(CYList));
	memset(YList, 0, (Size + 1) * sizeof(CYList));
	CYList * y = YList;
	FreeY = 0;
	(y++)->Next = 0;
	CNode * n = Nodes;
	for(int c = NodeCount; c--; n++) if(n->Type == CNode::USUAL || n->Type == CNode::MISSED) // Заполняем Y из узлов
	{
		n->Type = CNode::USUAL;
		y->Node = n;
		y->y = -n->y - n->y1;
		if(UseSXN)if(CSXN * SXN = n->SXN) SXN->ApplyToY(y->y, n);
		n->YList = y;
		n->BranchCount = 0;
		n->Mark = 0;
		y++;
	}
	else
		n->YList = 0;
	UsualNodeCount = (int)(y - YList) - 1;

	for(std::map<int, cplx>::iterator i = Shunts.begin(); i != Shunts.end(); i++) // Из шунтов
		if(CYList * yl = Nodes[i->first].YList)
			yl->y -= i->second;

	if(BranchCount)	for(CBranch * b = Branches + BranchCount - 1; b >= Branches; b--)  // Заполняем Y из ветвей
	{
		b->pYpq = 0;
		b->pYqp = 0;
		_ASSERTE(b->ip >= 0 && b->ip < NodeCount);
		_ASSERTE(b->iq >= 0 && b->iq < NodeCount);
		CNode * p = Nodes + b->ip;
		CNode * q = Nodes + b->iq;
		cplx * pYpq = 0, * pYqp = 0;
		if((p->Type == CNode::DISABLED) || (q->Type == CNode::DISABLED)) b->State = CBranch::DISC;
		if(b->State == CBranch::CONN)
		{
			if(p->Type == CNode::USUAL)
			{
				CYList * l = (CYList *)p->AddBranch(q, y);
				l->AddBranch(b->pYpq);
				pYpq = &l->y;
			}
			if(q->Type == CNode::USUAL)
			{
				CYList * l = (CYList *)q->AddBranch(p, y);
				l->AddBranch(b->pYqp);
				pYqp = &l->y;
			}
		}
		/*else
		{
			b->p b->pYpq = b->pYqp = 0;
			if(b->State == CBranch::P_DISC) b->
		}*/
		b->Apply<true>(&p->YList->y, &q->YList->y, pYpq, pYqp);
	}
	CYList * ylast = y - 1;
	YSize = (int)(ylast - YList);
	if(Size > YSize) 
	{
		for(int x = Size - YSize - 1; x; x--) 
			y = y->Next = y + 1;
		y->Next = 0;
	}
	if(UseFastTriangulation)
	{
		Y = (CYArray *) malloc(YSize * sizeof(CYArray));
		memset(Y, 0, YSize * sizeof(CYArray));
		YisValid = false;
		/*for(CYArray * y = Y + YSize - 1; y >= Y; y--, ylast--) 
			ylast-> = y;*/
	}
}

//! Метод формирования матрицы проводимостей Y в массиве
void CNetwork::PrepareY(void)
{
	PCTRSTART(PrepareY);
	if(UseSXN) for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--) // Заполняем Y из узлов
	{
		if(cplx * p = n->pY)
		{
			*p = -n->y - n->y1;
			if(CSXN * SXN = n->SXN) SXN->ApplyToY(*p, n);
		}
	}
	else for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--) // Заполняем Y из узлов
		if(cplx * p = n->pY) *p = -conj(n->y) - n->y1;

	memset(Y + UsualNodeCount, 0, (YSize - UsualNodeCount) * sizeof(CYArray));

	for(std::map<int, cplx>::iterator i = Shunts.begin(); i != Shunts.end(); i++) // Из шунтов
		if(cplx * pY = Nodes[i->first].pY) *pY -= i->second;
	for(CBranch * b = Branches + BranchCount - 1; b >= Branches; b--) if(b->State != CBranch::DISC)  // Заполняем Y из ветвей
	{
		CNode * p = Nodes + b->ip;
		CNode * q = Nodes + b->iq;
		_ASSERTE(!p->pY || (p->pY >= &Y->Yij) && (p->pY < &(Y + YSize)->Yij));
		_ASSERTE(!q->pY || (q->pY >= &Y->Yij) && (q->pY < &(Y + YSize)->Yij));
		_ASSERTE(!b->pYpq || (b->pYpq >= &Y->Yij) && (b->pYpq < &(Y + YSize)->Yij));
		_ASSERTE(!b->pYqp || (b->pYqp >= &Y->Yij) && (b->pYqp < &(Y + YSize)->Yij));
		b->Apply<true>(p->pY, q->pY, b->pYpq, b->pYqp);
	}
	YisValid = true;
	PCTRSTOP(PrepareY);
}

//! Метод инициализации LU
void CNetwork::ResetLU(CLU * First)
{
	PCTRSTART(ResetLU);
	CLU * Last = LU.First->Last;
	for(CLU * lu = First ? First : LU.First->GetFirst(); lu <= Last; lu++)
		if(CYArray * y = lu->Y)
		{
			lu->L = y->Yij;
			lu->U = y->Yji;
		}
		else
		{
			lu->L = 0;
			lu->U = 0;
		}
	PCTRSTOP(ResetLU);
}

/*************** Реализация методов триангуляции ************************************/

//! Метод исключения узла
void * CNetwork::ExcludeNode(CNode * Node)
{
	int MinBranchCount = Node->BranchCount;
	double maxd = MinimalDiagonalNorm;
#if _DEBUG
	int deb_bc = MinBranchCount;
#endif
	CNode * Result = 0;
	// Выделяем часть триангулированной матрицы, относящуюся к исключаемому узлу
	CLU * lu = LU.Alloc(MinBranchCount + 1);
	cplx * * plu;
	if(UseFastTriangulation)
		plu = pLU.Alloc(MinBranchCount * MinBranchCount);
	CYList * Yii = Node->YList;
	cplx Diag = 1.0 / Yii->y;
	// Отключаем узел от смежных
	CYList * Prev = Yii;
	for(CYList * j = Yii->Next; j; j = j->Next, lu++, LUIndex++)
	{
		_ASSERTE(deb_bc--);
		CNode * jNode = j->Node;
		jNode->BranchCount--;
		lu->L = Diag * j->y;
		if(UseFastTriangulation) 
		{
			j->FillSources(LUIndex);//&lu->L);
			if(j->Branches)
			{
				lu->Y = &Y[j - YList - 1];
				j->FillBranches(&lu->Y->Yij);
			}
			else
				lu->Y = 0;
		}
		lu->Node = jNode;
		if(jNode->Type != CNode::USUAL) 
		{
			lu->U = 0.0;
			continue;
		}
#ifdef _DEBUG
		int f = 1;
		int deb_bc1 = Node->BranchCount;
#endif
		for(CYList * k = jNode->YList; k; k = k->Next) // Идём вокруг j
		{
			CNode * kNode = k->Node;
			if(kNode == Node) 
			{
				_ASSERTE(f--);// Надо проверять - найден ли этот узел.
				lu->U = Diag * k->y;
				if(UseFastTriangulation) 
				{
					k->FillSources(LUIndex | 0x80000000);//&lu->U);
					if(k->Branches)
					{
						k->FillBranches(&Y[j - YList - 1].Yji);
						_ASSERTE(j - YList - 1 < YSize);
					}
				}
				k = Prev;
				DeleteNextElement(Prev, FreeY);
			}
			else
			{ // Отмечаем в узле k узел j и связь, приведшую от него в k.
				kNode->Mark = jNode;
				kNode->YMark = k;
				Prev = k;
			}
		}
		_ASSERTE(!f);
		for(CYList * k = Yii->Next; k; k = k->Next) // Идём вокруг i
		{
			CNode * kNode = k->Node;
			CYList * Yjk;
			if(kNode->Mark != jNode) // Связи Yjk не было, создаём
			{
				Yjk = NewElement(YList, FreeY);
				Prev = Prev->Next = Yjk;
				Prev->Node = kNode;
				Prev->y = -(k->y * lu->U);
				Prev->Sources = 0;
				Prev->Branches = 0;
				jNode->BranchCount++;
			}
			else
			{ 
				Yjk = kNode->YMark;
				Yjk->y -= k->y * lu->U;
			}
			_ASSERTE(deb_bc1--);
			if(UseFastTriangulation) Yjk->AddSource(*(plu++)); // Добавляем plu в список влияющих на Yjk
		}
		_ASSERTE(!deb_bc1);
		Prev->Next = 0; // Завершаем список ветвей j
		Prev = j;
		// Определяем - не стал ли узел оптимальным для следующего исключения
		int bc = jNode->BranchCount;
		if(MinBranchCount >= bc)
		{
			double d = norm(jNode->YList->y);
			if(d > ((MinBranchCount > bc) ? MinimalDiagonalNorm : maxd))
			{
				maxd = d;
				Result = jNode;
				MinBranchCount = bc;
			}
		}
	}
	_ASSERTE(!deb_bc);
	if(UseFastTriangulation)
		pLU.Last->Last = plu - 1;
	lu->Node = 0;
	lu->Y = Y + (Yii - YList - 1);
	lu->L = Diag;
	Node->pY = &lu->Y->Yij;
	Yii->FillSources(LUIndex++);//&lu->L);
	// Освобождаем элементы Yij для экономии памяти
	Prev->Next = FreeY;
	FreeY = Yii;
	return Result;
}

//! Метод триангуляции матрицы проводимостей Y
bool CNetwork::TriangulateYList(void)
{	
	LU.Reset();
	LUIndex = 1;
	pLU.Reset();

	// Инициализируем Sorted/Original, вытаскиваем базовые в конец
	CNode * Base = Nodes + NodeCount;
	CNode * Usual = Nodes;
	//for(CNode * Node = Nodes + NodeCount - 1; Node >= Nodes; Node--)
	for(CNode * Node = Nodes; Node < Nodes + NodeCount; Node++)
	{
		if(Node->Type != CNode::USUAL)
		{
			Node->Sorted = &(--Base)->SData;
			Base->SData.Original = Node;
		}
		else
		{
			Usual->SData.Original = Node;
			Node->Sorted = &(Usual++)->SData;
		}
	}
	this->Base = Base;
	int Threshold = 0;
	CNode * OptimalNode = 0;
	for(CNode::CSorted * Node = &Nodes->SData; (CNode *)Node < Base; (*(CNode * *)&Node)++)
	{	
		if(!OptimalNode) // Ищем оптимальный узел
		{
			int MinBC = INT_MAX;
			if(UseBestDiagonal) 
			{
				double max = MinimalDiagonalNorm;
				for(CNode::CSorted * n = Node; (CNode *)n < Base; (*(CNode * *)&n)++)
				{
					CNode * o = n->Original;
					int bc = o->BranchCount;
					if(bc <= MinBC)
					{
						double m = norm(o->YList->y);
						if(m > ((bc < MinBC) ? MinimalDiagonalNorm : max))
						{
							max = m;
							MinBC = bc;
							OptimalNode = o;
						}
					}
				}
				if(max < MinimalDiagonalNorm) OptimalNode = 0;
			}
			else for(CNode::CSorted * n = Node; (CNode *)n < Base; (*(CNode * *)&n)++)
			{
				CNode * o = n->Original;
				int bc = o->BranchCount;
				if(bc <= Threshold) 
				{
					//_ASSERTE(bc == Threshold);
					if(norm(o->YList->y) < MinimalDiagonalNorm)
						continue;
					OptimalNode = o;
					break;
				}
				if(bc < MinBC)
				{
					if(norm(o->YList->y) < MinimalDiagonalNorm)
						continue;
					MinBC = bc;
					OptimalNode = o;
				}
			}
		}
		if(!OptimalNode)
		{
			ATLTRACE("\nНе найден узел с надёжной диагональю. Осталось исключить %d. Текущий: %d", (CNode *)(&Base->SData) - (CNode *)Node, Node->Original - Nodes);
			this->Base = (CNode *)((char *)Node - ((char *)&Nodes->SData - (char *)Nodes));
			for(CNode * b = this->Base; b < Base; b++)
			{
				CNode * t = b->SData.Original;
				t->Type = CNode::MISSED;
				t->pY = 0;
			}

			CNode * iNode = Node->Original;
			for(CPFlat<CLU>::CPiece * List = LU.Last; List; List = List->Prev)
			{
				CLU * first = List->GetFirst();
				for(CLU * lu = List->Last; lu >= first; lu--)
				{
					if(CNode * jNode = lu->Node)
					{
						if(jNode->Type == CNode::MISSED)
						{
							iNode->Type = CNode::MISSED;
							iNode->pY = 0;
						}
					}
					else 
					{
						(*(CNode * *)&Node)--;
						iNode = Node->Original;
					}
				}
			}
			break;
			/*ATLTRACE("\nНе найден узел с надёжной диагональю. Осталось исключить %d. Текущий: %d", (CNode *)(&Base->SData) - (CNode *)Node, Node->Original - Nodes);
			DeleteList(YList);
			YList = 0;
			LU.Reset();
			pLU.Reset();
			return false;*/
		}
		// Ставим оптимальный узел на текущую позицию
		CNode * Old = Node->Original;
		if(Old != OptimalNode)
		{
			(Old->Sorted = OptimalNode->Sorted)->Original = Old;
			Node->Original = OptimalNode;
			OptimalNode->Sorted = Node;
		}
		// Исключаем узел
		Threshold = OptimalNode->BranchCount;
		OptimalNode = (CNode *)ExcludeNode(OptimalNode);
	}
	if(UseFastTriangulation) IdxToPointers();
	DeleteList(YList);
	YList = 0;
	return true;
}

void CNetwork::IdxToPointers(void)
{
	if(!LU.Valid() || !pLU.Valid()) return;
	LU.MakeFlat();
	pLU.MakeFlat();
	CLU * lu = LU.First->GetFirst();
	cplx * * fplu = pLU.First->GetFirst();
	for(cplx * * plu = pLU.First->Last; plu >= fplu; plu--)
	{
		if(unsigned int t = *(unsigned int *)plu)
		{
			unsigned int o = t & 0x80000000;
			t &= 0x7FFFFFFF;
			*plu = o ? &lu[t - 1].U : &lu[t - 1].L;
		}
	}
}


//! Метод ускоренной триангуляции матрицы проводимостей Y
bool CNetwork::TriangulateLU(void)
{	
	PCTRSTART(TriangulateLU);
	_ASSERTE(LU.Valid());
	_ASSERTE(pLU.Valid());
	CPFlat<cplx *>::CPiece * Piece = pLU.First;
	cplx * * plu = Piece->GetFirst();
	CPFlat<CLU>::CPiece * LUPiece = LU.First;
	CLU * lu = LUPiece->GetFirst();
	for(CNode::CSorted * Node = &Nodes->SData; true; )
	{
		CNode * Original = Node->Original;
		int bc = Original->BranchCount;
		CLU * lu_end = lu + bc;
		cplx &d = lu_end->L;
		if(norm(d) < MinimalDiagonalNorm) 
			return false;
		cplx D = d = 1.0 / d;
		CLU * lu_beg = lu;
		for(; lu < lu_end; lu++)
		{
			if(lu->Node->Type != CNode::USUAL) continue;
			lu->U *= D;
			_ASSERTE(Piece || (lu_beg == lu_end));
			for(CLU * luk = lu_beg; luk < lu_end; luk++) 
				**(plu++) -= lu->U * luk->L;
		}
		for(CLU * luk = lu_beg; luk < lu_end; luk++) luk->L *= D;
		lu++;

		(*(CNode * *)&Node)++;
		if((CNode *)Node >= Base) break;

		if(plu > Piece->Last)
		{
			_ASSERTE(plu == Piece->Last + 1);
			Piece = Piece->Next;
			plu = Piece->GetFirst();
		}
		if(lu > LUPiece->Last)
		{
			LUPiece = LUPiece->Next;
			_ASSERTE(LUPiece);
			lu = LUPiece->GetFirst();
		}
	}
	_ASSERTE(lu == LU.Last->Last + 1);
	PCTRSTOP(TriangulateLU);
	return true;
}

bool CNetwork::Prepare(void)
{
	if(!NodeCount) return false;
	PCTRSTART(Prepare);
	if(UseFastTriangulation && pLU.Valid())
	{
		if(!YisValid) PrepareY();
		ResetLU();
		if(TriangulateLU()) 
		{
			NeedPrepare = false;
			PCTRSTOP(Prepare);
			return true;
		}
	}
	PCTRSTART(PTYList);
	PrepareYList();
	NeedPrepare = !TriangulateYList();
	PCTRSTOP(PTYList);
	PCTRSTOP(Prepare);
	return !NeedPrepare;
}

/*************** Реализация метода решения ************************************/

//! Метод вычисления напряжений узлов V из задающих токов I по триангулированной матрице
bool CNetwork::Solve(void)
{
	PCTRSTART(Solve);
	PCTRSTART(Solve1);
	if(!Nodes || !NodeCount) return false;
	// Скопируем задающие токи, чтобы их не искажать
	if(UseSXN) for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--)
	{
		if(n->Type == CNode::USUAL || n->Type == CNode::MISSED)
		{
			if(CSXN * SXN = n->SXN)
			{
				NeedPrepare |= SXN->GetCurrent(n->Isxn, n);
				n->V = n->I + n->Isxn;
			}
			else
				n->V = n->I;
		}
	}
	else for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--)
		if(n->Type == CNode::USUAL || n->Type == CNode::MISSED) n->V = n->I;
	YisValid &= !NeedPrepare;
	PCTRSTOP(Solve1);
	PCTRSTART(Solve1a);

	if(NeedPrepare && !Prepare()) 
		return false;
	if(!LU.Valid())
		return false;
	PCTRSTOP(Solve1a);
	// Прямой ход решения
	PCTRSTART(Solve2);
	CNode::CSorted * Node = &Nodes->SData;
	CNode * iNode = Node->Original;
	for(CPFlat<CLU>::CPiece * List = LU.First; List; List = List->Next)
	{
		for(CLU * lu = List->GetFirst(); lu <= List->Last; lu++)
			if(CNode * jNode = lu->Node)
			{
				_ASSERTE(Node < &Base->SData);
				jNode->V -= iNode->V * lu->U;
			}
			else 
			{
				(*(CNode * *)&Node)++;
				iNode = Node->Original;
			}
		if(List == LU.Last) break;
	}
	_ASSERTE(Node == &Base->SData);
	// Обратный ход решения
	for(CPFlat<CLU>::CPiece * List = LU.Last; List; List = List->Prev)
	{
		CLU * first = List->GetFirst();
		for(CLU * lu = List->Last; lu >= first; lu--)
		{
			if(CNode * jNode = lu->Node)
			{
				_ASSERTE(Node >= &Nodes->SData);
				iNode->V -= jNode->V * lu->L;
			}
			else 
			{
				(*(CNode * *)&Node)--;
				iNode = Node->Original;
				iNode->V *= lu->L;
			}
		}
	}
	PCTRSTOP(Solve2);
	PCTRSTART(Solve3);
	_ASSERTE(Node == &Nodes->SData);
#ifdef _DEBUG
	double t = Test();
	if(t > 0.1)
		return false;
#endif
//!!!!!!!!! Тестируем CalcPolarV() !!!!!!!!!!!!!//////////////////////////
	/*{
		static PCTR Polarx87_, PolarSSE2_;
		struct tmp{ double Arg, Mod, Arg2, Mod2;} * b = (tmp *)malloc(NodeCount * sizeof(tmp));
		for(int x = 0; x < NodeCount; x++)
		{
			b[x].Arg = Nodes[x].ArgV;
			b[x].Mod = Nodes[x].ModV;
		}
		UseSSE2 = false;
		PCTRSTART(Polarx87);
		CalcPolarV();
		PCTRSTOP(Polarx87);
		for(int x = 0; x < NodeCount; x++)
		{
			b[x].Arg2 = Nodes[x].ArgV;
			b[x].Mod2 = Nodes[x].ModV;
			Nodes[x].ArgV = b[x].Arg;
			Nodes[x].ModV = b[x].Mod;
		}
		UseSSE2 = GetSSE2Present();
		PCTRSTART(PolarSSE2);
		CalcPolarV();
		PCTRSTOP(PolarSSE2);
		static double na = 0, nm = 0, t;
		for(int x = 0; x < NodeCount; x++)
		{
			t = abs(b[x].Arg2 - Nodes[x].ArgV);
			if(t > na) na = t;
			t = abs(b[x].Mod2 - Nodes[x].ModV);
			if(t > nm) nm = t;
		}
		if(na + nm > 1E-8)
			return false;
		free(b);
	}*/

	// E.M. 25.11.2008 пока для простоты
	CalcPolarV();
	PCTRSTOP(Solve3);
	PCTRSTOP(Solve);
	return true;
}

double atan2analog(double Im, double Re)
{
	const double Pi = M_PI, N = 30;
	double ctg = abs( Re / (Im ? Im : 1.0));
	double s = 1.0, d = 0.0;
	if(!Im)
	{
		ctg = 3.0;
		s = 0.0;
	}
	if(ctg > 1.0) ctg = 1 / ctg;
	else 
	{
		s = -1.0;
		d = Pi/2;
	}
	if(Re < 0)
	{
		d = Pi - d;
		s = -s;
	}
	if(Im < 0)
	{
		s = -s;
		d = -d;
	}
	if(ctg > 0.42) 
	{
		ctg = (1 - ctg) / (1 + ctg);
		d += s * Pi/4;
		s = -s;
	}
	double a = 0.0;
	for(double n = 1, x = ctg, x2 = -x * x; n < N; n += 2.0, x *= x2) 
		a += x / n;
	a = s * a + d;
	return a;
}

void CNetwork::CalcPolarV() 
{
	// Константы для SSE2
	__declspec(align(16)) static struct Ti{unsigned __int64 A, B;} 
		Sign = {0x8000000000000000, 0x8000000000000000}, nSign = {0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF},
		FFF = {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF};
	__declspec(align(16)) static struct T{double A, B;} One = {1.0, 1.0},
	mOne = {-1.0, -1.0}, Thr = {0.42, 0.42}, Two = {2.0, 2.0},
	PI = {M_PI, M_PI}, PI2 = {M_PI_2, M_PI_2}, PI4 = {M_PI_4, M_PI_4}, d2PI = {0.5 / M_PI, 0.5 / M_PI}, PIx2 = {2 * M_PI, 2 * M_PI},
	r3 = {1/3.0, 1/3.0}, r5 = {1/5.0, 1/5.0}, r7 = {1/7.0, 1/7.0}, r9 = {1/9.0, 1/9.0}, r11 = {1/11.0, 1/11.0},
	r13 = {1/13.0, 1/13.0}, r15 = {1/15.0, 1/15.0}, r17 = {1/17.0, 1/17.0}, r19 = {1/19.0, 1/19.0}, 
	r21 = {1/21.0, 1/21.0}, r23 = {1/23.0, 1/23.0}, r25 = {1/25.0, 1/25.0}, r27 = {1/27.0, 1/27.0},
	r29 = {1/29.0, 1/29.0}, r31 = {1/31.0, 1/31.0};
	//////////////////

	PCTRSTART(PolarV);

	CNode * Node = Nodes, * StopNode = Nodes + NodeCount; // Область действия функции
	int sz = sizeof(CNode);
	DWORD MXCSRold, MXCSRnew;

	if(UseSSE2) _asm{
		// Режим округления - к отрицательной бесконечности
#define s xmm6
#define d xmm5
#define Re xmm4
#define Im xmm3
		STMXCSR		MXCSRold
		MOV			EAX, MXCSRold
		OR			AH, 0x20
		AND			AH, 0xBF
		MOV			MXCSRnew, EAX
		LDMXCSR		MXCSRnew
		// Инициализация цикла
		MOV			ECX, sz			// Размер узла
		MOV			EAX, Node		// Первый узел
		MOV			ESI, StopNode	// Граничный узел
		SUB			EAX, ECX
		XOR			EBX, EBX
	CalcPolarCycle:
		// Ищем первый узел
		ADD			EAX, ECX
		CMP			EAX, ESI
		JNB			CalcPolarStop
		CMP			EBX, [EAX + CNode::Type] //CNode::DISABLED = 0
		JZ			CalcPolarCycle
		// Ищем второй узел
		MOV			EDX, EAX
		MOVAPD		Im,		[EAX + CNode::V]
	FindSecondNode:
		ADD			EDX, ECX
		CMP			EDX, ESI
		JNB			CalcPolarStop
		CMP			EBX, [EDX + CNode::Type] //CNode::DISABLED = 0
		JZ			FindSecondNode
		// Найденые узлы записаны в регистрах EAX и EDX

		///// Расчёт двух модулей
		MOVAPD		xmm1,	[EDX + CNode::V]
		MOVAPD		xmm2, Im
		UNPCKHPD	Im, xmm1 ; image
		MOVAPD		Re, xmm2
		UNPCKLPD	Re, xmm1 ; real
		MOVAPD		xmm1, Im // xmm7 = image !
		MULPD		xmm1, xmm1 
		MOVAPD		xmm5, Re // xmm6 = real !
		MULPD		xmm5, xmm5 
		ADDPD		xmm1, xmm5
		XORPD		d, d
		SQRTPD		xmm1, xmm1
		MOVAPD		s, One
		MOVAPD		xmm0, d
		MOVLPD		[EAX + CNode::ModV], xmm1
		MOVHPD		[EDX + CNode::ModV], xmm1
		// Расчёт угла
#define ctg xmm7
	// double ctg = abs( Re / (Im ? Im : 1.0));
	//double s = 1.0, d = 0.0;
		MOVAPD		xmm1, Im
		MOVAPD		ctg, Re
		CMPPD		xmm0, Im, 0 // xmm0 = (Im == 0.0)
		MOVAPD		xmm2, xmm0  // xmm2 = (Im == 0.0)
		ANDPD		xmm0, s		// xmm0 = Im ? 0.0 : 1.0
		ORPD		xmm1, xmm0	// xmm1 = Im ? Im : 1.0
		MOVAPD		xmm0, s
		DIVPD		ctg, xmm1
		ANDPD		xmm0, xmm2 // xmm0 = s & (Im == 0.0)
		ANDPD		ctg, nSign // Получаем модуль тангенса
	//if(Im == 0)
	//	s = 0.0;
		XORPD		s, xmm0			// s = s & (Im != 0.0)
	//if(ctg > 1.0) ctg = 1 / ctg; // ctg = (ctg > 1.0) ? 1.0 / ctg : ctg
	//else                         // a ? b : c = a & b | ~a & c
	//if(Im[&& ctg <= 1.0]){	   // ctg = (ctg > 1.0) & (1.0 / ctg) | (ctg <= 1.0) & ctg
	//	s = -1.0;
	//	d = Pi/2;
	//}
		XORPD		xmm1, xmm1
		MOVAPD		xmm0, One
		CMPPD		xmm1, ctg, 0	// xmm1 = (ctg == 0)
		ANDPD		xmm1, xmm0		// xmm1 = 1.0 & (ctg == 0.0)
		ORPD		xmm1, ctg		// xmm1 = ctg ? ctg : 1.0
		DIVPD		xmm0, xmm1		// xmm0 = 1.0 / (ctg ? ctg : 1.0)
		MOVAPD		xmm1, One
		CMPPD		xmm1, ctg, 5	// xmm1 = (ctg <= 1.0)

		ANDNPD		xmm2, xmm1		// xmm2 = Mask = (Im != 0.0) & (ctg <= 1.0)
		ANDPD		ctg, xmm1		// ctg = ctg & (ctg <= 1.0)
		ANDNPD		xmm1, xmm0		// xmm1 = (ctg > 1.0) & (1.0 / ctg)
		ORPD		ctg, xmm1		// ctg = (ctg > 1.0) ? 1.0 / ctg : ctg
		// s = -1.0 & (xmm2) | s & ~xmm2; d = Pi/2 & (xmm2) | d & ~xmm2;
		MOVAPD		xmm0, mOne
		MOVAPD		xmm1, PI2
		ANDPD		xmm0, xmm2		// xmm0 = Mask & -1.0
		ANDPD		xmm1, xmm2		// xmm1 = Mask & Pi / 2
		XORPD		xmm2, FFF
		ANDPD		s, xmm2			// s = ~Mask & s
		ANDPD		d, xmm2			// d = ~Mask & d
		ORPD		s, xmm0
		ORPD		d, xmm1
	//if(Re < 0)
	//{
	//	d = Pi - d;
	//	s = -s;
	//}
		XORPD		xmm0, xmm0
		CMPPD		Re, xmm0, 1		// Re = (Re < 0)
		MOVAPD		xmm1, PI
		MOVAPD		xmm0, Sign
		SUBPD		xmm1, d			// xmm1 = Pi - d
		MOVAPD		xmm2, xmm0		// xmm2 = Sign
		ANDPD		xmm0, Re		// xmm0 = Sign & (Re < 0)
		ANDPD		xmm1, Re		// xmm1 = (Pi - d) & (Re < 0)
		XORPD		s, xmm0			// s ^= Sign & (Re < 0)
		ANDNPD		Re, d			// Re = (Re >= 0) & d
		MOVAPD		d, xmm1
		ORPD		d, Re			// d = (Re >= 0) & d | (Re < 0) & (Pi - d)
#undef Re
	//if(Im < 0)
	//{
	//	s = -s;
	//	d = -d;
	//}
		XORPD		xmm0, xmm0
		CMPPD		Im, xmm0, 1 // Im = (Im < 0)
		ANDPD		Im, xmm2 // Sign; Im = Sign & (Im < 0)
		XORPD		s, Im
		XORPD		d, Im
#undef Im
	//if(ctg > 0.42) 
	//{
	//	ctg = (1 - ctg) / (1 + ctg);
	//	d += s * Pi/4;
	//	s = -s;
	//}
		MOVAPD		xmm0, Thr
		MOVAPD		xmm3, One
		CMPPD		xmm0, ctg, 1	// xmm0 = (ctg > 0.42)
		MOVAPD		xmm1, ctg
		MOVAPD		xmm4, PI4
		ADDPD		xmm1, xmm3		// xmm1 = 1 + ctg
		MULPD		xmm4, s			// xmm4 = s * Pi/4
		SUBPD		xmm3, ctg		// xmm3 = 1 - ctg
		DIVPD		xmm3, xmm1		// xmm3 = (1 - ctg) / (1 + ctg);
		ANDPD		xmm4, xmm0		// xmm4 = (s * Pi/4) & (ctg > 0.42)
		ANDPD		xmm2, xmm0		// xmm2 = Sign & (ctg > 0.42)
		ADDPD		d, xmm4			// d += (s * Pi/4) & (ctg > 0.42);
		XORPD		s, xmm2			// if(ctg > 0.42) s = -s;
		ANDPD		xmm3, xmm0		// xmm3 = (1 - ctg) / (1 + ctg) & (ctg > 0.42);
		ANDNPD		xmm0, ctg		// xmm0 = (ctg <= 0.42) & ctg
		ORPD		xmm3, xmm0		// xmm3 = (ctg > 0.42) ? (1 - ctg) / (1 + ctg) : ctg;
#undef ctg
#define ctg xmm3
	//double a = 0.0;
	//for(double n = 1, x = ctg, x2 = -x * x; n < N; n += 2.0, x *= x2) 
	//	a += x / n;
#define Angle xmm4
#define x2 xmm2
		MOVAPD		x2, ctg
		MOVAPD		xmm4, Two
		MULPD		x2, x2
		MOVAPD		Angle, ctg // Первая итерация
		XORPD		x2, Sign // x2 = -x * x
	// Цикл
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r3
		MULPD		xmm0, ctg
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r5 
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r7 
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r9 
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r11 
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r13 
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r15
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r17
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r19
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r21
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r23
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r25
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r27
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r29
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
		MULPD		ctg, x2 // x *= x2
		MOVAPD		xmm0, r31
		MULPD		xmm0, ctg 
		ADDPD		Angle, xmm0 // a += x / n
	// Конец цикла ////////////////////////////////////////////////////////////
#undef x2 
#undef ctg
	// a = s * a + d;
		ADDPD		d, PI
		MULPD		Angle, s
#undef s
#undef d // = xmm5
#undef Angle
	// atan2 окончен, xmm4 - угол //////////////////////////////////////////////////////////
	
	// double d = (atan2(n->V.imag(), n->V.real()) - n->ArgV + M_PI) / (2 * M_PI);
		MOVLPD		xmm0, [EAX + CNode::ArgV]
		ADDPD		xmm4, xmm5
		MOVHPD		xmm0, [EDX + CNode::ArgV]	// xmm0 = ArgV
		SUBPD		xmm4, xmm0
		MULPD		xmm4, d2PI					// xmm4 = d = (arg(n->V) - n->ArgV + M_PI) / (2 * M_PI)
	// n->ArgV += (d - floor(d)) * 2 * M_PI - M_PI;
		CVTPD2DQ	xmm1, xmm4 //ROUNDPD		xmm1, xmm4, 1
		CVTDQ2PD	xmm1, xmm1
		SUBPD		xmm4, xmm1					// xmm4 = d - floor(d);
		MULPD		xmm4, PIx2
		SUBPD		xmm4, PI
		ADDPD		xmm0, xmm4	// xmm0 = ArgV += (d - floor(d)) * 2 * M_PI - M_PI;
		MOVLPD		[EAX + CNode::ArgV], xmm0
		MOVHPD		[EDX + CNode::ArgV], xmm0

		MOV			EAX, EDX // Следующий узел
		JMP			CalcPolarCycle
	CalcPolarStop:
		MOV			Node, EAX
		LDMXCSR		MXCSRold
	}
	for(; Node < StopNode; Node++) if(Node->Type != CNode::DISABLED) 
	{ // Доделать то, что не доделала SSE2-реализация
		Node->ModV = sqrt(Node->V.real() * Node->V.real() + Node->V.imag() * Node->V.imag());
		double d = (atan2(Node->V.imag(), Node->V.real()) - Node->ArgV + M_PI) / (2 * M_PI);
		Node->ArgV += (d - floor(d)) * 2 * M_PI - M_PI;
	}
	PCTRSTOP(PolarV);
}

/*void CNetwork::CalcPolarV() 
{
	PCTRSTART(PolarV);
	for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--) if(n->Type != CNode::DISABLED)
	{

		//PCTRSTART(PolarV1);
		n->ModV = sqrt(n->V.real() * n->V.real() + n->V.imag() * n->V.imag());
		//PCTRSTOP(PolarV1);
		//PCTRSTART(PolarV2);
		double d = (atan2(n->V.imag(), n->V.real()) - n->ArgV + M_PI) / (2 * M_PI);
		//PCTRSTOP(PolarV2);
		//PCTRSTART(PolarV3);
		n->ArgV += (d - floor(d)) * 2 * M_PI - M_PI;
		//PCTRSTOP(PolarV3);
	}
	PCTRSTOP(PolarV);
}*/


////////////////////////// СХН ////////////////////////////////

/*void * CNetwork::CSXN::Apply(cplx * Y, cplx &I, const cplx &V, const double &Vnom, const cplx &Snom)
{
	double Vn_2 = 1 / (Vnom * Vnom);
	double Vr = sqrt(norm(V) * Vn_2);
	for(CFactors * f = &Factors; f; f = f->Next) if(!f->Next || f->Vrmax > Vr)
	{
		cplx KI = f->Factors[0] + f->Factors[1] * Vr;
		cplx Si = cplx(KI.real() * Snom.real(), KI.imag() * Snom.imag());
		I += conj(Si / V);
		if(Y)
			*Y -= cplx(f->Factors[2].real() * Snom.real(), - f->Factors[2].imag() * Snom.imag()) * Vn_2;
		return f;
	}
	return 0;
}*/

void CNetwork::CSXN::ApplyToY(cplx &Y, CNode * Node)
{
	double &Vnom = Node->Vnom;
	cplx &Snom = Node->Sn;
	if(CFactors * f = (CFactors *) Node->LastFactors)
		Y -= cplx(f->Factors[2].real() * Snom.real(), - f->Factors[2].imag() * Snom.imag()) / (Vnom * Vnom);
}

void CNetwork::CSXN::GetSn(cplx &Sn, CNode * Node)
{
	double Vr = sqrt(norm(Node->V)) / Node->Vnom;
	cplx &Snom = Node->Sn;
	CFactors * f = (CFactors *) Node->LastFactors;
	_ASSERTE(f);
	Sn = cplx(
		(f->Factors[0].real() + f->Factors[1].real() * Vr + f->Factors[2].real() * Vr * Vr + f->Frec * Node->SlipU) * Snom.real(),
		(f->Factors[0].imag() + f->Factors[1].imag() * Vr + f->Factors[2].imag() * Vr * Vr) * Snom.imag()
		);
}


bool CNetwork::CSXN::GetCurrent(cplx &I, CNode * Node)
{
	cplx &V = Node->V;
	double Vr = abs(V) / Node->Vnom;
	_ASSERTE(Factors);
	for(CFactors * f = Factors; f; f = f->Next) if(!f->Next || f->Vrmin < Vr)
	{
		cplx KI = f->Factors[0] + f->Factors[1] * Vr;
		cplx &Snom = Node->Sn;
		cplx Si = cplx((KI.real() + f->Frec * Node->SlipU) * Snom.real(), KI.imag() * Snom.imag());
		I = conj(Si / V);
		bool b = (Node->LastFactors != f);
		Node->LastFactors = f;
		return b;
	}
	return 0;
}

CNetwork::CSXN::~CSXN(void)
{
	DeleteList(Factors);
}

CNetwork::CSXN::CSXN(/*const cplx &f0, const cplx &f1, const cplx &f2*/)
{
	Factors = 0;/*.Next = 0;
	Factors.Factors[0] = f0;
	Factors.Factors[1] = f1;
	Factors.Factors[2] = f2;*/
}

void CNetwork::CSXN::AddFactors(const double &Vrmin, const cplx &f0, const cplx &f1, const cplx &f2, const double &Frec)
{
	CFactors * nf = (CFactors *)malloc(sizeof(CFactors));
	nf->Vrmin = Vrmin;
	nf->Factors[0] = f0;
	nf->Factors[1] = f1;
	nf->Factors[2] = f2;
	nf->Frec = Frec;
	CFactors * * p = &Factors;
	for(; *p; p = &((*p)->Next)) 
		if((*p)->Vrmin < Vrmin)
			break;
	nf->Next = *p;
	*p = nf;
}

void CNetwork::CSXN::SetMinVoltage(const double &Vrmin)
{
	_ASSERTE(Factors);
	for(CFactors * p = Factors; p; p = p->Next) 
		if(!p->Next || p->Vrmin < Vrmin)
		{
			DeleteList(p->Next);
			CFactors * f = (CFactors *)malloc(sizeof(CFactors));
			if(p->Vrmin < Vrmin) p->Vrmin = Vrmin;
			p->Next = f;
			f->Vrmin = 0;
			f->Frec = 0;
			f->Factors[0] = 0.0;
			f->Factors[1] = 0.0;
			f->Factors[2] = p->Factors[0] / (p->Vrmin * p->Vrmin) + p->Factors[1] / p->Vrmin + p->Factors[2];
			f->Next = 0;
			break;
		}
}

double CNetwork::CSXN::GetMaxBreak(void)
{
	_ASSERTE(Factors);
	double b = 0;
	for(CFactors * p = Factors, * np; p; p = np)
		if(np = p->Next)
		{
			double V = p->Vrmin;
			double V2 = V * V;
			double d = abs(np->Factors[0] + np->Factors[1] * V + np->Factors[2] * V2 - p->Factors[0] - p->Factors[1] * V - p->Factors[2] * V2);
			if(d > b) b = d;
		}
	return b;
}

/*void CNetwork::CreateStandardSXN(void)
{
	CSXN * SXN1 = CreateSXN(1, cplx(0.83, 0.721), cplx(-0.3, 0.158), cplx(0.47, 0.0));
	SXN1->AddFactors(0.815,    cplx(0.83, 3.7),   cplx(-0.3, -7.0),  cplx(0.47, 4.3));
	SXN1->AddFactors(1.2,      cplx(0.83, 1.49),  cplx(-0.3, 0.0),   cplx(0.47, 0.0));

	CSXN * SXN2 = CreateSXN(2, cplx(0.83, 0.657), cplx(-0.3, 0.158), cplx(0.47, 0.0));
	SXN2->AddFactors(0.815,    cplx(0.83, 4.9),   cplx(-0.3, -10.1), cplx(0.47, 6.2));
	SXN2->AddFactors(1.2,      cplx(0.83, 1.708), cplx(-0.3, 0.0),   cplx(0.47, 0.0));
}*/

double CNetwork::GetMaxSXNDisc(void)
{
	if(!UseSXN) return 0.0;
	double r = 0;
	for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--)
		if(CSXN * SXN = n->SXN)
		{
			cplx I;
			NeedPrepare |= SXN->GetCurrent(I, n);
			double d = abs(I - n->Isxn);
			if(r < d) r = d;
		}
	return r;
}

double CNetwork::Test(void)
{
	cplx * I = (cplx *)malloc(NodeCount * sizeof(cplx));

	cplx * i = I + NodeCount - 1;
	for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--, i--) // Заполняем Y из узлов
		*i = (-n->y - n->y1) * n->V;
	i = I + NodeCount - 1;
	if(UseSXN) for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--, i--)
		if(n->SXN) 
		{
			*i -= n->Isxn;
			cplx Y = 0.0;
			n->SXN->ApplyToY(Y, n);
			*i += Y * n->V;
		}

	for(std::map<int, cplx>::iterator it = Shunts.begin(); it != Shunts.end(); it++) // Из шунтов
		I[it->first] -= it->second * Nodes[it->first].V;
	for(CBranch * b = Branches + BranchCount - 1; b >= Branches; b--)  // Заполняем Y из ветвей
	{
		CNode * p = Nodes + b->ip;
		CNode * q = Nodes + b->iq;
		cplx Yp = 0, Yq = 0, Ypq = 0, Yqp = 0;
		b->Apply<true>(&Yp, &Yq, &Ypq, &Yqp);
		I[b->ip] += Yp * p->V + Ypq * q->V;
		I[b->iq] += Yq * q->V + Yqp * p->V;
	}

	double s = 0;
	for(int x = 0; x < NodeCount; x++) if(Nodes[x].Type == CNode::USUAL)
		s += norm(I[x] - Nodes[x].I);

	free(I);
	return s;
}

//////////////////// Уведомления //////////////////////////////////////

void CNetwork::SetBranchState(int Branch, CBranch::STATE State)
{
	_ASSERTE(Branch >= 0 && Branch < BranchCount);
	CBranch * b = Branches + Branch;
	if(b->State == State) return;
	NeedPrepare = true;
	if(!UseFastTriangulation)
	{
		b->State = State;
		return;
	}
	CNode * p = Nodes + b->ip;
	CNode * q = Nodes + b->iq;
	if(State == CBranch::CONN && (
		((p->Type == CNode::USUAL && !b->pYpq) || (q->Type == CNode::USUAL && !b->pYqp)) // Ветви нет в матрице проводимости, надо всё пересчитывать...
		|| p->Type == CNode::MISSED || q->Type == CNode::MISSED // Возможно включение потеряного острова
		))
	{ 
		b->State = State;
		LU.Reset();
		pLU.Reset();
		return;
	}
	b->Apply<false>(p->pY, q->pY, b->pYpq, b->pYqp);
	b->State = State;
	b->Apply<true>(p->pY, q->pY, b->pYpq, b->pYqp);
}

void CNetwork::SetBranchParams(int Branch, const cplx * y, const cplx * yp, const cplx * yq, const cplx * kt)
{
	NeedPrepare = true;
	_ASSERTE(Branch >= 0 && Branch < BranchCount);
	CBranch * b = Branches + Branch;
	CNode * p = Nodes + b->ip;
	CNode * q = Nodes + b->iq;
	if(UseFastTriangulation)
		b->Apply<false>(p->pY, q->pY, b->pYpq, b->pYqp);
	if(y) b->y = *y;
	if(yp) b->yp = *yp;
	if(yq) b->yq = *yq;
	if(kt) b->kt = *kt;
	if(UseFastTriangulation)
		b->Apply<true>(p->pY, q->pY, b->pYpq, b->pYqp);
}

void CNetwork::SetNodeType(int Node, CNode::TYPE Type)
{
	_ASSERTE(Node >= 0 && Node < NodeCount);
	CNode * n = Nodes + Node;
	if(n->Type == Type) return;
	n->Type = Type;
	LU.Reset();
	pLU.Reset();
	NeedPrepare = true;
}

void CNetwork::SetNodeParams(int Node, const cplx * y, const cplx * y1)//!< Устанавивает шунты в узле. y - шунт нагрузки (сети), y1 - шунт генератора.
{ // Если указатель равен 0, соответствующее значение не меняется.
	_ASSERTE(Node >= 0 && Node < NodeCount);
	CNode * n = Nodes + Node;
	NeedPrepare = true;
	if(cplx * p = n->pY)
	{
		if(y) *p += n->y - *y;
		if(y1) *p += n->y1 - *y1;
	}
	if(y) n->y = *y;
	if(y1) n->y1 = *y1;
}


void CNetwork::SetShunt(int Node, const cplx &Y) //!< Устанавивает дополнительный шунт в узле. Используется для установки КЗ.
{
	NeedPrepare = true;
	_ASSERTE(Node >= 0 && Node < NodeCount);
	CNode * n = Nodes + Node;
	std::map<int, cplx>::iterator i = Shunts.find(Node);
	if(i != Shunts.end())
	{
		if(UseFastTriangulation && n->pY) *(n->pY) += i->second;
		i->second = Y;
	}
	else
		Shunts.insert(std::make_pair(Node, Y));
	if(UseFastTriangulation && n->pY) *(n->pY) -= Y;
}

void CNetwork::RemoveShunt(int Node)
{
	_ASSERTE(Node >= 0 && Node < NodeCount);
	std::map<int, cplx>::iterator i = Shunts.find(Node);
	if(i != Shunts.end())
	{
		NeedPrepare = true;
		CNode * n = Nodes + Node;
		if(UseFastTriangulation && n->pY) *(n->pY) += i->second;
		Shunts.erase(i);
	}
}

const cplx& CNetwork::GetShunt(int Node) //!< Возвращает дополнительный шунт в узле.
{
	_ASSERTE(Node >= 0 && Node < NodeCount);
	std::map<int, cplx>::iterator i = Shunts.find(Node);
	if(i != Shunts.end()) return i->second;
	static cplx r(0.0);
	return r;
}

int CNetwork::SetShortCircuit(int Branch, const cplx * Shunt, double Position) // Позиция от 0 до 1.
{
	NeedPrepare = true;
	_ASSERTE(Branch >= 0 && Branch < BranchCount);
	_ASSERTE((Position >= 0.0) && (Position <= 1.0));
	CBranch * B1 = Branches + Branch;
	_ASSERTE(B1->yp == B1->yq);
	_ASSERTE(B1->kt == 1.0);

	CShortCurcuit * SC;
	CBranch * B2;
	// Определяем - делилась ли ветвь?
	std::map<int, CShortCurcuit>::iterator i = ShortCurcuits.find(Branch);
	if(i != ShortCurcuits.end())
	{ // Ветвь делилась, узел и ветвь существуют
		SC = &i->second;
		B2 = Branches + SC->Branch;
		SC->Position = Position;
	}
	else
	{ 
		CShortCurcuit t;
		SC = &t;
		// Определить параметры участков ветви по методике "Реализация короткого замыкания на линии"
		cplx ydyb = B1->yp / B1->y;
		SC->yc = sqrt((ydyb + 2.0) * B1->yp * B1->y); // (1 / yc) == Zc : волновое сопротивление
		SC->gl = log(ydyb + 1.0 - SC->yc / B1->y); // gl - постоянная распространения на длину
		// Создаём узел и ветвь
		Clean();
		SC->Node = NodeCount++;
		SC->Branch = BranchCount++;
		Nodes = (CNode *) realloc(Nodes, NodeCount * sizeof(CNode));
		Branches = (CBranch *) realloc(Branches, BranchCount * sizeof(CBranch));
		B1 = Branches + Branch;
		B2 = Branches + SC->Branch;
		memset(B2, 0, sizeof(*B2));
		memset(Nodes + SC->Node, 0, sizeof(CNode));
		B2->ip = SC->Node;
		B2->iq = B1->iq;
		B2->kt = 1.0;
		B1->iq = SC->Node;
		SC->Position = Position;
		ShortCurcuits.insert(std::make_pair(Branch, t));
	}

	// Расчёт параметров кусков линии
	cplx gl1 = SC->gl * Position;
	cplx gl2 = SC->gl - gl1;
	// Восстанавливаем параметры П-образной схемы первого отрезка линии
	cplx egl = exp(gl1);
	cplx e_gl = exp(-gl1);
	cplx yb = SC->yc * 2.0 / (e_gl - egl);
	cplx y = ((egl + e_gl) * 0.5 - 1.0) * yb;
	B1->y = yb;
	B1->yp = B1->yq = y;
	// Восстанавливаем параметры П-образной схемы второго отрезка линии
	egl = exp(gl2);
	e_gl = exp(-gl2);
	yb = SC->yc * 2.0 / (e_gl - egl);
	y = ((egl + e_gl) * 0.5 - 1.0) * yb;
	B2->y = yb;
	B2->yp = B2->yq = y;
	// Инициализируем узел
	CNode & Node = Nodes[SC->Node];
	if(Shunt && (*Shunt == 0.0))
	{
		Node.Type = CNode::CONST_V;
		Node.V = 0;
	}
	else
	{
		Node.Type = CNode::USUAL;
		Node.y = Shunt ? conj(1.0 / *Shunt) : 0;
	}
	YisValid = false; // Необходимо реализовать коррекцию Y
	return 0;
}

#define SQR_3 0.57735026918962576450914878050196

void CNetwork::GetBranchCurrent(int Branch, bool End, cplx &Current)
{
	_ASSERTE(Branch >= 0 && Branch < BranchCount);
	CBranch * b = Branches + Branch;
	if(b->State != CBranch::CONN)
	{
		Current = 0.0;
		return;
	}
	CNode * p = Nodes + b->ip;
	CNode * q = Nodes + b->iq;
	if(End)
	{
		//Current = (q->V * b->yq + (p->V - q->V * b->kt) * b->y / (b->kt * b->kt)) * SQR_3;
		Current = (q->V * b->yq + (q->V / b->kt - p->V) * b->y / conj(b->kt)) * SQR_3;
	}
	else
	{
		//Current = (p->V * b->yp + (q->V - p->V / b->kt) * b->y) * SQR_3;
		Current = (p->V * b->yp - ( q->V / b->kt - p->V ) * b->y) *SQR_3;
	}
}

void CNetwork::GetBranchPower(int Branch, bool End, cplx &Power)
{
	_ASSERTE(Branch >= 0 && Branch < BranchCount);
	CBranch * b = Branches + Branch;
	if(b->State != CBranch::CONN)
	{
		Power = 0.0;
		return;
	}
	CNode * p = Nodes + b->ip;
	CNode * q = Nodes + b->iq;
	cplx Current;
	if(End)
	{
		Current = (q->V * b->yq + (q->V / b->kt - p->V) * b->y / conj(b->kt));
		//Power = q->V * b->yq + (q->V - p->V * b->kt) * b->y / (b->kt * b->kt);
		Power = q->V * conj(Current);
	}
	else
	{
		//Current = (( q->V / b->kt - p->V ) * b->y - p->V * conj(b->yp));
		Current = (p->V * b->yp - ( q->V / b->kt - p->V ) * b->y);
		//Power = p->V * b->yp + (p->V - q->V / b->kt) * b->y;
		Power = p->V * conj(Current);
	}
	//Power = (End ? q : p)->V * conj(Power);
}

void CNetwork::GetBranchResistance(int Branch, bool End, cplx &Resistance)
{
	_ASSERTE(Branch >= 0 && Branch < BranchCount);
	CBranch * b = Branches + Branch;
	CNode * p = Nodes + b->ip;
	CNode * q = Nodes + b->iq;
	if(End)
		Resistance = q->V * b->yq + (q->V - p->V * b->kt) * b->y / (b->kt * b->kt);
	else
		Resistance = p->V * b->yp + (p->V - q->V / b->kt) * b->y;
	Resistance = (End ? q : p)->V / Resistance;
}

int CNetwork::MarkIslands(void)
{
	PCTRSTART(MarkIslands);
	_ASSERTE(UseFastTriangulation);
	if(!LU.Valid() && !Prepare()) 
		return 0;
	if(!YisValid) PrepareY();

	for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--) n->Island = 0;

	int Island = 0;
	CNode::CSorted * Node = &Base->SData;
	CNode * iNode = 0;
	int * IslandsMap = 0;
	int Size = 0;
	int iIsland = 1;
	for(CPFlat<CLU>::CPiece * List = LU.Last; List; List = List->Prev)
	{
		CLU * first = List->GetFirst();
		for(CLU * lu = List->Last; lu >= first; lu--)
		{
			if(CNode * jNode = lu->Node)
			{
				_ASSERTE(Node >= &Nodes->SData);
				if(!lu->Y || norm(lu->Y->Yij) < 1E-10) continue; // Тестируем связь
				if(iIsland)
				{
					int jIsland = jNode->Island;
					if(iIsland == jIsland) continue;
					if(jIsland)
					{ // Объединение островов
						int i = iIsland, j = jIsland;
						int mx = max(i, j);
						if(Size <= mx)
						{
							int ns = (mx + 1) | 31;
							IslandsMap = (int *) realloc(IslandsMap, ns * sizeof(int));
							memset(IslandsMap + Size, 0, (ns - Size) * sizeof(int));
							Size = ns;
						}
						while(i != j) if(i > j)
						{
							int t = i; i = IslandsMap[i];
							if(i < j) IslandsMap[t] = j;
						}
						else
						{
							int t = j; j = IslandsMap[j];
							if(j < i) IslandsMap[t] = i;
						}
					}
					else jNode->Island = iIsland; // Обратная раскраска
				}
				else
					if(int jIsland = jNode->Island)
						iIsland = iNode->Island = jIsland; // Прямая раскраска
			}
			else 
			{
				if(!iIsland)
				{ // Определить остров не удалось - создаём новый
					iIsland = iNode->Island = ++Island;
					lu += iNode->BranchCount + 1; // Откат
					continue;
				}
				(*(CNode * *)&Node)--;
				iNode = Node->Original;
				iIsland = iNode->Island;
			}
		}
	}
	if(!iIsland) // Определить остров не удалось - создаём новый
		iNode->Island = ++Island;
	_ASSERTE(Node == &Nodes->SData);
	
	if(IslandsMap) // Обрабатываем слияния островов
	{
		if(Size <= Island)
		{
			IslandsMap = (int *) realloc(IslandsMap, (Island + 1) * sizeof(int));
			memset(IslandsMap + Size, 0, (Island - Size + 1) * sizeof(int));
		}
		for(int * x = IslandsMap + Island; x >= IslandsMap; x--)
			while(*x && IslandsMap[*x]) *x = IslandsMap[*x];
		int i = 0;
		for(int * x = IslandsMap; x <= IslandsMap + Island; x++)
			if(!*x) *x = i++;
		for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--) 
			n->Island = IslandsMap[n->Island];
		free(IslandsMap);
		Island = i - 1;
	}
	for(CNode * n = Nodes + NodeCount - 1; n >= Base; n--)
	{
		CNode * t = n->SData.Original;
		if(!t->Island) t->Island = ++Island;
	}
	for(CBranch * b = Branches + BranchCount - 1; b >= Branches; b--) 
		switch(b->State)
		{
		case CBranch::CONN:
		case CBranch::Q_DISC:
			b->Island = Nodes[b->ip].Island;
			break;
		case CBranch::P_DISC:
			b->Island = Nodes[b->iq].Island;
			break;
		default:
			b->Island = 0;
		}
	PCTRSTOP(MarkIslands);
	return Island;
}

// Быстрая сортировка
void FastSort(int * First, int * Last)
{
	int * up = First;
	int * down = Last;
	int f = *up;
	int s = *(up + 1);
	while(down > up)
	{
		if(s < f)
		{
			*(up++) = s;
			s = *(up + 1);
		}
		else
		{
			int t = *(down);
			*(down--) = s;
			s = t;
		}
	}
	*up = f;
	if(First < --down) FastSort(First, down);
	if(Last > ++up) FastSort(up, Last);
}

bool CNetwork::ExportToR_pd(R_pdn * pd)
{
	pd->o = (R_pdn::outn *) malloc((NodeCount + 1/*???*/) * sizeof(R_pdn::outn));
	R_pdn::outn * o = pd->o + NodeCount - 1;
	for(CNode * n = Nodes + NodeCount - 1; n >= Nodes; n--, o--) // Первично инициализируем узлы
	{
		o->n1 = n->SData.Original - Nodes;
		o->n2 = (CNode *) n->Sorted - Nodes;
		o->n3 = 0;
		o->n4 = 0;
	}
	o++;
	// Считаем число элементов в Y и LU для каждого узла
	_ASSERTE(LU.Valid());
	CPFlat<CLU>::CPiece * List = LU.First;
	CLU * lu = List->GetFirst();
	for(CNode * n = Nodes; n < Nodes + NodeCount; n++, o++)
	{
		CNode * jNode;
		for( ; jNode = lu->Node; lu++) 
		{
			R_pdn::outn * ao = pd->o + (jNode - Nodes);
			o->n4++;
			ao->n4++;
			if(CYArray * y = lu->Y)
			{
				o->n3++;
				ao->n3++;
			}
		}
		if(lu <= List->Last) lu++;
		else 
		{
			List = List->Next;
			CLU * lu = List->GetFirst();
		}
	} 
	// Рассчитываем индексы в массивах и полные размеры массивов
	int iy = 0, ilu = 0;
	for(o = pd->o; o < pd->o + NodeCount; o++)
	{
		int t = o->n3;
		o->n3 = iy;
		iy += t + 1;
		t = o->n4;
		o->n4 = ilu;
		ilu += t + 1;
	}
	o->n3 = iy;
	o->n4 = ilu;

	// Выделяем место под матрицы
	pd->nk = iy;
	pd->may = (R_pdn::Rmay *) malloc(iy * sizeof(R_pdn::Rmay));
	pd->ndd = ilu;
	pd->maj = (int *) malloc(ilu * sizeof(int));

	R_pdn::Rmay * may = pd->may;
	int * maj = pd->maj;

	// Копируем элементы в матрицы
	List = LU.First;
	lu = List->GetFirst();
	o = pd->o;
	for(CNode * n = Nodes; n < Nodes + NodeCount; n++, o++)
	{
		CNode * jNode;
		maj[o->n4++] = n - Nodes; // ???
		int n3 = o->n3++; // Имеет ли значение порядок элементов в may?
		may[n3].nqy = n - Nodes;

		for( ; jNode = lu->Node; lu++) 
		{
			R_pdn::outn * ao = pd->o + (jNode - Nodes);
			maj[o->n4++] = jNode - Nodes;
			maj[ao->n4++] = n - Nodes;
			if(CYArray * y = lu->Y)
			{
				may[o->n3].nqy = jNode - Nodes;
				may[o->n3++].y = y->Yij;
				may[ao->n3].nqy = n - Nodes;
				may[ao->n3++].y = y->Yji;
			}
		}
		_ASSERTE(lu->Y);
		may[n3].y = lu->Y->Yij;
		if(lu <= List->Last) lu++;
		else 
		{
			List = List->Next;
			CLU * lu = List->GetFirst();
		}
	}
	// Восстанавливаем индексы
	for(o = pd->o + NodeCount; o >= pd->o; o--)
	{
		(o + 1)->n3 = o->n3;
		(o + 1)->n4 = o->n4;
	}
	o++;
	o->n3 = 0;
	o->n4 = 0;
	// Сортируем maj
	for(o = pd->o + NodeCount; o >= pd->o; o--)
		FastSort(maj + o->n4, maj + (o + 1)->n4 - 1);
	return true;
}


bool CNetwork::ImportFromR_pd(R_pdn * pd) // Загружаем матрицу проводимости
{
	_ASSERTE(NodeCount);
	LU.Reset();
	pLU.Reset();
	free(Y);
	YSize = pd->nk;
	Y = (CYArray *) malloc(YSize * sizeof(CYArray));
	memset(Y, 0, YSize * sizeof(CYArray));
	UsualNodeCount = 0;
	for(CNode * Node = Nodes + NodeCount - 1; Node >= Nodes; Node--)
		if(Node->Type == CNode::USUAL) UsualNodeCount++;
	CYArray * yd = Y + UsualNodeCount;
	CNode * s = Nodes;
	for(R_pdn::outn * n = pd->o; s < Nodes + UsualNodeCount; n++, s++)
	{
		s->SData.Original = Nodes + n->n1;
		s->Sorted = &(Nodes + n->n2)->SData;
	}

	s = Nodes;
	int i = 0;
	for(R_pdn::outn * n = pd->o; s < Nodes + UsualNodeCount; n++, s++, i++)
	{
		CNode * Node = s->SData.Original;
		//_ASSERTE(Node->Sorted == &s->SData);
		//R_pdn::outn * no = pd->o + i;
		int * x = pd->maj + n->n4, * e = pd->maj + (n + 1)->n4 ;
		while(x < e && *x <= i) x++;
		int Range = e - x;
		_ASSERTE(Range >= 0);
		Node->BranchCount = Range;
		CLU * lu = LU.Alloc(Range + 1);
		Node->LU = lu;
		CLU * d = lu + Range;
		d->Y = Y + i;
		for(R_pdn::Rmay * y = pd->may + n->n3, * z = pd->may + (n + 1)->n3; y < z; y++)
			if(y->nqy == i) // Ищем диагональный элемент
			{
				d->Y->Yij = y->y;
				d->Node = 0;
				//_ASSERTE(df--);
				break;
			}
		_ASSERTE(!d->Node);

		Node->pLU = pLU.Alloc(Range * Range);
		for(; x < e; x++, lu++)
		{
			lu->Node = Nodes[*x].SData.Original;
			lu->Y = 0;
#ifdef _DEBUG
	int f = 2;
#endif
			// Сканирование матрицы проводимости
			for(R_pdn::Rmay * y = pd->may + n->n3, * z = pd->may + (n + 1)->n3; y < z; y++)
			{
				if(y->nqy == *x) // Ищем прямой элемент
				{
					yd->Yij = y->y;
					lu->Y = yd;
					_ASSERTE(f--);
					break;
				}
			}
			
			R_pdn::outn * o = pd->o + *x;
			for(R_pdn::Rmay * y = pd->may + o->n3, * z = pd->may + (o + 1)->n3; y < z; y++)
				if(y->nqy == i) // Ищем обратный элемент
				{
					(yd++)->Yji = y->y;
					_ASSERTE(f--);
					break;
				}
			_ASSERTE(!(f & ~2));
		}
		_ASSERTE(d == lu);
	}
	// Инициализируем массив pLU
	s = Nodes;
	i = 0;
	for(R_pdn::outn * n = pd->o; s < Nodes + UsualNodeCount; n++, s++, i++)
	{
		CNode * Node = s->SData.Original;
		int * x = pd->maj + n->n4, * e = pd->maj + (n + 1)->n4;
		while(x < e && *x <= i) x++;
		cplx * * plu = Node->pLU;
		for(int * u = x; u < e; u++)
			for(int * l = x; l < e; l++, plu++)
				if(*u <= *l)
				{
					CNode * jNode = Nodes[*u].SData.Original;
					CLU * lu = jNode->LU;
					if(u == l)
						for(; lu->Node; lu++);
					else
					{
						CNode * o = Nodes[*l].SData.Original;
						for(; lu->Node != o; lu++)
						{
							_ASSERTE(lu->Node);
						}
					}
					*plu = &lu->L;
				}
				else
				{
					CNode * jNode = Nodes[*l].SData.Original;
					CNode * o = Nodes[*u].SData.Original;
					CLU * lu = jNode->LU;
					for( ; lu->Node != o; lu++)
					{
						_ASSERTE(lu->Node);
					}
					*plu = &lu->U;
				}
		_ASSERTE(plu == Node->pLU + Node->BranchCount * Node->BranchCount);
	}
	return true;
}
