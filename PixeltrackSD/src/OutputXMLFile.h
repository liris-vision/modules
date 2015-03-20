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

#ifndef TL_OUTPUTXMLFILE_H
#define TL_OUTPUTXMLFILE_H

//#include "Parameters.h"
#include "Rectangle.h"
#include "Output.h"


namespace TLInOut
{

  class OutputXMLFile : public Output
  {
    public:
      char* m_cFileName;
      FILE* m_File;
      int m_iLastNbPersons;
      int m_iCurrentNbPersons;
      int m_iLastIter;
      //int m_iLogLevel;
      faceinfo_t faceinfo[200];

      OutputXMLFile(const char* filename, int skip_frames=0); //, int loglevel);
      ~OutputXMLFile();

      int sendBB(TLImageProc::Rectangle* bb, int identifier, float pan, float confidence);
      int sendBB(int tl_x, int tl_y, int tr_x, int tr_y, int bl_x, int bl_y, int br_x, int br_y, int identifier, float pan, float confidence);
      int commit(int iter, unsigned long long timestamp, long long vce_timestamp);
 
  };
}

#endif
