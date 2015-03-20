#ifndef _ARRAYND_H_
#define _ARRAYND_H_

#include <limits.h>
#include <iostream>
#include "couple.h"

// #define ARRAYND_RUNTIME_CHECK

using namespace std;

template <class T> class CArray1D;
template <class T> class CArray2D;

template <class T> class CArray1D
{
  // Static members
#ifdef ARRAYND_RUNTIME_CHECK
  protected:
	// Element returned in case of invalid access with operator [] or function GetElementInterpolate()
	static T elementError;
#endif

  // Members
  protected:
	int iSize;
	T *pElements;

  // Member functions
  public:
	inline CArray1D();
	inline CArray1D(int);
	inline CArray1D(const CArray1D<T> &);

	inline ~CArray1D();

	inline int GetSize() const {return iSize;}
	inline T *GetBuffer() const {return pElements;}

	inline bool Init(int);
	inline bool Init(int, T *);
	template <class S> bool InitCast(const CArray1D<S> &);
	inline void Empty();
	inline void Fill(const T &);

	inline CArray1D &operator =(const CArray1D<T> &);

	// Access single element (read/write)
	inline T &operator [](int index) const
	{
		#ifdef ARRAYND_RUNTIME_CHECK
		if (index<0 || index>=iSize)
		{
			cerr<<"ERROR in CArray1D<T>::operator [](int): accessing element at index "<<index<<" out of range [0.."<<iSize-1<<"]"<<endl;
			return elementError;
		}
		#endif
		return pElements[index];
	}

	// Linear interpolation between elements (read only)
	inline T GetElementInterpolate(float fIndex) const
	{
		float fDiff;
		int iIndex;
		T elementInterpolate;

		#ifdef ARRAYND_RUNTIME_CHECK
		if (fIndex<0.0f || fIndex>=(float)(iSize-1))
		{
			cerr<<"ERROR in CArray1D<T>::GetElementInterpolate(float): accessing element at index "<<fIndex<<" out of range [0.."<<iSize-1<<"["<<endl;
			return elementError;
		}
		#endif

		iIndex = (int)fIndex;
		fDiff = fIndex-floor(fIndex);
		elementInterpolate = (T)((1.0f-fDiff)*pElements[iIndex] + fDiff*pElements[iIndex+1]);

		return elementInterpolate;
	}

	CArray1D<T> &operator +=(const CArray1D<T> &);
	CArray1D<T> &operator -=(const CArray1D<T> &);
	template <class S> CArray1D<T> &operator *=(const S &);
	template <class S> CArray1D<T> &operator /=(const S &);

	bool operator ==(const CArray1D<T> &) const;
	CArray1D<T> operator +(const CArray1D<T> &) const;
	CArray1D<T> operator -() const;
	CArray1D<T> operator -(const CArray1D<T> &) const;
	template <class S> CArray1D<T> operator *(const S &) const;

	template <class U, class S> friend CArray1D<U> operator *(const S &, const CArray1D<U> &);
	template <class S> CArray1D<T> operator /(const S &) const;
};

#ifdef ARRAYND_RUNTIME_CHECK
// Element returned in case of invalid access with operator [] or function GetElementInterpolate()
template <class T> T CArray1D<T>::elementError;
#endif

template <class T> CArray1D<T>::CArray1D()
{
	iSize = 0;
	pElements = NULL;
}

template <class T> CArray1D<T>::CArray1D(int size)
{
	iSize = size;
	pElements = new T[iSize];
}

template <class T> CArray1D<T>::CArray1D(const CArray1D<T> &array)
{
	T *pElementsTemp, *pElementsTemp2;
	int i;

	iSize = array.iSize;
	pElements = new T[iSize];

	pElementsTemp = pElements;
	pElementsTemp2 = array.pElements;
	for (i=0;i<iSize;i++)
		*pElementsTemp++ = *pElementsTemp2++;
}

template <class T> CArray1D<T>::~CArray1D()
{
	Empty();
}

template <class T> bool CArray1D<T>::Init(int size)
{
	if (pElements!=NULL)
		delete[] pElements;

	iSize = size;

	pElements = new T[iSize];
	if (pElements!=NULL)
		return true;
	else
		return false;
}

