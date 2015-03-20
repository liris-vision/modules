#include "arrayndfloat.h"
#include <string.h>

#ifndef min
#define min(a,b) (a<b?a:b)
#endif
#ifndef max
#define max(a,b) (a>b?a:b)
#endif

#ifdef ARRAYND_RUNTIME_CHECK
// Element returned in case of invalid access with operator [] or function GetElementInterpolate()
float CArray1D<float>::elementError;
#endif

CArray1D<float>::CArray1D(int size)
{
	iSize = size;
	pElements = new float[iSize];
}

CArray1D<float>::CArray1D(const CArray1D<float> &array)
{
	iSize = array.iSize;

	if (iSize!=0)
	{
		pElements = new float[iSize];
		memcpy(pElements, array.pElements, sizeof(float)*iSize);
	}
	else pElements = NULL;
}

CArray1D<float>::~CArray1D()
{
	Empty();
}

bool CArray1D<float>::Init(int size)
{
	if (pElements!=NULL)
		delete[] pElements;

	iSize = size;
	
	pElements = new float[iSize];
	if (pElements!=NULL)
		return true;
	else
		return false;
}

bool CArray1D<float>::Init(int size, float *pFloat)
{
	if (Init(size)==true)
	{
		float *pElementsTemp, *pFloatTemp;
		int i;

		pElementsTemp = pElements;
		pFloatTemp = pFloat;

		for (i=0;i<iSize;i++)
			*pElementsTemp++ = *pFloatTemp++;
		return true;
	}
	else
		return false;
}
/*
template <class S> bool CArray1D<float>::InitCast(const CArray1D<S> &array)
{
	if (pElements!=NULL)
		delete[] pElements;

	iSize = array.GetSize();
	
	pElements = new float[iSize];
	if (pElements!=NULL)
	{
		float *pElementsThis = pElements;
		S *pElementsParam = array.GetBuffer();
		int i;
		
		for (i=0;i<iSize;i++)
		{
			*pElementsThis = (float)(*pElementsParam);
			pElementsThis++;
			pElementsParam++;
		}
		return true;
	}
	else
		return false;
}
*/
void CArray1D<float>::Empty()
{
	if (pElements!=NULL)
		delete[] pElements;
	
	iSize = 0;
	pElements = NULL;
}

void CArray1D<float>::Fill(const float &t)
{
	int i;
	float *pElementsTemp = pElements;

	for (i=0;i<iSize;i++)
	{
		*pElementsTemp = t;
		pElementsTemp++;
	}
}

CArray1D<float> &CArray1D<float>::operator =(const CArray1D<float> &array)
{
	// Reallocate if necessary
	if (iSize!=array.iSize)
	{
		if (pElements!=NULL)
		{
			delete[] pElements;
			pElements = NULL;
		}
		iSize = array.iSize;
		pElements = new float[iSize];
	}

	// Copy elements
	if (iSize!=0)
		memcpy(pElements, array.pElements, sizeof(float)*iSize);

	return *this;
}

CArray1D<float> &CArray1D<float>::operator +=(const CArray1D<float> &array)
{
	if (iSize==array.iSize)
	{
		float *pElementsTemp, *pElementsTemp2;
		int i;

		pElementsTemp = pElements;
		pElementsTemp2 = array.pElements;

		for (i=0;i<iSize;i++)
			*pElementsTemp++ += *pElementsTemp2++;
		return *this;
	}
	else throw 0;
}

CArray1D<float> &CArray1D<float>::operator -=(const CArray1D<float> &array)
{
	if (iSize==array.iSize)
	{
		float *pElementsTemp, *pElementsTemp2;
		int i;

		pElementsTemp = pElements;
		pElementsTemp2 = array.pElements;

		for (i=0;i<iSize;i++)
			*pElementsTemp++ -= *pElementsTemp2++;
		return *this;
	}
	else throw 0;
}

CArray1D<float> &CArray1D<float>::operator *=(const float &t)
{
	float *pElementsTemp = pElements;
	int i;
	
	for (i=0;i<iSize;i++)
		*pElementsTemp++ *= t;
	return *this;
}

CArray1D<float> &CArray1D<float>::operator /=(const float &t)
{
	float *pElementsTemp = pElements;
	int i;
	
	for (i=0;i<iSize;i++)
		*pElementsTemp++ /= t;
	return *this;
}

