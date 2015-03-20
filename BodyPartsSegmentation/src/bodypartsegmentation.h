#ifndef BODYPARTSEGMENTATION_H
#define BODYPARTSEGMENTATION_H

#include "RandomForest.h"

#ifdef D_BUILDWINDLL
	#define DLL_EXPORT __declspec(dllexport)
#else
	#define DLL_EXPORT
#endif

/**
 * Mingyuan body part segmentation using depth information.
 */
class DLL_EXPORT BodyPartSegmentation
{
public:
	/*
	 * Constructor.
	 * @param  forestParamFileName  Forest configuration file name.
	 * @param  groundImgFileName  Ground truth (legend) file name.
	 * @param _rawDepthInMillimeters  If true, depth values are 
	 *        supposed to be in millimeters (MS Kinect SDK case), 
	 *        if false depth values are supposed to be coded 
	 *        in range [0,2047] (libfreenect case).
	 */
	BodyPartSegmentation( const char *forestParamFileName, const char *groundImgFileName, bool _rawDepthInMillimeters = false);
	virtual ~BodyPartSegmentation();

	// run each time this image processiong is selected
	void init(void);

	// run for each frame
	void run(const cv::Mat& depthImg, bool bLegend, cv::Mat& outputImg);

	/**
	 * Extract the data of human in the mask area and also
	 * Normalise the the human distance data into 4m
	 * @param src the smooth depth mat  
	 * @param mask the person mask
	 * @param clean the data of human area
	 */
	void ExtractDepthHuman( CvMat* src, CvMat* mask, CvMat* dst ); 

	/**
	 * Converts raw depth value to distance in meters.
	 * Raw depth is a coded value in [0,2047] as given by libfreenect.
	 * @param  raw_depth  raw depth value.
	 * @return  distance in meters.
	 */
	float raw_depth_to_meters(int raw_depth);  

	/**
	 * Converts a distance in meters to a raw depth value
	 * as given by libfreenect.
	 * @param  depthInMeters  depth in meters.
	 * @return  raw depth value in [0,2047].
	 */
	int meters_to_raw_depth(float depthInMeters);

	/**
	 * segment the data from the human using random forest 
	 * @param forest the loaded forest
	 * @param seg_depth the segmented depth data
	 * @param seg_edge the segmented edge data
	 * return the colored image only contains the human area  
	 */
	IplImage* SegmentParts( CvMat* seg_depth, CvMat* seg_edge=NULL ); 

	/**
	 * Segment the forground person from the depth image using Fisher's method
	 * @param mask the person's mask (1 is human and 0 is others)
	 * @param pro_mat the smooth depth mat
	*/
	void computePersonMask(const cv::Mat& depthImg, CvMat* mask, CvMat* pro_mat);


	RandomForest* forest; 
	IplImage *ground; // body parts legend
	bool depthIsInMillimeters; // input depth image in millimeters

protected:

};

#endif // BODYPARTSEGMENTATION_H

