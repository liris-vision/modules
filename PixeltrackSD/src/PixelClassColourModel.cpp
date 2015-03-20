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

#ifdef D_API_WIN32
// Windows
#define _USE_MATH_DEFINES  // for M_PI
#endif
#include <math.h>
#include "PixelClassColourModel.h"


using namespace TLImageProc;

PixelClassColourModel::PixelClassColourModel(BGR2HSVhistLUT** lut, int h_bins, int s_bins, int v_bins, int _niScales, int imgw, int imgh)
{
  for(int i=0; i<3; i++)
  {
    mHist[i] = new Histogram(h_bins, s_bins, v_bins, _niScales, imgw, imgh);
    mUpdateHist[i] = new Histogram(h_bins, s_bins, v_bins, _niScales, imgw, imgh);
  }
  mLUT = lut;
  mfMeanFGVoteErr = -1;
}

PixelClassColourModel::~PixelClassColourModel()
{
  for(int i=0; i<3; i++)
  {
    delete mHist[i];
    delete mUpdateHist[i];
  }
}

void PixelClassColourModel::setVoteErrParameters(float mean_pos, float var_pos, float mean_neg, float var_neg)
{
  mfMeanFGVoteErr = mean_pos;
  mfVarFGVoteErr = var_pos;
  mfMeanBGVoteErr = mean_neg;
  mfVarBGVoteErr = var_neg;
}

void PixelClassColourModel::create(Image8U* img, Rectangle* outer_object, Rectangle* object_region)
{
  Rectangle outer_top, outer_bottom, outer_left, outer_right, margin;
  int hdiff, wdiff;
  margin = *outer_object;
  Rectangle outer = margin;
  outer.enlarge(2.0);
  hdiff = outer.miHeight-margin.miHeight;
  wdiff = outer.miWidth-margin.miWidth;
  outer_top = outer_bottom = outer_left = outer_right = outer;
  outer_top.miHeight = hdiff/2;
  outer_bottom.miHeight = hdiff/2;
  outer_bottom.miFirstLine = outer.lastLine()-outer_bottom.miHeight+1;
  outer_left.miWidth = wdiff/2;
  outer_left.miFirstLine = margin.miFirstLine;
  outer_left.miHeight = margin.miHeight;
  outer_right.miWidth = wdiff/2;
  outer_right.miFirstColumn = outer.lastColumn()-outer_right.miWidth+1;
  outer_right.miFirstLine = margin.miFirstLine;
  outer_right.miHeight = margin.miHeight;

#ifdef SPATIAL_PRIOR_CREATE
  float sigmax = object_region->miWidth/8;;
  float sigmay = object_region->miHeight/8;;
  // FG model
  mHist[0]->compute(img, mLUT, object_region, sigmax, sigmay, true, true, false);
#else
  // FG model
  mHist[0]->compute(img, mLUT, object_region, NULL, true, true, false);
#endif

  // BG model 
  mHist[1]->compute(img, mLUT, &outer_top, NULL, true, false, false);
  mHist[1]->compute(img, mLUT, &outer_bottom, NULL, false, false, false);
  mHist[1]->compute(img, mLUT, &outer_left, NULL, false, false, false);
  mHist[1]->compute(img, mLUT, &outer_right, NULL, false, true, false);
}