template <class T> bool CArray1D<T>::Init(int size, T *ptr)
{
	if (pElements!=NULL)
		delete[] pElements;

	iSize = size;

	pElements = new T[iSize];
	if (pElements!=NULL)
	{
		T *pElementsTemp = pElements;
		T *ptrTemp = ptr;
		int i;

		for (i=0;i<iSize;i++)
			*pElementsTemp++ = *ptrTemp++;
		return true;
	}
	else
		return false;
}

template <class T> template <class S> bool CArray1D<T>::InitCast(const CArray1D<S> &array)
{
	if (pElements!=NULL)
		delete[] pElements;

	iSize = array.GetSize();

	pElements = new T[iSize];
	if (pElements!=NULL)
	{
		T *pElementsThis = pElements;
		S *pElementsParam = array.GetBuffer();
		int i;

		for (i=0;i<iSize;i++)
		{
			*pElementsThis = (T)(*pElementsParam);
			pElementsThis++;
			pElementsParam++;
		}
		return true;
	}
	else
		return false;
}

template <class T> void CArray1D<T>::Empty()
{
	if (pElements!=NULL)
		delete[] pElements;

	iSize = 0;
	pElements = NULL;
}

template <class T> void CArray1D<T>::Fill(const T &t)
{
	int i;
	T *pElementsTemp = pElements;

	for (i=0;i<iSize;i++)
	{
		*pElementsTemp = t;
		pElementsTemp++;
	}
}

template <class T> CArray1D<T> &CArray1D<T>::operator =(const CArray1D<T> &array)
{
	int i;

	// Reallocate if necessary
	if (iSize!=array.iSize)
	{
		if (pElements!=NULL)
		{
			delete[] pElements;
			pElements = NULL;
		}
		iSize = array.iSize;
		pElements = new T[iSize];
	}

	// Copy elements
	if (iSize!=0)
	{
		T *pElementsTemp, *pElementsTemp2;
		pElementsTemp = pElements;
		pElementsTemp2 = array.pElements;

		for (i=0;i<iSize;i++)
			*pElementsTemp++ = *pElementsTemp2++;
	}
	return *this;
}

template <class T> CArray1D<T> &CArray1D<T>::operator +=(const CArray1D<T> &array)
{
	if (iSize==array.iSize)
	{
		T *pElementsTemp, *pElementsTemp2;
		int i;

		pElementsTemp = pElements;
		pElementsTemp2 = array.pElements;

		for (i=0;i<iSize;i++)
			*pElementsTemp++ += *pElementsTemp2++;
		return *this;
	}
	else throw 0;
}

template <class T> CArray1D<T> &CArray1D<T>::operator -=(const CArray1D<T> &array)
{
	if (iSize==array.iSize)
	{
		T *pElementsTemp, *pElementsTemp2;
		int i;

		pElementsTemp = pElements;
		pElementsTemp2 = array.pElements;

		for (i=0;i<iSize;i++)
			*pElementsTemp++ -= *pElementsTemp2++;
		return *this;
	}
	else throw 0;
}

template <class T> template <class S> CArray1D<T> &CArray1D<T>::operator *=(const S &t)
{
	T *pElementsTemp = pElements;
	int i;

	for (i=0;i<iSize;i++)
		*pElementsTemp++ *= t;
	return *this;
}

template <class T> template <class S> CArray1D<T> &CArray1D<T>::operator /=(const S &t)
{
	T *pElementsTemp = pElements;
	int i;

	for (i=0;i<iSize;i++)
		*pElementsTemp++ /= t;
	return *this;
}

template <class T> bool CArray1D<T>::operator ==(const CArray1D<T> &array) const
{
	if (iSize!=array.iSize)
		return false;

	T *pElementsTemp, *pElementsTemp2;
	int i;

	pElementsTemp = this->pElements;
	pElementsTemp2 = array.pElements;
	for (i=0;i<this->iSize && *pElementsTemp==*pElementsTemp2; i++)
	{
		pElementsTemp++;
		pElementsTemp2++;
	}
	if (i==iSize) return true;
	else return false;
}

template <class T> CArray1D<T> CArray1D<T>::operator +(const CArray1D<T> &array) const
{
	if (iSize==array.iSize)
	{
		CArray1D<T> arrayResult(iSize);
		T *pElementsTemp, *pElementsTemp2, *pElementsTempResult;
		int i;

		pElementsTemp = pElements;
		pElementsTemp2 = array.pElements;
		pElementsTempResult = arrayResult.pElements;

		for (i=0;i<iSize;i++)
		{
			*pElementsTempResult = *pElementsTemp + *pElementsTemp2;
			pElementsTemp++;
			pElementsTemp2++;
			pElementsTempResult++;
		}
		return arrayResult;
	}
	else throw 0;
}

