/*
	activecontour.h

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

	The current header file corresponds to the explicit (parametric) implementation
	of the active contour model.
	See sections 2 and 5 in [Mille09] for the continuous variational model and its
	explicit implementation.
*/

#ifndef _ACTIVECONTOUR_H_
#define _ACTIVECONTOUR_H_

#include <list>
#include "deformablemodel.h"
#include "arrayndfloat.h"

using namespace std;



// Class CVertex2D
// Represents a single vertex with given position and unit inward normal vector
class CVertex2D
{
  // Members
  public:
	CCouple<float> pfPos;    // Position
	CCouple<float> vfNormal; // Unit inward normal vector

  // Member functions
  public:
	// Default constructor is empty
	CVertex2D() {}
};



// Abstract base class CActiveContourBase
// Contains pure virtual member functions and thus cannot be instantiated
class CActiveContourBase : public CDeformableModelBase
{
  // Members
  public:
	// Linked list of vertices
	// This implements the set denoted {p_i} in [Mille09]
	list<CVertex2D> listVertices;

	// Enable or disable resampling during evolution (after each gradient descent step)
	bool bResample;

	// Sampling distance
	// Corresponds to the minimal distance between two successive vertices
	// Denoted w and introduced in equation (50) in [Mille09]
	// Useful only when bResample is true
	// If so, the length of every edge will be mainted between fSampling and 2*fSampling
	float fSampling;

	// Vector field derived from image gradient magnitude
	// Used to compute gradient force if fWeightGradient is non-zero
	CArray2D<CCouple<float> > arrayGradientField;

	// Enable or disable the precomputation of image integrals
	// Estimation of average intensities/colors inside and outside the contour is performed in
	// ComputeRegionMeansGreenTheorem() in derived classes. If image integrals are precomputed,
	// memory cost is higher (though not critical at all for 2D images) and the estimation process
	// is faster. Only used when the inner or outer region is used (iInsideMode==INSIDEMODE_REGION
	// or/and iOutsideMode==OUTSIDEMODE_REGION)
	bool bUseImageIntegrals;

	// Enable or disable the use of bias force in the region force
	// This bias acts as a balloon force if inner and outer descriptors are close to each other
	// See section 5.5 in [Mille09]
	//bool bRegionEnergyBias;

	// Region and band areas
	float fInnerRegionArea, fOuterRegionArea;
	float fInnerNarrowBandArea, fOuterNarrowBandArea;
	float fInnerInitialRegionArea;

	// Display parameters used in DrawInImageRGB()
	bool bDisplayEdges, bDisplayVertices, bDisplayParallelCurves;
	CTriplet<unsigned char> rgbEdgeColor, rgbVertexColor, rgbEdgeParallelColor;
	int iVertexWidth;

    // Threshold on the maximum total motion of vertices:
    // When the stopping criterion is enabled, the evolution will be stopped if
    // the maximal displacement over all vertices falls below this value times the number of passes per iteration
    // Useful only when bStoppingCriterion is true
    // Beware, this is very sensitive! Actually did not find a value suitable for all experiments, as the
    // amount of motion depends on the absolute order of magnitude of energy weights
    // Maybe someting better in future versions...
    float fStoppingCriterionMotionThreshold;

  // Member functions
  protected:
	// Compute average intensities/colors in the inner and outer regions
	// Pure virtual function: overridden in CActiveContourGrayscale and CActiveContourColor
	virtual void ComputeRegionMeansGreenTheorem()=0;

	// Compute average intensities in the inner and outer narrow bands
	// Pure virtual function: overridden in CActiveContourGrayscale and CActiveContourColor
	virtual void ComputeBandMeans()=0;

	// Compute for each voxel the sum of intensities in X and Y dimensions
	// Pure virtual function: overridden in CActiveContourGrayscale and CActiveContourColor
	virtual void ComputeImageIntegrals()=0;

	// Performed when all vertices have been moved and resampled
	// Called by EnergyGradientDescent() after each evolution step
	// Updates global properties of the contour, like inner and outer descriptors
	virtual void UpdateAfterEvolution();

	// Performed when contour has been initialized (as a circle, ellipse, ...)
	// Updates global properties of the contour, like inner and outer descriptors
	// Pure virtual function: overridden in CActiveContourGrayscale and CActiveContourColor
	virtual void UpdateAfterInitialization()=0;

	// Return the region force at current vertex
	// This force is derived from the discretization of the variational derivative of the region term
	// Denoted f_region and introduced in section 5.3 in [Mille09] (see especially equations (53) and (54))
	// Depends on the selected region descriptors (iInsideMode and iOutsideMode)
	// Pure virtual function: overridden in CActiveContourGrayscale and CActiveContourColor
	// Params: pointers to previous, current and next vertex, respectively
	virtual CCouple<float> RegionForce(const CVertex2D *, const CVertex2D *, const CVertex2D *) const=0;

