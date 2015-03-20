//#include <endian.h>
#include <string.h>
#include "Error.h"
#include "Histogram.h"

using namespace TLUtil;
//using namespace std;

namespace TLImageProc
{

Histogram::Histogram(int h_bins, int s_bins, int v_bins, int _niScales, int imgw, int imgh)
{
  niScales = _niScales;
  hsv_count = new valarray<float>[niScales];
  mTmpres = new valarray<float>[niScales];
  miNHSBins = new int[niScales];
  miNVBins = new int[niScales];
  for(int s=0;s<niScales;s++){
    hsv_count[s].resize(h_bins*(s+1)*s_bins*(s+1)+v_bins*(s+1));
    mTmpres[s].resize(h_bins*(s+1)*s_bins*(s+1)+v_bins*(s+1));
    miNHSBins[s] = h_bins*(s+1)*s_bins*(s+1);
    miNVBins[s] = v_bins*(s+1);
  }
  mImageBB.initPosAndSize(0, 0, imgw, imgh);
}

Histogram::~Histogram(){
  delete [] hsv_count;
  delete [] mTmpres;
  delete [] miNHSBins;
  delete [] miNVBins;
}

// Loop over the ROI and compute
// the histogram using the lut
int Histogram::compute(Image<unsigned char>* img, BGR2HSVhistLUT** luts, Rectangle* roi, Image<float>* mask/*=NULL*/, bool set_zero/*=true*/, bool normalise_hist/*=true*/, bool grid/*=true*/){
  if(set_zero){
    setZero();
  }
  roi->intersection(mImageBB);
  if (roi->miFirstLine>2e6 || roi->miFirstColumn>2e6)
    return 0;

  int n_fg_pixels = 0;
  unsigned char r,g,b;
  int i,j,s;
  int rgb=0;
  unsigned char* prgb;
  unsigned char *ptr; 
  float *ptr_mask;
  unsigned char *pdat; 
  float* pmdat;
  int ws = img->widthStep();
  int mws;
  if (mask)
  {
    mws = mask->widthStep();
    pmdat = (float*)mask->data();
  }
  int xstep = ws-roi->miWidth*3;
  pdat = img->data();
  n_fg_pixels = roi->miWidth*roi->miHeight;

  if(mask == NULL){  
    ptr = ((unsigned char *)(pdat + roi->miFirstLine*ws)) + roi->miFirstColumn*3;
    if (grid && n_fg_pixels>600) 
    {
      int xgrid_step = max(1, roi->miWidth/20);
      int ygrid_step = max(1, roi->miHeight/20);
      int roinx = roi->miWidth/xgrid_step;
      int roiny = roi->miHeight/ygrid_step;
      int pixelstep = (xgrid_step-1)*3;
      xstep = ws-xgrid_step*roinx*3 + (ygrid_step-1)*ws;
      for(j=0;j<roiny;j++){
	for(i=0;i<roinx;i++){
	  b = *(ptr++);
	  g = *(ptr++);
	  r = *(ptr++);
	  for(s=0;s<niScales;s++){
	    hsv_count[s][luts[s]->bgr_to_hsv_bin[(((b<<8) + g)<<8) + r]]++;
	  }
	  ptr+=pixelstep;
	}
	ptr+=xstep;
      }
      n_fg_pixels = roinx*roiny;
    }
    else
    {
      for(j=0;j<roi->miHeight;j++){
	for(i=0;i<roi->miWidth;i++){
	  b = *(ptr++);
	  g = *(ptr++);
	  r = *(ptr++);
	  for(s=0;s<niScales;s++){
	    hsv_count[s][luts[s]->bgr_to_hsv_bin[(((b<<8) + g)<<8) + r]]++;
	  }
	  //ptr+=3;
	}
	ptr+=xstep;
      }
      //n_fg_pixels = roi->miWidth*roi->miHeight;
    }
  }
  else {
    ASSERT(mask->width() == img->width() && mask->height() == img->height(), "Image and mask must have the same size. In Histogram::compute().");

    int mxstep = mws-mask->width();
    for(j=roi->miFirstLine;j<roi->miHeight+roi->miFirstLine;j++){
      ptr = ((unsigned char *)(pdat + j*ws)) + roi->miFirstColumn*3;
      //ptr_mask = ((unsigned char *)(pmdat + j*mws)) + roi->miFirstColumn;
      ptr_mask = ((float*)(pmdat) + j*mws) + roi->miFirstColumn;
      for(i=0;i<roi->miWidth;i++){
	b = *(ptr++);
	g = *(ptr++);
	r = *(ptr++);
	if (*ptr_mask>0.5)
	{
	  for(s=0;s<niScales;s++){
	      hsv_count[s][luts[s]->bgr_to_hsv_bin[(((b<<8) + g)<<8) + r]]++;
	  }
	}
	n_fg_pixels++;
	ptr_mask++;
      }
      ptr+=xstep;
      ptr_mask+=mxstep;
    }
  }
  
  // TODO could compute integral before to optimise run-time 
  if(normalise_hist){
    normalise();
  }
  return n_fg_pixels;
}



int Histogram::compute(Image<unsigned char>* img, BGR2HSVhistLUT** luts,
                Rectangle* roi, 
		float kernel_sigma_x, float kernel_sigma_y,
                bool set_zero/*=true*/,
                bool normalise_hist/*=true*/,
		bool grid/*=true*/)
{
  if(set_zero){
    setZero();
  }
  roi->intersection(mImageBB);

  int n_fg_pixels = 0;
  unsigned char r,g,b;
  int i,j,s;
  int rgb=0;
  unsigned char* prgb;
  unsigned char *ptr;
  unsigned char *pdat, *pmdat;
  int ws = img->widthStep();
  int mws;
  int xstep = ws-roi->miWidth*3;
  pdat = img->data();
  n_fg_pixels = roi->miWidth*roi->miHeight;
  float spatial_prior;
  float dx, dy;
  int rw2=roi->miWidth/2, rh2=roi->miHeight/2;


    ptr = ((unsigned char *)(pdat + roi->miFirstLine*ws)) + roi->miFirstColumn*3;
    if (grid && n_fg_pixels>600) 
    {
      int xgrid_step = max(1, roi->miWidth/20);
      int ygrid_step = max(1, roi->miHeight/20);
      int roinx = roi->miWidth/xgrid_step;
      int roiny = roi->miHeight/ygrid_step;
      int pixelstep = (xgrid_step-1)*3;
      xstep = ws-xgrid_step*roinx*3 + (ygrid_step-1)*ws;
      for(j=0;j<roiny;j++){
        dy = j-rh2;
	for(i=0;i<roinx;i++){
	  dx = i-rw2;
	  spatial_prior = exp(-0.5*(dx*dx/kernel_sigma_x/kernel_sigma_x+dy*dy/kernel_sigma_y/kernel_sigma_y));

	  b = *(ptr++);
	  g = *(ptr++);
	  r = *(ptr++);
	  for(s=0;s<niScales;s++){
	    hsv_count[s][luts[s]->bgr_to_hsv_bin[(((b<<8) + g)<<8) + r]]+=spatial_prior;
	  }
	  ptr+=pixelstep;
	}
	ptr+=xstep;
      }
      n_fg_pixels = roinx*roiny;
    }
    else
    {
      for(j=0;j<roi->miHeight;j++){
        dy = j-rh2;
	for(i=0;i<roi->miWidth;i++){
	  dx = i-rw2;
	  spatial_prior = exp(-0.5*(dx*dx/kernel_sigma_x/kernel_sigma_x+dy*dy/kernel_sigma_y/kernel_sigma_y));

	  b = *(ptr++);
	  g = *(ptr++);
	  r = *(ptr++);
	  for(s=0;s<niScales;s++){
	    hsv_count[s][luts[s]->bgr_to_hsv_bin[(((b<<8) + g)<<8) + r]]+=spatial_prior;
	  }
	  //ptr+=3;
	}
	ptr+=xstep;
      }
      //n_fg_pixels = roi->miWidth*roi->miHeight;
    }
  
  // TODO could compute integral before to optimise run-time 
  if(normalise_hist){
    normalise();
  }
  return n_fg_pixels;
}




void Histogram::update(Histogram* h, float factor, bool norm/*=true*/)
{
  int i;
  int totalbins;
  for(int s=0;s<niScales;s++)
  {
    totalbins = miNHSBins[s]+miNVBins[s];
    for(i=0; i<totalbins; i++)
      hsv_count[s][i] += factor*h->hsv_count[s][i];
  }
  if (norm)
    normalise();
}

float Histogram::distance(Histogram* h)
{
  return distanceUnnormalised(h)/niScales;
}

float Histogram::distanceUnnormalised(Histogram* h)
{
  float distance = 0;
  for(int s=0;s<niScales;s++)
  {
    mTmpres[s] = sqrt(hsv_count[s]*h->hsv_count[s]);
    distance += sqrt(1.0-mTmpres[s].sum());
  }
  return max(0.0f, distance);
}

void Histogram::setZero()
{
  for(int s=0;s<niScales;s++){
    hsv_count[s] = 0;
  }
}

void Histogram::normalise(){
  float sum;
  for(int s=0;s<niScales;s++){
    sum = hsv_count[s].sum();
    if (sum>0)
    {
      hsv_count[s]/=sum;
    }
  }    
}

//Histogram* Histogram::createCopy(bool InitZero/*=false*/)
/*
{
  Histogram* newhist;
  if (niScales>0)
    newhist = new Histogram(hists[0]->h_bins, hists[0]->s_bins, hists[0]->v_bins, niScales);
  else
    return NULL;

  if (InitZero)
    setZero();
  else
  {
    for(int s=0;s<niScales;s++)
    {
      int h_bins = hists[s]->h_bins;
      int s_bins = hists[s]->s_bins;
      int v_bins = hists[s]->v_bins;
      for(int i=0;i<v_bins;i++){
	newhist->hists[s]->v_count[i] = hists[s]->v_count[i];
      }

      for(int i=0;i<h_bins;i++){
	for(int j=0;j<s_bins;j++){
	  newhist->hists[s]->hs_count[i*s_bins + j] = hists[s]->hs_count[i*s_bins + j];
	}
      }
    }
  }
  return newhist;
}
*/


}