void PixelClassColourModel::update(Image8U* img, Rectangle* outer_bb, Image<float>* segmentation, Image<float>* bp_img, float update_factor)
{
  Rectangle outer_top, outer_bottom, outer_left, outer_right, margin;
  int hdiff, wdiff;
  margin = *outer_bb;
  Rectangle outer = margin;
  outer.enlarge(2.0);
  hdiff = outer.miHeight-margin.miHeight;
  wdiff = outer.miWidth-margin.miWidth;
  outer_top = outer_bottom = outer_left = outer_right = outer;
  outer_top.miHeight = hdiff/2;
  outer_bottom.miHeight = hdiff/2;
  outer_bottom.miFirstLine = outer.lastLine()-outer_bottom.miHeight+1;
  outer_left.miWidth = wdiff/2;
  outer_left.miFirstLine = margin.miFirstLine;
  outer_left.miHeight = margin.miHeight;
  outer_right.miWidth = wdiff/2;
  outer_right.miFirstColumn = outer.lastColumn()-outer_right.miWidth+1;
  outer_right.miFirstLine = margin.miFirstLine;
  outer_right.miHeight = margin.miHeight;

  float sigmax = outer_bb->miWidth/8;;
  float sigmay = outer_bb->miHeight/8;;
  float dx, dy;
  int rw2=outer_bb->miWidth/2, rh2=outer_bb->miHeight/2;
  Rectangle imgbb(0,0,bp_img->width()-1,bp_img->height()-1);
  outer_bb->intersection(imgbb);
  int si, sj, si0, sj0;;
  int ll = outer_bb->lastLine(), lc = outer_bb->lastColumn();
  double spatial_prior;
  si0=0;
  for(si=outer_bb->miFirstLine; si<=ll; si++)
  {
    dy = si0-rh2;
    sj0=0;
    for(sj=outer_bb->miFirstColumn; sj<=lc; sj++)
    {
      dx = sj0-rw2;
      spatial_prior = 0.7*exp(-0.5*(dx*dx/sigmax/sigmax+dy*dy/sigmay/sigmay)); 
      bp_img->add(sj, si, spatial_prior);
      sj0++;
    }
    si0++;
  }

  // FG model
  mUpdateHist[0]->compute(img, mLUT, outer_bb, bp_img, true, true, false);
  mHist[0]->update(mUpdateHist[0], update_factor);

  // BG model 
  mUpdateHist[1]->compute(img, mLUT, &outer_top, NULL, true, false, false);
  mUpdateHist[1]->compute(img, mLUT, &outer_bottom, NULL, false, false, false);
  mUpdateHist[1]->compute(img, mLUT, &outer_left, NULL, false, false, false);
  mUpdateHist[1]->compute(img, mLUT, &outer_right, NULL, false, true, false);
  mHist[1]->update(mUpdateHist[1], update_factor);
}


void PixelClassColourModel::evaluateColour(Image8U* img, Rectangle* roi, bool use_spatial_prior, Image<float>* result)
{
  int lastline, lastcol;
  int index; 
  int width, height;
  width = img->width();
  height = img->height();
  Rectangle imgBB(1, 1, width-1, height-1);
  roi->intersection(imgBB); 
  lastline = roi->lastLine();
  lastcol = roi->lastColumn();
  unsigned char r, g, b;
  float xtrans, ytrans;
  int si, sj;
  int xgrid_step = 1; //max(1,roi->miWidth/30);
  int ygrid_step = 1; //max(1,roi->miHeight/30);
  int ws = img->widthStep();
  int rws = result->widthStep();
  int roinx = roi->miWidth/xgrid_step;
  int roiny = roi->miHeight/ygrid_step;
  int pixelstep = (xgrid_step-1)*3;
  int rpixelstep = (xgrid_step);
  int xstep = ws-xgrid_step*roinx*3 + (ygrid_step-1)*ws;
  int rxstep = rws-xgrid_step*roinx + (ygrid_step-1)*rws;
  unsigned char* iptr = (unsigned char*)img->data()+roi->miFirstLine*ws + roi->miFirstColumn*3;
  float* resptr = result->data()+roi->miFirstLine*rws + roi->miFirstColumn;
  float colour_prob;
  float spatial_prior;
  float dx, dy;
  int rw2=roi->miWidth/2, rh2=roi->miHeight/2;
  float sigmax = roi->miWidth/4;;
  float sigmay = roi->miHeight/4;;
  float tmpres;

  for(si=0; si<roiny; si++)
  {
    dy = si-rh2;
    for(sj=0; sj<roinx; sj++)
    {
      if (use_spatial_prior)
      {
	dx = sj-rw2;
	spatial_prior = exp(-0.5*(dx*dx/sigmax/sigmax+dy*dy/sigmay/sigmay)); ///(2*M_PI*sigmax*sigmay);
      }

      b = *iptr++; 
      g = *iptr++;
      r = *iptr++;

      tmpres=1.0;
      for(int s=0; s<mHist[0]->niScales; s++)
      {
      index = mLUT[s]->hsv_bin(r,g,b);
      if (use_spatial_prior)
	colour_prob = spatial_prior*mHist[0]->hsv_count[s][index]*FG_PRIOR_PROBABILITY + (1.0-spatial_prior)*mHist[1]->hsv_count[s][index]*(1.0-FG_PRIOR_PROBABILITY);
      else
	colour_prob = mHist[0]->hsv_count[s][index]*FG_PRIOR_PROBABILITY + mHist[1]->hsv_count[s][index]*(1.0-FG_PRIOR_PROBABILITY);
      if (colour_prob>0)
	if (use_spatial_prior)
	  tmpres *= spatial_prior*mHist[0]->hsv_count[s][index]*FG_PRIOR_PROBABILITY/colour_prob;
	else
	  tmpres *= mHist[0]->hsv_count[s][index]*FG_PRIOR_PROBABILITY/colour_prob;
      else
	tmpres = 0.0;
      }
      *resptr = tmpres;

      iptr+=pixelstep;
      resptr+=rpixelstep;
    }
    iptr+=xstep;
    resptr+=rxstep;
  }
  
}

