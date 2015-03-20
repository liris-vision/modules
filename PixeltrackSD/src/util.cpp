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

#include "util.h"

namespace TLUtil
{

/*
float sqrt_approx(float z)
{
    union
    {
        int tmp;
        float f;
    } u;
    u.f = z;
 
    u.tmp = (1 << 29) + (u.tmp >> 1) - (1 << 22) + a;
 
    return u.f;
}


float InvSqrt(float x)  // depending on 32 bit IEEE implementation 
{
    float xhalf = 0.5f*x;
    int i = *(int*)&x;
    i = 0x5f37a86 - (i >> 1);
    x = *(float*)&i;
    x = x*(1.5f - xhalf*x*x);
    return x;
}



float InvSqrt( float f )
{
  float fhalf = 0.5f*f;
  asm
  (
    "mov %1, %%eax;"
    "sar %%eax;"
    "mov $0x5F3759DF, %%ebx;"
    "sub %%eax, %%ebx;"
    "mov %%ebx, %0"
    :"=g"(f)
    :"g"(f)
    : "%eax", "%ebx"
  );
  f *= 1.5f - fhalf*f*f;
  return f;
}
*/

}

