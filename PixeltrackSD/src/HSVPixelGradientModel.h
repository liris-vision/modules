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

#ifndef HSVPIXELGRADIENTMODEL_H
#define HSVPIXELGRADIENTMODEL_H

#include <list>
#include "Image.h"
#include "Rectangle.h"
#include "GradDispLUT.h"
#include "BGR2HSVdistLUT.h"

#define CLUSTER_SIZE 3
#define MAXVOTES 20 // 1000

using namespace TLImageProc;
using namespace std;

typedef struct displacement_t
{
  public:
    displacement_t() {x=0; y=0; };
    displacement_t(int x_, int y_) {x=x_; y=y_; };
    bool operator==(const displacement_t& d1) { return (x==d1.x && y==d1.y); };
    int x;
    int y;
    float count;
} displacement_t;
  
bool disp_less(displacement_t lhs, displacement_t rhs);
bool disp_less_count(displacement_t lhs, displacement_t rhs);
  

class HSVPixelGradientModel
{
  public:
    HSVPixelGradientModel(int nb_hsbins, int nb_vbins, int nb_obins, int nb_mbins, float mag_thresh);
    ~HSVPixelGradientModel();

    void reset();
    void learn(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* segmentation);
    void vote(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* voting_map);
    void vote(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* voting_map, float scale, float angle);
    void backproject(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* bpimg, int maxlocx, int maxlocy);
    void backproject(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* bpimg, int maxlocx, int maxlocy, Image<float>* segmentation, float& mean_pos, float& variance_pos, float& mean_neg, float& variance_neg);
    void update(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* segmentation, float update_factor);

  private:
    BGR2HSVdistLUT* m_LUTColour;
    GradDispLUT* m_LUTGradient;
    list<displacement_t>** disp;
    int h_bins;        
    int s_bins;  
    int v_bins;
    int o_bins;
    int m_bins;
    int maxcolourbin;
    int totalbins;
    float magnitude_threshold;
};

#endif