void PixelClassColourModel::evaluateColourWithPrior(Image8U* img, Rectangle* roi, bool use_spatial_prior, Image<float>* prior, Image<float>* result)
{
  int lastline, lastcol;
  int index; 
  int width, height;
  width = img->width();
  height = img->height();
  Rectangle imgBB(1, 1, width-1, height-1);
  roi->intersection(imgBB); 
  lastline = roi->lastLine();
  lastcol = roi->lastColumn();
  unsigned char r, g, b;
  float xtrans, ytrans;
  int si, sj;
  int xgrid_step = 1; //max(1,roi->miWidth/30);
  int ygrid_step = 1; //max(1,roi->miHeight/30);
  int ws = img->widthStep();
  int rws = result->widthStep();
  int roinx = roi->miWidth/xgrid_step;
  int roiny = roi->miHeight/ygrid_step;
  int pixelstep = (xgrid_step-1)*3;
  int rpixelstep = (xgrid_step);
  int xstep = ws-xgrid_step*roinx*3 + (ygrid_step-1)*ws;
  int rxstep = rws-xgrid_step*roinx + (ygrid_step-1)*rws;
  unsigned char* iptr = (unsigned char*)img->data()+roi->miFirstLine*ws + roi->miFirstColumn*3;
  float* resptr = result->data()+roi->miFirstLine*rws + roi->miFirstColumn;
  float colour_prob;
  float spatial_prior;
  float dx, dy;
  int rw2=roi->miWidth/2, rh2=roi->miHeight/2;
  float sigmax = roi->miWidth/4;;
  float sigmay = roi->miHeight/4;;
  int i, j;
  float prior_val;
  float tmpres;

  for(si=0; si<roiny; si++)
  {
    i = si+roi->miFirstLine;
    dy = si-rh2;
    for(sj=0; sj<roinx; sj++)
    {
      j = sj+roi->miFirstColumn;
      if (use_spatial_prior)
      {
	dx = sj-rw2;
	spatial_prior = exp(-0.5*(dx*dx/sigmax/sigmax+dy*dy/sigmay/sigmay)); ///(2*M_PI*sigmax*sigmay);
      }

      b = *iptr++; 
      g = *iptr++;
      r = *iptr++;

      float trans;
      float trans_fg_to_fg, trans_fg_to_bg, trans_bg_to_fg, trans_bg_to_bg;
      prior_val = prior->get(j,i);  // TODO use pointer to traverse prior
      trans_fg_to_fg = 0.4; // transition probability from FG to FG
      trans_fg_to_bg = 0.6; // transition probability from FG to BG
      trans_bg_to_fg = 0.4; // transition probability from BG to FG
      trans_bg_to_bg = 0.6; // transition probability from BG to BG
      if (prior_val>0.5)
	trans=0.6;   // transition probability from FG to BG
      else 
	trans=0.4;   // transition probability from BG to FG

      float ttmp, ttmp_neg;
      tmpres=1.0;
      for(int s=0; s<mHist[0]->niScales; s++)
      {
      index = mLUT[s]->hsv_bin(r,g,b);
      if (use_spatial_prior)
      {
	colour_prob = spatial_prior*mHist[0]->hsv_count[s][index]*prior_val*trans + (1.0-spatial_prior)*mHist[1]->hsv_count[s][index]*(1.0-prior_val)*(1.0-trans);
	if (colour_prob>0)
	  tmpres *= spatial_prior*mHist[0]->hsv_count[s][index]*prior_val*trans/colour_prob;
	else
	  tmpres = 0.0;
      }
      else
      {
	ttmp = ((mHist[0]->hsv_count[s][index]*prior_val*trans_fg_to_fg) + mHist[0]->hsv_count[s][index]*(1.0-prior_val)*trans_bg_to_fg);
	ttmp_neg = ((mHist[1]->hsv_count[s][index]*prior_val*trans_fg_to_bg) + mHist[1]->hsv_count[s][index]*(1.0-prior_val)*trans_bg_to_bg);
	if (ttmp+ttmp_neg>0)
	{
	  tmpres *= ttmp / (ttmp + ttmp_neg);
	}

	else
	  tmpres = 0.0;
      }
      }
      *resptr = tmpres;

      iptr+=pixelstep;
      resptr+=rpixelstep;
    }
    iptr+=xstep;
    resptr+=rxstep;
  }
  
}


