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

#include <math.h>
#include <float.h>
#include "BGR2HSVhistLUT.h"

namespace TLImageProc
{

BGR2HSVhistLUT::BGR2HSVhistLUT(int _h_bins,
			       int _s_bins,
			       int _v_bins,
			       float s_threshold,
			       float v_threshold){
  h_bins = _h_bins;
  s_bins = _s_bins;
  v_bins = _v_bins;

  bgr_to_hsv_bin = new unsigned short int[256*256*256];
  //bgr_to_v_bin  = new unsigned short int[256*256*256];
  compute_luts(s_threshold,
	       v_threshold);
}

BGR2HSVhistLUT::~BGR2HSVhistLUT(){
  delete[] bgr_to_hsv_bin;
}

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//              if s == 0, then h = -1 (undefined)
void RGBtoHSV( const double r, const double g, const double b, 
	       double& h, double& s, double& v) {
  double vmin, diff;

  v = vmin = r;
  if( v < g ) v = g;
  if( v < b ) v = b;
  if( vmin > g ) vmin = g;
  if( vmin > b ) vmin = b;

  diff = v - vmin;
  s = diff/(float)(fabs(v) + FLT_EPSILON);
  diff = (float)(60./(diff + FLT_EPSILON));
  if( v == r )
    h = (g - b)*diff;
  else if( v == g )
    h = (b - r)*diff + 120.f;
  else
    h = (r - g)*diff + 240.f;

   if( h < 0 ) h += 360.f;
}

// Compute LUTs for histogram computation
// directly from the RGB pixel values
void BGR2HSVhistLUT::compute_luts(float s_threshold,
				  float v_threshold){

  double h, s, v;

  double h_step = 360./h_bins;
  double s_step = 1./s_bins;
  double v_step = 1./v_bins;
  double norm = 1.0/255;
  int indtmp1, indtmp2, ind;

  // changed to order BGR 
  for(int b=0;b<256;b++){
    indtmp1 = b*256;
    for(int g=0;g<256;g++){
      indtmp2 = (indtmp1 + g)*256;
      for(int r=0;r<256;r++){
	RGBtoHSV(b*norm, g*norm, r*norm, h, s, v);

	ind = indtmp2 + r;

	int h_index = floor(h/h_step);
	if(h_index == h_bins) h_index = h_bins-1;
	int s_index = floor(s/s_step);
	if(s_index == s_bins) s_index = s_bins-1;
	//bgr_to_hs_bin[ind] = h_index*s_bins + s_index;
	int v_index = floor(v/v_step);
	if(v_index == v_bins) v_index = v_bins-1;
	//bgr_to_v_bin[ind]  = v_index;

	if (s<s_threshold || v<v_threshold)
	  bgr_to_hsv_bin[ind] = h_bins*s_bins + v_index;
	else
	  bgr_to_hsv_bin[ind] = h_index*s_bins + s_index;

      }
    }
  }
}

}

