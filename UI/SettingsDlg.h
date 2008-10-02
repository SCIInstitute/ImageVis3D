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


//!    File   : SettingsDlg.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : July 2008
//
//!    Copyright (C) 2008 SCI Institute

#ifndef SETTINGSDLG_H
#define SETTINGSDLG_H

#include <Controller/MasterController.h>
#include <UI/AutoGen/ui_SettingsDlg.h>

class SettingsDlg : public QDialog, protected Ui_SettingsDlg
{
  Q_OBJECT
  public:

    enum TabID { MEM_TAB=0, UI_TAB };

    SettingsDlg(MasterController& masterController, TabID eTabID = MEM_TAB, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~SettingsDlg();

    UINT64 GetGPUMem();
    UINT64 GetCPUMem();

    void Data2Form(UINT64 iMaxCPU, UINT64 iMaxGPU);

  private:
    MasterController& m_MasterController;
    TabID             m_eTabID;

    void setupUi(QDialog *SettingsDlg);

};

#endif // SETTINGSDLG_H
