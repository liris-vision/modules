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

#ifndef TL_BGR2HSV_DIST_HISTOGRAM 
#define TL_BGR2HSV_DIST_HISTOGRAM

void RGBtoHSV( const double r, const double g, const double b, double& h, double& s, double& v); 

class BGR2HSVdistLUT {
  //private: 
  public:
    // LUT
    unsigned short int* bgr_to_dist;

  public:
    int h_bins;
    int s_bins;
    int v_bins;
    
    BGR2HSVdistLUT(int h_bins, 
                   int s_bins, 
                   int v_bins,
                   float s_threshold=0.1,
                   float v_threshold=0.2);

    ~BGR2HSVdistLUT();

    void compute_luts(float h_threshold,
                      float v_threshold);
    inline int hsv_bin(int r, int g, int b){
	  return bgr_to_dist[(((b<<8) + g)<<8) + r];
    }
};


#endif
