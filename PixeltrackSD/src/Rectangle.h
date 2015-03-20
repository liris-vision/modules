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

#ifndef TL_RECTANGLE_H
#define TL_RECTANGLE_H

#include <math.h>
#include <string>


namespace TLImageProc {

  class Rectangle
  {
    public: // members : same as opencv, so that we can do casting
      int miFirstColumn; 
      int miFirstLine; 
      int miWidth;
      int miHeight;

      Rectangle() { init(0,0,0,0); };
      Rectangle(std::string x_y_w_h);
      Rectangle(int first_col,int first_line, int last_col,int last_line);
      Rectangle(int nb_columns,int nb_lines);
      ~Rectangle() {}

      void init(int first_col,int first_line,int last_col,int last_line);
      void initPosAndSize(int first_col,int first_line, int width,int height);
      void initCenterAndSize(int center_x,int center_y, int width,int height);
      void initSize(int width,int height);

      int  firstLine() { return miFirstLine;}
      int  firstColumn() { return miFirstColumn; }
      int  lastLine() { return miFirstLine+miHeight-1;}
      int  lastColumn() { return miFirstColumn+miWidth-1; }

      void translate(int dep_co,int dep_li);
      bool inside(int column,int line);
      int area();

      bool empty();
      void setEmpty();
      void add(int col,int li);
      void add(float col,float  li);
      
      void setCenter(int x, int y);
      int centerX() { return miFirstColumn+miWidth/2; };
      int centerY() { return miFirstLine+miHeight/2; };

      void intersection(Rectangle & BBox2);
      bool isOverlapping(Rectangle& bb);
      float precision(Rectangle& ground_truth);
      float recall(Rectangle& ground_truth);
      float f_measure(Rectangle& ground_truth);
      float agarwal_measure(Rectangle& ground_truth);
      float pascal_measure(Rectangle& ground_truth);
      void outerBoundingBox(Rectangle & BBox2);

      void scale(float scalefactor);
      void scale(float width_scale, float height_scale);
      void halve(int n_times);
      void enlarge(float scalefactor);
      void enlarge(float scalefactor_x, float scalefactor_y);
      void enlargeY(float scalefactor);
      float normalisedDistance(Rectangle bb);
      float unnormalisedDistance(Rectangle bb);
      float distance(Rectangle bb);

    };
} 



#endif  