  public:
	// Default constructor
	CActiveContourBase();

	// Destructor
	virtual ~CActiveContourBase() {}

	// Attach the active contour model to the image
	// Precompute every image data useful for evolving the contour :
	// - array of intensities/colors stored as real numbers
	// - vector field derived from gradient magnitude (if gradient force is used)
	// - ...
	// Should be called before initialization (InitCircle(), InitEllipse(), ...)
	// Pure virtual function: overridden in CActiveContourGrayscale and CActiveContourColor
	virtual void AttachImage(const cv::Mat *)=0;

	// Resample the curve by merging vertices or adding vertices at the middle of edges
	// in order to maintain the length of every edge between fSampling and 2*fSampling.
	virtual void Resample();

	// Perform one iteration of evolution method, i.e. update vertices with forces
	// deriving from discretization of gradient descent of energy functional
	// Several passes of discrete evolution equation (53) in [Mille09] are applied to all vertices
	// Params: number of passes
	// Return value: TRUE if the active contour was actually evolved and may be evolved further,
	//               FALSE otherwise (e.g. stopping criterion was met or any other problem was encountered)
	virtual bool EnergyGradientDescent(int);

	// Curve smoothing with Gaussian kernel
	// Each vertex is moved with the Gaussian smoothing force of equation (55) in [Mille09]
	// To avoid shrinkage, we apply the two-pass method described in
	// G. Taubin, "Curve smoothing without shrinkage", ICCV, 1995
	virtual void GaussianSmooth();

	// Update unit inward normal vectors once vertices have been moved or initialized
	virtual void ComputeNormals();

	// Return position of approximated centroid of curve (just the average of vertices)
	virtual CCouple<float> GetCentroid() const;

	// Return length of curve by summing lengths of edges between vertices
	virtual float GetLength() const;

	// Initialize the contour as a circle
	// Params: center, radius
	virtual void InitCircle(const CCouple<float> &, float);

	// Initialize the contour as an ellipse
	// Params: center, x-radius, y-radius
	virtual void InitEllipse(const CCouple<float> &, float, float);

	// Translate the contour
	// Params: translation vector
	void Translate(const CCouple<float> &);

	// Rotate the contour. The centroid is taken as the center of rotation
	// Params: angle in radians
	void Rotate(float);

	// Draw the contour in a RGB image with a given zooming factor (vertices coordinates are multiplied
	// by this factor before drawing).
	// Params: RGB image, integer zooming factor
	virtual void DrawInImageRGB(cv::Mat &, int) const;

    // Make a binary image of the contour
    // Should be typically called at the end of the evolution
    // The image is cleared and reallocated if necessary. Pixels are set to 255 if located on contour, 0 otherwise
	// Params: output 8-bit image
	virtual void MakeBinaryMask(cv::Mat &) const;

	// Empty the active contour by deleting all vertices in the list
	virtual void Empty();
};



// Class CActiveContourGrayscale, derived from CActiveContourBase
// Implements the active contour model for grayscale images
class CActiveContourGrayscale : public CActiveContourBase
{
  // Members
  public:
	// Image data stored with real values, normalized between 0 and 1
	CArray2D<float> arrayImage;

	// Image integrals in X and Y dimensions
	// Used to approximate average intensity in the inner region using a
	// discrete implementation of Green-Riemann theorem
	CArray2D<float> arrayImageIntegralX, arrayImageIntegralY;

	// Sum of intensities in the entire image
	// Used to compute average intensity outside the contour after that
	// the sum of intensities inside it has been approximated
	float fImageSum;

	// Average intensities in inner and outer regions (inside and outside the contour)
	float fInnerMean, fOuterMean;

	// Average intensity in the initial inner region
	float fInnerInitialMean;

	// Average intensities in inner and outer narrow bands
	float fInnerNarrowBandMean, fOuterNarrowBandMean;

  // Member functions
  protected:
	// Computes average intensities in the inner and outer regions, i.e. inside
	// and outside the contour, using a discrete implementation of Green-Riemann theorem.
	// See section 5.6 in [Mille09]
	// Updates members fInnerRegionArea, fOuterRegionArea, fInnerMean and fOuterMean
	virtual void ComputeRegionMeansGreenTheorem();

	// Computes average intensities in the inner and outer narrow bands.
	// See section 5.3 and especially equation (51) in [Mille09]
	// Updates members fInnerNarrowBandArea, fOuterNarrowBandArea, fInnerNarrowBandMean
	// and fOuterNarrowBandMean
	virtual void ComputeBandMeans();

