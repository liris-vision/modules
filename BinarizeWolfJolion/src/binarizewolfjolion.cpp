#include "binarizewolfjolion.h"

#define uget(x,y)    at<unsigned char>(y,x)
#define uset(x,y,v)  at<unsigned char>(y,x)=v;
#define fget(x,y)    at<float>(y,x)
#define fset(x,y,v)  at<float>(y,x)=v;

//---------------------------------------------------------

BinarizeWolfJolion::BinarizeWolfJolion( int _winx, int _winy, double _k, NiblackVersion _type)
{
	winx = _winx;
	winy = _winy;
	k = _k;
	version = _type;
	dR = 128;
}

//---------------------------------------------------------

BinarizeWolfJolion::~BinarizeWolfJolion()
{
}

//---------------------------------------------------------

void BinarizeWolfJolion::process( cv::Mat *input1, cv::Mat *output1)
{
	// Prepare input and output, and convert to grayscale on the fly
	cv::Mat im;
	cvtColor(*input1, im, CV_RGB2GRAY);
	cv::Mat output = cv::Mat(im.rows, im.cols, CV_8U);

	double m, s, max_s;
	double th=0;
	double min_I, max_I;
	int wxh	= winx/2;
	int wyh	= winy/2;
	int x_firstth= wxh;
	int x_lastth = im.cols-wxh-1;
	int y_lastth = im.rows-wyh-1;
	int y_firstth= wyh;
	int mx, my;
	unsigned char result;

	// Treat the window size
	if (winx==0||winy==0) {
		winy = (int) (2.0 * im.rows-1)/3;
		winx = (int) im.cols-1 < winy ? im.cols-1 : winy;
		// if the window is too big, than we asume that the image
		// is not a single text box, but a document page: set
		// the window size to a fixed constant.
		if (winx > 100)
			winx = winy = 40;
		std::cerr << "Setting window size to [" << winx
			<< "," << winy << "].\n";
	}

	// Create local statistics and store them in a double matrices
	cv::Mat map_m = cv::Mat::zeros (im.rows, im.cols, CV_32F);
	cv::Mat map_s = cv::Mat::zeros (im.rows, im.cols, CV_32F);
	max_s = calcLocalStats (im, map_m, map_s, winx, winy);

	minMaxLoc(im, &min_I, &max_I);

	cv::Mat thsurf (im.rows, im.cols, CV_32F);

	// Create the threshold surface, including border processing
	// ----------------------------------------------------

	for	(int j = y_firstth ; j<=y_lastth; j++) {

		// NORMAL, NON-BORDER AREA IN THE MIDDLE OF THE WINDOW:
		for	(int i=0 ; i <= im.cols-winx; i++) {

			m  = map_m.fget(i+wxh, j);
			s  = map_s.fget(i+wxh, j);

			// Calculate the threshold
			switch (version) {

				case NIBLACK:
					th = m + k*s;
					break;

				case SAUVOLA:
					th = m * (1 + k*(s/dR-1));
					break;

				case WOLFJOLION:
					th = m + k * (s/max_s-1) * (m-min_I);
					break;

				default:
					std::cerr << "Unknown threshold type in ImageThresholder::surfaceNiblackImproved()\n";
					exit (1);
			}

			thsurf.fset(i+wxh,j,th);

			if (i==0) {
				// LEFT BORDER
				for (int i=0; i<=x_firstth; ++i)
					thsurf.fset(i,j,th);

				// LEFT-UPPER CORNER
				if (j==y_firstth)
					for (int u=0; u<y_firstth; ++u)
						for (int i=0; i<=x_firstth; ++i)
							thsurf.fset(i,u,th);

				// LEFT-LOWER CORNER
				if (j==y_lastth)
					for (int u=y_lastth+1; u<im.rows; ++u)
						for (int i=0; i<=x_firstth; ++i)
							thsurf.fset(i,u,th);
			}

			// UPPER BORDER
			if (j==y_firstth)
				for (int u=0; u<y_firstth; ++u)
					thsurf.fset(i+wxh,u,th);

			// LOWER BORDER
			if (j==y_lastth)
				for (int u=y_lastth+1; u<im.rows; ++u)
					thsurf.fset(i+wxh,u,th);
		}

		// RIGHT BORDER
		for (int i=x_lastth; i<im.cols; ++i)
			thsurf.fset(i,j,th);

		// RIGHT-UPPER CORNER
		if (j==y_firstth)
			for (int u=0; u<y_firstth; ++u)
				for (int i=x_lastth; i<im.cols; ++i)
					thsurf.fset(i,u,th);

		// RIGHT-LOWER CORNER
		if (j==y_lastth)
			for (int u=y_lastth+1; u<im.rows; ++u)
				for (int i=x_lastth; i<im.cols; ++i)
					thsurf.fset(i,u,th);
	}
	std::cerr << "surface created" << std::endl;


	for	(int y=0; y<im.rows; ++y) 
		for	(int x=0; x<im.cols; ++x) 
		{
			if (im.uget(x,y) >= thsurf.fget(x,y))
			{
				output.uset(x,y,255);
			}
			else
			{
				output.uset(x,y,0);
			}
		}

	*output1 = output;
}

//---------------------------------------------------------

double BinarizeWolfJolion::calcLocalStats( cv::Mat &im, cv::Mat &map_m, cv::Mat &map_s, int winx, int winy)
{
	double m,s,max_s, sum, sum_sq, foo;
	int wxh	= winx/2;
	int wyh	= winy/2;
	int x_firstth= wxh;
	int y_lastth = im.rows-wyh-1;
	int y_firstth= wyh;
	double winarea = winx*winy;

	max_s = 0;
	for	(int j = y_firstth ; j<=y_lastth; j++) 
	{
		// Calculate the initial window at the beginning of the line
		sum = sum_sq = 0;
		for	(int wy=0 ; wy<winy; wy++)
			for	(int wx=0 ; wx<winx; wx++) {
				foo = im.uget(wx,j-wyh+wy);
				sum    += foo;
				sum_sq += foo*foo;
			}
		m  = sum / winarea;
		s  = sqrt ((sum_sq - (sum*sum)/winarea)/winarea);
		if (s > max_s)
			max_s = s;
		map_m.fset(x_firstth, j, m);
		map_s.fset(x_firstth, j, s);

		// Shift the window, add and remove	new/old values to the histogram
		for	(int i=1 ; i <= im.cols-winx; i++) {

			// Remove the left old column and add the right new column
			for (int wy=0; wy<winy; ++wy) {
				foo = im.uget(i-1,j-wyh+wy);
				sum    -= foo;
				sum_sq -= foo*foo;
				foo = im.uget(i+winx-1,j-wyh+wy);
				sum    += foo;
				sum_sq += foo*foo;
			}
			m  = sum / winarea;
			s  = sqrt ((sum_sq - (sum*sum)/winarea)/winarea);
			if (s > max_s)
				max_s = s;
			map_m.fset(i+wxh, j, m);
			map_s.fset(i+wxh, j, s);
		}
	}
	
	return max_s;
}



