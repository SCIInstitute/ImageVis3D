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
#include "../Tuvok/IO/TuvokJPEG.h"
#include "../Tuvok/IO/DICOM/DICOMParser.h"
#include "../Tuvok/IO/3rdParty/jpeglib/jconfig.h"

QDataRadioButton::QDataRadioButton(std::shared_ptr<FileStackInfo> stack,
                                   QWidget *parent) :
  QRadioButton(parent),
  m_iCurrentImage((unsigned int)(-1)),
  m_stackInfo(stack),
  m_fScale(1.0f)
{
  SetupInfo();
}

QDataRadioButton::QDataRadioButton(std::shared_ptr<FileStackInfo> stack,
                                   const QString &text, QWidget *parent) :
  QRadioButton(text, parent),
  m_iCurrentImage((unsigned int)(-1)),
  m_stackInfo(stack),
  m_fScale(1.0f)
{
  SetupInfo();
}


void QDataRadioButton::leaveEvent(QEvent * event) {
  QRadioButton::leaveEvent(event);

  SetStackImage(int(m_stackInfo->m_Elements.size()/2));
}

void QDataRadioButton::mouseMoveEvent(QMouseEvent *event){
  QRadioButton::mouseMoveEvent(event);
  unsigned int i;

  i = (unsigned int)(float(event->x()) / float(width()) *
                     m_stackInfo->m_Elements.size());
  SetStackImage(i);
}

void QDataRadioButton::SetBrightness(float fScale) {
  m_fScale = fScale;
  SetStackImage(m_iCurrentImage, true);
}



#ifdef DETECTED_OS_WINDOWS
  #pragma warning(disable:4996)
#endif

void QDataRadioButton::SetStackImage(unsigned int i, bool bForceUpdate) {

  if (!bForceUpdate && m_iCurrentImage == i) return;

  m_iCurrentImage = i;

  QIcon icon;

  std::vector<char> vData;
  if (m_stackInfo->m_bIsJPEGEncoded) {
    tuvok::JPEG jpg(m_stackInfo->m_Elements[i]->m_strFileName,
                    dynamic_cast<SimpleDICOMFileInfo*>
                      (m_stackInfo->m_Elements[i])->GetOffsetToData());
    const char *jpg_data = jpg.data();
    // JPEG library automagically downsamples the data.
    m_stackInfo->m_iAllocated = BITS_IN_JSAMPLE;
    vData.resize(jpg.size());
    std::copy(jpg_data, jpg_data + jpg.size(), &vData[0]);
  } else {
    m_stackInfo->m_Elements[i]->GetData(vData);
  }
  QImage image(m_stackInfo->m_ivSize.x, m_stackInfo->m_ivSize.y,
               QImage::Format_RGB32);

  if (m_stackInfo->m_iComponentCount == 1) {
    switch (m_stackInfo->m_iAllocated) {
      case 8  :{
            unsigned int j = 0;
            unsigned char* pCharData = reinterpret_cast<unsigned char*>
                                                       (&vData[0]);
            for (int y = 0;y<image.height();y++)
              for (int x = 0;x<image.width();x++) {
                unsigned char value = (unsigned char)(std::min(255.0f,m_fScale*pCharData[j]));
                image.setPixel(x,y, qRgb(value,value,value));
                j++;
              }
             } break;
      case 16 : {
            unsigned int j = 0;
            unsigned short* pShortData = reinterpret_cast<unsigned short*>
                                                         (&vData[0]);
            for (int y = 0;y<image.height();y++)
              for (int x = 0;x<image.width();x++) {
                unsigned char value = (unsigned char)(
                  std::min(255.0f, 255.0f * m_fScale * float(pShortData[j]) /
                                   float((2<<m_stackInfo->m_iStored)))
                );
                image.setPixel(x,y, qRgb(value,value,value));
                j++;
              }
             } break;
      default  : break; /// \todo handle other bitwith data
    }
  } else {
    /// \todo handle color data
  }
  icon.addPixmap(QPixmap::fromImage(image), QIcon::Normal, QIcon::Off);

  setIcon(icon);
  update();
}

#ifdef DETECTED_OS_WINDOWS
  #pragma warning(default:4996)
#endif


void QDataRadioButton::SetupInfo() {
  setMouseTracking(true);

  size_t iElemCount = m_stackInfo->m_Elements.size();

  QString desc = tr(" %1 \n Size: %2 x %3 x %4\n")
    .arg(m_stackInfo->m_strDesc.c_str())
    .arg(m_stackInfo->m_ivSize.x)
    .arg(m_stackInfo->m_ivSize.y)
    .arg(m_stackInfo->m_ivSize.z*iElemCount);

  setText(desc);

  SetStackImage(uint32_t(iElemCount/2));
}
