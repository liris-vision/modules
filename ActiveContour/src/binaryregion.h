/*
	binaryregion.h

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

	The current header file corresponds to a binary version of the level set active
	contour model. See sections 2 and 6 in [Mille09] for the continuous variational
	model and its implicit implementation.
*/

#ifndef _BINARYREGION_H
#define _BINARYREGION_H

#include "arrayndfloat.h"
#include "deformablemodel.h"


// Class CBinaryRegionPixel
// Represents a single pixel in the binary region
class CBinaryRegionPixel
{
  // Members
  private:
	// Status byte
	// All relevant information about a binary region pixel holds in a single byte
	// The first three bits indicate domains which the pixel belongs to
	// Bit 0: region
	// Bit 1: boundary
	// Bit 2: narrow band
	// Whether the pixel belongs to the inner or outer boundary (similarly, the inner or outer narrow band)
	// is indicated by a combination of these bits
	// The evolution of the binary region involves several neighborhoods: the 4-connexity neighborhood
	// to determine boundary pixels or candidate pixels for growing, the smoothing neighborhood which size
	// depends on the standard deviation of the gaussian filter, the band neighborhood which size
	// depends on the band thickness, and the distance neighborhood for approximating the signed euclidean
	// distance to the boundary.
	// When the region boundary gets close to image borders, care should be
	// taken when scanning the different neighborhoods, so that resulting neighbors do not fall outside
	// the image domain. Instead of testing coordinates inside scanning loops, safe areas are used. A pixel
	// being in a safe area with respect to a given neighborhood can have its neighbors scanned without checking
	// if their coordinates are within the image domain.
	// Four bits in the status byte indicate whether the pixel belongs to safe areas or not.
	// Bit 3: grow-safe area
	// Bit 4: smoothing-safe area
	// Bit 5: band-safe area
	// Bit 6: distance-safe area
	// Testing these bits is faster than checking pixel coordinates in loops
	// The last bit indicates whether the pixel is marked for smoothing after evolution
	unsigned char status;

  // Member functions
  public:
	inline CBinaryRegionPixel() {status = 0;}

	inline void AddIntoRegion() {status |= 0x01;}        // Set bit 0
	inline void RemoveFromRegion() {status &= 0xFE;}     // Erase bit 0
	inline bool IsInRegion() const {return (status&0x01)!=0;}  // Test bit 0

	inline void AddIntoBoundary() {status |= 0x02;}        // Set bit 1
	inline void RemoveFromBoundary() {status &= 0xFD;}     // Erase bit 1
	inline bool IsInBoundary() const {return (status&0x02)!=0;}  // Test bit 1

	inline bool IsInInnerBoundary() const {return (status&0x03)==0x03;}  // Bits 0 and 1 should be set
	inline bool IsInOuterBoundary() const {return (status&0x03)==0x02;}  // Bit 0 should be cleared and bit 1 should be set

	inline void AddIntoNarrowBand() {status |= 0x04;}        // Set bit 2
	inline void RemoveFromNarrowBand() {status &= 0xFB;}     // Erase bit 2
	inline bool IsInNarrowBand() const {return (status&0x04)!=0;}  // Test bit 2

	inline bool IsInInnerNarrowBand() const {return (status&0x05)==0x05;}  // Bits 0 and 2 should be set
	inline bool IsInOuterNarrowBand() const {return (status&0x05)==0x04;}  // Bit 0 should be cleared and bit 2 should be set

	inline void AddIntoGrowSafeArea() {status |= 0x08;}        // Set bit 3
	inline void RemoveFromGrowSafeArea() {status &= 0xF7;}     // Erase bit 3
	inline bool IsInGrowSafeArea() const {return (status&0x08)!=0;}  // Test bit 3

	inline void AddIntoSmoothingSafeArea() {status |= 0x10;}        // Set bit 4
	inline void RemoveFromSmoothingSafeArea() {status &= 0xEF;}     // Erase bit 4
	inline bool IsInSmoothingSafeArea() const {return (status&0x10)!=0;}  // Test bit 4