template <class T> CArray1D<T> CArray1D<T>::operator -() const
{
	CArray1D<T> arrayResult(iSize);
	T *pElementsTemp, *pElementsTempResult;
	int i;

	pElementsTemp = pElements;
	pElementsTempResult = arrayResult.pElements;

	for (i=0;i<iSize;i++)
	{
		*pElementsTempResult = -(*pElementsTemp);
		pElementsTemp++;
		pElementsTempResult++;
	}
	return arrayResult;
}

template <class T> CArray1D<T> CArray1D<T>::operator -(const CArray1D<T> &array) const
{
	if (iSize==array.iSize)
	{
		CArray1D<T> arrayResult(iSize);
		T *pElementsTemp, *pElementsTemp2, *pElementsTempResult;
		int i;

		pElementsTemp = pElements;
		pElementsTemp2 = array.pElements;
		pElementsTempResult = arrayResult.pElements;

		for (i=0;i<iSize;i++)
		{
			*pElementsTempResult = (*pElementsTemp)-(*pElementsTemp2);
			pElementsTemp++;
			pElementsTemp2++;
			pElementsTempResult++;
		}
		return arrayResult;
	}
	else throw 0;
}

template <class T> template <class S> CArray1D<T> CArray1D<T>::operator *(const S &t) const
{
	CArray1D<T> arrayResult(iSize);
	T *pElementsTemp, *pElementsTempResult;
	int i;

	pElementsTemp = pElements;
	pElementsTempResult = arrayResult.pElements;
	for (i=0;i<iSize;i++)
	{
		*pElementsTempResult = (*pElementsTemp)*t;
		pElementsTemp++;
		pElementsTempResult++;
	}
	return arrayResult;
}

template <class T, class S> CArray1D<T> operator *(const S &t, const CArray1D<T> &v)
{
	CArray1D<T> arrayResult(v.iSize);
	T *pElementsTemp, *pElementsTempResult;
	int i;

	pElementsTemp = v.pElements;
	pElementsTempResult = arrayResult.pElements;
	for (i=0;i<v.iSize;i++)
	{
		*pElementsTempResult = (*pElementsTemp)*t;
		pElementsTemp++;
		pElementsTempResult++;
	}
	return arrayResult;
}

template <class T> template <class S> CArray1D<T> CArray1D<T>::operator /(const S &t) const
{
	CArray1D<T> arrayResult(iSize);
	T *pElementsTemp, *pElementsTempResult;
	int i;

	pElementsTemp = pElements;
	pElementsTempResult = arrayResult.pElements;
	for (i=0;i<iSize;i++)
	{
		*pElementsTempResult = (*pElementsTemp)/t;
		pElementsTemp++;
		pElementsTempResult++;
	}
	return arrayResult;
}

template <class T> class CArray2DIterator
{
  friend class CArray2D<T>;

  // Attributes
  protected:
	T *pCurrentElementY, *pCurrentElement;
	CCouple<int> ptCurrent, ptMin, ptMax;
	CArray2D<T> *pArrayParent;

  public:
	inline CArray2DIterator()
	{
		pArrayParent = NULL;
		pCurrentElementY = pCurrentElement = NULL;
	}
	T *operator ++() // Prefix operator ++
	{
		T *pRet;

		ptCurrent.x++;
		pCurrentElement++;
		if (ptCurrent.x>ptMax.x)
		{
			ptCurrent.x = ptMin.x;
			ptCurrent.y++;

			pCurrentElementY += pArrayParent;
			pCurrentElement = pCurrentElementY + ptMin.x;
		}
		pRet = pCurrentElement;
		return pRet;
	}
	T *operator ++(int) // Postfix operator ++
	{
		T *pRet;

		pRet = pCurrentElement;

		ptCurrent.x++;
		pCurrentElement++;
		if (ptCurrent.x>ptMax.x)
		{
			ptCurrent.x = ptMin.x;
			ptCurrent.y++;

			pCurrentElementY += pArrayParent->iWidth;
			pCurrentElement = pCurrentElementY + ptMin.x;
		}
		return pRet;
	}

	inline CCouple<int> GetPosition() {return ptCurrent;}
	inline T &Element() {return *pCurrentElement;}
	inline T *ElementPtr() {return pCurrentElement;}
	inline operator T *() {return pCurrentElement;}
	inline int End() {return (ptCurrent.y>ptMax.y);}
};

