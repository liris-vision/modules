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

#include <time.h>
#include <cstring>
#include "Output.h"


char* get_time_string(int mode24, int *hh, int *mm, int *ss)
{
  static char time_str[12] = {0};   /* Stroes the time as a string */
  struct tm *now = NULL;
  int hour = 0;
  time_t time_value = 0;

  time_value = time(NULL);          /* Get time value              */
  now = localtime(&time_value);     /* Get time and date structure */
  hour = now->tm_hour;              /* Save the hour value         */
  if(!mode24 && hour>12)            /* Adjust if 12 hour format    */
    hour -= 12;

  /*
 *   printf("%d:",hour);
 *     printf("%d:",now->tm_min);
 *       printf("%d",now->tm_sec);
 *         */
  *hh = hour;
  *mm = now->tm_min;
  *ss = now->tm_sec;

  /*
 *   if(!mode24)
 *       if(now->tm_hour>24){
 *              printf("PM\n");
 *                  }else{
 *                         printf("AM\n");
 *                              }
 *                                */
  return time_str;
}


namespace TLInOut
{

int string2Output(char* str, int& result, int& nb_outputs)
{
  char* tmp;
  nb_outputs=0;
  result=0;
  tmp = strtok(str, ",");
  if(!tmp)
  {
    nb_outputs=0;
    return -1;
  }
  do
  {
    if (!strcmp(tmp, "file"))
      result |= OutputFile;
    else if (!strcmp(tmp, "none"))
    {
      result = 0;
      nb_outputs = 0;
      return 0;
    }
    else
    {
      nb_outputs=0;
      return -2;
      //return 0; // allow 0 outputs
    }
    nb_outputs++;
    tmp = strtok(NULL, ",");
  } while (tmp);
  return 0;
}

Output::Output()
{
  mfWidthScale=1.0;
  mfHeightScale=1.0;
}
}

