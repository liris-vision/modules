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

#ifndef TL_GRADDISP_HISTOGRAM 
#define TL_GRADDISP_HISTOGRAM

class GradDispLUT {
  //private: 
  public:
    // LUT
    unsigned int* grad_to_disp;

  public:
    int o_bins;
    int m_bins;
    
    GradDispLUT(int o_bins, int m_bins, float m_threshold=50);

    ~GradDispLUT();

    void compute_luts(float m_threshold);
    inline unsigned int get_bin(short gx, short gy){
	  return grad_to_disp[((gx+1025)<<12) + gy+1025];
    }
};


#endif
