/*
	cvmat2array.cpp

	Copyright 2013 Julien Mille (julien.mille@liris.cnrs.fr)
	http://liris.cnrs.fr/~jmille/code.html

	Source file for converting OpenCV's cv::Mat to CArray2D
*/

#include "cvmat2array.h"

// Conversion of grayscale images

// Convert a cv::Mat of unsigned char (CV_8UC) to an array of real values between 0 and 1
// Params : [out] 2D array of real values, [in] OpenCV's cv::Mat
bool ConvertCvMatToArray2DFloat(CArray2D<float> &array2D, const cv::Mat &img)
{
	int x, y, iWidth, iHeight;
	const unsigned char *pBits;
	float *pFloat;

	if (img.type()!=CV_8UC1)
	{
		cerr<<"ERROR in ConvertCvMatToArray2DFloat(...): image is not 8-bit"<<endl;
		return false;
	}

	iWidth = img.cols;
	iHeight = img.rows;

	if (array2D.GetWidth()!=iWidth || array2D.GetHeight()!=iHeight)
	{
		if (array2D.Init(iWidth, iHeight)==false)
			return false;
	}

	pFloat = array2D.GetBuffer();
	for (y=0;y<iHeight;y++)
	{
	    pBits = img.ptr(y);
		for (x=0;x<iWidth;x++)
		{
			*pFloat = (float)(*pBits)/255.0f;
			pBits++;
			pFloat++;
		}
	}
	return true;
}

// Conversion of color RGB images
// In the three following functions, the cv::Mat parameter is assumed to contain RGB values with byte
// precision (OpenCV's CV_8UC3 predefined type macro)

// Convert a cv::Mat of byte precision RGB-values to an array of triplets of real values in scaled RGB space ( [0..1]^3 )
// Params : [out] 2D array of triplets of real values, [in] OpenCV's cv::Mat
bool ConvertCvMatToArray2DTripletFloatRGB(CArray2D<CTriplet<float> > &array2D, const cv::Mat &img)
{
    int x, y, iWidth, iHeight;
	const unsigned char *pBits;
	CTriplet<float> *pTripletFloat;

	if (img.type()!=CV_8UC3)
	{
		cerr<<"ERROR in ConvertCvMatToArray2DTripletFloatRGB(...): image is not 8-bit RGB"<<endl;
		return false;
	}

	iWidth = img.cols;
	iHeight = img.rows;

	if (array2D.GetWidth()!=iWidth || array2D.GetHeight()!=iHeight)
	{
		if (array2D.Init(iWidth, iHeight)==false)
			return false;
	}

    // cv::Vec3b vecImage;
	pTripletFloat = array2D.GetBuffer();
	// pBits = (unsigned char *)img.data;

	for (y=0;y<iHeight;y++)
	{
	    pBits = img.ptr(y);
		for (x=0;x<iWidth;x++)
		{
			// *pFloat = (float)(*pBits)/255.0f;
			// vecImage = img.at<cv::Vec3b>(y,x);
			pTripletFloat->x = (float)(pBits[2])/255.0f;
			pTripletFloat->y = (float)(pBits[1])/255.0f;
			pTripletFloat->z = (float)(pBits[0])/255.0f;

			pBits+=3;
			pTripletFloat++;
		}
	}
	return true;
}

// Convert a cv::Mat of byte precision RGB-values to an array of triplets of real values in scaled YUV space
// Params : [out] 2D array of triplets of real values, [in] OpenCV's cv::Mat
bool ConvertCvMatToArray2DTripletFloatYUV(CArray2D<CTriplet<float> > &array2D, const cv::Mat &img)
{
    int x, y, iWidth, iHeight;
	const unsigned char *pBits;
	CTriplet<float> *pTripletFloat;
	float fRed, fGreen, fBlue;

	if (img.type()!=CV_8UC3)
	{
		cerr<<"ERROR in ConvertCvMatToArray2DTripletFloatYUV(...): image is not 8-bit RGB"<<endl;
		return false;
	}

	iWidth = img.cols;
	iHeight = img.rows;

	if (array2D.GetWidth()!=iWidth || array2D.GetHeight()!=iHeight)
	{
		if (array2D.Init(iWidth, iHeight)==false)
			return false;
	}

    // cv::Vec3b vecImage;
	pTripletFloat = array2D.GetBuffer();
	// pBits = (unsigned char *)img.data;

	for (y=0;y<iHeight;y++)
	{
	    pBits = img.ptr(y);
		for (x=0;x<iWidth;x++)
		{
			fRed = (float)pBits[2];
			fGreen = (float)pBits[1];
			fBlue = (float)pBits[0];

			pTripletFloat->x =  (0.299f*fRed + 0.587f*fGreen + 0.114f*fBlue)/255.0f; // Luminance Y
			pTripletFloat->y = (-0.147f*fRed - 0.289f*fGreen + 0.436f*fBlue)/255.0f; // Chrominance U
			pTripletFloat->z =  (0.615f*fRed - 0.515f*fGreen - 0.100f*fBlue)/255.0f; // Chrominance V

			pBits+=3;
			pTripletFloat++;
		}
	}
	return true;
}

