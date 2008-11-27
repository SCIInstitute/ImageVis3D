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


//!    File   : ImageVis3D_Locking.cpp
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : November 2008
//
//!    Copyright (C) 2008 SCI Institute

#include "ImageVis3D.h"
#include <Basics/SysTools.h>

#include <QtGui/QMdiSubWindow>

using namespace std;

void MainWindow::UpdateLockView() {
  m_bUpdatingLockView = true;
  listWidget_Lock->clear();
  if (m_ActiveRenderWin == NULL) {
    label_LockWinowID->setVisible(false);
    return;
  }

  QString text = "Lock window " + m_ActiveRenderWin->GetWindowID();
  label_LockWinowID->setText(text);
  label_LockWinowID->setVisible(true);

  // find correct lock mode
  size_t iLockType = 0;
    if (radioButton_RenderModeLock->isChecked()) iLockType = 1; else
      if (radioButton_ToolsLock->isChecked()) iLockType = 2; else
        if (radioButton_FiltersLock->isChecked()) iLockType = 3;

  for (int i = 0;i<mdiArea->subWindowList().size();i++) {
    QWidget* w = mdiArea->subWindowList().at(i)->widget();
    RenderWindow* renderWin = qobject_cast<RenderWindow*>(w);

    if (renderWin != m_ActiveRenderWin) {
      listWidget_Lock->addItem(renderWin->GetWindowID());

      // check if lock for this item is allready set
      for (size_t j = 0;j<m_ActiveRenderWin->m_vpLocks[iLockType].size();j++) {
        if (renderWin == m_ActiveRenderWin->m_vpLocks[iLockType][j]) {
          listWidget_Lock->item(listWidget_Lock->count()-1)->setSelected(true);
        }
      }
    }
  }
  m_bUpdatingLockView = false;
}

void MainWindow::LockModalityChange() {
  UpdateLockView();
}

void MainWindow::ChangeLocks() {
  if (m_bUpdatingLockView) return;
  bool bAddedNewLocks = false;
  // find correct lock mode
  size_t iLockType = 0;
    if (radioButton_RenderModeLock->isChecked()) iLockType = 1; else
      if (radioButton_ToolsLock->isChecked()) iLockType = 2; else
        if (radioButton_FiltersLock->isChecked()) iLockType = 3;
  // update lock arrays
  m_ActiveRenderWin->m_vpLocks[iLockType].clear();

  RemoveAllLocks(m_ActiveRenderWin);


  for (int i = 0;i<listWidget_Lock->count();i++) {
    if (listWidget_Lock->item(i)->isSelected()) {
      // get corresponding renderwindow
      RenderWindow* otherWin = NULL;
      for (int j = 0;j<mdiArea->subWindowList().size();j++) {
         QWidget* w = mdiArea->subWindowList().at(j)->widget();
         RenderWindow* renderWin = qobject_cast<RenderWindow*>(w);
         if (QString(renderWin->GetWindowID()) == listWidget_Lock->item(i)->text() ) {
            otherWin = renderWin;
            break;
         } 
      }
      if (otherWin != NULL) {
        m_ActiveRenderWin->m_vpLocks[iLockType].push_back(otherWin);
        // if renderwin is not in the list of the other window yet
        bool bFound = false;
        for (size_t j = 0;j<otherWin->m_vpLocks[iLockType].size();j++) {
          if (otherWin->m_vpLocks[iLockType][j] == m_ActiveRenderWin) {
            bFound = true;
            break;
          }
        }
        if (!bFound) otherWin->m_vpLocks[iLockType].push_back(m_ActiveRenderWin);

        // resolve transitive locks by adding all the other locks of the other window to this one

        for (size_t j = 0;j<otherWin->m_vpLocks[iLockType].size();j++) {
          bFound = false;
          for (size_t k = 0;k<m_ActiveRenderWin->m_vpLocks[iLockType].size();k++) {
            if (otherWin->m_vpLocks[iLockType][j] == m_ActiveRenderWin->m_vpLocks[iLockType][k]) {
              bFound = true;
              break;
            }
          }
          if (!bFound) {
            m_ActiveRenderWin->m_vpLocks[iLockType].push_back(otherWin->m_vpLocks[iLockType][j]);
            otherWin->m_vpLocks[iLockType][j]->m_vpLocks[iLockType].push_back(m_ActiveRenderWin);
            bAddedNewLocks = true;
          }

        }
      }
    }
  }
  if (bAddedNewLocks) UpdateLockView();
}

void MainWindow::RemoveAllLocks(RenderWindow* sender) {
  for (int i = 0;i<RenderWindow::ms_iLockCount;i++) {
    RenderWindow* otherWin = NULL;
    for (int j = 0;j<mdiArea->subWindowList().size();j++) {
      QWidget* w = mdiArea->subWindowList().at(j)->widget();
      RenderWindow* otherWin = qobject_cast<RenderWindow*>(w);
      if (otherWin == sender) continue;

      for (int k = 0;k<otherWin->m_vpLocks[i].size();k++) {
        if (otherWin->m_vpLocks[i][k] == sender) {
           otherWin->m_vpLocks[i].erase(otherWin->m_vpLocks[i].begin()+k);
        }
      }

    }
  }
}