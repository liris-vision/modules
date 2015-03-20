/*
	cvmat2array.h

	Copyright 2013 Julien Mille (julien.mille@liris.cnrs.fr)
	http://liris.cnrs.fr/~jmille/code.html

	Header file for converting OpenCV's cv::Mat to CArray2D
*/

#ifndef _CVMAT2ARRAY_H_
#define _CVMAT2ARRAY_H_

#include "arrayndfloat.h"
#include "triplet.h"
#include <opencv2/core/core.hpp>

// Conversion of grayscale images

// Convert a cv::Mat of unsigned char (CV_8UC) to an array of real values between 0 and 1
// Params : [out] 2D array of real values, [in] OpenCV's cv::Mat
bool ConvertCvMatToArray2DFloat(CArray2D<float> &, const cv::Mat &);

// Conversion of color RGB images
// In the three following functions, the cv::Mat parameter is assumed to contain RGB values with byte precision (OpenCV's CV_8UC3 predefined type macro)

// Convert a cv::Mat of byte precision RGB-values to an array of triplets of real values in scaled RGB space ( [0..1]^3 )
// Params : [out] 2D array of triplets of real values, [in] OpenCV's cv::Mat
bool ConvertCvMatToArray2DTripletFloatRGB(CArray2D<CTriplet<float> > &, const cv::Mat &);

// Convert a cv::Mat of byte precision RGB-values to an array of triplets of real values in scaled YUV space
// Params : [out] 2D array of triplets of real values, [in] OpenCV's cv::Mat
bool ConvertCvMatToArray2DTripletFloatYUV(CArray2D<CTriplet<float> > &, const cv::Mat &);

// Convert a cv::Mat of byte precision RGB-values to an array of triplets of real values in scaled Lab space
// Params : [out] 2D array of triplets of real values, [in] OpenCV's cv::Mat
bool ConvertCvMatToArray2DTripletFloatLAB(CArray2D<CTriplet<float> > &, const cv::Mat &);

#endif
