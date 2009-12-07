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


//!    File   : QLightPreview.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : November 2009
//
//!    Copyright (C) 2009 SCI Institute

#include <iostream>

#include "QLightPreview.h"

#include <QtGui/QPainter>

#include "../Tuvok/Controller/Controller.h"
#include "../Tuvok/Basics/MathTools.h"

using namespace std;

QLightPreview::QLightPreview(QWidget *parent) :
  QWidget(parent),
  m_cAmbient(1.0f,1.0f,1.0f,0.4f),
  m_cDiffuse(1.0f,1.0f,1.0f,0.6f),
  m_cSpecular(1.0f,1.0f,1.0f,1.0f),
  m_iCachedHeight(0),
  m_iCachedWidth(0),
  m_pCachedImage(NULL),
  m_bBackdropCacheUptodate(false)
{
}

QLightPreview::~QLightPreview(void)
{
  // delete the backdrop cache pixmap
  delete m_pCachedImage;
}

void QLightPreview::DrawSphere(QImage* sphereImage) {
  // can be choosen arbitrarily
  FLOATVECTOR3 fLightDir(0.5,-0.5,1.0);
  fLightDir.normalize();


  FLOATVECTOR3 fViewDir(0,0,1);

  int w = width();
  int h = height();
  for (int y = 0;y<int(m_iCachedHeight);y++){
    float normY = float(y*2) / float(h);
    float fDistToCenterY = (normY - 1.0f);
    float fDistToCenterYQ = fDistToCenterY*fDistToCenterY;

    for (int x = 0;x<int(m_iCachedWidth);x++){

      float normX = float(x*2) / float(w);
      float fDistToCenterX = (normX - 1.0f);
      float fDistToCenterXQ = fDistToCenterX*fDistToCenterX;

      if ( fDistToCenterXQ + fDistToCenterYQ > 1 ) {
        sphereImage->setPixel(int(x),int(y),qRgb(0,0,0));
      } else {
        FLOATVECTOR3 vNormal(fDistToCenterX, fDistToCenterY, sqrt((1-fDistToCenterYQ)-fDistToCenterXQ));

        // compute diffuse and clamp to zero
        FLOATVECTOR3 diffuseColor = (fLightDir ^ vNormal) * m_cDiffuse.xyz() * m_cDiffuse.w;
        diffuseColor.x = (diffuseColor.x < 0) ? 0: diffuseColor.x;
        diffuseColor.y = (diffuseColor.y < 0) ? 0: diffuseColor.y;
        diffuseColor.z = (diffuseColor.z < 0) ? 0: diffuseColor.z;

        // compute specular and clamp to zero (reflection vector = light vector on a sphere)
        FLOATVECTOR3 specularColor = pow((fLightDir ^ vNormal),32) * m_cSpecular.xyz() * m_cSpecular.w;
        specularColor.x = (specularColor.x < 0) ? 0: specularColor.x;
        specularColor.y = (specularColor.y < 0) ? 0: specularColor.y;
        specularColor.z = (specularColor.z < 0) ? 0: specularColor.z;

        // add colors and clamp to 1
        FLOATVECTOR3 color = diffuseColor + specularColor + m_cAmbient.xyz() * m_cAmbient.w;
        color.x = (color.x > 1) ? 1: color.x;
        color.y = (color.y > 1) ? 1: color.y;
        color.z = (color.z > 1) ? 1: color.z;

        sphereImage->setPixel(int(x),int(y),
                 qRgb(int(color.x*255),
                      int(color.y*255),
                      int(color.z*255)));
      }
    }
  }
}

void QLightPreview::paintEvent(QPaintEvent *event) {
  // call superclass method
  QWidget::paintEvent(event);

  // as drawing the histogram can become quite expensive we'll cache it in an image and only redraw if needed
  if (!m_bBackdropCacheUptodate || (unsigned int)height() != m_iCachedHeight || (unsigned int)width() != m_iCachedWidth) {

    // delete the old pixmap an create a new one if the size has changed
    if (m_pCachedImage == NULL || (unsigned int)height() != m_iCachedHeight || (unsigned int)width() != m_iCachedWidth) {
      delete m_pCachedImage;
      m_pCachedImage = new QImage(width(),height(),QImage::Format_ARGB32);
    }

    // store the new image size
    m_iCachedHeight = height();
    m_iCachedWidth = width();

    // draw the backdrop into the image
    DrawSphere(m_pCachedImage);

    // update change detection states
    m_bBackdropCacheUptodate = true;
  }

  // now draw everything rest into this widget
  QPainter painter(this);

  // the image captured above (or cached from a previous call)
  painter.drawImage(0,0,*m_pCachedImage);
}


void QLightPreview::SetData(const FLOATVECTOR4& ambient,
                            const FLOATVECTOR4& diffuse,
                            const FLOATVECTOR4& specular) {

  m_cAmbient = ambient;
  m_cDiffuse = diffuse;
  m_cSpecular = specular;

  m_bBackdropCacheUptodate = false;
  update();
}

FLOATVECTOR4 QLightPreview::GetAmbient() const {
  return m_cAmbient;
}

FLOATVECTOR4 QLightPreview::GetDiffuse() const {
  return m_cDiffuse;
}

FLOATVECTOR4 QLightPreview::GetSpecular()const {
  return m_cSpecular;
}

