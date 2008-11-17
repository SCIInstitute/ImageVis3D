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
  \file    GLSBVR.h
  \author  Jens Krueger
           SCI Institute
           University of Utah
  \version 1.0
  \date    August 2008
*/


#pragma once

#ifndef GLSBVR_H
#define GLSBVR_H

#include <Renderer/GLRenderer.h>
#include <Renderer/SBVRGeogen.h>

/** \class GLSBVR
 * Slice-based GPU volume renderer.
 *
 * GLSBVR is a slice based volume renderer which uses GLSL. */
class GLSBVR : public GLRenderer {
  public:
    /** Constructs a VRer with immediate redraw, and
     * wireframe mode off.
     * \param pMasterController message routing object */
    GLSBVR(MasterController* pMasterController);
    virtual ~GLSBVR();

    virtual bool LoadDataset(const std::string& strFilename);

    /** Deallocates GPU memory allocated during the rendering process. */
    virtual void Cleanup();

    /** Loads GLSL vertex and fragment shaders. */
    virtual bool Initialize();

    /** Set the oversampling ratio (e.g. 2 means twice the slices as needed).  Causes a full redraw. */
    virtual void SetSampleRateModifier(float fSampleRateModifier);

  protected:
    SBVRGeogen    m_SBVRGeogen;

    GLSLProgram*  m_pProgram1DTrans[2];
    GLSLProgram*  m_pProgram2DTrans[2];
    GLSLProgram*  m_pProgramIso;

    void SetBrickDepShaderVars(const Brick& currentBrick);

    virtual void Render3DView();

    virtual const FLOATVECTOR2 SetDataDepShaderVars();

};

#endif // GLSBVR_H
