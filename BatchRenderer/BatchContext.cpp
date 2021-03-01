/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2010 Scientific Computing and Imaging Institute,
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
  \file    batchContext.cpp
  \author  Tom Fogal
           SCI Institute
           University of Utah
  \brief   Establishes an OpenGL context.
*/

#include <iostream>
#include <stdexcept>

#include <GLEW/GL/glew.h>

#include "Renderer/GL/GLStateManager.h"
#include "BatchContext.h"

#include "CGLContext.h"
#include "GLXContext.h"
#include "WGLContext.h"
#include "NSContext.h"

namespace tuvok
{

BatchContext::BatchContext() :
    Context(0)    // Tuvok specific
{
  ctx = nullptr;  // Tuvok specific
}

BatchContext* BatchContext::Create(uint32_t width, uint32_t height,
                                   uint8_t color_bits, uint8_t depth_bits,
                                   uint8_t stencil_bits, bool double_buffer,
                                   bool visible)
{
  BatchContext* bctx;
#ifdef DETECTED_OS_WINDOWS
  bctx = new WGLContext(width, height, color_bits, depth_bits, stencil_bits,
                       double_buffer, visible);
#elif defined(DETECTED_OS_APPLE) && defined(USE_CGL)
  bctx = new CGLContext(width, height, color_bits, depth_bits, stencil_bits,
                       double_buffer, visible);
#elif defined(DETECTED_OS_APPLE)
  bctx = new NSContext(width, height, color_bits, depth_bits, stencil_bits,
                      double_buffer, visible);
#else
  bctx = new GLXBatchContext(width, height, color_bits, depth_bits, stencil_bits,
                             double_buffer, visible);
#endif
  if (bctx->makeCurrent() == false)
    std::cerr << "Unable to make context current!" << std::endl;

  GLenum glerr = glewInit();
  if (GLEW_OK != glerr) 
  {
    std::cerr << "Error initializing GLEW: " << glewGetErrorString(glerr) << "\n";
    throw std::runtime_error("could not initialize GLEW.");
  }

  // Tuvok specific
  bctx->m_pState = std::shared_ptr<StateManager>(new GLStateManager());

  // Tuvok specific (this is really bad, but no one uses the pointer to the
  // context anyways...).
  bctx->ctx = (const void*)bctx;

  return bctx;
}

}
