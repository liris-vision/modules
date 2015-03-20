/*
	deformablemodel.h

	Copyright 2010 Julien Mille (julien.mille@liris.cnrs.fr)
	http://liris.cnrs.fr/~jmille/code.html

	Header file of library implementing the active contour model with narrow band
	region energy described in

	[Mille09] J. Mille, "Narrow band region-based active contours and surfaces for
	          2D and 3D segmentation", Computer Vision and Image Understanding,
			  volume 113, issue 9, pages 946-965, 2009

	A preprint version of this paper may be downloaded at
	http://liris.cnrs.fr/~jmille/doc/cviu09temp.pdf

	If you use this code for research purposes, please cite the aforementioned paper
	in any resulting publication.

	The current header file corresponds to the basic deformable model abstract class,
	which the active contour and binary region inherit from
*/

#ifndef _DEFORMABLEMODEL_H_
#define _DEFORMABLEMODEL_H_

#include <list>
#include <opencv2/core/core.hpp>
#include "couple.h"
#include "triplet.h"

using namespace std;

// Abstract base class CDeformableModelBase
// Contains pure virtual member functions and thus cannot be instantiated
class CDeformableModelBase
{
  // Types
  public:
	typedef enum {RGB, YUV, LAB} typeColorSpace;
	typedef enum {REGION, REGION_INIT, BAND, LINE} typeRegionMode;
 
  // Members
  public:
	// Weights on energies
	// These change the significance of force vectors in the parametric model or speeds in the implicit model
	// We actually define more weighting coefficients than in [Mille09]. This is to allow the addition of several
	// energies, for mixing or comparing various methods. Time step \Delta t (which appears in gradient descent
	// evolution equation (52) ) is included into these weights.
	float fWeightSmoothness;	   // Weight of smoothness energy (related to \omega introduced in equation (3) in [Mille09])
	float fWeightGradient;	       // Weight of gradient energy
	float fWeightBalloon;	       // Weight of balloon energy (also used in bias component)
	float fWeightRegion;	       // Weight of region energy (1-\omega in [Mille09] but more general here)

	// The following parameter allows an asymmetric configuration of the region term, in order to favor
	// minimization of intensity/color deviation inside or outside. This is useful when implementing
	// the data term of the Chan-Vese model (which is asymmetric) for comparison purpose.
	float fWeightRegionInOverOut;

	// Domains on which region energy is computed
	// These change the actual energy to be minimized and the resulting force or speed
	// Inside mode : domain on which the inner term of region energy is computed
	typeRegionMode iInsideMode, iOutsideMode;

	// If iInsideMode==INSIDEMODE_BAND and iOutsideMode==OUTSIDEMODE_BAND, both inner and outer
	// narrow bands are considered. The resulting region force is the one of equation (53), which
	// is derived from the first narrow band region energy related to equations (5) and (18) in [Mille09]

	// If iInsideMode==INSIDEMODE_BAND and iOutsideMode==OUTSIDEMODE_LINE, the inner narrow band and
	// local normal line segments outside the contour are considered. This makes the outer region term local.
	// The resulting region force is the one of equation (54), which is derived from the second narrow
	// band region energy (the local one) related to equations (6) and (19) in [Mille09]

	// If iInsideMode==INSIDEMODE_REGION and iOutsideMode==OUTSIDEMODE_REGION, the entire regions
	// inside and outside the contour are considered. In this case, the model turns out to be the
	// parametric counterpart of the Chan-Vese active contour model. This configuration should be
	// tested for comparison purpose

	// You may combine several inside and outside modes which are not actually described in [Mille09],
	// e.g. considering the entire region inside the contour (iInsideMode==INSIDEMODE_REGION)
	// and local line segments in the outer narrow band (iOutsideMode==OUTSIDEMODE_LINE)
	// The special value INSIDEMODE_REGIONINIT for iInsideMode enables to keep the initial average
	// intensity/color inside the contour as inner region descriptor. In this case, the inner descriptor
	// is computed once and for all at initialization and is not updated after gradient descent steps

	// Band thickness
	// Denoted B and introduced in equation (7) in [Mille09]
	int iBandThickness;

	// Enable or disable gaussian smoothing
	bool bUseGaussianFilter;

	// Standard deviation of gaussian filter
	// Denoted \sigma and introduced in equation (55) in [Mille09]
	// Useful only when bUseGaussianFilter is true and fWeightSmoothness is non-zero
	float fStdDeviationGaussianFilter;

	// Pointer to input image
	// Initialized as null in default constructor. Needs to be set by calling AttachImage()
	const cv::Mat *pInputImage;

	// Enable or disable the use of bias force in the region force
	// This bias acts as a balloon term if inner and outer descriptors are close to each other
	// See section 5.5 in [Mille09]
	bool bRegionEnergyBias;

	// Enable or disable the checking of a stopping criterion after each iteration of the evolution method
	bool bStoppingCriterion;

  public:
	// Default constructor
	CDeformableModelBase()
	{
		// Set energy weights
		fWeightSmoothness  = 0.5f;
		fWeightGradient = 0.0f;
		fWeightBalloon = 0.0f;
		fWeightRegion = 2.0f;

		// By default, region energy is symmetric (inner and outer region terms have the same contribution)
		fWeightRegionInOverOut = 0.5f;

		// Bands are considered inside and outside
		iInsideMode = BAND; // other possible values are INSIDEMODE_BAND and INSIDEMODE_REGIONINIT
		iOutsideMode = BAND; // other possible values are OUTSIDEMODE_REGION and OUTSIDEMODE_LINE

		iBandThickness = 20;

		// The bias may be useful for particular energy configurations, however it is disabled by default
		bRegionEnergyBias = false;

		// Parameters related to gaussian filtering
		bUseGaussianFilter = true;
		fStdDeviationGaussianFilter = 2.0f;

		// Model is not attached to image yet
		pInputImage = NULL;

		// Stopping criterion will be checked in the evolution method
		bStoppingCriterion = true;
	}

	// Destructor
	virtual ~CDeformableModelBase() {}

	// Attach the active contour model to the image
	// Precompute every image data useful for evolving the contour :
	// - array of intensities/colors stored as real numbers
	// - vector field derived from gradient magnitude (if gradient force is used)
	// - ...
	// Should be called before initialization (InitCircle(), InitEllipse(), ...)
	// Pure virtual function: overridden in CActiveContourGrayscale and CActiveContourColor
	virtual void AttachImage(const cv::Mat *)=0;

	// Perform one iteration of evolution method, i.e. update vertices with forces
	// deriving from discretization of gradient descent of energy functional
	// Several passes of discrete evolution equation (53) in [Mille09] are applied to all vertices
	// Params: number of passes
	// Return value: TRUE if the model was actually evolved and may be evolved further,
	//               FALSE otherwise (e.g. stopping criterion was met or any other problem was encountered)
	virtual bool EnergyGradientDescent(int)=0;

	// Initialize the model as a circle
	// Params: center, radius
	virtual void InitCircle(const CCouple<float> &, float)=0;

	// Initialize the model as an ellipse
	// Params: center, x-radius, y-radius
	virtual void InitEllipse(const CCouple<float> &, float, float)=0;

	// Draw the contour in a RGB image with a given zooming factor
	// Params: RGB image, integer zooming factor
	virtual void DrawInImageRGB(cv::Mat &, int) const=0;

    // Make a binary image of segmentation
    // Should be typically called at the end of the evolution
	// Params: output 8-bit image
	virtual void MakeBinaryMask(cv::Mat &) const=0;

	// Destroy deformable model
	virtual void Empty()=0;
};

#endif
