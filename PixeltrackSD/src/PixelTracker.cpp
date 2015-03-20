
#include "PixelTracker.h"

#include "BGR2HSVhistLUT.h"
#include "OutputXMLFile.h"
#include "OutputTXTFile.h"
#include "HSVPixelGradientModel.h"
#include "PixelClassColourModel.h"

//---------------------------------------------------------

PixelTracker::PixelTracker( int _bbox_x, int _bbox_y, int _bbox_width, int _bbox_height, float _detector_update_factor, float _segmentation_update_factor, float _search_size)
{
	initial_rect = new Rectangle(_bbox_x, _bbox_y, _bbox_x + _bbox_width - 1, _bbox_y + _bbox_height - 1);
	detector_update_factor = _detector_update_factor;
	segmentation_update_factor = _segmentation_update_factor;
	search_size = _search_size;

	xmlout = new OutputXMLFile("output.xml");
	txtout = new OutputTXTFile("output.txt");
	model = new HSVPixelGradientModel(16, 16, 8, 1, 60); // best

	cur_bb = new Rectangle();
	search_window = new Rectangle();
	outer_bb = new Rectangle();
	prev_bb = new Rectangle();

	prev_shift_x = 0;
	prev_shift_y = 0;

	lut_nscales = 2;

	width = 0;
	height = 0;

	current_frame = 0;
	firstImage = true;
}

//---------------------------------------------------------

PixelTracker::~PixelTracker()
{
	delete seg_prior;
	delete segmentation;
	delete voting_map;
	delete voting_map_normalised;
	delete model;

	delete grey_img;
	delete xgrad_img;
	delete ygrad_img;
	delete bp_img;
	delete bp_img_normalised;

	delete pccm;

	for(int s = 0; s < lut_nscales; s++)
		delete lut[s];
	delete [] lut;

	delete xmlout;
	delete txtout;

	delete cur_bb;
	delete search_window;
	delete outer_bb;
	delete prev_bb;
	delete initial_rect;
}

//---------------------------------------------------------

void PixelTracker::processFirstImage(TLImageProc::Image<unsigned char> *cur_image, int frameId, int time1, int time2)
{
	width = cur_image->width();
	height = cur_image->height();

	grey_img = new Image<unsigned char>(width, height, 1);

	xgrad_img = new Image<short>(width, height, 1);
	ygrad_img = new Image<short>(width, height, 1);
	xgrad_img->setZero();
	ygrad_img->setZero();
	cur_image->toGreyScale(grey_img);
	grey_img->sobelX(xgrad_img);
	grey_img->sobelY(ygrad_img);

	segmentation = new Image<float>(width, height, 1);
	segmentation->setZero();
	voting_map = new Image<float>(width, height, 1);
	voting_map_normalised = new Image<float>(width, height, 1);
	voting_map->setZero();
	bp_img = new Image<float>(width, height, 1);
	bp_img_normalised = new Image<float>(width, height, 1);
	bp_img->setZero();

	*outer_bb = *initial_rect;
	outer_bb->enlarge(1.5);
	*cur_bb = *initial_rect;
	*search_window = *cur_bb;
	search_window->enlarge(search_size);
	maxx = cur_bb->centerX();
	maxy = cur_bb->centerY();

	// learn pixel colour model (FG / BG)
	int nbins=12;
	lut = new BGR2HSVhistLUT*[lut_nscales];
	for( int s = 0; s < lut_nscales; s++)
		lut[s] = new BGR2HSVhistLUT(nbins*(s+1), nbins*(s+1), nbins*(s+1));

	// create colour segmentation model
	pccm = new PixelClassColourModel(lut, nbins, nbins, nbins, lut_nscales, width, height);
	pccm->create(cur_image, outer_bb, initial_rect);
	// do a first segmentation
	pccm->evaluateColour(cur_image, outer_bb, false, segmentation);
	seg_prior = segmentation->clone();
	// set first tracking result to average of (outer) selected rectangle and centre of mass of segmentation
	segmentation->centreOfMass(*outer_bb, cm_maxx, cm_maxy);
	cur_bb->setCenter(0.5*(maxx+cm_maxx), 0.5*(maxy+cm_maxy));
	*prev_bb = *cur_bb;

	Rectangle larger, smaller;
	int nbfg1, nbbg1, nbfg2, nbbg2, nbfg3, nbbg3;
	float fgbg_r1, fgbg_r2, fgbg_r3;

	larger = *cur_bb;
	larger.enlarge(1.2);
	smaller = *cur_bb;
	smaller.enlarge(1.0/1.2);
	nbfg1 = segmentation->sumGreaterThanThreshold(larger, 0.5);
	nbfg2 = segmentation->sumGreaterThanThreshold(*cur_bb, 0.5);
	nbfg3 = segmentation->sumGreaterThanThreshold(smaller, 0.5);
	nbbg1 = larger.area()-nbfg1;
	nbbg2 = cur_bb->area()-nbfg2;
	nbbg3 = smaller.area()-nbfg3;
	fgbg_r1 = (float)nbfg1/nbbg1;
	fgbg_r2 = (float)nbfg2/nbbg2;
	fgbg_r3 = (float)nbfg3/nbbg3;

	if (fgbg_r1>5*fgbg_r2)
	{
		if (fgbg_r1>fgbg_r3)
			*cur_bb = larger;
		else
			*cur_bb = smaller;
	}
	else
	{
		if (fgbg_r2*5<fgbg_r3)
			*cur_bb = smaller;
	}
	*search_window = *cur_bb;
	search_window->enlarge(search_size);

	erosion_type = cv::MORPH_ELLIPSE;
	erosion_w = int(float(cur_bb->miWidth)/100+.5);
	erosion_h = int(float(cur_bb->miHeight)/100+.5);

	// learn pixel model
	model->learn(cur_image, xgrad_img, ygrad_img, *search_window, segmentation);
	model->backproject(cur_image, xgrad_img, ygrad_img, *search_window, bp_img, maxx, maxy);
	// do a first update to re-inforce pixels with "correct" backprojection
	pccm->update(cur_image, cur_bb, segmentation, bp_img, 0.1);
	model->update(cur_image, xgrad_img, ygrad_img, *search_window, bp_img, 0.2); 

	// output first tracking result to XML file
	xmlout->sendBB(cur_bb, 0, 0, 1.0);
	txtout->sendBB(cur_bb, 0, 0, 1.0);
	if( frameId != -1 )
	{
		xmlout->commit( frameId, time1, time2);
		txtout->commit( frameId, time1, time2);
	}
	else
	{
		xmlout->commit( current_frame, time1, time2);
		txtout->commit( current_frame, time1, time2);
	}
}

