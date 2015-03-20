#include <iostream>
#include <fstream>
#include <string>
#include <assert.h>
#include <highgui.h>
#include "support_class.h"
#include "RandomForest.h"
#include "Histogram.h"
#include "bodypartsegmentation.h"


#ifdef WIN32
	#define round(x)  floor((x) + 0.5)
#endif

int PART_SIZE = 11; 
int FIXED_INF = 100; 

//------------- define the feature type and id -------------
enum { DEPT, EDGE_MAG, EDGE_ORI };	/* feature_id */
enum { DIFF, SUM, BOTH };               /* feature_type */

//---------------------------------------------------------

BodyPartSegmentation::BodyPartSegmentation( const char *forestParamFileName, const char *groundImgFileName, bool _depthIsInMillimeters)
{
	// basic initializations
	depthIsInMillimeters = _depthIsInMillimeters;
	forest = new RandomForest();
	std::string params_file(forestParamFileName); 
	std::ifstream input; 
	input.open( params_file.c_str(), std::ios::in );
	if( ! input.is_open() )
		std::cout << "BodyPartSegmentation::BodyPartSegmentation ERROR: failed to open forest parameters file " << params_file << "." << std::endl;
	forest->readForeset( input );
	input.close();
	ground = cvLoadImage(groundImgFileName);
	if( ground == NULL )
		std::cout << "BodyPartSegmentation::BodyPartSegmentation warning: failed to load " << groundImgFileName << "." << std::endl;

#if DEBUG
	if( depthIsInMillimeters )
		std::cout << "BodyPartSegmentation::BodyPartSegmentation info: depth is supposed to be in millimeters." << std::endl;
	else
		std::cout << "BodyPartSegmentation::BodyPartSegmentation info: depth is supposed to be coded in range [0, 2047]." << std::endl;
#endif
}

//---------------------------------------------------------

BodyPartSegmentation::~BodyPartSegmentation()
{
	// cleaning
	delete forest;
	cvReleaseImage(&ground);
}

//---------------------------------------------------------

void BodyPartSegmentation::init()
{
	// more initialization
}

//---------------------------------------------------------

float BodyPartSegmentation::raw_depth_to_meters(int raw_depth)
{
	// depth must be in [0,2047]
	if (raw_depth < 2047)
		return 1.0 / (raw_depth * -0.0030711016 + 3.3309495161);

	return 0;
}

//---------------------------------------------------------

int BodyPartSegmentation::meters_to_raw_depth(float depthInMeters)
{
	if( depthInMeters < 0.3002  ||  depthInMeters > 533.2248 )
		return 2047;
	else
		return round((1.0/0.0030711016) * (3.3309495161 - 1.0/depthInMeters));
}

//---------------------------------------------------------

void BodyPartSegmentation::ExtractDepthHuman( CvMat* src, CvMat* mask, CvMat* dst )
{
	int height = src->height; 
	int width = src->width; 

	// compute the distance data and find the max distance value
	float max_value = 0; 
	for ( int m=0; m<height; m++ )
		for( int n=0; n<width; n++ )
		{	if( *(mask->data.ptr+m*width+n)==1 )
			{	int depth_value = (int) *(src->data.fl+m*width+n); 
				*(dst->data.fl+m*width+n) = raw_depth_to_meters( depth_value ); 
				if ( *(dst->data.fl+m*width+n)>max_value )
					max_value = *(dst->data.fl+m*width+n); 
			} else
				*(dst->data.fl+m*width+n) = FIXED_INF; 
		}

	// normalized into 4m 
	for ( int m=0; m<height; m++ )
		for ( int n=0; n<width; n++ )
		{	if ( *(dst->data.fl+m*width+n)!=FIXED_INF )
			{	float cur_value = *(dst->data.fl+m*width+n); 
				*(dst->data.fl+m*width+n)= 4 * (cur_value /max_value); 
			}
		}
}

