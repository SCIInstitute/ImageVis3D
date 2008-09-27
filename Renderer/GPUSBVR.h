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
  \file    GPUSBVR.h
  \author    Jens Krueger
        SCI Institute
        University of Utah
  \version  1.0
  \date    August 2008
*/


#pragma once

#ifndef GPUSBVR_H
#define GPUSBVR_H

#include <Renderer/AbstrRenderer.h>
#include <Renderer/GPUMemMan/GPUMemMan.h>
#include <Renderer/SBVRGeogen.h>
#include <Renderer/GLFBOTex.h>

class GPUSBVR : public AbstrRenderer {
  public:
    GPUSBVR(MasterController* pMasterController);
    virtual ~GPUSBVR();

    void Initialize();
    void Paint();
    void Resize(int width, int height);
    void Cleanup();

    virtual void Set1DTrans(TransferFunction1D* p1DTrans);
    virtual void Set2DTrans(TransferFunction2D* p2DTrans);
    virtual void Changed1DTrans();
    virtual void Changed2DTrans();

    void SetWireFrame(bool bRenderWireframe) {m_bRenderWireframe = bRenderWireframe;}
    bool GetWireFrame() {return m_bRenderWireframe;}
    void CheckForRedraw();

    void SetRotation(FLOATVECTOR2 vRot) {m_vRot = vRot;}
    void SetCurrentView(int iCurrentView) {m_iCurrentView = iCurrentView;}
    int GetCurrentView() {return m_iCurrentView;}

  protected:
    FLOATMATRIX4  m_matModelView;
    SBVRGeogen    m_SBVRGeogen;
    GLTexture2D*  m_IDTex[3];
    int           m_iCurrentView;
    FLOATVECTOR2  m_vRot;
    bool          m_bDelayedCompleteRedraw;
    bool          m_bRenderWireframe;
    GLFBOTex*     m_pFBO3DImage;

    void DrawLogo();
    void UpdateGeoGen(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick);
};

#endif // GPUSBVR_H
