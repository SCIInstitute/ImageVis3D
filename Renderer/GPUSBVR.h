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
  \author  Jens Krueger
           SCI Institute
           University of Utah
  \version 1.0
  \date    August 2008
*/


#pragma once

#ifndef GPUSBVR_H
#define GPUSBVR_H

#include <Renderer/AbstrRenderer.h>
#include <Renderer/GPUMemMan/GPUMemMan.h>
#include <Renderer/SBVRGeogen.h>
#include <Renderer/GLRenderer.h>

/** \class GPUSBVR
 * Slice-based GPU volume renderer.
 *
 * GPUSBVR is a slice based volume renderer which uses GLSL. */
class GPUSBVR : public GLRenderer {
  public:
    /** Constructs a VRer with no view rotation, immediate redraws, and
     * wireframe mode off.
     * \param pMasterController message routing object */
    GPUSBVR(MasterController* pMasterController);
    virtual ~GPUSBVR();
    /** Determines whether we should redraw now.  Currently, if we've just
     * resized, we wait until something else changes before redrawing. */
    virtual bool CheckForRedraw();
    virtual bool LoadDataset(const std::string& strFilename);

    void Paint(bool bClearDepthBuffer=true);
    /** Change the size of the FBO we render to.  Any previous image is
     * destroyed, causing a full redraw on the next render.
     * \param width  new width of the view window
     * \param height new height of the view window */
    void Resize(int width, int height);
    /** Deallocates GPU memory allocated during the rendering process. */
    void Cleanup();

    /** Loads GLSL vertex and fragment shaders. */
    virtual void Initialize();

    /** Change the view rotation.  Causes a full redraw. */
    void SetRotation(FLOATVECTOR2 vRot) {if (m_vRot != vRot) {m_vRot = vRot; m_bCompleteRedraw = true;}}

  protected:
    FLOATMATRIX4  m_matModelView;
    SBVRGeogen    m_SBVRGeogen;
    GLTexture2D*  m_IDTex[3];
    FLOATVECTOR2  m_vRot;
    bool          m_bDelayedCompleteRedraw;

    GLFBOTex*     m_pFBO3DImage;
    GLSLProgram*  m_pProgram1DTrans[2];
    GLSLProgram*  m_pProgram2DTrans[2];
    GLSLProgram*  m_pProgramIso;


    void DrawLogo();
    void DrawBackGradient();
    void UpdateGeoGen(const std::vector<UINT64>& vLOD, const std::vector<UINT64>& vBrick);
    void RerenderPreviousResult();
    void SetDataDepShaderVars();

    void RenderSingle();
    void Render2by2();
    void Render1by3();

    void Render3DView();
    void Render2DView(EWindowMode eDirection, float fSliceIndex);
};

#endif // GPUSBVR_H