template <class T> class CArray2D : protected CArray1D<T>
{
	friend class CArray2DIterator<T>;

  protected:
	int iWidth, iHeight;

  // Member functions
  public:
	inline CArray2D();
	inline CArray2D(int, int);
	inline CArray2D(const CCouple<int> &);
	inline CArray2D(const CArray2D<T> &);

	inline ~CArray2D();

	inline int GetWidth() const {return iWidth;}
	inline int GetHeight() const {return iHeight;}
	inline CCouple<int> GetSize() const {return CCouple<int>(iWidth, iHeight);}
	inline int GetOffset(int x, int y) const {return y*iWidth+x;}
	inline int GetOffset(const CCouple<int> &coord) const {return coord.y*iWidth + coord.x;}

	inline T *GetBuffer() const {return this->pElements;}

	bool Init(int, int);
	bool Init(const CCouple<int> &);
	template <class S> bool InitCast(const CArray2D<S> &);
	void Empty();
	void Fill(const T &);

	CArray2D<T> &operator =(const CArray2D<T> &);

	// Access single element (read/write)
	inline T &Element(int x, int y) const
	{
		#ifdef ARRAYND_RUNTIME_CHECK
		if (x<0 || x>=iWidth || y<0 || y>=iHeight)
		{
			cerr<<"ERROR in CArray2D<T>::Element(int, int): accessing element ("<<x<<","<<y<<") out of range [0.."<<iWidth-1<<"]x[0.."<<iHeight-1<<"]"<<endl;
			return CArray1D<T>::elementError;
		}
		#endif
		return this->pElements[y*iWidth+x];
	}
	inline T &Element(const CCouple<int> &coord) const
	{
		#ifdef ARRAYND_RUNTIME_CHECK
		if (coord.x<0 || coord.x>=iWidth || coord.y<0 || coord.y>=iHeight)
		{
			cerr<<"ERROR in CArray2D<T>::Element(const CCouple<int> &): accessing element ("<<coord.x<<","<<coord.y<<") out of range [0.."<<iWidth-1<<"]x[0.."<<iHeight-1<<"]"<<endl;
			return CArray1D<T>::elementError;
		}
		#endif
		return this->pElements[coord.y*iWidth+coord.x];
	}

	// Linear interpolation between elements (read only)
	inline T GetElementInterpolate(float x, float y) const
	{
		float dx, dy;
		int xi, yi;
		T *pElementTemp;
		T elementInterpolate;

		#ifdef ARRAYND_RUNTIME_CHECK
		if (x<0.0f || x>=(float)(iWidth-1) || y<0.0f || y>=(float)(iHeight-1))
		{
			cerr<<"ERROR in CArray2D<T>::GetElementInterpolate(float,float): accessing pixel ("<<x<<","<<y<<") out of range [0.."<<iWidth-1<<"[x[0.."<<iHeight-1<<"["<<endl;
			return elementInterpolate;
		}
		#endif

		xi = (int)x;
		yi = (int)y;
		dx = x-floor(x);
		dy = y-floor(y);

		// Get address of nearest element with lower integer coordinates
		pElementTemp = this->pElements + yi*this->iWidth + xi;

		elementInterpolate = (T)(
			(1.0f-dx)*(1.0f-dy) * pElementTemp[0] +
			dx*(1.0f-dy)        * pElementTemp[1] +
			(1.0f-dx)*dy        * pElementTemp[iWidth] +
			dx*dy               * pElementTemp[iWidth+1]
		);
		return elementInterpolate;
	}
	inline T GetElementInterpolate(const CCouple<float> &coord) const {return GetElementInterpolate(coord.x,coord.y);}

	// Iterator
	inline CArray2DIterator<T> GetIterator()
	{
		CArray2DIterator<T> iterator;

		iterator.pArrayParent = this;
		iterator.pCurrentElementY = iterator.pCurrentElement = this->pElements;
		iterator.ptCurrent.Set(0,0);
		iterator.ptMin.Set(0,0);
		iterator.ptMax.Set(iWidth-1,iHeight-1);

		return iterator;
	}
	inline CArray2DIterator<T> GetIterator(const CCouple<int> &ptMin, const CCouple<int> &ptMax)
	{
		CArray2DIterator<T> iterator;
		CCouple<int> ptLimitLow(0), ptLimitHigh(GetSize()-CCouple<int>(1));

		iterator.pArrayParent = this;
		iterator.pCurrentElementY = this->pElements + ptMin.y*iWidth;
		iterator.pCurrentElement = iterator.pCurrentElementY + ptMin.x;
		iterator.ptCurrent = iterator.ptMin = ptMin;
		iterator.ptMax = ptMax;

		if (ptMax.IsInRange(ptLimitLow, ptLimitHigh)
			&& ptMin.IsInRange(ptLimitLow, ptMax))
		{
			iterator.ptCurrent = iterator.ptMin = ptMin;
			iterator.ptMax = ptMax;
		}
		else {
			iterator.ptMax.y = 0;
			iterator.ptCurrent.y = 1;
			cerr<<"ERROR: CArray2D<T>::GetIterator()"<<endl;
		}

		return iterator;
	}

	CArray2D<T> &operator +=(const CArray2D<T> &);
	CArray2D<T> &operator -=(const CArray2D<T> &);
	template <class S> CArray2D<T> &operator *=(const S &);
	template <class S> CArray2D<T> &operator /=(const S &);

	bool operator ==(const CArray2D<T> &) const;
	CArray2D<T> operator +(const CArray2D<T> &) const;
	CArray2D<T> operator -() const;
	CArray2D<T> operator -(const CArray2D<T> &) const;
	template <class S> CArray2D<T> operator *(const S &) const;
	template <class U, class S> friend CArray2D<U> operator *(const S &, const CArray2D<U> &);
	template <class S> CArray2D<T> operator /(const S &) const;

	CArray1D<int> GetNeighborOffsetsConnex4();
	CArray1D<int> GetNeighborOffsetsConnex8();

	// Convolution
	template <class S> CArray2D<T> Convolve(const CArray2D<S> &) const;
	template <class S> CArray2D<T> Convolve(const CArray1D<CCouple<int> > &, const CArray1D<S> &) const;
};

