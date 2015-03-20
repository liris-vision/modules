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

#include <stdio.h>
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
//#include "Parameters.h"
#include "Rectangle.h"
#include "OutputTXTFile.h"
#include "util.h"

namespace TLInOut
{

OutputTXTFile::OutputTXTFile(const char* filename, int skip_frames/*=0*/) //, int loglevel)
{
  m_cFileName = new char[(int)strlen(filename)+1];
  strcpy(m_cFileName, filename);

  m_File = fopen(m_cFileName, "w");
  if (m_File==NULL)
  {
    printf("Error: log file %s cannot be opened.\n", m_cFileName);
    exit(-1);
  }
  fprintf(m_File, "# frame number_objects obj1_id obj1_topleft_x obj1_topleft_y obj1_width obj1_height obj2_id ... \n");

  m_iLastNbPersons = 0;
  m_iCurrentNbPersons = 0;
  m_iLastIter = -1;
  //m_iLogLevel = loglevel;
}



int OutputTXTFile::sendBB(TLImageProc::Rectangle* bb, int identifier, float pan, float confidence)
{
  TLImageProc::Rectangle bbtmp = *bb;
  bbtmp.scale(mfWidthScale, mfHeightScale);

  return sendBB(bbtmp.miFirstColumn, bbtmp.miFirstLine, bbtmp.lastColumn(), bbtmp.miFirstLine, bbtmp.miFirstColumn, bbtmp.lastLine(), bbtmp.lastColumn(), bbtmp.lastLine(), identifier, pan, confidence);
}

int OutputTXTFile::sendBB(int tl_x, int tl_y, int tr_x, int tr_y, int bl_x, int bl_y, int br_x, int br_y, int identifier, float pan, float confidence)
{
  if (identifier<0)
    return 0;

  faceinfo[m_iCurrentNbPersons].id = identifier;
  faceinfo[m_iCurrentNbPersons].tl_x = round(tl_x*mfWidthScale);
  faceinfo[m_iCurrentNbPersons].tl_y = round(tl_y*mfHeightScale); 
  faceinfo[m_iCurrentNbPersons].tr_x = round(tr_x*mfWidthScale);
  faceinfo[m_iCurrentNbPersons].tr_y = round(tr_y*mfHeightScale);
  faceinfo[m_iCurrentNbPersons].bl_x = round(bl_x*mfWidthScale);
  faceinfo[m_iCurrentNbPersons].bl_y = round(bl_y*mfHeightScale);
  faceinfo[m_iCurrentNbPersons].br_x = round(br_x*mfWidthScale);
  faceinfo[m_iCurrentNbPersons].br_y = round(br_y*mfHeightScale);
  faceinfo[m_iCurrentNbPersons].pan = pan;
  faceinfo[m_iCurrentNbPersons].confidence = confidence;

  m_iCurrentNbPersons++;
  return 0;
}

      
int OutputTXTFile::commit(int iter, unsigned long long timestamp, long long vce_timestamp)
{
  setlocale(LC_ALL, "C");

  if (m_iCurrentNbPersons!=m_iLastNbPersons)
  {
    //TODO output person number change events
  }

  fprintf(m_File, "%d ", iter);
  fprintf(m_File, "%d ", m_iCurrentNbPersons);
  for(int i=0; i<m_iCurrentNbPersons; i++)
  {
    fprintf(m_File, "%d ", faceinfo[i].id);

    fprintf(m_File, "%d %d %d %d ", faceinfo[i].tl_x, faceinfo[i].tl_y, faceinfo[i].br_x-faceinfo[i].tl_x, faceinfo[i].br_y-faceinfo[i].tl_y);

    fflush(m_File);
  }
  fprintf(m_File, "\n");

  m_iLastNbPersons = m_iCurrentNbPersons;
  m_iCurrentNbPersons=0;
  m_iLastIter = iter;
  return 0;
}

OutputTXTFile::~OutputTXTFile()
{
  fclose(m_File);
  delete [] m_cFileName;
}



}




