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

/**
  \file    AbstrRenderer.h
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    August 2008
*/


#pragma once

#ifndef ABSTRRENDERER_H
#define ABSTRRENDERER_H

#include <string>

#include <StdDefines.h>
#include "../IO/VolumeDataset.h"
#include "../IO/TransferFunction1D.h"
#include "../IO/TransferFunction2D.h"
#include "../Renderer/GLTexture1D.h"
#include "../Renderer/GLTexture2D.h"

class MasterController;

/** \class AbstrRenderer
 * Base for all renderers. */
class AbstrRenderer {
  public:
    enum ERenderMode {
      RM_1DTRANS = 0,  /**< one dimensional transfer function */
      RM_2DTRANS,      /**< two dimensional transfer function */
      RM_ISOSURFACE,   /**< render isosurfaces                */
      RM_INVALID
    };
    ERenderMode GetRendermode() {return m_eRenderMode;}
    virtual void SetRendermode(ERenderMode eRenderMode);

    enum EViewMode {
      VM_SINGLE = 0,  /**< a single large image */
      VM_TWOBYTWO,    /**< four small images */
      VM_ONEBYTREE,   /**< one large and three small images */
      VM_INVALID
    };
    EViewMode GetViewmode() {return m_eViewMode;}
    virtual void SetViewmode(EViewMode eViewMode);

    enum EWindowMode {
      WM_3D = 0,      
      WM_CORONAL,    
      WM_AXIAL,   
      WM_SAGITTAL,   
      WM_INVALID
    };
    EWindowMode GetWindowmode(unsigned int iWindowIndex) {return m_eWindowMode[iWindowIndex];}
    virtual void SetWindowmode(unsigned int iWindowIndex, EWindowMode eWindowMode);

    bool GetUseLigthing() {return m_bUseLigthing;}
    virtual void SetUseLigthing(bool bUseLigthing);

    /** Default settings: 1D transfer function, one by three view, white text, black BG.
     * @param pMasterController message router */
    AbstrRenderer(MasterController* pMasterController);
    /** Deallocates dataset and transfer functions. */
    virtual ~AbstrRenderer();
    /** Sends a message to the master to ask for a dataset to be loaded.
     * The dataset is converted to UVF if it is not one already.
     * @param strFilename path to a file */
    virtual bool LoadDataset(const std::string& strFilename);
    /** Query whether or not we should redraw the next frame, else we should
     * reuse what is already rendered. */
    virtual bool CheckForRedraw() = 0;

    VolumeDataset*      GetDataSet() {return m_pDataset;}
    TransferFunction1D* Get1DTrans() {return m_p1DTrans;}
    TransferFunction2D* Get2DTrans() {return m_p2DTrans;}

    /** Force a redraw if we're currently using a one dimensional TF. */ 
    virtual void Changed1DTrans();
    /** Force a redraw if we're currently using a two dimensional TF. */ 
    virtual void Changed2DTrans();

    /** Sets up a gradient background which fades vertically.
     * @param vColors[0] is the color at the bottom;
     * @param vColors[1] is the color at the top. */
    virtual void SetBackgroundColors(FLOATVECTOR3 vColors[2]) {
        m_vBackgroundColors[0]=vColors[0];
        m_vBackgroundColors[1]=vColors[1];
    }
    virtual void SetTextColor(FLOATVECTOR4 vColor) {m_vTextColor=vColor;}
    FLOATVECTOR3 GetBackgroundColor(int i) const {return m_vBackgroundColors[i];}
    FLOATVECTOR4 GetTextColor() const {return m_vTextColor;}

    virtual void SetSampleRateModifier(float fSampleRateModifier);
    float GetSampleRateModifier() {return m_fSampleRateModifier;}

    virtual void SetIsoValue(float fIsovalue);
    float GetIsoValue() {return m_fIsovalue;}

  protected:
    MasterController*   m_pMasterController;
    bool                m_bRedraw;
    bool                m_bCompleteRedraw;
    ERenderMode         m_eRenderMode;
    EViewMode           m_eViewMode;
    EWindowMode         m_eWindowMode[4];
    bool                m_bUseLigthing;
    float               m_fSliceIndex[4];
    VolumeDataset*      m_pDataset;
    TransferFunction1D* m_p1DTrans;
    TransferFunction2D* m_p2DTrans;
    float               m_fSampleRateModifier;
    float               m_fIsovalue;


    FLOATVECTOR3        m_vBackgroundColors[2];
    FLOATVECTOR4        m_vTextColor;
};

#endif // ABSTRRENDERER_H