template <class T> CArray2D<T>::CArray2D():CArray1D<T>()
{
	iWidth = iHeight = 0;
}

template <class T> CArray2D<T>::CArray2D(int width, int height):CArray1D<T>(width*height)
{
	iWidth = width;
	iHeight = height;
}

template <class T> CArray2D<T>::CArray2D(const CCouple<int> &iDimensions):CArray1D<T>(iDimensions.x*iDimensions.y)
{
	iWidth = iDimensions.x;
	iHeight = iDimensions.y;
}

template <class T> CArray2D<T>::CArray2D(const CArray2D<T> &array):CArray1D<T>(array)
{
	iWidth = array.iWidth;
	iHeight = array.iHeight;
}

template <class T> CArray2D<T>::~CArray2D()
{
	Empty();
}

template <class T> bool CArray2D<T>::Init(int width, int height)
{
	iWidth = width;
	iHeight = height;
	return CArray1D<T>::Init(iWidth*iHeight);
}

template <class T> bool CArray2D<T>::Init(const CCouple<int> &iDimensions)
{
	iWidth = iDimensions.x;
	iHeight = iDimensions.y;
	return CArray1D<T>::Init(iWidth*iHeight);
}

template <class T> template <class S> bool CArray2D<T>::InitCast(const CArray2D<S> &array)
{
	iWidth = array.GetWidth();
	iHeight = array.GetHeight();

	if (this->pElements!=NULL)
		delete[] this->pElements;

	this->iSize = iWidth*iHeight;

	this->pElements = new T[this->iSize];
	if (this->pElements!=NULL)
	{
		T *pElementsThis = this->pElements;
		S *pElementsParam = array.GetBuffer();
		int i;

		for (i=0;i<this->iSize;i++)
		{
			*pElementsThis = (T)(*pElementsParam);
			pElementsThis++;
			pElementsParam++;
		}
		return true;
	}
	else
		return false;
}

template <class T> void CArray2D<T>::Empty()
{
	iWidth = iHeight = 0;
	CArray1D<T>::Empty();
}

