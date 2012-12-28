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

#include <iostream>
#include <stdexcept>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include "CGLContext.h"

namespace tuvok
{

struct CGLContextInfo
{
  CGLContextObj ctx;
};

CGLContext::CGLContext(uint32_t, uint32_t, uint8_t color_bits,
                             uint8_t depth_bits, uint8_t stencil_bits,
                             bool double_buffer, bool) :
  ci(new struct CGLContextInfo())
{
  CGLPixelFormatAttribute attribs[] = {
    kCGLPFAAccelerated,
    kCGLPFANoRecovery,          // Do not fall back to software.
    kCGLPFAColorSize, static_cast<CGLPixelFormatAttribute>(color_bits),
    kCGLPFAAlphaSize, static_cast<CGLPixelFormatAttribute>(8),
    kCGLPFADepthSize, static_cast<CGLPixelFormatAttribute>(depth_bits),
    kCGLPFAStencilSize, static_cast<CGLPixelFormatAttribute>(stencil_bits),
    double_buffer ? kCGLPFADoubleBuffer :
                    static_cast<CGLPixelFormatAttribute>(0),
    static_cast<CGLPixelFormatAttribute>(NULL)
  };

  int nscreens;
  CGLPixelFormatObj pix;
  CGLError glerr = CGLChoosePixelFormat(attribs, &pix, &nscreens);
  if (glerr != kCGLNoError)
  {
    std::cerr << "CGL pixel format error: " << static_cast<int>(glerr);
    throw std::runtime_error("CGL error.");
  }

  glerr = CGLCreateContext(pix, NULL, &ci->ctx);
  if (glerr != kCGLNoError)
  {
    std::cerr << "CGL ctx creation error: %d" << static_cast<int>(glerr);
    throw std::runtime_error("CGL error.");
  }
  CGLDestroyPixelFormat(pix);
}

//------------------------------------------------------------------------------
CGLContext::~CGLContext()
{
  CGLReleaseContext(ci->ctx);
}

//------------------------------------------------------------------------------
bool CGLContext::isValid() const
{
  return ci->ctx != NULL;
}

//------------------------------------------------------------------------------
bool CGLContext::makeCurrent()
{
  return CGLSetCurrentContext(ci->ctx) == kCGLNoError;
}

//------------------------------------------------------------------------------
bool CGLContext::swapBuffers()
{
  return CGLFlushDrawable(ci->ctx) == kCGLNoError;
}

}
