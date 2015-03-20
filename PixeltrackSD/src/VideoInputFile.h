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

#ifndef TL_VIDEO_INPUT_FILE_H
#define TL_VIDEO_INPUT_FILE_H

#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Error.h"

#include "VideoInput.h"

using namespace std;
//using namespace cv;

namespace TLInOut
{

class VideoInputFile : public VideoInput
{
  private:
    string msFilename;
    cv::VideoCapture* mCapture;
    cv::Mat mFrame;
    bool mbFrameGrabbed;
    bool mbNextImageAvailable;
    Image<unsigned char>* mCurrentImage;

    int determineFPS_ffmpeg(string filename);

  public:
    VideoInputFile(string filename, float fps=-1);
    ~VideoInputFile();

    bool nextImageAvailable();
    Image<unsigned char>* nextImage();
    unsigned long long getCurrentTimestampMs() { return (miFramesRead-1UL)*1000.0/mfFPS; };
};

}


#endif