template <class T> void CArray2D<T>::Fill(const T &t)
{
	CArray1D<T>::Fill(t);
}

template <class T> CArray2D<T> &CArray2D<T>::operator =(const CArray2D<T> &array)
{
	iWidth = array.iWidth;
	iHeight = array.iHeight;

	CArray1D<T>::operator =(array);

	return *this;
}

template <class T> CArray2D<T> &CArray2D<T>::operator +=(const CArray2D<T> &array)
{
	if (iWidth==array.iWidth && iHeight==array.iHeight)
	{
		CArray1D<T>::operator +=(array);
		return *this;
	}
	else
		throw 0;
}

template <class T> CArray2D<T> &CArray2D<T>::operator -=(const CArray2D<T> &array)
{
	if (iWidth==array.iWidth && iHeight==array.iHeight)
	{
		CArray1D<T>::operator -=(array);
		return *this;
	}
	else
		throw 0;
}

template <class T> template <class S> CArray2D<T> &CArray2D<T>::operator *=(const S &t)
{
	CArray1D<T>::operator *=(t);
	return *this;
}

template <class T> template <class S> CArray2D<T> &CArray2D<T>::operator /=(const S &t)
{
	CArray1D<T>::operator /=(t);
	return *this;
}

template <class T> bool CArray2D<T>::operator ==(const CArray2D<T> &array) const
{
	if (iWidth!=array.iWidth || iHeight!=array.iHeight)
		return false;

	T *pElementsTemp, *pElementsTemp2;
	int i;

	pElementsTemp = this->pElements;
	pElementsTemp2 = array.pElements;
	for (i=0;i<this->iSize && *pElementsTemp==*pElementsTemp2; i++)
	{
		pElementsTemp++;
		pElementsTemp2++;
	}
	if (i==this->iSize) return true;
	else return false;
}

template <class T> CArray2D<T> CArray2D<T>::operator +(const CArray2D<T> &array) const
{
	if (iWidth==array.iWidth && iHeight==array.iHeight)
	{
		CArray2D<T> arrayRes(*this);
		arrayRes.CArray1D<T>::operator +=(array);
		return arrayRes;
	}
	else
		throw 0;
}

template <class T> CArray2D<T> CArray2D<T>::operator -() const
{
	CArray2D<T> arrayResult(iWidth, iHeight);
	T *pElementsTemp, *pElementsTempResult;
	int i;

	pElementsTemp = this->pElements;
	pElementsTempResult = arrayResult.pElements;

	for (i=0;i<this->iSize;i++)
	{
		*pElementsTempResult = -(*pElementsTemp);
		pElementsTemp++;
		pElementsTempResult++;
	}
	return arrayResult;
}

template <class T> CArray2D<T> CArray2D<T>::operator -(const CArray2D<T> &array) const
{
	if (iWidth==array.iWidth && iHeight==array.iHeight)
	{
		CArray2D<T> arrayRes(*this);
		arrayRes.CArray1D<T>::operator -=(array);
		return arrayRes;
	}
	else
		throw 0;
}

template <class T> template <class S> CArray2D<T> CArray2D<T>::operator *(const S &t) const
{
	CArray2D<T> arrayRes(*this);
	arrayRes.CArray1D<T>::operator *=(t);
	return arrayRes;
}

template <class T, class S> CArray2D<T> operator *(const S &t, const CArray2D<T> &array)
{
	CArray2D<T> arrayRes(array);
	arrayRes.CArray1D<T>::operator *=(t);
	return arrayRes;
}

template <class T> template <class S> CArray2D<T> CArray2D<T>::operator /(const S &t) const
{
	CArray2D<T> arrayRes(*this);
	arrayRes.CArray1D<T>::operator /=(t);
	return arrayRes;
}

template <class T>  CArray1D<int> CArray2D<T>::GetNeighborOffsetsConnex4()
{
	CArray1D<int> arrayOffsets(4);

	arrayOffsets[0] = -1;
	arrayOffsets[1] = 1;
	arrayOffsets[2] = -iWidth;
	arrayOffsets[3] = iWidth;

	return arrayOffsets;
}

