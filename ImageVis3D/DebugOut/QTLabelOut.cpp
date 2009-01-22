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
  \file    QTLabelOut.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    September 2008
*/

#include "QTLabelOut.h"
#include <StdDefines.h>
#include <QtCore/QCoreApplication>
#include <cstdarg>
#include <cstdio>
#include <sstream>

using namespace std;

#ifdef WIN32
  #include <windows.h>
  // undef stupid windows defines to max and min
  #ifdef max
  #undef max
  #endif

  #ifdef min
  #undef min
  #endif
#endif

QTLabelOut::QTLabelOut(QLabel *label, QDialog *parent) :
  m_label(label),
  m_parent(parent)
{
}

QTLabelOut::~QTLabelOut() {
}

void QTLabelOut::printf(const char* format, ...)
{
  if (!m_label && !m_bShowOther) return;
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  m_label->setText ( buff );
  m_label->repaint();
#ifdef TUVOK_OS_APPLE
  QCoreApplication::processEvents();
#endif
}

void QTLabelOut::_printf(const char* format, ...)
{
  if (!m_label) return;
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  m_label->setText ( buff );
  m_label->repaint();
#ifdef TUVOK_OS_APPLE
  QCoreApplication::processEvents();
#endif
}

void QTLabelOut::Message(const char* , const char* format, ...) {
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  if (!m_bShowMessages) return;
  _printf(buff);
}

void QTLabelOut::Warning(const char* , const char* format, ...) {
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  if (!m_bShowWarnings) return;
  _printf("WARNING: %s",buff);
}

void QTLabelOut::Error(const char* , const char* format, ...) {
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  if (!m_bShowErrors) return;
  _printf("ERROR: %s", buff);
}