//---------------------------------------------------------

IplImage* BodyPartSegmentation::SegmentParts( CvMat* seg_depth, CvMat* seg_edge )
{

	int height = seg_depth->height; 
	int width = seg_depth->width; 

	IplImage* color = cvCreateImage( cvSize(width, height), IPL_DEPTH_8U, 3); 

	float* vote = new float [ PART_SIZE ]; 
	float ** filtered_depth = new float* [height]; 
	for ( int m=0; m<height; m++ )
	{	filtered_depth[m] = new float [width]; 
		for ( int n=0; n<width; n++ )
			filtered_depth[m][n] = *(seg_depth->data.fl+m*width+n); 
	}
	
	float ** filtered_edge = 0; 
	if ( seg_edge )
	{	filtered_edge = new float* [height]; 
		for ( int m=0; m<height; m++ )
		{	filtered_edge[m] = new float [width]; 
			for ( int n=0; n<width; n++ )
				filtered_edge[m][n] = *(seg_edge->data.fl+m*width+n); 
		}
	}

	for ( int m=0; m<height; m++ )
		for ( int n=0; n<width; n++ )
		{
			if ( filtered_depth[m][n]!=FIXED_INF )
			{	Feature* feat_depth = new Feature( DEPT, cvPoint( m, n), filtered_depth, height, width, FIXED_INF );
				Feature* feat_rgb = NULL;
				if ( seg_edge ) 
					feat_rgb = new Feature( EDGE_MAG, cvPoint( m, n ), filtered_edge, height, width, FIXED_INF );
				Feature* feat_rgb_ori = NULL; 
 				FeatureVector* feat_vec = new FeatureVector( feat_depth, FIXED_INF ); 
				feat_vec->add_feature( feat_rgb ); 
				feat_vec->add_feature( feat_rgb_ori ); 

				forest->testFeatInForest( feat_vec, vote, PART_SIZE ); 		
										
				float max = 0; 
				int classified = -1; 
				for ( int w=0; w<PART_SIZE; w++ )
				{	if ( vote[w]>max )
					{	max = vote[w];  classified = w;     }
				}
				delete feat_vec; 

				switch ( classified )
				{
					case 0:  cvSet2D( color, m, n, cvScalar(0, 0, 255) ); break;   // head
					case 1:  cvSet2D( color, m, n, cvScalar(0, 255, 0) ); break;   // neck
					case 2:  cvSet2D( color, m, n, cvScalar(125, 50, 0) ); break;  // left shoulder
					case 3:  cvSet2D( color, m, n, cvScalar(50, 0, 125) ); break;  // right shoulder
					case 4:  cvSet2D( color, m, n, cvScalar(192, 182, 32) ); break;  // left upper arm    (0, 125, 50)
					case 5:  cvSet2D( color, m, n, cvScalar(25, 100, 0) ); break;  // left forearm
					case 6:  cvSet2D( color, m, n, cvScalar(9, 246, 253) ); break;  // right upper arm    (0, 25, 100)
					case 7:  cvSet2D( color, m, n, cvScalar(8, 165, 255)  ); break;  // right forearm    (100, 0, 25)
					case 8:  cvSet2D( color, m, n, cvScalar(255, 0, 0) ); break;   // left hip (wrist)
					case 9:  cvSet2D( color, m, n, cvScalar(26, 80, 255) ); break;   // right hip   (100, 125, 125)
					case 10:  cvSet2D( color, m, n, cvScalar(255, 255, 255) ); break;   // other parts  cvScalar (147, 94, 213)
				}
			} else 
				cvSet2D( color, m, n, cvScalar(0, 0, 0) );
		}

	// release the memories 
	for ( int m=0; m<height; m++ )		
		delete[] filtered_depth[m]; 
	if ( seg_edge )
	{	for ( int m=0; m<height; m++ )
			delete[] filtered_edge[m]; 
		delete[] filtered_edge; 
	}
	delete[] filtered_depth; 
	delete[] vote; 

	return color; 

}

