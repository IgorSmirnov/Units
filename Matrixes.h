#ifndef _MATRIXES_H_
#define _MATRIXES_H_
#include <math.h>
#include <crtdbg.h>

template<class T, int M, int N>
class TMatrix;

template<class T, int N>
class TVector
{
private:
	T v[N];
public:
	inline T& operator [](int n) 
	{
		_ASSERTE((n >= 0) && (n < N));
		return v[n];
	}
	inline const T& operator [](int n) const
	{
		_ASSERTE((n >= 0) && (n < N));
		return v[n];
	}
	inline operator T * () {return v;}
	inline operator const T * () const {return v;}
	TVector<T, N> operator+(const TVector<T, N> & A) const
	{
		TVector<T, N> Res;
		for(int x = N - 1; x >= 0; x--) Res.v[x] = v[x] + A.v[x];
		return Res;
	}
	inline TVector<T, N> operator-(const TVector<T, N> & A) const
	{
		TVector<T, N> Res;
		for(int x = N - 1; x >= 0; x--) Res.v[x] = v[x] - A.v[x];
		return Res;
	}
	inline TVector<T, N> operator-() const
	{
		TVector<T, N> Res;
		for(int x = N - 1; x >= 0; x--) Res.v[x] = -v[x];
		return Res;
	}	
	inline TVector<T, N> &operator+=(const TVector<T, N> & A)
	{
		for(int x = N - 1; x >= 0; x--) v[x] += A.v[x];
		return *this;
	}		
	inline TVector<T, N> &operator-=(const TVector<T, N> & A)
	{
		for(int x = N - 1; x >= 0; x--) v[x] -= A.v[x];
		return *this;
	}		
	inline TVector<T, N> &operator*=(const T & A)
	{
		for(int x = N - 1; x >= 0; x--) v[x] *= A;
		return *this;
	}
	inline TVector<T, N> &operator =(const T * A) {for(int x = 0; x < N; x++) v[x] = *(A++); return *this;}
	T operator*(const TVector<T, N> & A) const //!< Скалярное умножение
	{
		T Res = 0;
		for(int x = N - 1; x >= 0; x--) Res += v[x] * A.v[x];
		return Res;
	}
	inline TVector<T, N> &BoundRange(const T & Min, const T & Max)
	{
		for(int x = N - 1; x >= 0; x--) 
		{
			if(v[x] < Min) v[x] = Min;
			if(v[x] > Max) v[x] = Max;
		}
		return *this;
	}
	TVector<T, N> operator*(const T & A) const
	{
		TVector<T, N> Res;
		for(int x = N - 1; x >= 0; x--) Res.v[x] = v[x] * A;
		return Res;
	}
	TVector<T, N> operator/(const T & A) const
	{
		TVector<T, N> Res;
		T b = 1 / A;
		for(int x = N - 1; x >= 0; x--) Res.v[x] = v[x] * b;
		return Res;
	}
	TVector<T, N> operator/(const TMatrix<T, N, N> & M) const
	{
		TVector<T, N> Res;
		T d = M.GetDeterminant3();
		_ASSERTE(abs(d) > 1E-10);
		T t = 1 / d;
		for(int x = 2; x >= 0; x--)
		{
			T r = 0;
			int x1 = (x + 1) % 3, x2 = (x + 2) % 3;
			for(int y = 2; y >= 0; y--)
			{
				int y1 = (y + 1) % 3, y2 = (y + 2) % 3;
				r += v[y] * M[y1][x1] * M[y2][x2] - v[y2] * M[y1][x1] * M[y][x2];
			}
			Res[x] = r * t;
		}
		return Res;
	}
	bool Solve(const TMatrix<T, N, N> & M, TVector<T, N> &Res) const
	{
		T d = M.GetDeterminant3();
		if(abs(d) < 1E-10)return false;
		T t = 1 / d;
		for(int x = 2; x >= 0; x--)
		{
			T r = 0;
			int x1 = (x + 1) % 3, x2 = (x + 2) % 3;
			for(int y = 2; y >= 0; y--)
			{
				int y1 = (y + 1) % 3, y2 = (y + 2) % 3;
				r += v[y] * M[y1][x1] * M[y2][x2] - v[y2] * M[y1][x1] * M[y][x2];
			}
			Res[x] = r * t;
		}
		return true;
	}
	friend inline T norm(const TVector<T, N> &A)
	{
		T Res = 0;
		for(int x = N - 1; x >= 0; x--) Res += A.v[x] * A.v[x];
		return Res;
	}
	friend inline T len(const TVector<T, N> &A)
	{
		T Res = 0;
		for(int x = N - 1; x >= 0; x--) Res += A.v[x] * A.v[x];
		return sqrt(Res);
	}
	friend inline TVector<T, N> VMul(const TVector<T, N> &A, const TVector<T, N> &B)
	{
		_ASSERTE(N >= 3);
		TVector<T, N> Res;
		Res.v[0] = A.v[1] * B.v[2] - A.v[2] * B.v[1];
		Res.v[1] = A.v[2] * B.v[0] - A.v[0] * B.v[2];
		Res.v[2] = A.v[0] * B.v[1] - A.v[1] * B.v[0];
		return Res;
	}
	friend T GetDeterminant(const TVector<T, N> &A, const TVector<T, N> &B, const TVector<T, N> &C)
	{
		_ASSERTE(N >= 3);
		return A[0] * B[1] * C[2] + B[0] * C[1] * A[2] + A[1] * B[2] * C[0]
		     - A[2] * B[1] * C[0] - A[1] * B[0] * C[2] - A[0] * B[2] * C[1];
	}
	void ToZero(void) { memset(v, 0, N * sizeof(T));}
	void ToOne(void) { T t = norm(*this); _ASSERTE(t); *this *= 1 / sqrt(t); };
	inline TVector(void){};
	inline TVector(T x, T y, T z = 0, T t = 0)
	{
		v[0] = x;
		if(N > 1) v[1] = y;
		if(N > 2) v[2] = z;
		if(N > 3) v[3] = t;
	}
	template<class T1>
	TVector(const TVector<T1, N> &V)
	{
		for(int x = N - 1; x >= 0; x--)
			v[x] = (T)V[x];
	}
	void Dump(char * Title = 0)
	{
		if(Title) _RPT1(_CRT_WARN, "\n%s = ", Title);
		for(int x = 0; x < N; x++) _RPT1(_CRT_WARN, x ? ", %.3g" : "(%.3g", v[x]);
		_RPT0(_CRT_WARN, ")");
	}
};

