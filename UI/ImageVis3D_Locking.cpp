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
  bool bAddedTransitiveLocks = false;
  // find correct lock mode
  size_t iLockType = 0;
    if (radioButton_RenderModeLock->isChecked()) iLockType = 1; else
      if (radioButton_ToolsLock->isChecked()) iLockType = 2; else
        if (radioButton_FiltersLock->isChecked()) iLockType = 3;

  size_t iLocksInList = 0;
  for (int i = 0;i<listWidget_Lock->count();i++)
    if (listWidget_Lock->item(i)->isSelected()) iLocksInList++;

  size_t iLocksInVector = m_ActiveRenderWin->m_vpLocks[iLockType].size();

  // otherwise update the locklists
  m_ActiveRenderWin->m_vpLocks[iLockType].clear();
  RemoveAllLocks(m_ActiveRenderWin, iLockType);

  // if we removed one lock we have to remove all locks, as they would be restored via transitivity otherwise
  if (iLocksInList < iLocksInVector) {
    UpdateLockView();
    return;
  }

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
      if (otherWin != NULL) bAddedTransitiveLocks = SetLock(iLockType, m_ActiveRenderWin, otherWin);
    }
  }
  if (bAddedTransitiveLocks) UpdateLockView();
}


bool MainWindow::SetLock(int iLockType, RenderWindow* winA, RenderWindow* winB) {
  bool bAddedTransitiveLocks = false;

  winA->m_vpLocks[iLockType].push_back(winB);
  // if renderwin is not in the list of the other window yet
  bool bFound = false;
  for (size_t j = 0;j<winB->m_vpLocks[iLockType].size();j++) {
    if (winB->m_vpLocks[iLockType][j] == winA) {
      bFound = true;
      break;
    }
  }
  if (!bFound) winB->m_vpLocks[iLockType].push_back(winA);

  // resolve transitive locks by adding all the other locks of the other window to this one

  for (size_t j = 0;j<winB->m_vpLocks[iLockType].size();j++) {
    bFound = false;
    for (size_t k = 0;k<winA->m_vpLocks[iLockType].size();k++) {
      if (winB->m_vpLocks[iLockType][j] == winA->m_vpLocks[iLockType][k]) {
        bFound = true;
        break;
      }
    }
    if (!bFound) {
      winA->m_vpLocks[iLockType].push_back(winB->m_vpLocks[iLockType][j]);
      winB->m_vpLocks[iLockType][j]->m_vpLocks[iLockType].push_back(winA);
      bAddedTransitiveLocks = true;
    }
  }

  return bAddedTransitiveLocks;
}

void MainWindow::RemoveAllLocks(RenderWindow* sender) {
  for (size_t i = 0;i<RenderWindow::ms_iLockCount;i++)
    RemoveAllLocks(sender, i);
}

void MainWindow::RemoveAllLocks(RenderWindow* sender, int iLockType) {
  RenderWindow* otherWin = NULL;
  for (int j = 0;j<mdiArea->subWindowList().size();j++) {
    QWidget* w = mdiArea->subWindowList().at(j)->widget();
    RenderWindow* otherWin = qobject_cast<RenderWindow*>(w);
    if (otherWin == sender) continue;

    for (size_t k = 0;k<otherWin->m_vpLocks[iLockType].size();) {
      if (otherWin->m_vpLocks[iLockType][k] == sender) {
         otherWin->m_vpLocks[iLockType].erase(otherWin->m_vpLocks[iLockType].begin()+k);
      } else k++;
    }
  }
}