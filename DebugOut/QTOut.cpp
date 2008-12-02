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
  \file    QTOut.cpp
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    September 2008
*/

#include "QTOut.h"
#include <stdarg.h>
#include <stdio.h>
#include <sstream>

using namespace std;

#ifdef WIN32
  #include <windows.h>
#endif

QTOut::QTOut(QListWidget *listWidget) :
  m_listWidget(listWidget)
{
  m_bRecordLists[0] = false;
  m_bRecordLists[1] = false;
  m_bRecordLists[2] = false;
  Message("QTOut::QTOut","Starting up QTListviewDebug out");
}

QTOut::~QTOut() {
  Message("QTOut::~QTOut","Shutting down QTListviewDebug out");
}

void QTOut::printf(const char* format, ...)
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
  m_listWidget->addItem ( buff );
}

void QTOut::_printf(const char* format, ...)
{
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  m_listWidget->addItem ( buff );
}

void QTOut::Message(const char* source, const char* format, ...) {
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  if (m_bRecordLists[2]) {
    stringstream s;
    s << source << "->" << buff;
    m_strMessageList.push_back(s.str());
  }
  if (!m_bShowMessages) return;
  _printf("MESSAGE (%s): %s",source, buff);
}

void QTOut::Warning(const char* source, const char* format, ...) {
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  if (m_bRecordLists[1]) {
    stringstream s;
    s << source << "->" << buff;
    m_strWarningList.push_back(s.str());
  }
  if (!m_bShowWarnings) return;
  _printf("WARNING (%s): %s",source, buff);
}

void QTOut::Error(const char* source, const char* format, ...) {
  char buff[16384];
  va_list args;
  va_start(args, format);
#ifdef WIN32
  _vsnprintf_s( buff, 16384, sizeof(buff), format, args);
#else
  vsnprintf( buff, sizeof(buff), format, args);
#endif
  if (m_bRecordLists[0]) {
    stringstream s;
    s << source << "->" << buff;
    m_strErrorList.push_back(s.str());
  }
  if (!m_bShowErrors) return;
  _printf("ERROR (%s): %s",source, buff);
}


void QTOut::PrintErrorList() {
  m_listWidget->addItem ( "Printing recorded errors:" );
  for (std::deque< std::string >::iterator i = m_strErrorList.begin();i<m_strErrorList.end();i++) {
    m_listWidget->addItem ( i->c_str() );
  }
  m_listWidget->addItem ( "end of recorded errors" );
}

void QTOut::PrintWarningList() {
  m_listWidget->addItem ( "Printing recorded warnings:" );
  for (std::deque< std::string >::iterator i = m_strWarningList.begin();i<m_strWarningList.end();i++) {
    m_listWidget->addItem ( i->c_str() );
  }
  m_listWidget->addItem ( "end of recorded warnings" );
}

void QTOut::PrintMessageList() {
  m_listWidget->addItem ( "Printing recorded messages:" );
  for (std::deque< std::string >::iterator i = m_strMessageList.begin();i<m_strMessageList.end();i++) {
    m_listWidget->addItem ( i->c_str() );
  }
  m_listWidget->addItem ( "end of recorded messages" );
}
