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

#include <algorithm>
#include <float.h>
#include "tltypes.h"
#include "HSVPixelGradientModel.h"
#include "BGR2HSVdistLUT.h"

using namespace std;
//using namespace cv;



bool disp_less(displacement_t lhs, displacement_t rhs)
{
  if (lhs.y<rhs.y)
    return true;
  else if (lhs.y>rhs.y)
    return false;
  else return (lhs.x<rhs.x);
};

bool disp_less_count(displacement_t lhs, displacement_t rhs)
{
  return (lhs.count<rhs.count);
};




HSVPixelGradientModel::HSVPixelGradientModel(int nb_hsbins, int nb_vbins, int nb_obins, int nb_mbins, float mag_thresh)
{
  h_bins = nb_hsbins;
  s_bins = nb_hsbins;
  v_bins = nb_vbins;
  o_bins = nb_obins;
  m_bins = nb_mbins;
  maxcolourbin = h_bins*s_bins+v_bins;
  totalbins = (h_bins*s_bins+v_bins)*(o_bins*m_bins+1);
  magnitude_threshold = mag_thresh;

  disp = new list<displacement_t>*[totalbins];
  for(int i=0; i<totalbins; i++)
    disp[i] = new list<displacement_t>;

  m_LUTColour = new BGR2HSVdistLUT(h_bins, s_bins, v_bins); 
  m_LUTGradient = new GradDispLUT(o_bins, m_bins); 
  reset();
}

HSVPixelGradientModel::~HSVPixelGradientModel()
{
  for(int i=0; i<totalbins; i++)
    delete disp[i];
  delete [] disp;
  delete m_LUTColour;
  delete m_LUTGradient;
}

void HSVPixelGradientModel::reset()
{
  for(int i=0; i<totalbins; i++)
    disp[i]->clear();
}


void HSVPixelGradientModel::learn(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* segmentation)
{
  int curline, curcol;
  unsigned int index_colour, index_gradient, index;
  displacement_t cur_disp;
  list<displacement_t>::iterator it;
  int pi, pj;
  Colour pix;
  short gx, gy;
  int ii, jj;
  Rectangle imageBB;
  imageBB.initPosAndSize(1, 1, img->width()-2, img->height()-2);
  bb.intersection(imageBB);
  int ll=bb.lastLine(), lc = bb.lastColumn();

  ii=0;
  for(int i=bb.miFirstLine; i<ll; i++)
  {
    jj=0;
    for(int j=bb.miFirstColumn; j<lc; j++)
    {
      if (segmentation->get(j, i)>0.5)
      {
      pix = img->getColourBGR(j, i);
      gx = xgradimg->get(j, i);
      gy = ygradimg->get(j, i);
      
      index_colour = m_LUTColour->hsv_bin(pix.r, pix.g, pix.b);
      index_gradient = m_LUTGradient->get_bin(gx, gy);
      index = index_gradient*maxcolourbin+index_colour;
      cur_disp.y=ii-bb.miHeight/2; 
      cur_disp.x=jj-bb.miWidth/2; 
      cur_disp.x = cur_disp.x/CLUSTER_SIZE;
      cur_disp.y = cur_disp.y/CLUSTER_SIZE;

      it = find(disp[index]->begin(), disp[index]->end(), cur_disp);
      if (it==disp[index]->end())
      {
	cur_disp.count=1;
	disp[index]->push_back(cur_disp);
      }
      else
        it->count++;
      }
      jj++;
    }
    ii++;
  }
  for(int i=0; i<totalbins; i++)
    disp[i]->sort(disp_less_count); 
}


