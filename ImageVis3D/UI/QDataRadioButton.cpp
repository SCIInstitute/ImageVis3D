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


//!    File   : QDataRadioButton.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : September 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "QDataRadioButton.h"
#include <QtGui/QMouseEvent>

QDataRadioButton::QDataRadioButton(FileStackInfo* stack, QWidget *parent) : 
  QRadioButton(parent), 
  m_iCurrentImage((unsigned int)(-1)),  
  m_stackInfo(stack) 
{
  SetupInfo();
}
QDataRadioButton::QDataRadioButton(FileStackInfo* stack, const QString &text, QWidget *parent) :
  QRadioButton(text, parent), 
  m_iCurrentImage((unsigned int)(-1)),  
  m_stackInfo(stack)
{
  SetupInfo();
}


void QDataRadioButton::leaveEvent ( QEvent * event ) {
  QRadioButton::leaveEvent(event);

  SetStackImage(int(m_stackInfo.m_Elements.size()/2));
}

void QDataRadioButton::mouseMoveEvent(QMouseEvent *event){
  QRadioButton::mouseMoveEvent(event);

  unsigned int i = (unsigned int)(float(event->x())/float(width())*m_stackInfo.m_Elements.size());

  SetStackImage(i);
}

void QDataRadioButton::SetStackImage(unsigned int i) {

  if (m_iCurrentImage == i) return;
  
  m_iCurrentImage = i;

  QIcon icon;
  QImage image(m_stackInfo.m_ivSize.x, m_stackInfo.m_ivSize.y, QImage::Format_RGB32);

  void* pData = NULL;
  m_stackInfo.m_Elements[i]->GetData(&pData);

  if (m_stackInfo.m_iComponentCount == 1) {
    switch (m_stackInfo.m_iAllocated) {
      case 8  :{
            unsigned int i = 0;
            unsigned char* pCharData = (unsigned char*)pData;
            for (int y = 0;y<image.height();y++) 
              for (int x = 0;x<image.width();x++) {
                image.setPixel(x,y, qRgb(pCharData[i],pCharData[i],pCharData[i]));
                i++;
              }
             } break;
      case 16 : {
            unsigned int i = 0;
            unsigned short* pShortData = (unsigned short*)pData;
            for (int y = 0;y<image.height();y++) 
              for (int x = 0;x<image.width();x++) {
                unsigned char value = (unsigned char)(255.0f*float(pShortData[i])/float((m_stackInfo.m_iStored)));
                image.setPixel(x,y, qRgb(value,value,value));
                i++;
              }
             } break;
      default  : break; /// \todo handle other bitwith data
    }
  } else {
    /// \todo handle color data
  }

  delete [] (char*)pData;

  icon.addPixmap(QPixmap::fromImage(image), QIcon::Normal, QIcon::Off);
  setIcon(icon);
  update();
}


void QDataRadioButton::SetupInfo() {
  setMouseTracking(true); 

  size_t iElemCount = m_stackInfo.m_Elements.size();

  QString desc = tr(" %1 \n Size: %2 x %3 x %4\n")
    .arg(m_stackInfo.m_strDesc.c_str())
    .arg(m_stackInfo.m_ivSize.x)
    .arg(m_stackInfo.m_ivSize.y)
    .arg(m_stackInfo.m_ivSize.z*iElemCount);

  setText(desc);

  SetStackImage(uint(iElemCount/2));
}
