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

#ifndef PIXELCLASSCOLOURMODEL_H
#define PIXELCLASSCOLOURMODEL_H

#include "Histogram.h"
#include "Image.h"
#include "BGR2HSVhistLUT.h"

//#define SPATIAL_PRIOR
#define SPATIAL_PRIOR_CREATE

#define FG_PRIOR_PROBABILITY 0.3

using namespace TLImageProc;


class PixelClassColourModel
{
  private:
    Histogram* mHist[3]; 
    Histogram* mUpdateHist[3]; 
    BGR2HSVhistLUT** mLUT;
    float mfMeanFGVoteErr;
    float mfVarFGVoteErr;
    float mfMeanBGVoteErr;
    float mfVarBGVoteErr;

  public:
    PixelClassColourModel(BGR2HSVhistLUT** lut, int h_bins, int s_bins, int v_bins, int _niScales, int imgw, int imgh);
    ~PixelClassColourModel();

    void setVoteErrParameters(float mean_pos, float var_pos, float mean_neg, float var_neg);
    void create(Image8U* img, Rectangle* outer_bb, Rectangle* object_region);
    void update(Image8U* img, Rectangle* outer_bb, Image<float>* segmentation, Image<float>* bp_img, float update_factor);
    void evaluateColour(Image8U* img, Rectangle* roi, bool use_spatial_prior, Image<float>* result); 
    void evaluateColourWithPrior(Image8U* img, Rectangle* roi, bool use_spatial_prior, Image<float>* prior, Image<float>* result);
    void evaluate(Image8U* img, Rectangle* roi, Image<float>* bp_img, Image<float>* result); 
};


#endif
