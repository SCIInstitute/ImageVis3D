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


//!    File   : MergeDlg.h
//!    Author : Jens Krueger
//!             SCI Institute
//!             University of Utah
//!    Date   : February 2009
//
//!    Copyright (C) 2009 SCI Institute

#ifndef MERGEDLG_H
#define MERGEDLG_H

#include <ui_MergeDlg.h>
#include <vector>
#include <string>
#include <StdDefines.h>
#include "../Tuvok/Basics/Vectors.h"

class MainWindow;

class DataSetListElem {
public:
  DataSetListElem(const std::wstring strFilename) :
    m_strFilename(strFilename),
    m_strDisplayName(strFilename), /// \TODO: maybe come up with something "nicer" for display
    m_bAnalyzed(false),
    m_fScale(1.0),
    m_fBias(0.0),
    m_vDomainSize(0,0,0),
    m_vAspect(0,0,0),
    m_iComponentSize(0),
    m_iValueType(-1)
  {
  }

  std::wstring         m_strFilename;
  std::wstring         m_strDisplayName;

  bool                m_bAnalyzed;
  double              m_fScale;
  double              m_fBias;

  UINT64VECTOR3               m_vDomainSize;
  FLOATVECTOR3                m_vAspect;
  uint64_t                      m_iComponentSize;
  int                         m_iValueType;
  std::pair<double, double>   m_fRange;
  std::pair<int64_t, int64_t> m_iRange;
  std::pair<uint64_t, uint64_t>   m_uiRange;
};

class MergeDlg : public QDialog, protected Ui_MergeDlg
{
  Q_OBJECT
  public:
    MergeDlg(MainWindow* parent, Qt::WindowFlags flags = Qt::Tool);
    virtual ~MergeDlg();
    std::vector<DataSetListElem*> m_vDataSetList;
    bool UseMax() const { return radioButton_max->isChecked(); }
    bool UseCustomExpr() const { return grpCustomExpressionMode->isChecked(); }
    std::wstring GetCustomExpr() const { return txtExpression->text().toStdWString(); }

  protected slots:
    void AnalyzeCurrentDataset();
    void ChangedActiveDataset();
    void AddDataset();
    void RemoveDataset();
    void ExecuteMerge();
    void CancelMerge();
    void ChangedScale(double fScale);
    void ChangedBias(double fBias);
    void ToggleDefaultMergeSet(bool bChecked);
    void ToggleCustomMergeSet(bool bChecked);

  private:
    MainWindow*                     m_pMainWindow;

    void UpadeListView();
    void setupUi();
    void IsDatasetSelected(bool bIsDatasetsSelected);
    void UpdateValueFields();
};

#endif // MERGEDLG_H
