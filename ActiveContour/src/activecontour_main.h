#ifndef _ACTIVECONTOUR_MAIN_H_
#define _ACTIVECONTOUR_MAIN_H_


#ifdef D_BUILDWINDLL
	#define DLL_EXPORT __declspec(dllexport)
#else
	#define DLL_EXPORT
#endif

#include <opencv2/core/core.hpp>
#include "deformablemodel.h"

typedef struct
{
	float weightRegion;
	float weightGradient;
	float weightBalloon;
	float stdDeviationGaussianSmooth;
	CDeformableModelBase::typeColorSpace colorSpace;
	CDeformableModelBase::typeRegionMode insideMode, outsideMode;
} ActiveContour_OpenParams;

// Params :
// [in] 8-bit grayscale or 24-bit RGB input image
// [out] 8-bit grayscale output binary mask of the region
// [in] Centre of initial ellipse
// [in] Radii of initial ellipse
DLL_EXPORT void ExecActiveContour(const cv::Mat *, cv::Mat *, const cv::Point *, const cv::Point *, const ActiveContour_OpenParams &);

#endif

