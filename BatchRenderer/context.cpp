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
  \file    context.cpp
  \author  Tom Fogal
           SCI Institute
           University of Utah
  \brief   Establishes an OpenGL context.
*/
#include "StdTuvokDefines.h"
#include <stdexcept>
#include <GL/glew.h>
#include "Controller/Controller.h"

#include "context.h"

#include "agl-context.h"
#include "cgl-context.h"
#include "glx-context.h"
#include "wgl-context.h"

namespace tuvok {

TvkContext::~TvkContext() { }

TvkContext* TvkContext::Create(uint32_t width, uint32_t height,
                               uint8_t color_bits, uint8_t depth_bits,
                               uint8_t stencil_bits, bool double_buffer,
                               bool visible)
{
  TvkContext* ctx;
#ifdef DETECTED_OS_WINDOWS
  ctx = new TvkWGLContext(width, height, color_bits, depth_bits, stencil_bits,
                          double_buffer, visible);
#elif defined(DETECTED_OS_APPLE) && defined(USE_CGL)
  ctx = new TvkCGLContext(width, height, color_bits, depth_bits, stencil_bits,
                          double_buffer, visible);
#elif defined(DETECTED_OS_APPLE)
  ctx = new TvkAGLContext(width, height, color_bits, depth_bits, stencil_bits,
                          double_buffer, visible);
#else
  ctx = new TvkGLXContext(width, height, color_bits, depth_bits, stencil_bits,
                          double_buffer, visible);
#endif
  GLenum glerr = glewInit();
  if(GLEW_OK != glerr) {
    T_ERROR("Error initializing GLEW: %s", glewGetErrorString(glerr));
    throw std::runtime_error("could not initialize GLEW.");
  }
  return ctx;
}

}
