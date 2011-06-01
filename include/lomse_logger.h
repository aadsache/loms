//---------------------------------------------------------------------------------------
//  This file is part of the Lomse library.
//  Copyright (c) 2010-2011 Lomse project
//
//  Lomse is free software; you can redistribute it and/or modify it under the
//  terms of the GNU General Public License as published by the Free Software Foundation,
//  either version 3 of the License, or (at your option) any later version.
//
//  Lomse is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
//  PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with Lomse; if not, see <http://www.gnu.org/licenses/>.
//
//  For any comment, suggestion or feature request, please contact the manager of
//  the project at cecilios@users.sourceforge.net
//
//---------------------------------------------------------------------------------------

#ifndef __LOMSE_LOGGER_H__
#define __LOMSE_LOGGER_H__


#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;

namespace lomse
{

extern ofstream dbgLogger;


class Logger
{
private:
    //ofstream m_debugFile;

public:
    Logger()
    {
        dbgLogger.open("dbg_tables.txt");
    };
    ~Logger()
    {
        dbgLogger.close();
    }

//    Logger& operator << (const char* t)
//    {
//        m_debugFile << t;
//        return *this;
//    }

};


}   //namespace lomse

#endif      //__LOMSE_LOGGER_H__