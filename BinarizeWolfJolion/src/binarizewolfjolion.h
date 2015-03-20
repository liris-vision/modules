#ifndef BINARIZEWOLFJOLION_H
#define BINARIZEWOLFJOLION_H

#ifdef D_BUILDWINDLL
	#define DLL_EXPORT __declspec(dllexport)
#else
	#define DLL_EXPORT
#endif

#include <opencv2/opencv.hpp>

/*
 * Fake class to test Starling.
 */
class DLL_EXPORT BinarizeWolfJolion
{
public:
	enum NiblackVersion 
	{
		NIBLACK=0,
		SAUVOLA,
		WOLFJOLION,
	};

	/*
	 * Constructor.
	 * Constructor parameters are Starling block parameters.
	 * Called once per block instance in "initialization" section.
	 */
	BinarizeWolfJolion( int _winx, int _winy, double _k, NiblackVersion type);

	/*
	 * Destructor.
	 * Release resources used by module.
	 * Called once per block instance in "cleaning" section.
	 */
	~BinarizeWolfJolion();

	/*
	 * The binarization routine.
	 * Function parameters are starling block inputs and outputs
	 * (pointer types).
	 * Called once per block instance in "processing" section.
	 */
	void process( cv::Mat *input1, cv::Mat *output1);

protected:

	/*
	 * Glide a window across the image and
	 * create two maps: mean and standard deviation.
	 */
	double calcLocalStats( cv::Mat &im, cv::Mat &map_m, cv::Mat &map_s, int winx, int winy);

	int winx;
	int winy;
	double k;
	NiblackVersion version;
	double dR;
};

#endif // BINARIZEWOLFJOLION_H