template<class T, int M, int N>
class TMatrix
{
private:
	T v[M][N];
public:
	inline TVector<T, N>& operator [](int m) 
	{
		_ASSERTE((m >= 0) && (m < M));
		return *((TVector<T, N> *)v + m);
	}
	inline const TVector<T, N>& operator [](int m) const
	{
		_ASSERTE((m >= 0) && (m < M));
		return *((TVector<T, N> *)v + m);
	}	
	inline T GetDeterminant3(void) const
	{
		_ASSERTE(M >= 3 && N >= 3);
		return GetDeterminant((*this)[0], (*this)[1], (*this)[2]);
	}
	TMatrix<T, M, N> operator+(const TMatrix<T, M, N> &A) const
	{
		TMatrix<T, M, N> Res;
		for(int x = M - 1; x >= 0; x--) 
			for(int y = N - 1; y >= 0; y--) 
				Res.v[x][y] = v[x][y] + A.v[x][y];
		return Res;
	}
	TMatrix<T, M, N> operator-(const TMatrix<T, M, N> &A) const
	{
		TMatrix<T, M, N> Res;
		for(int x = M - 1; x >= 0; x--) 
			for(int y = N - 1; y >= 0; y--) 
				Res.v[x][y] = v[x][y] - A.v[x][y];
		return Res;
	}
	inline TVector<T, M> operator*(const TVector<T, N> &A) const
	{
		TVector<T, M> Res;
		for(int x = M - 1; x >= 0; x--) 
			Res[x] = (*this)[x] * A;
		return Res;
	}
	inline TVector<T, M> operator*(const TVector<T, N - 1> &A) const
	{
		TVector<T, M> Res;
		for(int x = M - 1; x >= 0; x--) 
		{
			Res[x] = v[x][N - 1];
			for(int y = N - 2; y >= 0; y--)
				Res[x] += A.v[y] * v[x][y];
		}
		return Res;
	}
	template<int K>
	inline TMatrix<T, M, K> operator*(const TMatrix<T, N, K> &A) const
	{
		TMatrix<T, M, K> Res;
		for(int x = M - 1; x >= 0; x--)
			for(int y = K - 1; y >= 0; y--)
			{
				Res[x][y] = 0;
				for(int z = N - 1; z >= 0; z--)
					Res[x][y] += v[x][z] * A[z][y];
			}
		return Res;
	}
	inline void ToZero(void) { memset(v, 0, sizeof(v));}
	inline TMatrix & operator *= (T Scaler)
	{
		for(int x = M - 1; x >= 0; x--) 
			for(int y = N - 1; y >= 0; y--) 
				v[x][y] *= Scaler;
		return *this;
	}
	inline void ToOne(void)
	{
		ToZero();
		for(int x = ((M > N) ? N : M) - 1; x >= 0; x--) v[x][x] = 1;
	}
	void Dump(char * Title = 0)
	{
		if(Title) _RPT1(_CRT_WARN, "\n%s:", Title);
		for(int x = 0; x < M; x++)
		{
			_RPT0(_CRT_WARN, "\n");
			(*this)[x].Dump();
		}
	}
	inline TMatrix<T, N, M> Transposed(void) const
	{
		TMatrix<T, N, M> Res;
		for(int x = M - 1; x >= 0; x--) 
			for(int y = N - 1; y >= 0; y--) 
				Res.v[y][x] = v[x][y];
		return Res;
	}
	TMatrix<T, M, N> Inversed(void) const
	{
		TMatrix<T, N, M> Res;
		Res.ToOne();
		Res[0] = Res[0] / *this;
		Res[1] = Res[1] / *this;
		Res[2] = Res[2] / *this;
		return Res.Transposed();
	}
	template<int Axis> void RotateGlobal(const T &Angle)
	{
		_ASSERTE(N >= 3);
		T a = sin(Angle);
		T b = cos(Angle);
		int ax = (Axis + 1) % 3;
		int ay = (Axis + 2) % 3;
		for(int x = M - 1; x >= 0; x--)
		{
			T nx     = v[x][ax] * b + v[x][ay] * a;
			v[x][ay] = v[x][ay] * b - v[x][ax] * a;
			v[x][ax] = nx;
		}
	}
	template<int Axis> void RotateGlobalT(const T &Angle)
	{
		_ASSERTE(N >= 3);
		T a = sin(Angle);
		T b = cos(Angle);
		int ax = (Axis + 1) % 3;
		int ay = (Axis + 2) % 3;
		for(int x = M - 1; x >= 0; x--)
		{
			T nx     = v[ax][x] * b + v[ay][x] * a;
			v[ay][x] = v[ay][x] * b - v[ax][x] * a;
			v[ax][x] = nx;
		}
	}
	template<int Axis> void RotateLocal(const T &Angle)
	{
		_ASSERTE(M >= 3);
		T a = sin(Angle);
		T b = cos(Angle);
		int ax = (Axis + 1) % 3;
		int ay = (Axis + 2) % 3;
		TVector<T, N> nx = (*this)[ax] * b + (*this)[ay] * a;
		(*this)[ay]		 = (*this)[ay] * b - (*this)[ax] * a;
		(*this)[ax] = nx;
	}
	template<int Axis> void RotateLocalT(const T &Angle)
	{
		_ASSERTE(M >= 3);
		T a = sin(Angle);
		T b = cos(Angle);
		int ax = (Axis + 1) % 3;
		int ay = (Axis + 2) % 3;
		for(int x = M - 1; x >= 0; x--)
		{
			T nx     = v[x][ax] * b + v[x][ay] * a;
			v[x][ay] = v[x][ay] * b - v[x][ax] * a;
			v[x][ax] = nx;
		}
	}
};

#endif