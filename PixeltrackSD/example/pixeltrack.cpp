/*
   Copyright (C) 2013 Stefan Duffner, LIRIS, INSA de Lyon, France

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
   */

//#define PROFILE


#include <getopt.h>
#ifdef PROFILE
#include <google/profiler.h>
#endif

#include "../src/Image.h"
#include "../src/VideoInputFile.h"
#include "../src/ImageOutput.h"
//ELrm #include "utils_cv.h"
#include "../src/VideoOutput.h"
#include "../src/Timer.h"
#include "../src/Error.h"

#include "../src/PixelTracker.h"

using namespace TLImageProc;
using namespace TLInOut;
using namespace TLUtil;

#define VERSION_MAJOR 0
#define VERSION_MINOR 1

//#define DEBUG_VERBOSITY 2

typedef struct params
{
	string filename;
	bool silent;
	int skip_frames;
	float search_size;
	string bbox;
	int from_frame;
	int to_frame;
	bool output;
	float detector_update_factor;
	float segmentation_update_factor;;
} params;


void set_default_params(params* p)
{
	p->filename="";
	p->silent=false;
	p->skip_frames=0;
	p->search_size=2;
	p->bbox="";
	p->from_frame=0;
	p->to_frame=1e8;
	p->output=false;
	p->detector_update_factor=0.1;
	p->segmentation_update_factor=0.1;
}


void print_usage(params* p)
{
	cout << "pixeltrack " << VERSION_MAJOR << "." << VERSION_MINOR << endl;
	cout << "© 2012 by Stefan Duffner, LIRIS/CNRS, France" << endl;

	cout << "Usage: pixeltrack <options> <video_file>" << endl;
	cout << "  options:" << endl;
	cout << "    -b (--bbox) x,y,w,h   initial bounding box parameters (default: " << p->bbox << ")" << endl;
	cout << "    -f (--from) N         start from frame number N (default: " << p->from_frame << ")" << endl;
	cout << "    -k (--skip_frames) N  skip N frames at each iteration(default: " << p->skip_frames << ")" << endl;
	cout << "    -o (--output)         output result in a video file (default: " << p->output << ")" << endl;

	cout << "    -s (--silent)         no image output  (default: " << p->silent << ")" << endl;
	cout << "    -t (--to) N           stop at frame number N (default: " << p->to_frame << ")" << endl;
	cout << "    -u (--det_update)     detector update factor (default: " << p->detector_update_factor << ")" << endl;
	cout << "    -v (--seg_update)     segmentation update factor (default: " << p->segmentation_update_factor << ")" << endl;
	cout << "    -w (--search_size)    relative search window size (default: " << p->search_size << ")" << endl;
}


