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

#ifndef TL_HISTOGRAM_H
#define TL_HISTOGRAM_H

#include <valarray>
#include "Image.h"
#include "Rectangle.h"
#include "BGR2HSVhistLUT.h"

using namespace std;
using namespace TLImageProc;

namespace TLImageProc
{

class Histogram {
  private:
    valarray<float>* mTmpres;
    int* miNHSBins;
    int* miNVBins;
    Rectangle mImageBB;

  public:
    int niScales;
    valarray<float>* hsv_count;
 
    Histogram(int h_bins, int s_bins, int v_bins, int _niScales, int imgw, int imgh);

    ~Histogram();

    int compute(Image<unsigned char>* image, BGR2HSVhistLUT** luts,
                Rectangle* roi, 
                //Image<unsigned char>* mask=NULL,
                Image<float>* mask=NULL,
                bool setZero=true,
                bool normalise_hist=true,
		bool grid=true);
    int compute(Image<unsigned char>* image, BGR2HSVhistLUT** luts,
                Rectangle* roi, 
		float kernel_sigma_x, float kernel_sigma_y,
                bool setZero=true,
                bool normalise_hist=true,
		bool grid=true);
    void update(Histogram* h, float factor, bool norm=true);

    float distanceUnnormalised(Histogram* h);
    float distance(Histogram* h);
    void setZero();
    void normalise();
};

}

#endif
