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

#include <ostream>

#include "utils_cv.h"


using namespace std;


namespace TLInOut
{

void drawText(Image8U* img, char* text, int fontsize, int x, int y, Colour c)
{
  cv::Mat m(img->height(), img->width(), img->nChannels()==3?CV_8UC3:CV_8UC1, img->data(), img->widthStep());
  cv::putText(m, text, cvPoint(x, y), cv::FONT_HERSHEY_PLAIN, 1.0, CV_RGB(c.r,c.g,c.b));
}


void saveImage(Image8U* img, string filename)
{
  cv::Mat m(img->height(), img->width(), img->nChannels()==3?CV_8UC3:CV_8UC1, img->data(), img->widthStep());
  vector<int> compression_params;
  imwrite(filename, m, compression_params);
}


}


