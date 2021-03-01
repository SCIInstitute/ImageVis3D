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


//!    File   : LODDlg.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : January 2009
//
//!    Copyright (C) 2008 SCI Institute

#include "LODDlg.h"

LODDlg::LODDlg(QString title, int iMinLOD, int iMaxLOD, const std::vector< QString >& vvLODs, QWidget* parent, Qt::WindowFlags flags) :
  QDialog(parent, flags),
  m_vvLODs(vvLODs)
{
  setupUi(this);
  label_Title->setText(title);
  verticalSlider_LOD->setMinimum(iMinLOD);
  verticalSlider_LOD->setMaximum(iMaxLOD);
  verticalSlider_LOD->setValue(iMinLOD);
  ChangeLOD(iMinLOD);
  label_lowres->setText(tr("%1 %2").arg(label_lowres->text()).arg(m_vvLODs[m_vvLODs.size()-1]));
  label_highres->setText(tr("%1 %2").arg(label_highres->text()).arg(m_vvLODs[0]));
}

LODDlg::~LODDlg(void)
{
}

void LODDlg::ChangeLOD(int iLOD)
{
  label_selectRes->setText(tr("Selected Resolution %1").arg(m_vvLODs[iLOD-verticalSlider_LOD->minimum()]));
}