template <class T>  CArray1D<int> CArray2D<T>::GetNeighborOffsetsConnex8()
{
	CArray1D<int> arrayOffsets(8);
	CCouple<int> pi;
	int i;

	i = 0;
	for (pi.y=-1;pi.y<=1;pi.y++)
	{
		for (pi.x=-1;pi.x<=1;pi.x++)
		{
			if (pi.x!=0 || pi.y!=0)
			{
				arrayOffsets[i++] = pi.y*iWidth + pi.x;
			}
		}
	}
	return arrayOffsets;
}

// Convolution with a centered mask
template <class T> template <class S> CArray2D<T> CArray2D<T>::Convolve(const CArray2D<S> &arrayCenteredMask) const
{
	CArray1D<CCouple<int> > arrayNeighbors;
	CArray1D<S> arrayMask;
	CCouple<int> neighbor, *pNeighbor;
	S *pMask, *pCenteredMask;
	int iSizeMask;

	iSizeMask = arrayCenteredMask.GetWidth()*arrayCenteredMask.GetHeight();
	arrayNeighbors.Init(iSizeMask);
	arrayMask.Init(iSizeMask);

	pMask = arrayMask.GetBuffer();
	pCenteredMask = arrayCenteredMask.GetBuffer();
	pNeighbor = arrayNeighbors.GetBuffer();

	for (neighbor.y=-arrayCenteredMask.GetHeight()/2; neighbor.y<=arrayCenteredMask.GetHeight()/2; neighbor.y++)
	{
		for (neighbor.x=-arrayCenteredMask.GetWidth()/2; neighbor.x<=arrayCenteredMask.GetWidth()/2; neighbor.x++)
		{
			*pNeighbor = neighbor;
			*pMask = *pCenteredMask;

			pMask++;
			pCenteredMask++;
			pNeighbor++;
		}
	}
	return Convolve(arrayNeighbors, arrayMask);
}

