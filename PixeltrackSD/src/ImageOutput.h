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

#ifndef TL_IMAGE_OUTPUT_H
#define TL_IMAGE_OUTPUT_H

#include <string>
#include <opencv2/opencv.hpp>

#include "Image.h"
#include "Rectangle.h"

using namespace std;
//using namespace cv;
using namespace TLImageProc;

namespace TLInOut
{

  class ImageOutput
  {
    private:
      string msWindowName;
      Image8U* mCurrentImage;

      static void mouseHandler(int event, int x, int y, int flags, void* param);

    public:
      ImageOutput(string window_name);
      ~ImageOutput();

      string windowName() { return msWindowName; };
      void setCurrentImage(Image8U* img) { mCurrentImage = img; };

      /** 
       * Shows a colour image and waites @a wait_ms. 
       * @param wait_ms Time in ms to wait after displaying the image. 
       *   If negative, waits infinitely for a key press.
       * @returns Number of key pressed or if @a wait_ms >= 0.
       */
      int showImage(Image8U* image, int wait_ms);
      int showFloatImage(Image<float>* image, int wait_ms);
      Rectangle selectROI();
      int wait(int ms);

      void draw(Rectangle r, Colour c, int Thickness=1);
      void draw(int x, int y, Colour c, int Thickness=1);
      void draw(char* text, int fontsize, int x, int y, Colour c);
      void draw(int x0, int y0, int x1, int y1, Colour c, int Thickness=1);
  };

  // structure for mouse handler
  typedef struct mh_params {
    cv::Point loc1;
    cv::Point loc2;
    string win_name;
    cv::Mat orig_img;
    //Mat cur_img;
  } mh_params;
}



#endif
