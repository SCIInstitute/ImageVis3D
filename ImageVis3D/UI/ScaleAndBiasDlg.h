/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2010 Interactive Visualization and Data Analysis Group.


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

//!    File   : ScaleAndBiasDlg.h
//!    Author : Jens Krueger
//!             IVCI & DFKI & MMCI, Saarbruecken
//!             SCI Institute, University of Utah
//!    Date   : July 2010
//
//!    Copyright (C) 2010 DFKI, MMCI, SCI Institute


#ifndef SCALEANDBIASDLG_H
#define SCALEANDBIASDLG_H

#include <memory>

#include "UI/AutoGen/ui_ScaleAndBiasDlg.h"
#include <StdDefines.h>
#include "../Tuvok/Basics/Vectors.h"

namespace tuvok {
  class RenderMesh;
};

class ScaleAndBiasDlg : public QDialog, protected Ui_ScaleAndBiasDlg
{
  Q_OBJECT
  public:
    ScaleAndBiasDlg(std::shared_ptr<tuvok::RenderMesh> mesh,
                    size_t index,
                    const FLOATVECTOR3& vmin, 
                    const FLOATVECTOR3& vmax,
                    QWidget* parent = 0 , 
                    Qt::WindowFlags flags = 0);
    virtual ~ScaleAndBiasDlg();

    FLOATVECTOR3 m_scaleVec;
    FLOATVECTOR3 m_biasVec;

    bool GetApplyAll() const;
    FLOATMATRIX4 GetExpertTransform() const;
    void SetExpertTransform(const FLOATMATRIX4& m);

    size_t m_index;
    std::shared_ptr<tuvok::RenderMesh> m_pMesh;

  protected slots:
    void ScaleIsotropic();
    void ScaleUnisotropic();
    void ScaleIsotropicVol();
    void ScaleUnisotropicVol();
    void ValuesChanged();
    void ApplyExpertMatrix();
    void CopyScaleAndBias();
    void ToggleExpertView();
    void InvertMatrix();
    void RestoreLast();
    void Restore();
    void Save();
    void Apply();

  signals:
    void SaveTransform(ScaleAndBiasDlg*);
    void RestoreTransform(ScaleAndBiasDlg*);
    void ApplyTransform(ScaleAndBiasDlg*);
    void ApplyMatrixTransform(ScaleAndBiasDlg*);

  private:
    FLOATVECTOR3 m_min;
    FLOATVECTOR3 m_max;
    FLOATVECTOR3 m_minVolume;
    FLOATVECTOR3 m_maxVolume;

    void UpdatePreSize();
    void UpdatePostSize();
    void setupUi(QDialog *ScaleAndBiasDlg, const QString& strDesc);

};

#endif // SCALEANDBIASDLG_H