	inline void AddIntoBandSafeArea() {status |= 0x20;}        // Set bit 5
	inline void RemoveFromBandSafeArea() {status &= 0xDF;}     // Erase bit 5
	inline bool IsInBandSafeArea() const {return (status&0x20)!=0;}  // Test bit 5

	inline void AddIntoDistanceSafeArea() {status |= 0x40;}        // Set bit 6
	inline void RemoveFromDistanceSafeArea() {status &= 0xBF;}     // Erase bit 6
	inline bool IsInDistanceSafeArea() const {return (status&0x40)!=0;}  // Test bit 6

	inline void MarkForSmoothing() {status |= 0x80;}               // Set bit 7
	inline void UnmarkForSmoothing() {status &= 0x7F;}             // Erase bit 7
	inline bool IsMarkedForSmoothing() const {return (status&0x80)!=0;}  // Test bit 7

	// Reset all data flags on current pixel, except region information
	// Clear all bits except bit 0
	inline void ClearStatusByte() {status &= 0x01;}
};



// Abstract base class CBinaryRegionBase
// Contains pure virtual member functions and thus cannot be instantiated
class CBinaryRegionBase : public CArray2D<CBinaryRegionPixel>, public CDeformableModelBase
{
  // Static members
  private:
	// The following sets of neighbors are declared as static members, since they
	// are independent from the current class instance

	// 4-connexity neighborhood
	static CArray1D<CCouple<int> > vectNeighborhood;

	// Neighorhood for signed euclidean distance
	static const int iNeighborhoodDistanceHalfWidth;
	static CArray1D<CCouple<int> > vectNeighborhoodDistance;
	static CArray1D<float> vectNeighborhoodWeightsDistance;

  // Members
  private:
	// Set of offsets with respect to 4-connexity neighborhood
	CArray1D<int> vectNeighborhoodOffsets;

	// Set of offsets with respect to distance neighborhood
	CArray1D<int> vectNeighborhoodOffsetsDistance;

	// Band thickness neighorhood and corresponding set of offsets
	//  of ball of radius iBandThickness
	CArray1D<CCouple<int> > vectNeighborhoodBand;
	CArray1D<int> vectNeighborhoodOffsetsBand;

	// Smoothing neighorhood and corresponding set of offsets and weights
	CArray1D<CCouple<int> > vectNeighborhoodSmoothing;
	CArray1D<int> vectNeighborhoodOffsetsSmoothing;
	CArray1D<float> vectNeighborhoodWeightsSmoothing;

  public:
	// Region area: number of pixels in region
	int iNbPixels;

	// Region bounds: top-left and bottom-right coordinates of bounding rectangle of region
	CCouple<int> piRegionMin, piRegionMax;

	// Image gradient magnitude
	// Used to compute gradient energy if fWeightGradient is non-zero
	CArray2D<float> arrayGradientNorm;

	// Distance map
	// Stores the signed euclidean distance to the inner boundary (negative inside, positive outside)
	// Used to approximate normal vectors and curvatures for the local outer data term
	// (when iOutsideMode==OUTSIDEMODE_LINE)
	// Distance values are necessary only in the vicinity of the inner boundary and is thus
	// computed in a 2-pixel wide narrow band both sides apart from the inner boundary
	CArray2D<float> arraySignedDistance;

     // Previous states of the region, to compute the stopping criterion in the evolution method
    // Several previous regions need to be stored so that the detection of convergence is robust
    // against the possible oscillations that may arise around the local minimum
    list<cv::Mat> listPreviousRegions;

    // The maximum amount of previous regions to be stored.
    // The appropriate value depends on the number of passes per iteration that are performed in the evolution method
    // Typically, this should be at least 3 for 1 pass per iteration.
    // It seems that it should go up to 20 for 5 passes per iteration (these were found experimentally !)
    unsigned int iNbPreviousRegionsMax;

