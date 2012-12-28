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
#include <stdexcept>
#include <CGLCurrent.h>
#include <CGLTypes.h>
#include <OpenGL.h>
#include "cgl-context.h"
#include "Controller/Controller.h"

namespace tuvok {

struct cinfo {
  CGLPixelFormatObj pix;
  CGLContextObj ctx;
};

TvkCGLContext::TvkCGLContext(uint32_t, uint32_t, uint8_t color_bits,
                             uint8_t depth_bits, uint8_t stencil_bits,
                             bool double_buffer, bool) :
  ci(new struct cinfo())
{
  ci->pix = NULL; ci->ctx = NULL;
  CGLPixelFormatAttribute attribs[] = {
    kCGLPFAColorSize, static_cast<CGLPixelFormatAttribute>(color_bits),
    kCGLPFAAlphaSize, static_cast<CGLPixelFormatAttribute>(8),
    kCGLPFADepthSize, static_cast<CGLPixelFormatAttribute>(depth_bits),
    kCGLPFAStencilSize, static_cast<CGLPixelFormatAttribute>(stencil_bits),
    double_buffer ? kCGLPFADoubleBuffer :
                    static_cast<CGLPixelFormatAttribute>(0),
    static_cast<CGLPixelFormatAttribute>(NULL)
  };
  int nscreens;
  CGLError glerr;
  glerr = CGLChoosePixelFormat(attribs, &ci->pix, &nscreens);
  if(glerr != kCGLNoError) {
    T_ERROR("CGL pixel format error: %d", static_cast<int>(glerr));
    throw std::runtime_error("CGL error.");
  }

  glerr = CGLCreateContext(ci->pix, NULL, &ci->ctx);
  if(glerr != kCGLNoError) {
    T_ERROR("CGL ctx creation error: %d", static_cast<int>(glerr));
    throw std::runtime_error("CGL error.");
  }
}

TvkCGLContext::~TvkCGLContext() {
  CGLReleaseContext(ci->ctx);
  CGLReleasePixelFormat(ci->pix);
}

bool TvkCGLContext::isValid() const { return ci->ctx != NULL; }
bool TvkCGLContext::makeCurrent() {
  return CGLSetCurrentContext(ci->ctx) == kCGLNoError;
}
bool TvkCGLContext::swapBuffers() {
  return CGLFlushDrawable(ci->ctx) == kCGLNoError;
}

}
