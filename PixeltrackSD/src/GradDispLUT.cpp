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

#include <iostream>

#ifdef D_API_WIN32
// Windows
#define _USE_MATH_DEFINES  // for M_PI
#endif
#include <math.h>

#include <float.h>
#include "BGR2HSVdistLUT.h"
#include "GradDispLUT.h"


GradDispLUT::GradDispLUT(int _o_bins, int _m_bins, float m_threshold)
{
  o_bins = _o_bins;
  m_bins = _m_bins;
  grad_to_disp = new unsigned int[(2050<<12)+2050];

  compute_luts(m_threshold);
}

GradDispLUT::~GradDispLUT(){
  delete[] grad_to_disp;
}


// Compute LUTs for histogram computation
// directly from the RGB pixel values
void GradDispLUT::compute_luts(float m_threshold)
{
  unsigned int indtmp1, ind;
  float gx, gy;
  float angle, magnitude;
  int obin, mbin;
  int maxmag = sqrtf(2*1025*1025);

  // changed to order BGR 
  for(int x=0; x<2050; x++){
    indtmp1 = x<<12;
    for(int y=0; y<2050; y++){
      ind = indtmp1 + y;

      gx = x-1025;
      gy = y-1025;
      angle = atan2f(gy, gx);
      magnitude = sqrtf(gx*gx+gy*gy);
      if (magnitude<m_threshold)
      {
	grad_to_disp[ind] = (o_bins*m_bins-1);
      }
      else
      {
	obin = int((angle+M_PI)/(2*M_PI)*o_bins); 
	if (obin>=o_bins)
	  obin=o_bins-1;
	mbin = int((float)magnitude/maxmag * m_bins);
	if (mbin>=m_bins)
	  mbin=m_bins-1;
	grad_to_disp[ind] = obin*m_bins + mbin;
      }
    }
  }
}


