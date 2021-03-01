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


//!    File   : QLightPreview.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : November 2009
//
//!    Copyright (C) 2009 SCI Institute

#pragma once

#ifndef QLIGHTPREVIEW_H
#define QLIGHTPREVIEW_H

#include <QtWidgets/QWidget>
#include "../Tuvok/Basics/Vectors.h"

class QLightPreview : public QWidget
{
  Q_OBJECT
public:

  QLightPreview(QWidget *parent=0);
  virtual ~QLightPreview(void);

  void SetData(const FLOATVECTOR4& ambient,
               const FLOATVECTOR4& diffuse,
               const FLOATVECTOR4& specular,
               const FLOATVECTOR3& lightDir);

  FLOATVECTOR4 GetAmbient() const;
  FLOATVECTOR4 GetDiffuse() const;
  FLOATVECTOR4 GetSpecular()const;
  FLOATVECTOR3 GetLightDir()const;

signals:
  void lightMoved();

protected:
  virtual void paintEvent(QPaintEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);

private:
  FLOATVECTOR4 m_cAmbient;
  FLOATVECTOR4 m_cDiffuse;
  FLOATVECTOR4 m_cSpecular;
  FLOATVECTOR3 m_vLightDir;

  unsigned int m_iCachedHeight;
  unsigned int m_iCachedWidth;
  QImage*      m_pCachedImage;
  bool         m_bBackdropCacheUptodate;
  bool         m_bMousePressed;

  void DrawSphere(QImage* sphereImage);
};

#endif // QLIGHTPREVIEW_H