void HSVPixelGradientModel::vote(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* voting_map)
{
  int lastline, lastcol;
  int index_colour, index_gradient, index; 
  displacement_t cur_disp;
  int width, height;
  width = img->width();
  height = img->height();
  Rectangle imgBB(1, 1, width-1, height-1);
  bb.intersection(imgBB); 
  lastline = bb.lastLine();
  lastcol = bb.lastColumn();
  list<displacement_t>::iterator it;
  list<displacement_t>::iterator beg;
  int counter;
  unsigned char r, g, b;
  float* voting_ptr;
  int nx, ny;
  int ii, jj;
  float xtrans, ytrans;
  int i, j;
  int si, sj;
  int xgrid_step = 1; //max(1,bb.miWidth/30);
  int ygrid_step = 1; //max(1,bb.miHeight/30);
  unsigned char* iptr;
  short *xptr, *yptr;
  int ws = img->widthStep();
  int vmws = voting_map->widthStep();
  int roinx = bb.miWidth/xgrid_step;
  int roiny = bb.miHeight/ygrid_step;
  int pixelstep = (xgrid_step-1)*3;
  int xstep = ws-xgrid_step*roinx*3 + (ygrid_step-1)*ws;
  int gradpixelstep = (xgrid_step-1);
  int gradxstep = xgradimg->widthStep()-xgrid_step*roinx + (ygrid_step-1)*xgradimg->widthStep();
  iptr = (unsigned char*)img->data()+bb.miFirstLine*ws + bb.miFirstColumn*3;
  xptr = xgradimg->data()+bb.miFirstLine*xgradimg->widthStep() + bb.miFirstColumn;
  yptr = ygradimg->data()+bb.miFirstLine*ygradimg->widthStep() + bb.miFirstColumn;
  float* vmd = voting_map->data();
  short gx, gy;

  for(si=0; si<roiny; si++)
  {
    i=bb.miFirstLine+si*ygrid_step;
    for(sj=0; sj<roinx; sj++)
    {
      j=bb.miFirstColumn+sj*xgrid_step;
      b = *iptr++; 
      g = *iptr++;
      r = *iptr++;
      gx = *xptr++;
      gy = *yptr++;

      index_colour = m_LUTColour->hsv_bin(r, g, b);
      index_gradient = m_LUTGradient->get_bin(gx, gy);
      index = index_gradient*maxcolourbin+index_colour;
      it=disp[index]->end();
      beg=disp[index]->begin();
      counter=0;
      while (counter<MAXVOTES && it!=beg)
      {
	--it;
	ny = int(i-it->y*CLUSTER_SIZE);
	nx = int(j-it->x*CLUSTER_SIZE);
	if (ny>0 && nx>0 && ny<height && nx<width)
	{
	  voting_ptr = &(( (float*)(vmd + vmws*(ny)) )[nx]);
	  *voting_ptr += it->count;
	}
	counter++;
      }
      iptr+=pixelstep;
      xptr+=gradpixelstep;
      yptr+=gradpixelstep;
    }
    iptr+=xstep;
    xptr+=gradxstep;
    yptr+=gradxstep;
  }
}

void HSVPixelGradientModel::vote(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* voting_map, float scale, float angle)
{
  int lastline, lastcol;
  int index_colour, index_gradient, index; 
  displacement_t cur_disp;
  int width, height;
  width = img->width();
  height = img->height();
  Rectangle imgBB(1, 1, width-1, height-1);
  bb.intersection(imgBB); 
  lastline = bb.lastLine();
  lastcol = bb.lastColumn();
  list<displacement_t>::iterator it;
  list<displacement_t>::iterator beg;
  int counter;
  unsigned char r, g, b;
  float* voting_ptr;
  int nx, ny;
  int ii, jj;
  float xtrans, ytrans;
  int i, j;
  int si, sj;
  int xgrid_step = 1; //max(1,bb.miWidth/30);
  int ygrid_step = 1; //max(1,bb.miHeight/30);
  unsigned char* iptr;
  short *xptr, *yptr;
  int ws = img->widthStep();
  int vmws = voting_map->widthStep();
  int roinx = bb.miWidth/xgrid_step;
  int roiny = bb.miHeight/ygrid_step;
  int pixelstep = (xgrid_step-1)*3;
  int xstep = ws-xgrid_step*roinx*3 + (ygrid_step-1)*ws;
  int gradpixelstep = (xgrid_step-1);
  int gradxstep = xgradimg->widthStep()-xgrid_step*roinx + (ygrid_step-1)*xgradimg->widthStep();
  iptr = (unsigned char*)img->data()+bb.miFirstLine*ws + bb.miFirstColumn*3;
  xptr = xgradimg->data()+bb.miFirstLine*xgradimg->widthStep() + bb.miFirstColumn;
  yptr = ygradimg->data()+bb.miFirstLine*ygradimg->widthStep() + bb.miFirstColumn;
  float* vmd = voting_map->data();
  short gx, gy;

  for(si=0; si<roiny; si++)
  {
    i=bb.miFirstLine+si*ygrid_step;
    for(sj=0; sj<roinx; sj++)
    {
      j=bb.miFirstColumn+sj*xgrid_step;
      b = *iptr++; 
      g = *iptr++;
      r = *iptr++;
      gx = *xptr++;
      gy = *yptr++;

      index_colour = m_LUTColour->hsv_bin(r, g, b);
      index_gradient = m_LUTGradient->get_bin(gx, gy);
      index = index_gradient*maxcolourbin+index_colour;
      it=disp[index]->end();
      beg=disp[index]->begin();
      counter=0;
      while (counter<MAXVOTES && it!=beg)
      {
	--it;
	ny = int(i-it->y*CLUSTER_SIZE*scale);
	nx = int(j-it->x*CLUSTER_SIZE*scale);
	if (ny>0 && nx>0 && ny<height && nx<width)
	{
	  voting_ptr = &(( (float*)(vmd + vmws*(ny)) )[nx]);
	  *voting_ptr += it->count;
	}
	counter++;
      }
      iptr+=pixelstep;
      xptr+=gradpixelstep;
      yptr+=gradpixelstep;
    }
    iptr+=xstep;
    xptr+=gradxstep;
    yptr+=gradxstep;
  }
}