	// Display parameters used in DrawInImageRGB()
	float fDisplayOpacity;     // Opacity for displaying
	bool bDisplayRegionPixels;  // Display region pixels with variable opacity
	bool bDisplayBand;          // Display narrow band ?
	bool bDisplayBoundary;      // Display boundary pixels ?
	bool bDisplayNormals;       // Display normal vectors ?
	CTriplet<unsigned char> rgbRegionColor, rgbBandColor;

  // Static member functions
  public:
	static void InitNeighborhood();

  // Member functions
  protected:
	// Add a single pixel into the region
	// Pure virtual function: overridden in CBinaryRegionGrayscale and CBinaryRegionColor
	virtual void AddPixel(const CCouple<int> &)=0;

	// Remove a single pixel from the region
	// Pure virtual function: overridden in CBinaryRegionGrayscale and CBinaryRegionColor
	virtual void RemovePixel(const CCouple<int> &)=0;

	// Compute narrow band means and standards deviations from histograms on 2D slice
	// Pure virtual function: overridden in CBinaryRegionGrayscale and CBinaryRegionColor
	virtual void ComputeBandMeans()=0;

	// Performed when all points have been moved
	// Called by EnergyGradientDescent() after each evolution step
	// Updates global properties of the region, like inner and outer descriptors
	// Pure virtual function: overridden in CBinaryRegionGrayscale and CBinaryRegionColor
	virtual void UpdateAfterEvolution();

	// Performed when region has been initialized (as a circle, ellipse, ...)
	// Updates global properties of the region, like inner and outer descriptors
	// Pure virtual function: overridden in CBinaryRegionGrayscale and CBinaryRegionColor
	virtual void UpdateAfterInitialization();

	// Check if the pixel at the given address is on the inner boundary
	// (belongs to the region and has at least neighbor which does not)
	inline bool IsInnerBoundaryPixel(const CBinaryRegionPixel *pPixel) const {return pPixel->IsInInnerBoundary();}

	// Check if the pixel at the given address is on the outer boundary
	// (does not belong to the region and has at least a neighbor which does)
	inline bool IsOuterBoundaryPixel(const CBinaryRegionPixel *pPixel) const {return pPixel->IsInOuterBoundary();}

	// Check if the pixel is candidate to be added to or removed from the region.
	// If the returned value is true, the speed is computed at the pixel
	// Arguments : point, point address
	inline bool IsCandidatePixel(const CBinaryRegionPixel *pPixel) const {return pPixel->IsInBoundary();}

	// Update flags indicating safe areas pixels belong to
	// Called once in AttachImage()
	virtual void UpdateSafeAreas();

	// Update flags indicating whether pixels belong to the inner narrow band,
	// outer narrow band or none of both
	// Called after region initialization and after each evolution iteration
	virtual void UpdateNarrowBand();

	// Update flags indicating whether pixels belong to the inner boundary,
	// outer boundary or none of both
	// Called after region initialization and after each evolution iteration
	virtual void UpdateBoundary();

	// Update the approximation of the signed euclidean distance to the boundary (if needed)
	// Negative inside the region and positive outside
	// Called after region initialization and after each evolution iteration
	virtual void UpdateSignedDistance();

	// Return the region speed at current pixel
	// This is the speed counterpart of the region force in the parametric active contour model
	// Denoted F_region and introduced in section 6 in [Mille09]
	// Depends on the selected region descriptors (iInsideMode and iOutsideMode)
	// Pure virtual function: overridden in CBinaryRegionGrayscale and CBinaryRegionColor
	// Params: current pixel
	virtual float RegionSpeed(const CCouple<int> &) const=0;

  public:
	// Default constructor
	CBinaryRegionBase();

	// Destructor
	virtual ~CBinaryRegionBase() {}

