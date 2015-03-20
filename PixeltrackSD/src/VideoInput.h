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

#ifndef TL_VIDEO_INPUT_H
#define TL_VIDEO_INPUT_H

#include "Image.h"

using namespace TLImageProc;

namespace TLInOut
{

class VideoInput
{
  protected:
    int miFramesRead;
    float mfFPS;

  public:
    VideoInput();
    virtual ~VideoInput();

    float getFPS() { return mfFPS; };

    virtual bool nextImageAvailable()=0;
    virtual Image<unsigned char>* nextImage()=0;

    virtual unsigned long long getCurrentTimestampMs()=0;
};


}

#endif
