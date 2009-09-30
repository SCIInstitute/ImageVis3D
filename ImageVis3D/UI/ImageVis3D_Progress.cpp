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


//!    File   : ImageVis3D_Progress.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : November 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "ImageVis3D.h"
#include "../Tuvok/Basics/SysTools.h"

using namespace std;

void MainWindow::ClearProgressViewAndInfo() {
  label_ProgressDesc->setVisible(true);
  groupBox_RenderProgress->setVisible(false);

  lineEdit_DatasetName->setText("");
  lineEdit_MaxSize->setText("");
  lineEdit_MaxLODLevels->setText("");
  lineEdit_SizeForView->setText("");
  lineEdit_LODLevelForCurrentView->setText("");
}

void MainWindow::SetRenderProgressAnUpdateInfo(unsigned int iLODCount, 
                                               unsigned int iCurrentCount, 
                                               unsigned int iBrickCount, 
                                               unsigned int iWorkingBrick,
                                               unsigned int iMinLODIndex,
                                               RenderWindow* pRenderWin) {
  if (dockWidget_ProgressView->isVisible()) {
    if (label_ProgressDesc->isVisible()) label_ProgressDesc->setVisible(false);
    if (!groupBox_RenderProgress->isVisible()) groupBox_RenderProgress->setVisible(true);

    QString msg = tr("LOD %1/%2").arg(iCurrentCount).arg(iLODCount);
#ifdef DETECTED_OS_APPLE
    /// todo: label update has been removed until the delay-LOD issue on the mac is resolved
    //label_LODProgress->setText(msg);
    //label_LODProgress->repaint();
#else
    msg = msg + " (%p%)";
    progressBar_Frame->setFormat(msg);
#endif
    progressBar_Frame->setValue((unsigned int)((float(iCurrentCount) - 1.0f + float(iWorkingBrick)/float(iBrickCount)) * 100.0f / float(iLODCount)));
    progressBar_Frame->repaint();

    msg = tr("Brick %1/%2 of LOD %3").arg(iWorkingBrick).arg(iBrickCount).arg(iCurrentCount);
#ifdef DETECTED_OS_APPLE
    /// todo: label update has been removed until the delay-LOD issue on the mac is resolved
    //label_BrickProgress->setText(msg);
    //label_BrickProgress->repaint();
#else
    msg = msg + " (%p%)";
    progressBar_Level->setFormat(msg);
#endif

    progressBar_Level->setValue((unsigned int)(iWorkingBrick * 100.0f / iBrickCount));
    progressBar_Level->repaint();


#ifdef DETECTED_OS_APPLE
  QCoreApplication::processEvents();
#endif
  }

  if (pRenderWin) {
    UINT64VECTOR3 vSize = pRenderWin->GetRenderer()->GetDataset().GetDomainSize(iMinLODIndex);
    QString strSize = tr("%1 x %2 x %3").arg(vSize.x).arg(vSize.y).arg(vSize.z);
    if (strSize != lineEdit_SizeForView->text()) lineEdit_SizeForView->setText(strSize);

    QString strLODLevel = tr("%1").arg(pRenderWin->GetRenderer()->GetDataset().GetLODLevelCount()-iMinLODIndex);
    if (strLODLevel != lineEdit_LODLevelForCurrentView->text()) lineEdit_LODLevelForCurrentView->setText(strLODLevel);
  }


}

void MainWindow::MinLODLimitChanged() {
  if (m_pActiveRenderWin) {
    m_pActiveRenderWin->GetRenderer()->SetLODLimits(UINTVECTOR2(horizontalSlider_minLODLimit->value(),
                                                                horizontalSlider_maxLODLimit->value()));
    UpdateMinMaxLODLimitLabel();
  }
}

void MainWindow::MaxLODLimitChanged() {
  MinLODLimitChanged();   // that call updates both parameters
}