void HSVPixelGradientModel::backproject(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* bpimg, int maxlocx, int maxlocy)
{
  int lastline, lastcol;
  int index_colour, index_gradient, index; 
  displacement_t cur_disp;
  int width, height;
  width = img->width();
  height = img->height();
  Rectangle imgBB(1, 1, width-1, height-1);
  bb.intersection(imgBB); 
  lastline = bb.lastLine();
  lastcol = bb.lastColumn();
  list<displacement_t>::iterator it;
  list<displacement_t>::iterator beg;
  int counter;
  unsigned char r, g, b;
  float* voting_ptr;
  int nx, ny;
  int ii, jj;
  int i, j;
  int si, sj;
  int xgrid_step = 1; //max(1,bb.miWidth/30);
  int ygrid_step = 1; //max(1,bb.miHeight/30);
  unsigned char* iptr;
  short *xptr, *yptr;
  int ws = img->widthStep();
  int roinx = bb.miWidth/xgrid_step;
  int roiny = bb.miHeight/ygrid_step;
  int pixelstep = (xgrid_step-1)*3;
  int xstep = ws-xgrid_step*roinx*3 + (ygrid_step-1)*ws;
  int gradpixelstep = (xgrid_step);
  int gradxstep = xgradimg->widthStep()-xgrid_step*roinx + (ygrid_step-1)*xgradimg->widthStep();
  iptr = (unsigned char*)img->data()+bb.miFirstLine*ws + bb.miFirstColumn*3;
  xptr = xgradimg->data()+bb.miFirstLine*xgradimg->widthStep() + bb.miFirstColumn;
  yptr = ygradimg->data()+bb.miFirstLine*ygradimg->widthStep() + bb.miFirstColumn;
  short gx, gy;
  int i_minus_maxlocy, j_minus_maxlocx;
  int xtrans, ytrans;
  float center_dist;

  for(si=0; si<roiny; si++)
  {
    i=bb.miFirstLine+si*ygrid_step;
    i_minus_maxlocy = i-maxlocy;
    for(sj=0; sj<roinx; sj++)
    {
      j=bb.miFirstColumn+sj*xgrid_step;
      j_minus_maxlocx = j-maxlocx;
      b = *iptr++; 
      g = *iptr++;
      r = *iptr++;
      gx = *xptr;
      gy = *yptr;

      index_colour = m_LUTColour->hsv_bin(r, g, b);
      index_gradient = m_LUTGradient->get_bin(gx, gy);
      index = index_gradient*maxcolourbin+index_colour;
      it=disp[index]->end();
      beg=disp[index]->begin();
      center_dist=0;
      counter=0;
      while (counter<MAXVOTES && it!=beg)
      {
	--it;
	xtrans = it->x*CLUSTER_SIZE; 
	ytrans = it->y*CLUSTER_SIZE; 
	center_dist += sqrtf((ytrans-i_minus_maxlocy)*(ytrans-i_minus_maxlocy) + (xtrans-j_minus_maxlocx)*(xtrans-j_minus_maxlocx));
	if (center_dist<CLUSTER_SIZE)
	  counter+=it->count;
      }
      if (counter>0)
	bpimg->set(j, i, counter*exp(-0.3*center_dist/CLUSTER_SIZE));

      iptr+=pixelstep;
      xptr+=gradpixelstep;
      yptr+=gradpixelstep;
    }
    iptr+=xstep;
    xptr+=gradxstep;
    yptr+=gradxstep;
  }
}


