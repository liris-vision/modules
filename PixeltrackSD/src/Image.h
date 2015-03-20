#ifndef TL_IMAGE_H
#define TL_IMAGE_H

#include <string.h>
#include "tltypes.h"
#include "Rectangle.h"
#include "Error.h"
#include <opencv2/opencv.hpp>

#ifdef D_API_WIN32
// Windows
#define roundf(x)  (floorf((x) + 0.5))
#endif

using namespace TLUtil;

namespace TLImageProc
{


template<class Type>
class Image
{
  private:
    int miWidth;
    int miHeight;
    int miChannels;
    int miWidthStep;
    bool mbExternalData;;
    Rectangle mBB;
    Type* mData;  

  public:
    Image<Type>(int width, int height, int channels);
    Image<Type>(int width, int height, int channels, int widthstep, Type* data, bool externaldata=true);
    ~Image();

    int height() { return miHeight; };
    int width() { return miWidth; };
    int widthStep() { return miWidthStep; };
    int nChannels() { return miChannels; };
    Type* data() { return mData; };
    Type operator()(int x, int y) { return get(x,y); };
    Type get(int x, int y) { return *(mData+miWidthStep*y + x); };
    Type get(int x, int y, int channel) { return *(mData+miWidthStep*y + miChannels*x + channel); };
    void set(int x, int y, Type value) { ASSERT(miChannels==1, "Image::set() only for single channel images."); *(mData+miWidthStep*y + x) = value; };
    void set(int x, int y, int channel, Type value) { *(mData+miWidthStep*y + x*miChannels + channel) = value; };
    void add(int x, int y, Type value) { *(mData+miWidthStep*y + x) += value; };
    void inc(int x, int y) { (*(mData+miWidthStep*y + x))++; };
    void setColourBGR(int x, int y, Colour c);
    Colour getColourBGR(int x, int y);
    void init(Type value);
    void init(Type value, Rectangle roi);
    void setZero();
    Image<Type>* clone();
    Image<Type>* resize(Rectangle roi, int dest_width, int dest_height);
    Image<Type>* flip();

    Image<unsigned char>* toGreyScale();
    bool toGreyScale(Image<unsigned char>* res);

    void multiply(Type f);
    void multiply(Type f, Image<Type>* result);
    void add(Type a, Rectangle roi);

    void sobelX(Image<short>* result);
    void sobelY(Image<short>* result);
    void average(Image<Type>* result);
    void binarise(float thresh);
    float entropy(Rectangle roi);
    float varianceFromCentre(Rectangle roi);
    Type maxLoc(Rectangle roi, int& maxx, int& maxy);
    void centreOfMass(Rectangle roi, int& maxx, int& maxy);
    Type sum(Rectangle roi);
    int sumGreaterThanThreshold(Rectangle roi, float thresh);

    Image<Type>* equaliseHist();
    float uniformity(Rectangle roi);
    float percentageChanged(Rectangle roi, int offx, int offy, Image<Type>* img);

