/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2008 Scientific Computing and Imaging Institute,
   University of Utah.

   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

/**
  \file    HRConsoleOut.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    August 2008
*/

#include <iostream>
using namespace std;

#include "HRConsoleOut.h"
#include "../../Tuvok/Basics/Console.h"

HRConsoleOut::HRConsoleOut() {
}

HRConsoleOut::~HRConsoleOut() {
}

void HRConsoleOut::printf(const char* format, ...)
{
  if (!m_bShowOther) return;

  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  Console::printf("%s\n",buff);
}

void HRConsoleOut::Message(const char* , const char* format, ...) {
  if (!m_bShowMessages) return;
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  cout << buff << endl;
}

void HRConsoleOut::Warning(const char* source, const char* format, ...) {
  if (!m_bShowWarnings) return;
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  cout << "WARNING (" << source << "): "<< buff << endl;
}

void HRConsoleOut::Error(const char* source, const char* format, ...) {
  if (!m_bShowErrors) return;
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  cout << "ERROR (" << source << "): "<< buff << endl;
}