//---------------------------------------------------------

void PixelTracker::process( TLImageProc::Image<unsigned char> *cur_image, int frameId, int time1, int time2)
{
	if( firstImage )
	{
		processFirstImage( cur_image, frameId, time1, time2);
		firstImage = false;
		return;
	}

	current_frame++;

	voting_map->setZero();
	bp_img->setZero();
	segmentation->setZero();

	cur_image->toGreyScale(grey_img);
	grey_img->sobelX(xgrad_img);
	grey_img->sobelY(ygrad_img);

	// ***** do the Hough voting **********
	model->vote(cur_image, xgrad_img, ygrad_img, *search_window, voting_map);
	voting_map->maxLoc(*search_window, maxx, maxy);

	// segmentation
	pccm->evaluateColourWithPrior(cur_image, search_window, false, seg_prior, segmentation);
	cur_seg_change = segmentation->percentageChanged(*cur_bb, prev_shift_x, prev_shift_y, seg_prior);
	// set segmentation prior for next iteration
	delete seg_prior;
	seg_prior = segmentation->clone();

	uncertainty = max(0.2f, min(0.8f, cur_seg_change));
	//MESSAGE(0, "uncertainty: " << uncertainty);

	// opening
	cv::Mat element = cv::getStructuringElement( erosion_type, cv::Size( 2*erosion_w + 1, 2*erosion_h+1 ), cv::Point( erosion_w, erosion_h ) );
	cv::Mat mseg(segmentation->height(), segmentation->width(), CV_32F, segmentation->data(), segmentation->width()*sizeof(float));
	cv::erode( mseg, mseg, element );
	cv::dilate( mseg, mseg, element );

	// backprojection
	model->backproject(cur_image, xgrad_img, ygrad_img, *search_window, bp_img, maxx, maxy);

	segmentation->centreOfMass(*search_window, cm_maxx, cm_maxy);
	*prev_bb = *cur_bb;
	cur_bb->setCenter((1.0-uncertainty)*maxx+uncertainty*cm_maxx, (1.0-uncertainty)*maxy+uncertainty*cm_maxy);
	prev_shift_x = cur_bb->centerX() - prev_bb->centerX();
	prev_shift_y = cur_bb->centerY() - prev_bb->centerY();

	*search_window = *cur_bb;
	search_window->enlarge(search_size);

	pccm->update(cur_image, search_window, segmentation, bp_img, detector_update_factor);
	model->update(cur_image, xgrad_img, ygrad_img, *search_window, segmentation, segmentation_update_factor); 

	// output tracking result to XML/TXT file
	xmlout->sendBB(cur_bb, 0, 0, 1.0);
	txtout->sendBB(cur_bb, 0, 0, 1.0);
	if( frameId != -1 )
	{
		xmlout->commit( frameId, time1, time2);
		txtout->commit( frameId, time1, time2);
	}
	else
	{
		xmlout->commit( current_frame, time1, time2);
		txtout->commit( current_frame, time1, time2);
	}
}

//---------------------------------------------------------

void PixelTracker::process(const cv::Mat *img)
{
	if( img->type() != CV_8UC3  &&  img->type() != CV_8UC1 )
	{
		std::cout << "PixelTracker::process() error: bad image type (CV_8UC3 or CV_8UC1 required)." << std::endl; 	
		return;
	}
	
	Image<unsigned char> image( img->cols, img->rows, 3, img->step, img->data, true);
	process(&image);
}