void PixelClassColourModel::evaluate(Image8U* img, Rectangle* roi, Image<float>* bp_img, Image<float>* result)
{
  int lastline, lastcol;
  int index; 
  int width, height;
  width = img->width();
  height = img->height();
  Rectangle imgBB(1, 1, width-1, height-1);
  roi->intersection(imgBB); 
  lastline = roi->lastLine();
  lastcol = roi->lastColumn();
  unsigned char r, g, b;
  float xtrans, ytrans;
  int si, sj;
  int xgrid_step = 1; //max(1,roi->miWidth/30);
  int ygrid_step = 1; //max(1,roi->miHeight/30);
  int ws = img->widthStep();
  int rws = result->widthStep();
  int roinx = roi->miWidth/xgrid_step;
  int roiny = roi->miHeight/ygrid_step;
  int pixelstep = (xgrid_step-1)*3;
  int rpixelstep = (xgrid_step);
  int xstep = ws-xgrid_step*roinx*3 + (ygrid_step-1)*ws;
  int rxstep = rws-xgrid_step*roinx + (ygrid_step-1)*rws;
  unsigned char* iptr = (unsigned char*)img->data()+roi->miFirstLine*ws + roi->miFirstColumn*3;
  float* resptr = result->data()+roi->miFirstLine*rws + roi->miFirstColumn;
  float colour_prob;
  float err_max = 30;
  float p_err_fg, p_err_bg = 1.0/err_max;;
  int i, j;
  float tmpexp;
  float den;
#ifdef SPATIAL_PRIOR
  float spatial_prior;
  float dx, dy;
  int rw2=roi->miWidth/2, rh2=roi->miHeight/2;
  float sigmax = roi->miWidth/4;;
  float sigmay = roi->miHeight/4;;
#endif

  for(si=0; si<roiny; si++)
  {
#ifdef SPATIAL_PRIOR
    dy = si-rh2;
#endif
    i = roi->miFirstLine+si*ygrid_step;
    for(sj=0; sj<roinx; sj++)
    {
#ifdef SPATIAL_PRIOR
      dx = sj-rw2;
      spatial_prior = exp(-0.5*(dx*dx/sigmax/sigmax+dy*dy/sigmay/sigmay)); ///(2*M_PI*sigmax*sigmay);
#endif

      j = roi->miFirstColumn+sj*xgrid_step;
      b = *iptr++; 
      g = *iptr++;
      r = *iptr++;
      tmpexp = (bp_img->get(j,i)-mfMeanFGVoteErr);
      p_err_fg = 1.0/(sqrtf(2.0*M_PI*mfVarFGVoteErr))*exp(-0.5*(tmpexp*tmpexp)/mfVarFGVoteErr);

      index = mLUT[1]->hsv_bin(r,g,b);

#ifdef SPATIAL_PRIOR
      den = spatial_prior*mHist[0]->hsv_count[1][index]*p_err_fg*0.5 + (1.0-spatial_prior)*mHist[1]->hsv_count[1][index]*p_err_bg*0.5;
#else
      den = mHist[0]->hsv_count[1][index]*p_err_fg*0.5 + mHist[1]->hsv_count[1][index]*p_err_bg*0.5;
#endif
      if (den>0)
#ifdef SPATIAL_PRIOR
	*resptr = spatial_prior*mHist[0]->hsv_count[1][index]*p_err_fg*0.5/den;
#else
	*resptr = mHist[0]->hsv_count[1][index]*p_err_fg*0.5/den;
#endif
      else
	*resptr = 0;

      iptr+=pixelstep;
      resptr+=rpixelstep;
    }
    iptr+=xstep;
    resptr+=rxstep;
  }
  
}


