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

#include "VideoOutput.h"

namespace TLInOut
{

VideoOutput::VideoOutput(std::string filename, int width, int height, float fps/*=25*/)
{
  mVWriter=NULL;
  mfFPS=fps;
  miWidth=width;
  miHeight=height;
  mVWriter = new cv::VideoWriter(filename.c_str(), CV_FOURCC('D','I','V','X'), mfFPS, cvSize(miWidth, miHeight), true);
  if (mVWriter==NULL)
  {
    MESSAGE(0, "Could not open video file out.avi.");
  }
}

VideoOutput::~VideoOutput()
{
  if (mVWriter)
    delete mVWriter;
}

void VideoOutput::addFrame(TLImageProc::Image8U* img)
{
  ASSERT(img->width()==miWidth && img->height()==miHeight, "Image and video dimensions do not match in VideoOutput::addFrame().");
  cv::Mat mat(miHeight, miWidth, CV_8UC3, img->data(), img->widthStep());
  mVWriter->write(mat);
}


}