CArray1D<float> CArray1D<float>::operator +(const CArray1D<float> &array) const
{
	if (iSize==array.iSize)
	{
		CArray1D<float> arrayResult(iSize);
		float *pElementsTemp, *pElementsTemp2, *pElementsTempResult;
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

CArray1D<float> CArray1D<float>::operator -() const
{
	CArray1D<float> arrayResult(iSize);
	float *pElementsTemp, *pElementsTempResult;
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

CArray1D<float> CArray1D<float>::operator -(const CArray1D<float> &array) const
{
	if (iSize==array.iSize)
	{
		CArray1D<float> arrayResult(iSize);
		float *pElementsTemp, *pElementsTemp2, *pElementsTempResult;
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

CArray1D<float> CArray1D<float>::operator *(const float &t) const
{
	CArray1D<float> arrayResult(iSize);
	float *pElementsTemp, *pElementsTempResult;
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

CArray1D<float> operator *(const float &t, const CArray1D<float> &v)
{
	CArray1D<float> arrayResult(v.iSize);
	float *pElementsTemp, *pElementsTempResult;
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

CArray1D<float> MultiplyElements(const CArray1D<float> &array1, const CArray1D<float> &array2)
{
	if (array1.iSize==array2.iSize)
	{
		CArray1D<float> arrayResult(array1.iSize);
		float *pElementsTemp1, *pElementsTemp2, *pElementsTempResult;
		int i;

		pElementsTemp1 = array1.pElements;
		pElementsTemp2 = array2.pElements;
		pElementsTempResult = arrayResult.pElements;
		
		for (i=0;i<array1.iSize;i++)
		{
			*pElementsTempResult = (*pElementsTemp1)*(*pElementsTemp2);
			pElementsTemp1++;
			pElementsTemp2++;
			pElementsTempResult++;
		}
		return arrayResult;
	}
	else throw 0;
}
	
CArray1D<float> CArray1D<float>::operator /(const float &t) const
{
	CArray1D<float> arrayResult(iSize);
	float *pElementsTemp, *pElementsTempResult;
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

CArray1D<float> &CArray1D<float>::AbsElements()
{
	float *pElementsTemp;
	int i;

	pElementsTemp = pElements;
	
	for (i=0;i<iSize;i++)
	{
		*pElementsTemp = fabs(*pElementsTemp);
		pElementsTemp++;
	}
	return *this;
}


CArray1D<float> &CArray1D<float>::SquareElements()
{
	float *pElementsTemp;
	int i;

	pElementsTemp = pElements;
	
	for (i=0;i<iSize;i++)
	{
		*pElementsTemp = (*pElementsTemp)*(*pElementsTemp);
		pElementsTemp++;
	}
	return *this;
}

CArray1D<float> &CArray1D<float>::SqrtElements()
{
	float *pElementsTemp;
	int i;

	pElementsTemp = pElements;
	
	for (i=0;i<iSize;i++)
	{
		*pElementsTemp = sqrt(*pElementsTemp);
		pElementsTemp++;
	}
	return *this;
}

CCouple<float> CArray1D<float>::GetMinMax() const
{
	CCouple<float> fMinMax;
	int i;
	float *pFloat;
	
	pFloat = pElements;
	
	fMinMax.x = *pFloat;
	fMinMax.y = *pFloat;
	for (i=0;i<iSize;i++)
	{
		if (*pFloat<fMinMax.x)
			fMinMax.x = *pFloat;
		else if (*pFloat>fMinMax.y)
			fMinMax.y = *pFloat;
		pFloat++;
	}
	return fMinMax;
}

float CArray1D<float>::Sum() const
{
	float *pElementsTemp;
	float fSum;
	int i;

	pElementsTemp = pElements;
	fSum = 0.0f;
	for (i=0;i<iSize;i++)
	{
		fSum += *pElementsTemp;
		pElementsTemp++;
	}
	return fSum;
}

float CArray1D<float>::L1Norm() const
{
	float *pElementsTemp;
	float fNorm;
	int i;

	pElementsTemp = pElements;
	fNorm = 0.0f;
	for (i=0;i<iSize;i++)
	{
		fNorm += fabs(*pElementsTemp);
		pElementsTemp++;
	}
	return fNorm;
}

float CArray1D<float>::L2Norm() const
{
	float *pElementsTemp;
	float fNorm;
	int i;

	pElementsTemp = pElements;
	fNorm = 0.0f;
	for (i=0;i<iSize;i++)
	{
		fNorm += (*pElementsTemp)*(*pElementsTemp);
		pElementsTemp++;
	}
	return sqrt(fNorm);
}

float CArray1D<float>::InfiniteNorm() const
{
	float *pElementsTemp;
	float fNorm;
	int i;

	pElementsTemp = pElements;
	fNorm = fabs(*pElementsTemp);
	for (i=0;i<iSize;i++)
	{
		if (fabs(*pElementsTemp)>fNorm)
			fNorm = fabs(*pElementsTemp);
		pElementsTemp++;
	}
	return fNorm;
}

float L1Distance(const CArray1D<float> &array1, const CArray1D<float> &array2)
{
	return (array1-array2).L1Norm();
}

float L2Distance(const CArray1D<float> &array1, const CArray1D<float> &array2)
{
	return (array1-array2).L2Norm();
}

float InfiniteDistance(const CArray1D<float> &array1, const CArray1D<float> &array2)
{
	return (array1-array2).InfiniteNorm();
}

void CArray1D<float>::Limit(float fMin, float fMax)
{
	int i;
	float *pFloat;
	
	pFloat = pElements;
	for (i=0;i<iSize;i++)
	{
		if (*pFloat<fMin)
			*pFloat = fMin;
		else if (*pFloat>fMax)
			*pFloat = fMax;
		pFloat++;
	}
}

void CArray1D<float>::Normalize(float fMin, float fMax, float &fMinInit, float &fMaxInit)
{
	int i;
	float *pFloat;
	float a, b;
	
	pFloat = pElements;
	
	fMinInit = *pFloat;
	fMaxInit = *pFloat;
	for (i=0;i<iSize;i++)
	{
		if (*pFloat<fMinInit)
			fMinInit = *pFloat;
		else if (*pFloat>fMaxInit)
			fMaxInit = *pFloat;
		pFloat++;
	}
	
	if (fMinInit==fMaxInit)
		return;
	
	a = (fMax-fMin)/(fMaxInit-fMinInit);
	b = fMin - fMinInit*a;

	pFloat = pElements;
	for (i=0;i<iSize;i++)
	{
		(*pFloat) = a*(*pFloat)+b;
		pFloat++;
	}
}

// CArray2D

CArray2D<float>::CArray2D(int width, int height):CArray1D<float>::CArray1D(width*height)
{
	iWidth = width;
	iHeight = height;
}

CArray2D<float>::CArray2D(const CCouple<int> &iDimensions):CArray1D<float>::CArray1D(iDimensions.x*iDimensions.y)
{
	iWidth = iDimensions.x;
	iHeight = iDimensions.y;
}

CArray2D<float>::CArray2D(const CArray2D<float> &array):CArray1D<float>::CArray1D(array)
{
	iWidth = array.iWidth;
	iHeight = array.iHeight;
}

CArray2D<float>::~CArray2D()
{
	Empty();
}

bool CArray2D<float>::Init(int width, int height)
{
	iWidth = width;
	iHeight = height;
	return CArray1D<float>::Init(iWidth*iHeight);
}

bool CArray2D<float>::Init(int width, int height, float *pFloat)
{
	iWidth = width;
	iHeight = height;
	return CArray1D<float>::Init(iWidth*iHeight, pFloat);
}

bool CArray2D<float>::Init(const CCouple<int> &iDimensions)
{
	iWidth = iDimensions.x;
	iHeight = iDimensions.y;
	return CArray1D<float>::Init(iWidth*iHeight);
}

/*
template <class S> bool CArray2D<float>::InitCast(const CArray2D<S> &array)
{
	iWidth = array.GetWidth();
	iHeight = array.GetHeight();

	if (pElements!=NULL)
		delete[] pElements;

	iSize = iWidth*iHeight;
	
	pElements = new float[iSize];
	if (pElements!=NULL)
	{
		float *pElementsThis = pElements;
		S *pElementsParam = array.GetBuffer();
		int i;
		
		for (i=0;i<iSize;i++)
		{
			*pElementsThis = (float)(*pElementsParam);
			pElementsThis++;
			pElementsParam++;
		}
		return true;
	}
	else
		return false;
}
*/

void CArray2D<float>::Empty()
{
	iWidth = iHeight = 0;
	CArray1D<float>::Empty();
}

void CArray2D<float>::Fill(float t)
{
	CArray1D<float>::Fill(t);
}

CArray2D<float> &CArray2D<float>::operator =(const CArray2D<float> &array)
{
	iWidth = array.iWidth;		
	iHeight = array.iHeight;

	CArray1D<float>::operator =(array);

	return *this;
}

CArray2D<float> &CArray2D<float>::operator +=(const CArray2D<float> &array)
{
	if (iWidth==array.iWidth && iHeight==array.iHeight)
	{
		CArray1D<float>::operator +=(array);
		return *this;
	}
	else
		throw 0;
}

CArray2D<float> &CArray2D<float>::operator -=(const CArray2D<float> &array)
{
	if (iWidth==array.iWidth && iHeight==array.iHeight)
	{
		CArray1D<float>::operator -=(array);
		return *this;
	}
	else
		throw 0;
}

CArray2D<float> &CArray2D<float>::operator *=(float t)
{
	CArray1D<float>::operator *=(t);
	return *this;
}

CArray2D<float> &CArray2D<float>::operator /=(float t)
{
	CArray1D<float>::operator /=(t);
	return *this;
}

CArray2D<float> CArray2D<float>::operator +(const CArray2D<float> &array) const
{
	if (iWidth==array.iWidth && iHeight==array.iHeight)
	{
		CArray2D<float> arrayRes(*this);
		return (arrayRes+=array);
	}
	else
		throw 0;
}

CArray2D<float> CArray2D<float>::operator -() const
{
	CArray2D<float> arrayRes(iWidth, iHeight);
	int i;
	float *pFloat, *pFloatRes;

	pFloat = pElements;
	pFloatRes = arrayRes.pElements;

	for (i=0;i<iSize;i++)
		(*pFloatRes++) = -(*pFloat++);

	return arrayRes;
}

CArray2D<float> CArray2D<float>::operator -(const CArray2D<float> &array) const
{
	if (iWidth==array.iWidth && iHeight==array.iHeight)
	{
		CArray2D<float> arrayRes(*this);
		return (arrayRes-=array);
	}
	else
		throw 0;
}

CArray2D<float> CArray2D<float>::operator *(float t) const
{
	CArray2D<float> arrayRes(*this);
	return arrayRes*=t;
}

CArray2D<float> operator *(float t, const CArray2D<float> &v)
{
	CArray2D<float> arrayRes(v);
	return arrayRes*=t;
}


CArray2D<float> MultiplyElements(const CArray2D<float> &array1, const CArray2D<float> &array2)
{
	if (array1.iWidth==array2.iWidth && array1.iHeight==array2.iHeight)
	{
		CArray2D<float> arrayResult(array1.iWidth, array1.iHeight);
		float *pElementsTemp1, *pElementsTemp2, *pElementsTempResult;
		int i;

		pElementsTemp1 = array1.pElements;
		pElementsTemp2 = array2.pElements;
		pElementsTempResult = arrayResult.pElements;
		
		for (i=0;i<array1.iSize;i++)
		{
			*pElementsTempResult = (*pElementsTemp1)*(*pElementsTemp2);
			pElementsTemp1++;
			pElementsTemp2++;
			pElementsTempResult++;
		}
		return arrayResult;
	}
	else
		throw 0;
}

CArray2D<float> CArray2D<float>::operator /(float t) const
{
	CArray2D<float> arrayRes(*this);
	return arrayRes/=t;
}

CArray2D<float> &CArray2D<float>::AbsElements()
{
	CArray1D<float>::AbsElements();
	return *this;
}

CArray2D<float> &CArray2D<float>::SquareElements()
{
	CArray1D<float>::SquareElements();
	return *this;
}

CArray2D<float> &CArray2D<float>::SqrtElements()
{
	CArray1D<float>::SqrtElements();
	return *this;
}

CCouple<float> CArray2D<float>::GetMinMax() const
{
	return CArray1D<float>::GetMinMax();
}

float CArray2D<float>::Sum() const
{
	return CArray1D<float>::Sum();
}

float CArray2D<float>::L1Norm() const
{
	return CArray1D<float>::L1Norm();
}

float CArray2D<float>::L2Norm() const
{
	return CArray1D<float>::L2Norm();
}

float CArray2D<float>::InfiniteNorm() const
{
	return CArray1D<float>::InfiniteNorm();
}

float L1Distance(const CArray2D<float> &array1, const CArray2D<float> &array2)
{
	return (array1-array2).L1Norm();
}

float L2Distance(const CArray2D<float> &array1, const CArray2D<float> &array2)
{
	return (array1-array2).L2Norm();
}

float InfiniteDistance(const CArray2D<float> &array1, const CArray2D<float> &array2)
{
	return (array1-array2).InfiniteNorm();
}

CArray1D<int> CArray2D<float>::GetNeighborOffsetsConnex4()
{
	CArray1D<int> arrayOffsets(4);
	
	arrayOffsets[0] = -1;
	arrayOffsets[1] = 1;
	arrayOffsets[2] = -iWidth;
	arrayOffsets[3] = iWidth;
	
	return arrayOffsets;
}

CArray1D<int> CArray2D<float>::GetNeighborOffsetsConnex8()
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

void CArray2D<float>::Limit(float fMin, float fMax)
{
	CArray1D<float>::Limit(fMin, fMax);
}

void CArray2D<float>::Normalize(float fMin, float fMax, float &fMinInit, float &fMaxInit)
{
	CArray1D<float>::Normalize(fMin, fMax, fMinInit, fMaxInit);
}

CArray2D<float> CArray2D<float>::DerivativeX(int iOrder, int iScheme) const
{
	CArray2D<float> arrayRes(iWidth, iHeight);
	int x, y;
	float *pFloatSrc, *pFloatDest;
	
	if (iOrder==0)
		arrayRes = *this;
	else if (iOrder==1)
	{
		pFloatSrc = pElements;
		pFloatDest = arrayRes.pElements;
		for (y=0;y<iHeight;y++)
		{
			if (iScheme==ARRAYNDFLOAT_BACKWARD)
			{
				(*pFloatDest) = 0.0f;
				pFloatSrc++;
				pFloatDest++;
				for (x=1;x<iWidth;x++)
				{
					*pFloatDest = *pFloatSrc - *(pFloatSrc-1);
					pFloatSrc++;
					pFloatDest++;
				}
			}
			else if (iScheme==ARRAYNDFLOAT_FORWARD)
			{
				for (x=0;x<iWidth-1;x++)
				{
					*pFloatDest = *(pFloatSrc+1) - *pFloatSrc;
					pFloatSrc++;
					pFloatDest++;
				}
				(*pFloatDest) = 0.0f;
				pFloatSrc++;
				pFloatDest++;
			}
			else if (iScheme==ARRAYNDFLOAT_CENTERED)
			{
				(*pFloatDest) = 0.0f;
				pFloatSrc++;
				pFloatDest++;
				for (x=1;x<iWidth-1;x++)
				{
					*pFloatDest = (*(pFloatSrc+1) - *(pFloatSrc-1))*0.5f;
					pFloatSrc++;
					pFloatDest++;
				}
				(*pFloatDest) = 0.0f;
				pFloatSrc++;
				pFloatDest++;
			}
		}
	}
	else if (iOrder==2)
	{
		pFloatSrc = pElements;
		pFloatDest = arrayRes.pElements;
		for (y=0;y<iHeight;y++)
		{
			*pFloatDest = 0.0f;
			pFloatSrc++;
			pFloatDest++;
			for (x=1;x<iWidth-1;x++)
			{
				*pFloatDest = *(pFloatSrc+1) - 2.0f*(*pFloatSrc) + *(pFloatSrc-1);
				pFloatSrc++;
				pFloatDest++;
			}
			*pFloatDest = 0.0f;
			pFloatSrc++;
			pFloatDest++;
		}
	}
	else {
		pFloatSrc = pElements;
		pFloatDest = arrayRes.pElements;
		for (y=0;y<iHeight;y++)
		{
			*pFloatDest = 0.0f;
			pFloatSrc++;
			pFloatDest++;
			for (x=1;x<iWidth-1;x++)
			{
				*pFloatDest = 1.0f;
				pFloatSrc++;
				pFloatDest++;
			}
			*pFloatDest = 0.0f;
			pFloatSrc++;
			pFloatDest++;
		}
	}
	return arrayRes;
}

CArray2D<float> CArray2D<float>::DerivativeY(int iOrder, int iScheme) const
{
	CArray2D<float> arrayRes(iWidth, iHeight);
	int x, y;
	float *pFloatSrc, *pFloatDest;

	if (iOrder==0)
		arrayRes = *this;
	else if (iOrder==1)
	{
		int i, iSizeTemp;

		if (iScheme==ARRAYNDFLOAT_BACKWARD)
		{
			pFloatDest = arrayRes.pElements;
			for (x=0;x<iWidth;x++)
				*pFloatDest++ = 0.0f;
			
			pFloatSrc = pElements + iWidth;
			iSizeTemp = iWidth*(iHeight-1);
			for (i=0;i<iSizeTemp;i++)
			{
				*pFloatDest = *pFloatSrc - *(pFloatSrc-iWidth);
				pFloatSrc++;
				pFloatDest++;
			}
		}
		else if (iScheme==ARRAYNDFLOAT_FORWARD)
		{
			pFloatDest = arrayRes.pElements;
			pFloatSrc = pElements;
			iSizeTemp = iWidth*(iHeight-1);
			for (i=0;i<iSizeTemp;i++)
			{
				*pFloatDest = *(pFloatSrc+iWidth) - *pFloatSrc;
				pFloatSrc++;
				pFloatDest++;
			}
			for (x=0;x<iWidth;x++)
				*pFloatDest++ = 0.0f;
		}
		else if (iScheme==ARRAYNDFLOAT_CENTERED)
		{
			pFloatDest = arrayRes.pElements;
			for (x=0;x<iWidth;x++)
				*pFloatDest++ = 0.0f;
			
			pFloatSrc = pElements + iWidth;
			iSizeTemp = iWidth*(iHeight-2);
			for (i=0;i<iSizeTemp;i++)
			{
				*pFloatDest = (*(pFloatSrc+iWidth) - *(pFloatSrc-iWidth))*0.5f;
				pFloatSrc++;
				pFloatDest++;
			}
			for (x=0;x<iWidth;x++)
				*pFloatDest++ = 0.0f;
		}
	}
	else if (iOrder==2)
	{
		int i, iSizeTemp;
		
		pFloatDest = arrayRes.pElements;
		for (x=0;x<iWidth;x++)
			*pFloatDest++ = 0.0f;
		pFloatSrc = pElements + iWidth;
		
		iSizeTemp = iWidth*(iHeight-2);
		/*
		for (y=1;y<iHeight-1;y++)
		{
			for (x=0;x<iWidth;x++)
			{
		*/
		for (i=0;i<iSizeTemp;i++)
		{
			*pFloatDest = *(pFloatSrc+iWidth) - 2.0f*(*pFloatSrc) + *(pFloatSrc-iWidth);
			pFloatSrc++;
			pFloatDest++;
		}
		//	}
		//}
		for (x=0;x<iWidth;x++)
			*pFloatDest++ = 0.0f;
	}
	else {
		pFloatSrc = pElements;
		pFloatDest = arrayRes.pElements;
		for (y=0;y<iHeight;y++)
		{
			*pFloatDest = 0.0f;
			pFloatSrc++;
			pFloatDest++;
			for (x=1;x<iWidth-1;x++)
			{
				*pFloatDest = 1.0f;
				pFloatSrc++;
				pFloatDest++;
			}
			*pFloatDest = 0.0f;
			pFloatSrc++;
			pFloatDest++;
		}
	}
	return arrayRes;
}

CArray2D<CCouple<float> > CArray2D<float>::Gradient(int iScheme) const
{
	CArray2D<CCouple<float> > arrayRes(iWidth, iHeight);
	int x, y;
	float *pFloatSrc;
	CCouple<float> *pFloatDest;
	
	pFloatSrc = pElements;
	pFloatDest = arrayRes.GetBuffer();
	
	if (iScheme==ARRAYNDFLOAT_BACKWARD)
	{
		pFloatSrc += iWidth;
		for (x=0; x<iWidth;x++)
		{
			pFloatDest->Set(0.0f, 0.0f);
			pFloatDest++;
		}
		
		for (y=1;y<iHeight;y++)
		{
			(*pFloatDest) = 0.0f;
			pFloatSrc++;
			pFloatDest++;
		
			for (x=1;x<iWidth;x++)
			{
				pFloatDest->x = *pFloatSrc - *(pFloatSrc-1);
				pFloatDest->y = *pFloatSrc - *(pFloatSrc-iWidth);
				pFloatSrc++;
				pFloatDest++;
			}
		}
	}
	else if (iScheme==ARRAYNDFLOAT_FORWARD)
	{
		for (y=0; y<iHeight-1; y++)
		{
			for (x=0;x<iWidth-1;x++)
			{
				pFloatDest->x = *(pFloatSrc+1) - *pFloatSrc;
				pFloatDest->y = *(pFloatSrc+iWidth) - *pFloatSrc;
				pFloatSrc++;
				pFloatDest++;
			}
			(*pFloatDest) = 0.0f;
			pFloatSrc++;
			pFloatDest++;
		}
		for (x=0; x<iWidth;x++)
		{
			pFloatDest->Set(0.0f, 0.0f);
			pFloatDest++;
		}
	}
	else if (iScheme==ARRAYNDFLOAT_CENTERED)
	{
		pFloatSrc += iWidth;
		for (x=0; x<iWidth;x++)
		{
			pFloatDest->Set(0.0f, 0.0f);
			pFloatDest++;
		}
		
		for (y=1; y<iHeight-1; y++)
		{
			(*pFloatDest) = 0.0f;
			pFloatSrc++;
			pFloatDest++;
		
			for (x=1;x<iWidth-1;x++)
			{
				pFloatDest->x = (*(pFloatSrc+1) - *(pFloatSrc-1))*0.5f;
				pFloatDest->y = (*(pFloatSrc+iWidth) - *(pFloatSrc-iWidth))*0.5f;
				pFloatSrc++;
				pFloatDest++;
			}
			(*pFloatDest) = 0.0f;
			pFloatSrc++;
			pFloatDest++;
		}
		for (x=0; x<iWidth;x++)
		{
			pFloatDest->Set(0.0f, 0.0f);
			pFloatDest++;
		}
	}
	return arrayRes;
}

CArray2D<float> CArray2D<float>::GradientNorm() const
{
	CArray2D<float> arrayRes(iWidth, iHeight);
	int x, y;
	float *pFloatSrc, *pFloatDest;
	float dx, dy;

	pFloatDest = arrayRes.pElements;
	for (x=0;x<iWidth;x++)
		*(pFloatDest++) = 0.0f;

	pFloatSrc = pElements+iWidth;
	for (y=1;y<iHeight;y++)
	{
		(*pFloatDest) = 0.0f;
		pFloatSrc++;
		pFloatDest++;
		for (x=1;x<iWidth;x++)
		{
			dx = *pFloatSrc - *(pFloatSrc-1);
			dy = *pFloatSrc - *(pFloatSrc-iWidth);
			*pFloatDest = sqrt(dx*dx+dy*dy);
			pFloatSrc++;
			pFloatDest++;
		}
	}
	return arrayRes;
}

CArray2D<float> CArray2D<float>::Laplacian() const
{
	CArray2D<float> arrayRes(iWidth, iHeight);
	int x, y;
	float *pFloatSrc, *pFloatDest;
	float dx2, dy2;

	pFloatDest = arrayRes.pElements;
	for (x=0;x<iWidth;x++)
		*(pFloatDest++) = 0.0f;

	pFloatSrc = pElements+iWidth;
	for (y=1;y<iHeight-1;y++)
	{
		(*pFloatDest) = 0.0f;
		pFloatSrc++;
		pFloatDest++;
		for (x=1;x<iWidth-1;x++)
		{
			dx2 = *(pFloatSrc+1) -2*(*pFloatSrc) + *(pFloatSrc-1);
			dy2 = *(pFloatSrc+iWidth) -2*(*pFloatSrc) + *(pFloatSrc-iWidth);
			*pFloatDest = dx2+dy2;
			pFloatSrc++;
			pFloatDest++;
		}
		(*pFloatDest) = 0.0f;
		pFloatSrc++;
		pFloatDest++;
	}
	for (x=0;x<iWidth;x++)
		*(pFloatDest++) = 0.0f;

	return arrayRes;
}

void CArray2D<float>::SetL2DistanceMap(const CCouple<int> &coord)
{
	float *pFloatSrc;
	float dx, dy;
	int x, y;

	pFloatSrc = pElements;
	for (y=0;y<iHeight;y++)
	{
		dy = (float)(y-coord.y);
		for (x=0;x<iWidth;x++)
		{
			dx = (float)(x-coord.x);
			*pFloatSrc = sqrt(dx*dx+dy*dy);
			pFloatSrc++;
		}
	}
}

// Convolution with a centered mask
CArray2D<float> CArray2D<float>::Convolve(const CArray2D<float> &arrayCenteredMask) const
{
	CArray1D<CCouple<int> > arrayNeighbors;
	CArray1D<float> arrayMask;
	CCouple<int> neighbor, *pNeighbor;
	float *pMask, *pCenteredMask;
	
	arrayNeighbors.Init(arrayCenteredMask.CArray1D<float>::GetSize());
	arrayMask.Init(arrayCenteredMask.CArray1D<float>::GetSize());

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
CArray2D<float> CArray2D<float>::Convolve(const CArray1D<CCouple<int> > &arrayNeighbors, const CArray1D<float> &arrayMask) const
{
	CArray2D<float> arrayRes;
	int iNeighbor;
	float *pMask;
	int *pOffset;
	float *pFloatY, *pFloat;
	float *pFloatDestY, *pFloatDest;
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
	pFloatY = pElements + minBound.y*iWidth;
	pFloatDestY = arrayRes.pElements + minBound.y*iWidth;
	for (p.y=minBound.y; p.y<=maxBound.y; p.y++)
	{
		pFloat = pFloatY + minBound.x;
		pFloatDest = pFloatDestY + minBound.x;
		for (p.x=minBound.x; p.x<=maxBound.x; p.x++)
		{
			*pFloatDest = 0.0f;
			pOffset = arrayNeighborsOffsets.GetBuffer();
			pMask = arrayMask.GetBuffer();
			for (iNeighbor=0; iNeighbor<arrayNeighbors.GetSize(); iNeighbor++)
			{
				*pFloatDest += pFloat[*pOffset]*(*pMask);
				pOffset++;
				pMask++;
			}
			pFloat++;
			pFloatDest++;
		}
		pFloatY+=iWidth;
		pFloatDestY+=iWidth;
	}
	
	// Bounds
	CCouple<int> trZero, trSize;
	trZero.Set(0, 0);
	trSize.Set(iWidth-1, iHeight-1);

	// y < minBound.y
	pFloat = pElements;
	pFloatDest = arrayRes.pElements;
	for (p.y=0; p.y<minBound.y; p.y++)
	{
		for (p.x=0; p.x<iWidth; p.x++)
		{
			*pFloatDest = 0.0f;
			pNeighbor = arrayNeighbors.GetBuffer();
			pOffset = arrayNeighborsOffsets.GetBuffer();
			pMask = arrayMask.GetBuffer();
			for (iNeighbor=0; iNeighbor<arrayNeighbors.GetSize(); iNeighbor++)
			{
				if ((p+*pNeighbor).IsInRange(trZero, trSize))
					*pFloatDest += pFloat[*pOffset]*(*pMask);
				else {
					p2 = p+*pNeighbor;
					p2.LimitWithinRange(trZero, trSize);
					*pFloatDest += Element(p2)*(*pMask);
				}
				pNeighbor++;
				pOffset++;
				pMask++;
			}
			pFloat++;
			pFloatDest++;
		}
	}

	// minBound.y <= y <= minBound.y
	pFloatY = pElements + minBound.y*iWidth;
	pFloatDestY = arrayRes.pElements + minBound.y*iWidth;
	for (p.y=minBound.y; p.y<=maxBound.y; p.y++)
	{
		// x < minBound.x
		pFloat = pFloatY;
		pFloatDest = pFloatDestY;
		for (p.x=0; p.x<minBound.x; p.x++)
		{
			*pFloatDest = 0.0f;
			pNeighbor = arrayNeighbors.GetBuffer();
			pOffset = arrayNeighborsOffsets.GetBuffer();
			pMask = arrayMask.GetBuffer();
			for (iNeighbor=0; iNeighbor<arrayNeighbors.GetSize(); iNeighbor++)
			{
				if ((p+*pNeighbor).IsInRange(trZero, trSize))
					*pFloatDest += pFloat[*pOffset]*(*pMask);
				else {
					p2 = p+*pNeighbor;
					p2.LimitWithinRange(trZero, trSize);
					*pFloatDest += Element(p2)*(*pMask);
				}
				pNeighbor++;
				pOffset++;
				pMask++;
			}
			pFloat++;
			pFloatDest++;
		}

		// x > maxBound.x
		pFloat = pFloatY + (maxBound.x+1);
		pFloatDest = pFloatDestY + (maxBound.x+1);
		for (p.x=maxBound.x+1; p.x<iWidth; p.x++)
		{
			*pFloatDest = 0.0f;
			pNeighbor = arrayNeighbors.GetBuffer();
			pOffset = arrayNeighborsOffsets.GetBuffer();
			pMask = arrayMask.GetBuffer();
			for (iNeighbor=0; iNeighbor<arrayNeighbors.GetSize(); iNeighbor++)
			{
				if ((p+*pNeighbor).IsInRange(trZero, trSize))
					*pFloatDest += pFloat[*pOffset]*(*pMask);
				else {
					p2 = p+*pNeighbor;
					p2.LimitWithinRange(trZero, trSize);
					*pFloatDest += Element(p2)*(*pMask);
				}
				pNeighbor++;
				pOffset++;
				pMask++;
			}
			pFloat++;
			pFloatDest++;
		}

		pFloatY+=iWidth;
		pFloatDestY+=iWidth;
	}
		
	// y > maxBound.y
	pFloat = pElements + (maxBound.y+1)*iWidth;
	pFloatDest = arrayRes.pElements + (maxBound.y+1)*iWidth;
	for (p.y=maxBound.y+1; p.y<iHeight; p.y++)
	{
		for (p.x=0; p.x<iWidth; p.x++)
		{
			*pFloatDest = 0.0f;
			pNeighbor = arrayNeighbors.GetBuffer();
			pOffset = arrayNeighborsOffsets.GetBuffer();
			pMask = arrayMask.GetBuffer();
			for (iNeighbor=0; iNeighbor<arrayNeighbors.GetSize(); iNeighbor++)
			{
				if ((p+*pNeighbor).IsInRange(trZero, trSize))
					*pFloatDest += pFloat[*pOffset]*(*pMask);
				else {
					p2 = p+*pNeighbor;
					p2.LimitWithinRange(trZero, trSize);
					*pFloatDest += Element(p2)*(*pMask);
				}
				pNeighbor++;
				pOffset++;
				pMask++;
			}
			pFloat++;
			pFloatDest++;
		}
	}
	return arrayRes;
}

// Gaussian smoothing with one standard deviation
void CArray2D<float>::SetGaussianKernel(float fSigma, int iHalfSize)
{
	CCouple<int> p;
	int iSize;
	CCouple<float> fCoefs;
	float fSumCoefs, fExponent, fSigma2;
	float *pFloat;
	
	if (iHalfSize<=0)
		iHalfSize = max(1, (int)(3.0f * fSigma));
	iSize = 2*iHalfSize+1;

	Init(iSize, iSize);

	fSumCoefs = 0.0f;
	fSigma2 = 2.0f*fSigma*fSigma;
	pFloat = pElements;
	
	for (p.y=-iHalfSize;p.y<=iHalfSize;p.y++)
	{
		fCoefs.y = (float)(p.y*p.y);
		for (p.x=-iHalfSize;p.x<=iHalfSize;p.x++)
		{
			fCoefs.x = (float)(p.x*p.x);
			fExponent = (fCoefs.x+fCoefs.y)/fSigma2;
			*pFloat = exp(-fExponent);
			fSumCoefs += *pFloat;
			pFloat++;
		}
	}
	*this/=fSumCoefs;
}

// Gaussian smoothing with given standard deviation
void CArray2D<float>::SetLaplacianOfGaussianKernel(float fSigma, int iHalfSize)
{
	CCouple<int> p;
	int iSize;
	CCouple<float> fCoefs;
	float f2Sigma2, fDen;
	float *pFloat;
	
	if (iHalfSize<=0)
		iHalfSize = max(1, (int)(5.0f * fSigma));
	iSize = 2*iHalfSize+1;

	Init(iSize, iSize);

	f2Sigma2 = 2.0f*fSigma*fSigma;
	fDen = pow(fSigma, 5)*2.5066f; // sqrt(2*pi)
	pFloat = pElements;
	
	for (p.y=-iHalfSize;p.y<=iHalfSize;p.y++)
	{
		fCoefs.y = (float)(p.y*p.y);
		for (p.x=-iHalfSize;p.x<=iHalfSize;p.x++)
		{
			fCoefs.x = (float)(p.x*p.x);
			*pFloat = exp(-(fCoefs.x+fCoefs.y)/f2Sigma2)*(fCoefs.x+fCoefs.y-f2Sigma2)/fDen;
			pFloat++;
		}
	}
}