	/**
	 * Write image data to file.
	 * For floating point image, only xml and yml file name extension is allowed.
	 */
	void writeToFile(const std::string& filename);
};


typedef Image<unsigned char> Image8U;


template<class Type>
Image<Type>::Image(int width, int height, int channels)
{
  mData = new Type[width*height*channels];
  mbExternalData = false;
  miWidth = width;
  miWidthStep = width*channels;
  miHeight = height;
  miChannels = channels;
  mBB.initPosAndSize(0, 0, miWidth, miHeight);
}


template<class Type>
Image<Type>::Image(int width, int height, int channels, int widthstep, Type* data, bool externaldata/*=true*/)
{
  miWidth = width;
  miWidthStep = widthstep;
  miHeight = height;
  miChannels = channels;
  mbExternalData = externaldata;

  if (externaldata)
    mData = data;
  else
  {
    mData = new Type[miHeight*miWidthStep];
    memcpy(mData, data, miHeight*miWidthStep*sizeof(Type));
  }
  mBB.initPosAndSize(0, 0, miWidth, miHeight);
}

template<class Type>
Image<Type>::~Image()
{
  if (!mbExternalData)
    delete [] mData;
}


template<typename Type>
void Image<Type>::init(Type value)
{
  int size = miWidthStep*miHeight;
  Type* ptr = mData;
  for(int i=0; i<size; i++)
    *ptr++ = value;
}


template<typename Type>
void Image<Type>::init(Type value, Rectangle roi)
{
  ASSERT(miChannels==1, "Image::init(Type,Rectangle) requires a single-channel image.");

  roi.intersection(mBB);

  Type* ptr = mData + miWidthStep*roi.miFirstLine + roi.miFirstColumn;
  int linestep = miWidthStep-roi.miWidth;

  for(int i=0; i<roi.miHeight; i++)
  {
    for(int j=0; j<roi.miWidth; j++)
      *ptr++ = value;
    ptr+=linestep;
  }
}


template<typename Type>
void Image<Type>::setZero()
{
  memset(mData, 0, miWidthStep*miHeight*sizeof(Type));
}

template<typename Type>
void Image<Type>::setColourBGR(int x, int y, Colour c)
{
  Type* data_ptr = mData+(y*miWidthStep*sizeof(Type))+x*sizeof(Type)*3;
  data_ptr[0] = c.b;
  data_ptr[1] = c.g;
  data_ptr[2] = c.r;
}

template<typename Type>
Colour Image<Type>::getColourBGR(int x, int y)
{
  Type* data_ptr = mData+(y*miWidthStep*sizeof(Type))+x*sizeof(Type)*3;
  return Colour(data_ptr[2], data_ptr[1], data_ptr[0]);;
}


template<class Type>
Image<Type>* Image<Type>::clone()
{
  return new Image<Type>(miWidth, miHeight, miChannels, miWidthStep, mData, false);
}


template<>
inline bool Image<unsigned char>::toGreyScale(Image<unsigned char>* res)
{
  if (miChannels==3)  // colour image (assuming BGR)
  {
    unsigned char* ptr = mData;
    unsigned char* resptr = res->mData;
    int r, g, b;
    int xstep = miWidthStep-miWidth*3;
    int res_xstep = res->miWidthStep-res->miWidth;
    for(int i=0; i<miHeight; i++)
    {
      for(int j=0; j<miWidth; j++)
      {
	b = *ptr++;
	g = *ptr++;
	r = *ptr++;
        *resptr++ = 0.299*r+0.587*g+0.114*b;
      }
      ptr+=xstep;
      resptr+=res_xstep;
    }
    return true;
  }
  else if (miChannels==1)
  {
    ASSERT(miWidthStep = res->miWidthStep, "Grey scale images have different width step in Image::toGreyScale()");
    memcpy(res->mData, mData, miHeight*miWidthStep);
    return true;
  }
  return false;
}


template<>
inline Image<unsigned char>* Image<unsigned char>::toGreyScale()
{
  Image<unsigned char>* res = new Image<unsigned char>(miWidth, miHeight, 1);
  if (toGreyScale(res))
    return res;
  else 
    return NULL;
}


template<class Type>
void Image<Type>::multiply(Type f)
{
  int size = miWidthStep*miHeight;
  Type* ptr = mData;
  for(int i=0; i<size; i++)
    *ptr++ *= f;
}

template<class Type>
void Image<Type>::multiply(Type f, Image<Type>* result)
{
  int size = miWidthStep*miHeight;
  Type* ptr = mData;
  Type* optr = result->data();
  for(int i=0; i<size; i++)
    *optr++ = *ptr++ * f;
}

template<class Type>
void Image<Type>::add(Type a, Rectangle roi)
{
  ASSERT(miChannels==1, "Image::add() requires a single-channel image.");

  roi.intersection(mBB);

  Type* ptr = mData + miWidthStep*roi.miFirstLine + roi.miFirstColumn;
  int linestep = miWidthStep-roi.miWidth;

  for(int i=0; i<roi.miHeight; i++)
  {
    for(int j=0; j<roi.miWidth; j++)
      *ptr++ += a;
    ptr+=linestep;
  }
}


template<class Type>
void Image<Type>::sobelX(Image<short>* result)
{
  Type *i1, *i2, *i3, *i4, *i5, *i6, *i7, *i8, *i9;
  short *out;
  int instep, outstep;
  int out_widthstep = result->widthStep();;
  Type* input = mData;
  short* output = result->data();

  instep = miWidthStep-miWidth+2;
  outstep = out_widthstep-miWidth+2;

  i1 = input;
  //i2 = input+1;
  i3 = input+2;
  i4 = input+miWidthStep;
  //i5 = input+miWidthStep+1;  // centre pixel
  i6 = input+miWidthStep+2;
  i7 = input+2*miWidthStep;
  //i8 = input+2*miWidthStep+1;
  i9 = input+2*miWidthStep+2;

  out = output+out_widthstep+1;
  for(int i=0; i<miHeight-2; i++)
  {
    for(int j=0; j<miWidth-2; j++)
    {
      *out = (short)(*i3 + 2*(*i6) + *i9 - *i1 - 2*(*i4) - *i7);
      out++;
      i1++; i3++; i4++; i6++; i7++; i9++;
    }
    out+=outstep;
    i1+=instep;
    //i2+=instep;
    i3+=instep;
    i4+=instep;
    //i5+=instep;
    i6+=instep;
    i7+=instep;
    //i8+=instep;
    i9+=instep;
  }
}


template<class Type>
void Image<Type>::sobelY(Image<short>* result)
{
  Type *i1, *i2, *i3, *i4, *i5, *i6, *i7, *i8, *i9;
  short *out;
  int instep, outstep;
  int out_widthstep = result->widthStep();;
  Type* input = mData;
  short* output = result->data();

  instep = miWidthStep-miWidth+2;
  outstep = out_widthstep-miWidth+2;

  i1 = input;
  i2 = input+1;
  i3 = input+2;
  //i4 = input+miWidthStep;
  //i5 = input+miWidthStep+1; // centre pixel
  //i6 = input+miWidthStep+2;
  i7 = input+2*miWidthStep;
  i8 = input+2*miWidthStep+1;
  i9 = input+2*miWidthStep+2;

  out = output+out_widthstep+1;
  for(int i=0; i<miHeight-2; i++)
  {
    for(int j=0; j<miWidth-2; j++)
    {
      *out = (short)(*i7 + 2*(*i8) + *i9 - *i1 - 2*(*i2) - *i3);
      out++;
      i1++; i2++; i3++; i7++; i8++; i9++;
    }
    out+=outstep;
    i1+=instep;
    i2+=instep;
    i3+=instep;
    //i4+=instep;
    //i5+=instep;
    //i6+=instep;
    i7+=instep;
    i8+=instep;
    i9+=instep;
  }
}


template<class Type>
void Image<Type>::average(Image<Type>* result)
{
  Type *i1, *i2, *i3, *i4, *i5, *i6, *i7, *i8, *i9;
  short *out;
  int instep, outstep;
  int out_widthstep = result->miWidthStep;
  Type* input = mData;
  Type* output = result->mData;

  instep = miWidthStep-miWidth+2;
  outstep = out_widthstep-miWidth+2;

  i1 = input;
  i2 = input+1;
  i3 = input+2;
  i4 = input+miWidthStep;
  i5 = input+miWidthStep+1; // centre pixel
  i6 = input+miWidthStep+2;
  i7 = input+2*miWidthStep;
  i8 = input+2*miWidthStep+1;
  i9 = input+2*miWidthStep+2;

  out = output+out_widthstep+1;
  for(int i=0; i<miHeight-2; i++)
  {
    for(int j=0; j<miWidth-2; j++)
    {
      *out = (Type)(((double) *i1 + *i2 + *i3 + *i4 + *i5 + *i6 + *i7 + *i8 + *i9)*0.111111111);
      out++;
      i1++; i2++; i3++; i4++; i5++; i6++; i7++; i8++; i9++;
    }
    out+=outstep;
    i1+=instep;
    i2+=instep;
    i3+=instep;
    i4+=instep;
    i5+=instep;
    i6+=instep;
    i7+=instep;
    i8+=instep;
    i9+=instep;
  }
}

template<class Type>
Type Image<Type>::maxLoc(Rectangle roi, int& maxx, int& maxy)
{
  ASSERT(miChannels==1, "Image::maxLoc() requires a single-channel image.");

  roi.intersection(mBB);

  Type* ptr = mData + miWidthStep*roi.miFirstLine + roi.miFirstColumn;
  int linestep = miWidthStep-roi.miWidth;
  Type maxval = *ptr;
  Type val;
  int fl, fc, ll, lc;
  fl = roi.miFirstLine;
  fc = roi.miFirstColumn;
  ll = roi.lastLine();
  lc = roi.lastColumn();

  for(int i=fl; i<=ll; i++)
  {
    for(int j=fc; j<=lc; j++)
    {
      val = *ptr++;
      if (val>maxval)
      {
	maxval = val;
	maxx = j;
	maxy = i;
      }
    }
    ptr+=linestep;
  }
  return maxval;
}


template<class Type>
void Image<Type>::centreOfMass(Rectangle roi, int& maxx, int& maxy)
{
  ASSERT(miChannels==1, "Image::centreOfMass() requires a single-channel image.");

  roi.intersection(mBB);

  Type* ptr = mData + miWidthStep*roi.miFirstLine + roi.miFirstColumn;
  int linestep = miWidthStep-roi.miWidth;
  Type maxval = *ptr;
  Type val;
  int fl, fc, ll, lc;
  fl = roi.miFirstLine;
  fc = roi.miFirstColumn;
  ll = roi.lastLine();
  lc = roi.lastColumn();
  float xsum=0, ysum=0;
  float fmaxx, fmaxy;
  fmaxx=fmaxy=0;

  for(int i=fl; i<=ll; i++)
  {
    for(int j=fc; j<=lc; j++)
    {
      val = *ptr++;
      fmaxx+=val*j;
      fmaxy+=val*i;
      xsum+=val;
      ysum+=val;
    }
    ptr+=linestep;
  }
  if (xsum==0 || ysum==0)
  {
    maxx = roi.centerX();
    maxy = roi.centerY();
  }
  else
  {
    maxx=roundf(fmaxx/xsum);
    maxy=roundf(fmaxy/ysum);
  }
}



template<class Type>
void Image<Type>::binarise(float thresh)
{
  ASSERT(miChannels==1, "Image::maxLoc() requires a single-channel image.");

  Type* ptr = mData;
  Type val;
  int linestep = miWidthStep-miWidth;

  for(int i=0; i<miHeight; i++)
  {
    for(int j=0; j<miWidth; j++)
    {
      val = *ptr;
      if (val>thresh)
	*ptr = 1.0;
      else
	*ptr = 0.0;
      ptr++; 
    }
    ptr+=linestep;
  }
}



template<class Type>
float Image<Type>::entropy(Rectangle roi)
{
  ASSERT(miChannels==1, "Image::maxLoc() requires a single-channel image.");

  roi.intersection(mBB);

  Type* ptr = mData + miWidthStep*roi.miFirstLine + roi.miFirstColumn;
  int linestep = miWidthStep-roi.miWidth;
  Type val;
  int fl, fc, ll, lc;
  fl = roi.miFirstLine;
  fc = roi.miFirstColumn;
  ll = roi.lastLine();
  lc = roi.lastColumn();
  float result;
  float sumroi = sum(roi); 

  for(int i=fl; i<=ll; i++)
  {
    for(int j=fc; j<=lc; j++)
    {
      val = *ptr++;
      val/=sumroi;
      if (val>0)
	result -= val*log(val);
    }
    ptr+=linestep;
  }
  return result/roi.area();
}

template<class Type>
Type Image<Type>::sum(Rectangle roi)
{
  ASSERT(miChannels==1, "Image::sum() requires a single-channel image.");

  roi.intersection(mBB);

  Type* ptr = mData + miWidthStep*roi.miFirstLine + roi.miFirstColumn;
  int linestep = miWidthStep-roi.miWidth;
  Type val;
  int fl, fc, ll, lc;
  fl = roi.miFirstLine;
  fc = roi.miFirstColumn;
  ll = roi.lastLine();
  lc = roi.lastColumn();
  Type result=0;

  for(int i=fl; i<=ll; i++)
  {
    for(int j=fc; j<=lc; j++)
    {
      result += *ptr++;
    }
    ptr+=linestep;
  }
  return result;
}



template<class Type>
float Image<Type>::varianceFromCentre(Rectangle roi)
{
  ASSERT(miChannels==1, "Image::maxLoc() requires a single-channel image.");

  roi.intersection(mBB);

  Type* ptr = mData + miWidthStep*roi.miFirstLine + roi.miFirstColumn;
  int linestep = miWidthStep-roi.miWidth;
  Type val;
  int fl, fc, ll, lc;
  fl = roi.miFirstLine;
  fc = roi.miFirstColumn;
  ll = roi.lastLine();
  lc = roi.lastColumn();
  float result=0;
  float dx, dy;
  float w2 = 0.5*roi.miWidth, h2 = 0.5*roi.miHeight;
  float dist;
  float sumroi = sum(roi); 
  if (sumroi==0)
    return 0;

  for(int i=fl; i<=ll; i++)
  {
    dy = i-h2;
    for(int j=fc; j<=lc; j++)
    {
      dx = j-w2;
      dist = (dx*dx+dy*dy);

      val = *ptr++/sumroi;
      result += dist*val;
    }
    ptr+=linestep;
  }
  return result;
}



template<class Type>
Image<Type>* Image<Type>::resize(Rectangle roi, int dest_width, int dest_height)
{
  Image<Type>* dest = new Image<Type>(dest_width, dest_height, miChannels);
  float source_pixsize_x = (float)roi.miWidth/dest_width;
  float source_pixsize_y = (float)roi.miHeight/dest_height;
  float source_x, source_y;
  int isource_x, isource_y;

  // no interpolation !!!
  source_y=roi.miFirstLine;
  for(int i=0; i<dest_height; i++)
  {
    isource_y = int(source_y+.5);
    source_x=roi.miFirstColumn;
    for(int j=0; j<dest_width; j++)
    {
      isource_x = int(source_x+.5)*miChannels;
      for(int c=0; c<miChannels; c++)
      {
        dest->set(j, i, c, *(mData+miWidthStep*isource_y + isource_x + c)); 
      }
      source_x += source_pixsize_x;
    }
    source_y += source_pixsize_y;
  }

  return dest;
}

template<class Type>
Image<Type>* Image<Type>::flip()
{
  Image<Type>* res = new Image<Type>(miWidth, miHeight, miChannels);
  Type* ptr;
  Type* resptr = res->data();
  int xstep = miWidthStep - miWidth*miChannels;
  int resxstep = res->widthStep() - res->width()*res->nChannels();

  for(int i=1; i<=miHeight; i++)
  {
    ptr = mData+(i*miWidthStep) - xstep;
    for(int j=0; j<miWidth; j++)
    {
      *resptr++ = *ptr--;
    }
    resptr+= resxstep;
  }
  return res;
}


template<class Type>
Image<Type>* Image<Type>::equaliseHist()
{
  ASSERT(miChannels==1, "Image::equaliseHist() requires a single-channel image.");

  Image<Type>* res = new Image<Type>(miWidth, miHeight, miChannels);
  Type* ptr = mData;;
  Type* resptr = res->data();
  int xstep = miWidthStep - miWidth*miChannels;
  int resxstep = res->widthStep() - res->width()*res->nChannels();

  float orig_hist[256];
  float factor = 255.0/(miWidth*miHeight);
  float cum_hist[256];
  int i, j;

  for(i=1; i<256; i++)
    orig_hist[i] = 0;
  for(i=1; i<=miHeight; i++)
  {
    for(j=0; j<miWidth; j++)
      orig_hist[(int)*ptr++]+=factor;
    ptr += xstep;
  }
  cum_hist[0]=orig_hist[0];
  for(i=1; i<256; i++)
    cum_hist[i] = cum_hist[i-1] + orig_hist[i];

  ptr = mData;
  for(i=1; i<=miHeight; i++)
  {
    for(j=0; j<miWidth; j++)
      *resptr++ = (Type)cum_hist[*ptr++];
    ptr += xstep;
    resptr+= resxstep;
  }
  return res;
}


template<class Type>
float Image<Type>::uniformity(Rectangle roi)
{
  ASSERT(miChannels==1, "Image::uniformity() requires a single-channel image.");

  roi.intersection(mBB);
  if (roi.area()==0)
    return 1.0;

  float res=0;
  Type* ptr = mData + miWidthStep*roi.miFirstLine + roi.miFirstColumn;
  //int xstep = miWidthStep - miWidth*miChannels;
  Type prevval = *ptr;;
  int linestep = miWidthStep-roi.miWidth;
  float changes=0;
  int nonzeros=0;
  int i,j;

  for(i=1; i<=roi.miHeight; i++)
  {
    for(j=0; j<roi.miWidth; j++)
    {
      if (*ptr!=prevval)
        changes++;
      if (*ptr>0.01)
	nonzeros++;
      prevval = *ptr++;
    }
    ptr += linestep;
  }
  res = 1.0-changes/roi.area();
  //res = max(0.0, 1.0-changes/nonzeros);
  return res;
}

template<class Type>
int Image<Type>::sumGreaterThanThreshold(Rectangle roi, float thresh)
{
  ASSERT(miChannels==1, "Image::maxLoc() requires a single-channel image.");

  roi.intersection(mBB);

  Type* ptr = mData + miWidthStep*roi.miFirstLine + roi.miFirstColumn;
  int linestep = miWidthStep-roi.miWidth;
  Type val;
  int fl, fc, ll, lc;
  fl = roi.miFirstLine;
  fc = roi.miFirstColumn;
  ll = roi.lastLine();
  lc = roi.lastColumn();
  int result=0;

  for(int i=fl; i<=ll; i++)
  {
    for(int j=fc; j<=lc; j++)
    {
      if (*ptr++>thresh)
	result++;
    }
    ptr+=linestep;
  }
  return result;
}


template<class Type>
float Image<Type>::percentageChanged(Rectangle roi, int offx, int offy, Image<Type>* img)
{
  ASSERT(miChannels==1, "Image::percentageChanged() requires a single-channel image.");
  roi.intersection(mBB);
  roi.translate(offx, offy);
  roi.intersection(mBB);
  roi.translate(-offx, -offy);

  float res=0;
  Type* ptr = mData + miWidthStep*(roi.miFirstLine+offy) + roi.miFirstColumn+offx;
  Type* imgptr = img->mData + img->miWidthStep*roi.miFirstLine + roi.miFirstColumn;
  int linestep = miWidthStep-roi.miWidth;
  int imglinestep = img->miWidthStep-roi.miWidth;
  float changes=0;
  int i,j;

  for(i=1; i<=roi.miHeight; i++)
  {
    for(j=0; j<roi.miWidth; j++)
    {
      if (fabs(*ptr-*imgptr)>0.5)
	changes++;
      ptr++;
      imgptr++;
    }
    ptr += linestep;
    imgptr += imglinestep;
  }
  res = changes/roi.area();
  return res;
}

// generic case
template<class Type>
void Image<Type>::writeToFile(const std::string& filename)
{
	std::cout << "Image<Type>::writeToFile: not implemented for this type" << std::endl;
}

// specialization for unsigned char
template<>
inline void Image<unsigned char>::writeToFile(const std::string& filename)
{
	cv::Mat m( this->height(), this->width(), this->nChannels()==3?CV_8UC3:CV_8UC1, this->data(), this->widthStep());
	cv::imwrite( filename, m);
}

// specialization for float
template<>
inline void Image<float>::writeToFile(const std::string& filename)
{
	cv::Mat m( this->height(), this->width(), CV_32F, this->data(), this->widthStep()*sizeof(float));

	// use xml or yml extension only
	cv::FileStorage file( filename, cv::FileStorage::WRITE);
	file << "image";
	file << m;
}

} // namespace

#endif
