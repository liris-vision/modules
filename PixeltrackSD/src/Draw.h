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

#ifndef TL_DRAW_H
#define TL_DRAW_H

#include "Image.h"
#include "Rectangle.h"
#include "tltypes.h"


namespace TLImageProc
{

void drawRectangle(Image8U* img, Rectangle r, Colour c, int Thickness=1);
void drawCross(Image8U* img, int x, int y, Colour c, int Thickness=1);
void drawLine(Image8U* img, int x0, int y0, int x1, int y1, Colour c, int Thickness=1);

}

#endif