// Convolve with respect to a given neighborhood
template <class T> template <class S> CArray2D<T> CArray2D<T>::Convolve(const CArray1D<CCouple<int> > &arrayNeighbors, const CArray1D<S> &arrayMask) const
{
	CArray2D<T> arrayRes;
	int iNeighbor;
	S *pMask;
	int *pOffset;
	T *pElemY, *pElem;
	T *pElemDestY, *pElemDest;
	CCouple<int> p, p2, minBound, maxBound, *pNeighbor;
	CArray1D<int> arrayNeighborsOffsets(arrayNeighbors.GetSize());

	// Initialize result and offsets arrays
	arrayRes.Init(iWidth, iHeight);
	arrayNeighborsOffsets.Init(arrayNeighbors.GetSize());

	// Compute bounds
	minBound.Set(INT_MAX, INT_MAX);
	maxBound.Set(-INT_MAX, -INT_MAX);

	pNeighbor = arrayNeighbors.GetBuffer();
	pOffset = arrayNeighborsOffsets.GetBuffer();
	for (iNeighbor=0;iNeighbor<arrayNeighbors.GetSize();iNeighbor++)
	{
		minBound = coupleMin(minBound, *pNeighbor);
		maxBound = coupleMax(maxBound, *pNeighbor);
		*pOffset = GetOffset(*pNeighbor);

		pNeighbor++;
		pOffset++;
	}

	minBound = -minBound;
	maxBound = CCouple<int>(iWidth-1, iHeight-1)-maxBound;

	// Main loop
	pElemY = this->pElements + minBound.y*iWidth;
	pElemDestY = arrayRes.pElements + minBound.y*iWidth;
	for (p.y=minBound.y; p.y<=maxBound.y; p.y++)
	{
		pElem = pElemY + minBound.x;
		pElemDest = pElemDestY + minBound.x;
		for (p.x=minBound.x; p.x<=maxBound.x; p.x++)
		{
			*pElemDest = 0.0f;
			pOffset = arrayNeighborsOffsets.GetBuffer();
			pMask = arrayMask.GetBuffer();
			for (iNeighbor=0; iNeighbor<arrayNeighbors.GetSize(); iNeighbor++)
			{
				*pElemDest += pElem[*pOffset]*(*pMask);
				pOffset++;
				pMask++;
			}
			pElem++;
			pElemDest++;
		}
		pElemY+=iWidth;
		pElemDestY+=iWidth;
	}

	// Bounds
	CCouple<int> trZero, trSize;
	trZero.Set(0, 0);
	trSize.Set(iWidth-1, iHeight-1);

	// y < minBound.y
	pElem = this->pElements;
	pElemDest = arrayRes.pElements;
	for (p.y=0; p.y<minBound.y; p.y++)
	{
		for (p.x=0; p.x<iWidth; p.x++)
		{
			*pElemDest = 0.0f;
			pNeighbor = arrayNeighbors.GetBuffer();
			pOffset = arrayNeighborsOffsets.GetBuffer();
			pMask = arrayMask.GetBuffer();
			for (iNeighbor=0; iNeighbor<arrayNeighbors.GetSize(); iNeighbor++)
			{
				if ((p+*pNeighbor).IsInRange(trZero, trSize))
					*pElemDest += pElem[*pOffset]*(*pMask);
				else {
					p2 = p+*pNeighbor;
					p2.LimitWithinRange(trZero, trSize);
					*pElemDest += Element(p2)*(*pMask);
				}
				pNeighbor++;
				pOffset++;
				pMask++;
			}
			pElem++;
			pElemDest++;
		}
	}

	// minBound.y <= y <= minBound.y
	pElemY = this->pElements + minBound.y*iWidth;
	pElemDestY = arrayRes.pElements + minBound.y*iWidth;
	for (p.y=minBound.y; p.y<=maxBound.y; p.y++)
	{
		// x < minBound.x
		pElem = pElemY;
		pElemDest = pElemDestY;
		for (p.x=0; p.x<minBound.x; p.x++)
		{
			*pElemDest = 0.0f;
			pNeighbor = arrayNeighbors.GetBuffer();
			pOffset = arrayNeighborsOffsets.GetBuffer();
			pMask = arrayMask.GetBuffer();
			for (iNeighbor=0; iNeighbor<arrayNeighbors.GetSize(); iNeighbor++)
			{
				if ((p+*pNeighbor).IsInRange(trZero, trSize))
					*pElemDest += pElem[*pOffset]*(*pMask);
				else {
					p2 = p+*pNeighbor;
					p2.LimitWithinRange(trZero, trSize);
					*pElemDest += Element(p2)*(*pMask);
				}
				pNeighbor++;
				pOffset++;
				pMask++;
			}
			pElem++;
			pElemDest++;
		}

		// x > maxBound.x
		pElem = pElemY + (maxBound.x+1);
		pElemDest = pElemDestY + (maxBound.x+1);
		for (p.x=maxBound.x+1; p.x<iWidth; p.x++)
		{
			*pElemDest = 0.0f;
			pNeighbor = arrayNeighbors.GetBuffer();
			pOffset = arrayNeighborsOffsets.GetBuffer();
			pMask = arrayMask.GetBuffer();
			for (iNeighbor=0; iNeighbor<arrayNeighbors.GetSize(); iNeighbor++)
			{
				if ((p+*pNeighbor).IsInRange(trZero, trSize))
					*pElemDest += pElem[*pOffset]*(*pMask);
				else {
					p2 = p+*pNeighbor;
					p2.LimitWithinRange(trZero, trSize);
					*pElemDest += Element(p2)*(*pMask);
				}
				pNeighbor++;
				pOffset++;
				pMask++;
			}
			pElem++;
			pElemDest++;
		}

		pElemY+=iWidth;
		pElemDestY+=iWidth;
	}

	// y > maxBound.y
	pElem = this->pElements + (maxBound.y+1)*iWidth;
	pElemDest = arrayRes.pElements + (maxBound.y+1)*iWidth;
	for (p.y=maxBound.y+1; p.y<iHeight; p.y++)
	{
		for (p.x=0; p.x<iWidth; p.x++)
		{
			*pElemDest = 0.0f;
			pNeighbor = arrayNeighbors.GetBuffer();
			pOffset = arrayNeighborsOffsets.GetBuffer();
			pMask = arrayMask.GetBuffer();
			for (iNeighbor=0; iNeighbor<arrayNeighbors.GetSize(); iNeighbor++)
			{
				if ((p+*pNeighbor).IsInRange(trZero, trSize))
					*pElemDest += pElem[*pOffset]*(*pMask);
				else {
					p2 = p+*pNeighbor;
					p2.LimitWithinRange(trZero, trSize);
					*pElemDest += Element(p2)*(*pMask);
				}
				pNeighbor++;
				pOffset++;
				pMask++;
			}
			pElem++;
			pElemDest++;
		}
	}
	return arrayRes;
}

#endif