// Convert a cv::Mat of byte precision RGB-values to an array of triplets of real values in scaled Lab space
// Params : [out] 2D array of triplets of real values, [in] OpenCV's cv::Mat
bool ConvertCvMatToArray2DTripletFloatLAB(CArray2D<CTriplet<float> > &array2D, const cv::Mat &img)
{
   int x, y, iWidth, iHeight;
	const unsigned char *pBits;
	CTriplet<float> *pTripletFloat;
	CTriplet<float> fReferenceWhiteXYZ, fPixelXYZ, fNormalizedXYZ, fTransformedXYZ;
	CTriplet<float> fRGBCorrected;
	float fRed, fGreen, fBlue;

	static float fGamma = 2.2f; // Gamma value for RGB correction (sRGB)
	static float fEpsilon = 0.008856f, fKappa = 903.3f; // Constants given by the CIE standards (for XYZ->Lab conversion)

	if (img.type()!=CV_8UC3)
	{
		cerr<<"ERROR in ConvertCvMatToArray2DTripletFloatLAB(...): image is not 8-bit RGB"<<endl;
		return false;
	}

	iWidth = img.cols;
	iHeight = img.rows;

	if (array2D.GetWidth()!=iWidth || array2D.GetHeight()!=iHeight)
	{
		if (array2D.Init(iWidth, iHeight)==false)
			return false;
	}

    // cv::Vec3b vecImage;
	pTripletFloat = array2D.GetBuffer();
	// pBits = (unsigned char *)img.data;
    // Get reference white in XYZ color space

	fRGBCorrected.Set(1.0f, 1.0f, 1.0f);

	// Convert to XYZ
	fReferenceWhiteXYZ.x = 0.576700f*fRGBCorrected.x + 0.297361f*fRGBCorrected.y + 0.0270328f*fRGBCorrected.z;
	fReferenceWhiteXYZ.y = 0.185556f*fRGBCorrected.x + 0.627355f*fRGBCorrected.y + 0.0706879f*fRGBCorrected.z;
	fReferenceWhiteXYZ.z = 0.188212f*fRGBCorrected.x + 0.075285f*fRGBCorrected.y + 0.9912480f*fRGBCorrected.z;

	for (y=0;y<iHeight;y++)
	{
	    pBits = img.ptr(y);
		for (x=0;x<iWidth;x++)
		{
			fRed = (float)pBits[2];
			fGreen = (float)pBits[1];
			fBlue = (float)pBits[0];

			// The conversion methods were taken from
			// http://www.brucelindbloom.com/

			// RGB correction with gamma function
			fRGBCorrected.x = pow(fRed/255.0f, fGamma);
			fRGBCorrected.y = pow(fGreen/255.0f, fGamma);
			fRGBCorrected.z = pow(fBlue/255.0f, fGamma);

			// Convert to XYZ
			fPixelXYZ.x = 0.576700f*fRGBCorrected.x + 0.297361f*fRGBCorrected.y + 0.0270328f*fRGBCorrected.z;
			fPixelXYZ.y = 0.185556f*fRGBCorrected.x + 0.627355f*fRGBCorrected.y + 0.0706879f*fRGBCorrected.z;
			fPixelXYZ.z = 0.188212f*fRGBCorrected.x + 0.075285f*fRGBCorrected.y + 0.9912480f*fRGBCorrected.z;

			// Normalize with respect to reference white
			fNormalizedXYZ.x = fPixelXYZ.x/fReferenceWhiteXYZ.x;
			fNormalizedXYZ.y = fPixelXYZ.y/fReferenceWhiteXYZ.y;
			fNormalizedXYZ.z = fPixelXYZ.z/fReferenceWhiteXYZ.z;

			// Transform
			if (fNormalizedXYZ.x<=fEpsilon)
				fTransformedXYZ.x = (fKappa*fNormalizedXYZ.x+16.0f)/116.0f;
			else
				fTransformedXYZ.x = pow(fNormalizedXYZ.x, 1.0f/3.0f);
			if (fNormalizedXYZ.y<=fEpsilon)
				fTransformedXYZ.y = (fKappa*fNormalizedXYZ.y+16.0f)/116.0f;
			else
				fTransformedXYZ.y = pow(fNormalizedXYZ.y, 1.0f/3.0f);
			if (fNormalizedXYZ.z<=fEpsilon)
				fTransformedXYZ.z = (fKappa*fNormalizedXYZ.z+16.0f)/116.0f;
			else
				fTransformedXYZ.z = pow(fNormalizedXYZ.z, 1.0f/3.0f);

			// These is the standard scaling to obtain
			// L in [0,100], a in [-100,100] and b in [-100,100]
			// L = 116.0f*fTransformedXYZ.y-16.0f;
			// a = 500.0f*(fTransformedXYZ.x-fTransformedXYZ.y);
			// b = 200.0f*(fTransformedXYZ.y-fTransformedXYZ.z);

			// We actually apply the following scaling to have approximately
			// L in [0,1], a in [-0.5,0.5] and b in [-0.5,0.5]
			pTripletFloat->x = fTransformedXYZ.y; // L
			pTripletFloat->y = 2.5f*(fTransformedXYZ.x-fTransformedXYZ.y); // a
			pTripletFloat->z = fTransformedXYZ.y-fTransformedXYZ.z; // b

			pBits+=3;
			pTripletFloat++;
		}
	}
	return true;
}