	// Attach the binary region model to the image
	// Precompute every image data useful for evolving the region
	// Should be called before initialization (InitCircle(), InitEllipse(), ...)
	// Pure virtual function: overridden in CBinaryRegionGrayscale and CBinaryRegionColor
	virtual void AttachImage(const cv::Mat *)=0;

	// Initialize vector of offsets in the pixel array, corresponding to the relative displacements
	// of the 4-connexity neighborhood
	virtual void InitNeighborhoodOffsets();

	// Initialize relative neighborhoods for smoothing and finding pixels in the narrow bands.
	// Neighborhoods are stored as vectors of relative displacements and corresponding offsets
	// in the pixel array. The size of smoothing neighborhood depends on the standard deviation
	// of the gaussian filter, where as the size of the band neighborhood depends on the band thickness
	virtual void InitNeighborhoodsSmoothingBand();

	// Perform one iteration of evolution method, i.e. add or remove boundary pixels to/from the
	// region according to their speed function
	// Several passes of evolution equation (57) in [Mille09] are applied on the 1-pixel large
	// narrow band both sides apart from the contour (i.e. on inner and outer boundary pixels)
	// Params: number of passes
	// When the stopping criterion is enabled, the number of passes should not exceed a certain threshold
	// so that stability can be properly detected. Typically, keep it <= 5.
	// Return value: TRUE if the binary region was actually evolved and may be evolved further,
	//               FALSE otherwise (e.g. stopping criterion was met or any other problem was encountered)
	virtual bool EnergyGradientDescent(int);

	// Gaussian smoothing
	virtual void GaussianSmooth();

	// Gaussian smoothing
	// Only affects marked pixels
	virtual void GaussianSmoothMarked();

	// Return approximate inward unit normal vector and curvature at given boundary pixel
	// The approximation is based on the gradient of the signed euclidean distance to the boundary
	// Use the distance map array arraySignedDistance, which should be allocated and up-to-date
	// Param: pixel (input), normal vector (output), curvature (output)
	virtual void GetNormalAndCurvature(const CCouple<int> &, CCouple<float> &, float &) const;

	// Initialize the region as a filled circle
	// Params: center, radius
	virtual void InitCircle(const CCouple<float> &, float);

	// Initialize the region as a filled ellipse
	// Params: center, x-radius, y-radius
	virtual void InitEllipse(const CCouple<float> &, float, float);

	virtual void Empty();

	// Draw the region in a RGB image with a given zooming factor (vertices coordinates are multiplied
	// by this factor before drawing).
	// Params: RGB image, integer zooming factor
	virtual void DrawInImageRGB(cv::Mat &, int) const;

	// Make a binary image of segmentation
    // Should be typically called at the end of the evolution
    // The image is cleared and reallocated if necessary. Pixels are set to 255 if inside region, 0 otherwise
	// Params: output 8-bit image
	virtual void MakeBinaryMask(cv::Mat &) const;
};



// Class CBinaryRegionGrayscale, derived from CBinaryRegionBase
// Implements the implicit region model for grayscale images
class CBinaryRegionGrayscale : public CBinaryRegionBase
{
  // Members
  public:
    // Image data stored with real values, normalized between 0 and 1
	CArray2D<float> arrayImage;

	// Sum of intensities in inner and outer regions
	float fInnerSum, fOuterSum;

	// Average intensities in inner and outer regions
	float fInnerMean, fOuterMean;

	// Average intensity in the initial inner region
	float fInnerInitialMean;

	// Average intensities in inner and outer narrow bands
	float fInnerNarrowBandMean, fOuterNarrowBandMean;

  // Member functions
  protected:
	// Add a single pixel to region
	// Params: pixel
	virtual void AddPixel(const CCouple<int> &);

	// Remove a single pixel from region
	// Params: pixel
	virtual void RemovePixel(const CCouple<int> &);

	// Computes average intensities in the inner and outer narrow bands.
	// See section 5.3 and especially equation (51) in [Mille09]
	// Updates members fInnerNarrowBandArea, fOuterNarrowBandArea, fInnerNarrowBandMean
	// and fOuterNarrowBandMean
	virtual void ComputeBandMeans();

