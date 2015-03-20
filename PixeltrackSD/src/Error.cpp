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

#include "Error.h"

namespace TLUtil
{

FileOpen_Ex::FileOpen_Ex(std::string& message) : std::runtime_error( std::string("File can not be opened: ").append(message))
{
}

CameraOpen_Ex::CameraOpen_Ex(std::string& id) : std::runtime_error( std::string("Camera can not be opened: ").append(id))
{
}


OptionNotFound_Ex::OptionNotFound_Ex(std::string& message) : std::runtime_error( std::string("Option not found: ").append(message))
{
}

}