int main(int argc, char** argv)
{
	setlocale(LC_ALL, "C");
	params par;
	set_default_params(&par);

	int option_index=0;
	int opt;
	opterr=0;
	static struct option long_options[] =
	{
		{"bbox",          required_argument, 0, 'b'},
		{"from",          required_argument, 0, 'f'},
		{"skip_frames",   required_argument, 0, 'k'},
		{"output",        no_argument,       0, 'o'},
		{"silent",        no_argument,       0, 's'},
		{"to",            required_argument, 0, 't'},
		{"det_update",    required_argument, 0, 'u'},
		{"seg_update",    required_argument, 0, 'v'},
		{"search_size",   required_argument, 0, 'w'},
		{0, 0, 0, 0}
	};
	do
	{
		opt = getopt_long(argc, argv, "b:f:k:ost:u:v:w:", long_options, &option_index);
		if (opt==-1)
			break;

		switch (opt)
		{
			case 'b':
				par.bbox = optarg;
				break;
			case 'f':
				par.from_frame = atoi(optarg);
				break;
			case 'k':
				par.skip_frames = atoi(optarg);
				break;
			case 'o':
				par.output = true;
				break;
			case 's':
				par.silent = true;
				break;
			case 't':
				par.to_frame = atoi(optarg);
				break;
			case 'u':
				par.detector_update_factor = atof(optarg);;
				break;
			case 'v':
				par.segmentation_update_factor = atof(optarg);;
				break;
			case 'w':
				par.search_size = atof(optarg);;
				break;
			case '?':
				print_usage(&par);
				return -1;
		}
	} while (opt!=-1);

	if (argc!=optind+1)
	{
		cerr << "Please specify video input file." << endl;
		print_usage(&par);
		return -1;
	}
	par.filename = argv[optind];

	VideoInput* vinput = new VideoInputFile(par.filename);
	ImageOutput output_window("pixeltrack output");
	ImageOutput voting_window("voting map");
	ImageOutput bp_window("backprojection");
	ImageOutput seg_window("segmentation");
	VideoOutput* voutput=NULL;
	Image<unsigned char>* cur_image; 

	Rectangle bbox(par.bbox);
	Rectangle initial_rect;

	int current_frame = 0;

	for(int i=0; i<par.from_frame; i++)
	{
		if (!vinput->nextImage())
			break;
		current_frame++;
	}

	// manual initialisation
	if (vinput->nextImageAvailable())
	{
		cur_image = vinput->nextImage();
		if (bbox.miWidth<=0 || bbox.miHeight<=0)
		{
			output_window.showImage(cur_image, 20);
			initial_rect = output_window.selectROI();
		}
		else
			initial_rect = bbox;
	}
	else
	{
		cerr << "No input data." << endl;
		return EXIT_FAILURE;
	}

	MESSAGE(0, "FPS: " << vinput->getFPS());
	int width = cur_image->width();
	int height = cur_image->height();

	if (par.output)
	{
		voutput = new VideoOutput("out.avi", width, height, vinput->getFPS());
	}

	PixelTracker tracker(initial_rect.miFirstColumn, initial_rect.miFirstLine, initial_rect.miWidth, initial_rect.miHeight, par.detector_update_factor, par.segmentation_update_factor, par.search_size);
	tracker.process( cur_image, current_frame, vinput->getCurrentTimestampMs(), vinput->getCurrentTimestampMs());

	output_window.setCurrentImage(cur_image);
	// draw tracking result
	if (!par.silent || par.output)
	{
		Rectangle *cur_bb = tracker.getCurBb();
		int maxx = tracker.getMaxx();
		int maxy = tracker.getMaxy();
		int cm_maxx = tracker.getCmMaxx();
		int cm_maxy = tracker.getCmMaxy();

		output_window.draw( *cur_bb, Colour(255,0,0), 2);
		output_window.draw( maxx, maxy, Colour(255,0,0), 2);
		output_window.draw( cm_maxx, cm_maxy, Colour(0,0,255), 2);
		output_window.draw( 0.5*(maxx+cm_maxx), 0.5*(maxy+cm_maxy), Colour(0,255,0), 2);
	}

	if (par.output)
	{
		voutput->addFrame(cur_image);
	}

	if (!par.silent)
	{
		//Elrm bp_img->multiply(1, bp_img_normalised);
		//ELrm bp_window.showFloatImage(bp_img_normalised, 2);
		//ELrm segmentation->binarise(0.5);
		//ELrm seg_window.showFloatImage(segmentation, 2);
		bp_window.showFloatImage( tracker.getBpImgNormalised(), 2);
		tracker.getSegmentation()->binarise(0.5);
		seg_window.showFloatImage(tracker.getSegmentation(), 2);

		output_window.wait(0);
	}


	Timer avgfps_timer;
	avgfps_timer.reset();
	long long current_time, prev_time;
	float avgfps=0;
#ifdef PROFILE
	ProfilerStart("profile");
#endif

	// 
	// MAIN TRACKING LOOP
	// 
	current_time = avgfps_timer.getRunTime();
	do
	{
		current_frame++;
		MESSAGE(0, "+++++++++++ Frame: " << current_frame << "++++++++++++++++++++++++")

		// retrieve next image
		for(int i=0; i<par.skip_frames; i++)
		{
			if (!vinput->nextImage())
				break;
			current_frame++;
		}

		cur_image = vinput->nextImage();
		if (!cur_image)
			break;

		tracker.process( cur_image, current_frame, vinput->getCurrentTimestampMs(), vinput->getCurrentTimestampMs());

		// display result
		output_window.setCurrentImage(cur_image);

		// draw tracking result
		if (!par.silent || par.output)
		{
			/*
			   output_window.draw(cur_bb, Colour(255,0,0), 4);
			   output_window.draw(maxx, maxy, Colour(255,0,0), 3);
			   output_window.draw(cm_maxx, cm_maxy, Colour(0,0,255), 3);
			   output_window.draw(cur_bb.centerX(), cur_bb.centerY(), Colour(0,255,0), 3);
			   */
			// draw just a red cross with centre position
			Rectangle *cur_bb = tracker.getCurBb();
			output_window.draw(cur_bb->centerX(), cur_bb->centerY(), Colour(255,0,0), 3);
		}

		// output video
		if (par.output)
		{
			voutput->addFrame(cur_image);
		}
		// display tracking result
		if (!par.silent)
		{
			output_window.showImage(cur_image, 2);

			//ELrm voting_map->multiply(0.01, voting_map_normalised);
			//ELrm voting_window.showFloatImage(voting_map_normalised, 2);
			//ELrm bp_img->multiply(1, bp_img_normalised);
			//ELrm bp_window.showFloatImage(bp_img_normalised, 2);
			//ELrm segmentation->binarise(0.5);
			//ELrm seg_window.showFloatImage(segmentation, 2);
			voting_window.showFloatImage(tracker.getVotingMapNormalised(), 2);
			bp_window.showFloatImage(tracker.getBpImgNormalised(), 2);
			tracker.getSegmentation()->binarise(0.5);
			seg_window.showFloatImage(tracker.getSegmentation(), 2);

			output_window.wait(0);
		}


		// calculate average processing time
		prev_time = current_time;
		current_time = avgfps_timer.getRunTime();

		if (current_frame<=3)
			avgfps = 1000000.0/avgfps_timer.computeDelta(current_time, prev_time);
		else
			avgfps = 0.99*avgfps + 0.01* 1000000.0/avgfps_timer.computeDelta(current_time, prev_time);

		MESSAGE(0, "Average processing speed: " << avgfps << " fps");

	} while (vinput->nextImageAvailable() && current_frame<=par.to_frame);

#ifdef PROFILE
	ProfilerStop();
#endif


	return 0;
}
