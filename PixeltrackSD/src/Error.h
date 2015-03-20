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

#ifndef TL_ERROR_H
#define TL_ERROR_H

#include <stdexcept>
#include <iostream>
#include <cstdlib>


namespace TLUtil
{
#define DEBUG_VERBOSITY 0

// Debug
#ifndef DEBUG_VERBOSITY
#define DEBUG_VERBOSITY 0
  // 0 : high-level important information
  // 1 : intermediate run-time information
  // 2 : more detailed debugging information
  // 3 : debugging details (used for temporary debugging)
#endif

  /*
#ifdef NDEBUG
  #define DEBUG(LEVEL,MESSAGE)
#else
*/
  #define MESSAGE(LEVEL,MESSAGE) if(LEVEL <= DEBUG_VERBOSITY)  std::cout << MESSAGE << std::endl;
//#endif


// Assert
#ifdef NDEBUG
  // release version
  #define ASSERT(condition,message) { \
      if (! (condition)) { \
        std::cerr << "Assertion failed: " << #condition << message << std::endl; \
      } \
  }
#else
  // debug version
  #define ASSERT(condition,message) { \
      if (! (condition)) { \
        std::cerr << "Assertion failed: " << #condition << message << std::endl; \
        exit(EXIT_FAILURE); \
      } \
  }
#endif




class FileOpen_Ex : public std::runtime_error
{
  public:
    FileOpen_Ex(std::string& message);
};

class CameraOpen_Ex : public std::runtime_error
{
  public:
    CameraOpen_Ex(std::string& id);
};


class OptionNotFound_Ex : public std::runtime_error
{
  public:
    OptionNotFound_Ex(std::string& message);
};
  




}

#endif