	// Computes for each voxel the sum of intensities in X and Y dimensions
	// Called in AttachImage() if precomputation of image integrals is enabled
	// Initializes member arrays arrayImageIntegralX and arrayImageIntegralY
	virtual void ComputeImageIntegrals();

	// Performed when contour has been initialized (as a circle, square, ...)
	// Updates global properties of the contour, like inner and outer descriptors
	virtual void UpdateAfterInitialization();

	// Return the region force at current vertex
	// This force is derived from the discretization of the variational derivative of the region term
	// Denoted f_region and introduced in section 5.3 in [Mille09] (see especially equations (53) and (54))
	// Depends on the selected region descriptors (iInsideMode and iOutsideMode)
	// Params: pointers to previous, current and next vertex, respectively
	virtual CCouple<float> RegionForce(const CVertex2D *, const CVertex2D *, const CVertex2D *) const;

  public:
	// Default constructor
	CActiveContourGrayscale();

	// Destructor
	virtual ~CActiveContourGrayscale() {}

	// Attach the active contour model to the image
	// Precompute every image data useful for evolving the contour :
	// - array of intensities as real numbers
	// - vector field derived from gradient magnitude (if gradient force is used)
	// - image integrals (arrays of cumulative intensities in x and y directions)
	// Should be called before initialization (InitCircle(), InitEllipse(), ...)
	virtual void AttachImage(const cv::Mat *);
};


// Class CActiveContourColor, derived from CActiveContourBase
// Implements the active contour model for color images
class CActiveContourColor : public CActiveContourBase
{
  // Members
  public:
	// Image data stored in chosen color space, with real values
	CArray2D<CTriplet<float> > arrayImage;

	// Color space
	typeColorSpace iColorSpace;

	// Ignore brightness component so that segmentation tends to be invariant to illumination
	// Used only for color spaces separating light and color components (YUV, LAB, ...)
	bool bIgnoreBrightnessComponent;

	// Image integrals in X and Y dimensions
	// Used to approximate average color in the inner region using a
	// discrete implementation of Green-Riemann theorem
	CArray2D<CTriplet<float> > arrayImageIntegralX, arrayImageIntegralY;

	// Sum of colors in the entire image
	// Used to compute average color outside the contour after that
	// the sum of colors inside it has been approximated
	CTriplet<float> fcolImageSum;

	// Average colors in inner and outer regions (inside and outside the contour)
	CTriplet<float> fcolInnerMean, fcolOuterMean;

	// Average color in the initial inner region
	CTriplet<float> fcolInnerInitialMean;

	// Average colors in inner and outer narrow bands
	CTriplet<float> fcolInnerNarrowBandMean, fcolOuterNarrowBandMean;

  // Member functions
  protected:
   // Computes average colors in the inner and outer regions, i.e. inside
	// and outside the contour, using a discrete implementation of Green-Riemann theorem.
	// See section 5.6 in [Mille09] for the grayscale equivalent
	// Updates members fInnerRegionArea, fOuterRegionArea, fcolInnerMean and fcolOuterMean
	virtual void ComputeRegionMeansGreenTheorem();

	// Computes average colors in the inner and outer narrow bands.
	// See section 5.3 and especially equation (51) in [Mille09] for the grayscale equivalent
	// Updates members fInnerNarrowBandArea, fOuterNarrowBandArea, fcolInnerNarrowBandMean
	// and fcolOuterNarrowBandMean
	virtual void ComputeBandMeans();

	// Computes for each voxel the sum of colors in X and Y dimensions
	// Called in AttachImage() if precomputation of image integrals is enabled
	// Initializes member arrays arrayImageIntegralX and arrayImageIntegralY
	virtual void ComputeImageIntegrals();

	// Performed when contour has been initialized (as a circle, ellipse, ...)
	// Updates global properties of the contour, like inner and outer descriptors
	virtual void UpdateAfterInitialization();

	// Return the region force at current vertex
	// This force is derived from the discretization of the variational derivative of the region term
	// Denoted f_region and introduced in section 5.3 in [Mille09] (see especially equations (53) and (54))
	// Depends on the selected region descriptors (iInsideMode and iOutsideMode)
	// Params: pointers to previous, current and next vertex, respectively
	virtual CCouple<float> RegionForce(const CVertex2D *, const CVertex2D *, const CVertex2D *) const;

  public:
	// Default constructor
	CActiveContourColor();

	// Destructor
	virtual ~CActiveContourColor() {}

	// Attach the active contour model to the image
	// Precompute every image data useful for evolving the contour :
	// - array of colors as real numbers (expressed in chosen color space)
	// - vector field derived from gradient magnitude (if gradient force is used)
	// - image integrals (arrays cumulative colors in x and y directions)
	// Should be called before initialization (InitCircle(), InitEllipse(), ...)
	virtual void AttachImage(const cv::Mat *);
};

#endif