void HSVPixelGradientModel::backproject(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* bpimg, int maxlocx, int maxlocy, Image<float>* segmentation, float& mean_pos, float& variance_pos, float& mean_neg, float& variance_neg)
{
  int lastline, lastcol;
  int index_colour, index_gradient, index; 
  displacement_t cur_disp;
  int width, height;
  width = img->width();
  height = img->height();
  Rectangle imgBB(1, 1, width-1, height-1);
  bb.intersection(imgBB); 
  lastline = bb.lastLine();
  lastcol = bb.lastColumn();
  list<displacement_t>::iterator it;
  list<displacement_t>::iterator beg;
  int counter;
  unsigned char r, g, b;
  float* voting_ptr;
  int nx, ny;
  int ii, jj;
  int i, j;
  int si, sj;
  int xgrid_step = 1; //max(1,bb.miWidth/30);
  int ygrid_step = 1; //max(1,bb.miHeight/30);
  unsigned char* iptr;
  short *xptr, *yptr;
  int ws = img->widthStep();
  int roinx = bb.miWidth/xgrid_step;
  int roiny = bb.miHeight/ygrid_step;
  int pixelstep = (xgrid_step-1)*3;
  int xstep = ws-xgrid_step*roinx*3 + (ygrid_step-1)*ws;
  int gradpixelstep = (xgrid_step);
  int gradxstep = xgradimg->widthStep()-xgrid_step*roinx + (ygrid_step-1)*xgradimg->widthStep();
  iptr = (unsigned char*)img->data()+bb.miFirstLine*ws + bb.miFirstColumn*3;
  xptr = xgradimg->data()+bb.miFirstLine*xgradimg->widthStep() + bb.miFirstColumn;
  yptr = ygradimg->data()+bb.miFirstLine*ygradimg->widthStep() + bb.miFirstColumn;
  short gx, gy;
  int i_minus_maxlocy, j_minus_maxlocx;
  int xtrans, ytrans;
  float vote_err_pos=0, vote_err_pos2=0;
  float vote_err_neg=0, vote_err_neg2=0;
  float fg, bg, fg_sum=0, bg_sum=0;
  float center_dist;
  float mcd;

  for(si=0; si<roiny; si++)
  {
    i=bb.miFirstLine+si*ygrid_step;
    i_minus_maxlocy = i-maxlocy;
    for(sj=0; sj<roinx; sj++)
    {
      j=bb.miFirstColumn+sj*xgrid_step;
      j_minus_maxlocx = j-maxlocx;
      fg = segmentation->get(j, i);
      bg = 1.0-fg;

      b = *iptr++; 
      g = *iptr++;
      r = *iptr++;
      gx = *xptr;
      gy = *yptr;

      index_colour = m_LUTColour->hsv_bin(r, g, b);
      index_gradient = m_LUTGradient->get_bin(gx, gy);
      index = index_gradient*maxcolourbin+index_colour;
      it=disp[index]->end();
      beg=disp[index]->begin();
      center_dist=0;
      mcd=FLT_MAX;;
      counter=0;
      int count_votes=0;
      if (fg>0.5)
      {
	while (counter<MAXVOTES && it!=beg)
	{
	  --it;
	  xtrans = it->x*CLUSTER_SIZE; 
	  ytrans = it->y*CLUSTER_SIZE; 
	  center_dist += sqrtf((ytrans-i_minus_maxlocy)*(ytrans-i_minus_maxlocy) + (xtrans-j_minus_maxlocx)*(xtrans-j_minus_maxlocx));
	  count_votes++;
	  if (center_dist<CLUSTER_SIZE)
	    counter+=it->count;
	}
	if (counter>0)
	{
	  vote_err_pos += fg*center_dist/count_votes;
	  vote_err_pos2 += fg*center_dist*center_dist/count_votes/count_votes;
	  fg_sum += fg;
	  bpimg->set(j, i, counter);
	}
      }
      else
      {
	while (counter<MAXVOTES && it!=beg)
	{
	  --it;
	  xtrans = it->x*CLUSTER_SIZE; 
	  ytrans = it->y*CLUSTER_SIZE; 
	  center_dist += sqrtf((ytrans-i_minus_maxlocy)*(ytrans-i_minus_maxlocy) + (xtrans-j_minus_maxlocx)*(xtrans-j_minus_maxlocx));
	  count_votes++;
	  if (center_dist<CLUSTER_SIZE)
	    counter+=it->count;
	}
	if (counter>0)
	{
	  vote_err_neg += bg*center_dist/count_votes;
	  vote_err_neg2 += bg*center_dist*center_dist/count_votes/count_votes;
	  bg_sum += bg;
	  bpimg->set(j, i, counter);
	}
      }
      iptr+=pixelstep;
      xptr+=gradpixelstep;
      yptr+=gradpixelstep;
    }
    iptr+=xstep;
    xptr+=gradxstep;
    yptr+=gradxstep;
  }
  if (fg_sum>0)
  {
    // we force the mean to be zero
    mean_pos = 0; //vote_err_pos/fg_sum;
    variance_pos = vote_err_pos2/fg_sum - mean_pos*mean_pos;
  }
  if (bg_sum>0)
  {
    mean_neg = vote_err_neg/bg_sum;
    variance_neg = vote_err_neg2/bg_sum - mean_neg*mean_neg;
  }
  cout << "voting error positive:  mean: " << mean_pos << endl;
  cout << "voting error positive:  var:  " << variance_pos << endl;
  cout << "voting error negative:  mean: " << mean_neg << endl;
  cout << "voting error negative:  var:  " << variance_neg << endl;
}





