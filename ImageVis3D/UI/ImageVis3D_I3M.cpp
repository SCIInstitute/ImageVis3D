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


//!    File   : ImageVis3D_I3M.cpp
//!    Author : Jens Krueger
//!             DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : July 2009
//
//!    Copyright (C) 2009 DFKI, MMCI, SCI Institute

#include "ImageVis3D.h"
#include "I3MDialog.h"
#include "DatasetServerDialog.h"
#include <QtGui/QMessageBox>

using namespace tuvok;
using namespace std;

void MainWindow::TransferToI3M() {
  if (!m_pActiveRenderWin) return;

  const UVFDataset* currentDataset = dynamic_cast<UVFDataset*>(&(m_pActiveRenderWin->GetRenderer()->GetDataset()));

  if (currentDataset) {
    I3MDialog d(currentDataset, m_strTempDir, this);
    d.exec();
  } else {
    QMessageBox errorMessage;
    errorMessage.setText("ImageVis3D Mobile Device Transfer only supported for UVF datasets.");
    errorMessage.setIcon(QMessageBox::Critical);
    errorMessage.exec();  
    T_ERROR("ImageVis3D Mobile Device Transfer only supported for UVF datasets.");
  }
}


void MainWindow::StartDatasetServer() {
  DatasetServerDialog d(m_strTempDir, this);
  d.exec();
}
