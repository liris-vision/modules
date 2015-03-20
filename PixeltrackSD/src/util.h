#ifndef TL_UTIL_H
#define TL_UTIL_H

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

namespace TLUtil
{

/*
float sqrt_approx(float z)
float InvSqrt(float x)  
float InvSqrt( float f )
*/


#define ROUND_TO_INT(x) ( ((x)>0)? (int((x)+.5)) : (int((x)-.5)) )
#define ROUND_TO_INTP(x) (int((x)+.5))
#define ROUND_TO_INTN(x) (int((x)-.5))

}

#ifdef D_API_WIN32
// Windows
#define round(x)  (floor((x) + 0.5))
#endif

#endif  // TL_UTIL_H
