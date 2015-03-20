
#ifndef PIXELTRACKER_H
#define PIXELTRACKER_H

#ifdef D_BUILDWINDLL
	#define DLL_EXPORT __declspec(dllexport)
#else
	#define DLL_EXPORT
#endif

#include <opencv2/opencv.hpp>

#include "Image.h"

namespace TLInOut
{
	class OutputXMLFile;
	class OutputTXTFile;
}

namespace TLImageProc
{
	class Rectangle;
	class BGR2HSVhistLUT;
}

class HSVPixelGradientModel;
class PixelClassColourModel;

using namespace TLImageProc;
using namespace TLInOut;

/*
 * SD pixel tracker
 */
class DLL_EXPORT PixelTracker
{
	public:
		/*
		 * Constructor.
		 * @param _bbox_x  bounding box initial x position 
		 * @param _bbox_y  bounding box initial y position 
		 * @param _bbox_w  bounding box initial width
		 * @param _bbox_h  bounding box initial height
		 * @param _detector_update_factor  update factor (gamma) for the detection model
		 * @param _segmentation_update_factor  update factor (delta) for the segmentation model
		 * @param _search_size  relative enlargement factor of the search window w.r.t. the current object bounding box
		 */
		PixelTracker( int _bbox_x, int _bbox_y, int _bbox_width, int _bbox_height, float _detector_update_factor, float _segmentation_update_factor, float _search_size);

		/*
		 * Destructor.
		 */
		~PixelTracker();

		/*
		 * Track object in image.
		 * @param img  image to process.
		 */
		void process(const cv::Mat *img);

		/*
		 * Track object in image.
		 * @param image  image to process,
		 * @param frameId  frame id if given by caller,
		 * @param time1  frame time, if given by caller (used in output.xml file),
		 * @param time2  frame time, if given by caller.
		 */
		void process(TLImageProc::Image<unsigned char> *image, int frameId = -1, int time1 = 0, int time2 = 0);

		/*
		 * Get last processing result.
		 * @return  bounding box around tracked object.
		 */
		TLImageProc::Rectangle *getCurBb(void)  { return cur_bb; }

		/*
		 * Access to usefull informations for debugging.
		 */

		int getMaxx(void)  { return maxx; }
		int getMaxy(void)  { return maxy; }
		int getCmMaxx(void)  { return cm_maxx; }
		int getCmMaxy(void)  { return cm_maxy; }

		Image<float>* getBpImgNormalised(void)
		{
			bp_img->multiply(1, bp_img_normalised);
			return bp_img_normalised;
		}

		Image<float>* getVotingMapNormalised(void)
		{
			voting_map->multiply(0.01, voting_map_normalised);
			return voting_map_normalised;
		}

		Image<float>* getSegmentation(void)  { return segmentation; }

	protected:
		/*
		 * First image processing.
		 * Do some initializations.
		 * @param  see process(...) function.
		 */
		void processFirstImage(TLImageProc::Image<unsigned char> *cur_image, int frameId, int time1, int time2);

		/*
		 * Class members.
		 */

		float detector_update_factor;
		float segmentation_update_factor;
		float search_size;

		bool firstImage; // first image flag

		OutputXMLFile *xmlout;
		OutputTXTFile *txtout;

		TLImageProc::Rectangle *initial_rect; // initial search box

		int current_frame; // images counter
		int width; // images width
		int height; // images height
  
		int lut_nscales;
		HSVPixelGradientModel* model;
		TLImageProc::Rectangle *cur_bb, *search_window;
		TLImageProc::Rectangle *outer_bb;
		int maxx, maxy;
		int cm_maxx, cm_maxy;
		float uncertainty;
		float cur_seg_change;
		int prev_shift_x, prev_shift_y;
		TLImageProc::Rectangle *prev_bb;

		Image<unsigned char> *grey_img;
		Image<short> *xgrad_img;
		Image<short> *ygrad_img;
		Image<float> *segmentation;
		Image<float> *seg_prior;
		Image<float> *voting_map;
		Image<float> *voting_map_normalised;
		Image<float> *bp_img;
		Image<float> *bp_img_normalised;
		BGR2HSVhistLUT **lut;
		PixelClassColourModel *pccm;
		int erosion_type;
		int erosion_w;
		int erosion_h;
};

#endif // PIXELTRACKER_H