//------------------------------------------------------------

void BodyPartSegmentation::run(const cv::Mat& depthImg, bool bLegend, cv::Mat& outputImg)
{
	if( depthImg.empty() || depthImg.rows != 480 || depthImg.cols != 640 || depthImg.channels() != 1 || depthImg.depth() != CV_16U )
	{
		// bad input, exit
		printf( "BodyPartSegmentation::run: input depth image must be a 1 channel, 16 bits, 640x480 pixels image.\n");
		return;
	}

	int height = depthImg.rows; 
	int width = depthImg.cols; 
	CvMat* mask = cvCreateMat( height, width, CV_8UC1 ); 
	CvMat* pro_mat = cvCreateMat( height, width, CV_32FC1 ); 
	CvMat* dst = cvCreateMat( height, width, CV_32FC1 ); 

	// do your image processing
	computePersonMask( depthImg, mask, pro_mat);
	ExtractDepthHuman( pro_mat, mask, dst );  
	IplImage *tmpOutputImg = SegmentParts( dst, NULL );

	// display legend on output image
	if( bLegend && ground != NULL )
	{
		cvSetImageROI( tmpOutputImg, cvRect( 0, 0, ground->width, ground->height)); 
		cvCopy( ground, tmpOutputImg); 
		cvResetImageROI(tmpOutputImg);
	}

	cv::Mat(tmpOutputImg).copyTo(outputImg);

	cvReleaseMat( &dst ); 
	cvReleaseMat( &pro_mat ); 
	cvReleaseMat( &mask ); 
	cvReleaseImage( &tmpOutputImg ); 
}

//------------------------------------------------------------

void BodyPartSegmentation::computePersonMask(const cv::Mat& depthImg, CvMat* mask, CvMat* pro_mat)
{
	int height = depthImg.rows; 
	int width = depthImg.cols; 

	// define the tmp variables
	CvMat* pre_mat = cvCreateMat( height, width, CV_32FC1 ); 

	// convert depth values to float
	assert( sizeof(unsigned short) == 2 );
	unsigned short* data= (unsigned short*) depthImg.data; 
	for( int m=0; m<height; m++)
	{
		for( int n=0; n<width; n++)
		{
			if( depthIsInMillimeters )
			{
				float depthInMeters = (*(data + m*width + n)) / 1000.0;
				*(pre_mat->data.fl+m*width+n) = meters_to_raw_depth(depthInMeters); 
			}
			else
			{
				*(pre_mat->data.fl+m*width+n)=(float)*(data+m*width+n); 
				// if depth is coded on the 11 most significant bits
				//*(pre_mat->data.fl+m*width+n) /= 32;
			}
		}
	}

	// smooth the depth image
	cvSmooth( pre_mat, pro_mat, CV_GAUSSIAN );

	// Create new histogram to get a thresold to get a mask
	Histogram<float>* H = new Histogram<float> (1086, 0, 1085, true);
	H->clear(); 

	// Add image depth values to the histogram
	for ( int m=0; m<height; m++ )
		for ( int n=0; n<width; n++ ) 
    		{	float depth_value = *(pro_mat->data.fl+m*width+n); 
			if ( depth_value>0 && depth_value<1086 )
				H->add(depth_value);
		}
	
	// Compute the threshold
	int thres = H->getThresholdValueFisher (-1,-1, NULL, NULL);
	for ( int m=0; m<height; m++ )
		for ( int n=0; n<width; n++ )
		{	int depth_value = (int)*(pro_mat->data.fl+m*width+n); 
			if ( depth_value<=thres && depth_value>0 )
				*(mask->data.ptr+m*width+n) = 1; 
			else
				*(mask->data.ptr+m*width+n) = 0; 
		}

	cvReleaseMat( &pre_mat ); 
	delete H; 	
} 
