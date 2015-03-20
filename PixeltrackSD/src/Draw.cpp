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

#include "Draw.h"

namespace TLImageProc
{

void drawRectangle(Image8U* img, Rectangle r, Colour c, int Thickness/*=1*/)
{
  Rectangle C(img->width(), img->height());
  int li,co;
  int left, right, top, bottom;

  if(Thickness>1){
    C.initPosAndSize(Thickness/2+1,Thickness/2+1,img->width()-Thickness-2,img->height()-Thickness-2);
  }
  C.intersection(r);

  if(C.area()>0){
    if (Thickness==1)
    {
      co=C.lastColumn(); //-1;
      for(li=C.firstLine();li<=C.lastLine();li++){
	img->setColourBGR(C.firstColumn(),li,c);
	img->setColourBGR(co,li,c);
      }
      li=C.lastLine(); //-1;
      for(co=C.firstColumn();co<=C.lastColumn();co++){
	img->setColourBGR(co,C.firstLine(),c);
	img->setColourBGR(co,li,c);
      }
    }
    else
    {
      int tmin, tmax;
      if (Thickness%2==0)
	tmin = -Thickness/2+1;
      else
	tmin = -Thickness/2;
      tmax = Thickness/2;

      for(int t=tmin; t<=tmax; t++)
      {
	left=C.firstColumn()-t;
	right=C.lastColumn()+t;
	for(li=C.firstLine()-t;li<C.lastLine()+t;li++)
	{
	  img->setColourBGR(left,li,c);
	  img->setColourBGR(right,li,c);
	}
	top=C.firstLine()-t;
	bottom=C.lastLine()+t;
	for(co=C.firstColumn()-t;co<C.lastColumn()+t+1;co++)
	{
	  img->setColourBGR(co,top,c);
	  img->setColourBGR(co,bottom,c);
	}
      }
    }
  }
}


void drawCross(Image8U* img, int x, int y, Colour c, int Thickness/*=1*/)
{
  int li,co;
  int left, right, top, bottom;
  int width, height;
  width = img->width();
  height = img->height();

  int len = 2*Thickness;
  if (x-len<0 || x>width-len)
    return;
  if (y-len<0 || y>height-len)
    return;

  /*
  // Thickness=1
  for(int i=y-2; i<y+2; i++)
    img->setColourBGR(x, i, c);
  for(int j=x-2; j<x+2; j++)
    img->setColourBGR(j, y, c);
    */
  for(int t=-Thickness/2; t<Thickness/2; t++)
    for(int i=y-len; i<y+len; i++)
      img->setColourBGR(x+t, i, c);
  for(int t=-Thickness/2; t<Thickness/2; t++)
    for(int j=x-len; j<x+len; j++)
      img->setColourBGR(j, y+t, c);
}



void drawLine(Image8U* img, int x0, int y0, int x1, int y1, Colour c, int Thickness/*=1*/)
{
  int dx = x0 - x1;
  int dy = y0 - y1;
  double a, b;
  int x, y;
  int x_start, x_end, y_start, y_end;
  int width = img->width();
  int height = img->height();
  int tend=round((float)Thickness/2);
  //MESSAGE(0, x0 << " " << y0 << " " << x1 << " " << y1 << "   dx: " << dx << "  dy: " << dy);

  if(x0>x1)
  {
    x_start = x1;
    x_end = x0;
  }
  else
  {
    x_start = x0;
    x_end = x1;
  }
  if(y0>y1)
  {
    y_start = y1;
    y_end = y0;
  }
  else
  {
    y_start = y0;
    y_end = y1;
  }

  if (dx == 0)
  {
    // vertical line
    if((x0>0) && (x0+1<width))
    {
      for(int t=x0-Thickness/2; t<x0+tend; t++)
      for(y=y_start; y<y_end; y++)
      {
	if((y>0) && (y<height))
	{
	  //img->setColourBGR(x0,y, c);
	  img->setColourBGR(t,y, c);
	}
      }
    }
  }
  else if(dy == 0)
  {
    // horizontal line
    if((y0>0) && (y0+1<height))
    {
      for(int t=y0-Thickness/2; t<y0+tend; t++)
      for(x=x_start; x<x_end; x++)
      {
	if((x>0) && (x<width))
	{
	  //img->setColourBGR(x,y0, c);
	  img->setColourBGR(x,t, c);
	}
      }
    }
  }
  else
  {
    if(abs(dx)>abs(dy))
    {
      // flat slope
      a = (double) dy / (double) dx;
      b = (double) y0 - a * x0;
      for(int t=-Thickness/2; t<tend; t++)
      for(x=x_start; x<=x_end; x++)
      {
	y = (int) (a*x+b)+t;
	if((x>0) && (x<width) && (y>0) && ((y+1)<height))
	{
	  img->setColourBGR(x,y, c);
	}
      }
    }
    else
    {
      // steep slope
      a = (double) dx / (double) dy;
      b = (double) x0 - a * y0;
      for(int t=-Thickness/2; t<tend; t++)
      for(y=y_start; y<=y_end; y++)
      {
	x = (int) (a*y+b)+t;
	if((x>0) && ((x+1)<width) && (y>0) && (y<height))
	{
	  img->setColourBGR(x,y, c);
	}
      }
    }
  }
}


}