	// Performed when all points have been moved
	// Called by EnergyGradientDescent() after each evolution step
	// Updates global properties of the region, like inner and outer descriptors
	virtual void UpdateAfterEvolution();

	// Performed when region has been initialized (as a circle, ellipse, ...)
	// Updates global properties of the region, like inner and outer descriptors
	virtual void UpdateAfterInitialization();

	// Return the region speed at current point
	// This speed is derived as the level set counterpart of the region force f_region
	// Denoted F_region and introduced in section 6 in [Mille09] (see especially equations (53) and (54)
	// for the corresponding force in the parametric active contour)
	// Depends on the selected region descriptors (iInsideMode and iOutsideMode)
	// Params: current point
	virtual float RegionSpeed(const CCouple<int> &) const;

  public:
	// Default constructor
	CBinaryRegionGrayscale();

	// Destructor
	virtual ~CBinaryRegionGrayscale() {}

	// Attach the active contour model to the image
	// Precompute every image data useful for evolving the region:
	// - array of colors as real numbers (expressed in chosen color space)
	// - array of gradient magnitudes (if gradient data is used)
	// Allocate the distance map array if needed
	// Should be called before initialization (InitCircle(), InitEllipse(), ...)
	virtual void AttachImage(const cv::Mat *);

	virtual void Empty();
};



// Class CBinaryRegionColor, derived from CBinaryRegionBase
// Implements the implicit region model for color images
class CBinaryRegionColor : public CBinaryRegionBase
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

	// Sum of colors in inner and outer regions
	CTriplet<float> fcolInnerSum, fcolOuterSum;

	// Average colors in inner and outer regions
	CTriplet<float> fcolInnerMean, fcolOuterMean;

	// Average color in the initial inner region
	CTriplet<float> fcolInnerInitialMean;

	// Average colors in inner and outer narrow bands
	CTriplet<float> fcolInnerNarrowBandMean, fcolOuterNarrowBandMean;

  // Member functions
  protected:
	// Add a single pixel to region
	// Params: pixel
	virtual void AddPixel(const CCouple<int> &);

	// Remove a single pixel from region
	// Params: pixel
	virtual void RemovePixel(const CCouple<int> &);

	// Computes average colors in the inner and outer narrow bands.
	// See section 5.3 and especially equation (51) in [Mille09] for the grayscale equivalent
	// Updates members fInnerNarrowBandArea, fOuterNarrowBandArea, fInnerNarrowBandMean
	// and fOuterNarrowBandMean
	virtual void ComputeBandMeans();

	// Performed when all points have been moved
	// Called by EnergyGradientDescent() after each evolution step
	// Updates global properties of the region, like inner and outer descriptors
	virtual void UpdateAfterEvolution();

	// Performed when region has been initialized (as a circle, ellipse, ...)
	// Updates global properties of the region, like inner and outer descriptors
	virtual void UpdateAfterInitialization();

	// Return the region speed at current point
	// This speed is derived as the level set counterpart of the region force f_region
	// Denoted F_region and introduced in section 6 in [Mille09] (see especially equations (53) and (54)
	// for the corresponding force in the parametric active contour)
	// Depends on the selected region descriptors (iInsideMode and iOutsideMode)
	// Params: current point
	virtual float RegionSpeed(const CCouple<int> &) const;

  public:
	// Default constructor
	CBinaryRegionColor();

	// Destructor
	virtual ~CBinaryRegionColor() {}

	// Attach the active contour model to the image
	// Precompute every image data useful for evolving the region:
	// - array of colors as real numbers (expressed in chosen color space)
	// - array of gradient magnitudes (if gradient data is used)
	// Allocate the distance map array if needed
	// Should be called before initialization (InitCircle(), InitEllipse(), ...)
	virtual void AttachImage(const cv::Mat *);

	virtual void Empty();
};

#endif