void HSVPixelGradientModel::update(Image8U* img, Image<short>* xgradimg, Image<short>* ygradimg, Rectangle bb, Image<float>* segmentation, float update_factor)
{
  int curline, curcol;
  unsigned int index_colour, index_gradient, index;
  displacement_t cur_disp;
  list<displacement_t>::iterator it;
  int pi, pj;
  Colour pix;
  short gx, gy;
  int ii, jj;
  float one_minus_uf = 1.0-update_factor;
  Rectangle imageBB;
  imageBB.initPosAndSize(1, 1, img->width()-2, img->height()-2);
  bb.intersection(imageBB);
  int ll=bb.lastLine(), lc = bb.lastColumn();

  // down-weight all previous votes
  for(int i=0; i<totalbins; i++)
    for(it=disp[i]->begin(); it!=disp[i]->end(); ++it)
      it->count*=one_minus_uf;

  ii=0;
  for(int i=bb.miFirstLine; i<ll; i++)
  {
    jj=0;
    for(int j=bb.miFirstColumn; j<lc; j++)
    {
      if (segmentation->get(j, i)>0.3)
      {
      pix = img->getColourBGR(j, i);
      gx = xgradimg->get(j, i);
      gy = ygradimg->get(j, i);
      
      index_colour = m_LUTColour->hsv_bin(pix.r, pix.g, pix.b);
      index_gradient = m_LUTGradient->get_bin(gx, gy);
      index = index_gradient*maxcolourbin+index_colour;
      cur_disp.y=ii-bb.miHeight/2; 
      cur_disp.x=jj-bb.miWidth/2; 
      cur_disp.x = cur_disp.x/CLUSTER_SIZE;
      cur_disp.y = cur_disp.y/CLUSTER_SIZE;

      it = find(disp[index]->begin(), disp[index]->end(), cur_disp);
      if (it==disp[index]->end())
      {
	cur_disp.count=1.0*update_factor*segmentation->get(j,i); 
	disp[index]->push_back(cur_disp);
      }
      else
        it->count=update_factor*segmentation->get(j,i);
      }
      jj++;
    }
    ii++;
  }
  for(int i=0; i<totalbins; i++)
  {
    disp[i]->sort(disp_less_count); 
    // cut off irrelevant votes
    it = disp[i]->end();
	for(int di=0; it!=disp[i]->begin() && di<MAXVOTES; di++)
      --it;
    disp[i]->erase(disp[i]->begin(), it);
  }
}


