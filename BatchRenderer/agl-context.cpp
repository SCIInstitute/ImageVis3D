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
#include <AGL/agl.h>
#include <AGL/aglRenderers.h>
#include "agl-context.h"
#include "Controller/Controller.h"

namespace tuvok {

struct cinfo {
  AGLContext ctx;
};

TvkAGLContext::TvkAGLContext(uint32_t, uint32_t, uint8_t color_bits,
                             uint8_t depth_bits, uint8_t stencil_bits,
                             bool double_buffer, bool visible) :
  ci(new struct cinfo())
{
  ci->ctx = NULL;
  GLint attribs[] = {
    AGL_BUFFER_SIZE, color_bits,
    AGL_RGBA,
    AGL_DEPTH_SIZE, depth_bits,
    AGL_STENCIL_SIZE, stencil_bits,
    AGL_NONE, // space for potential AGL_DOUBLEBUFFER
    AGL_NONE, // space for potential AGL_OFFSCREEN
    AGL_NONE,
  };
  if(double_buffer && !visible) {
    attribs[sizeof(attribs)/sizeof(GLint) - 2] = AGL_DOUBLEBUFFER;
    attribs[sizeof(attribs)/sizeof(GLint) - 1] = AGL_OFFSCREEN;
  } else if(double_buffer) {
    attribs[sizeof(attribs)/sizeof(GLint) - 2] = AGL_DOUBLEBUFFER;
  } else if(!visible) {
    attribs[sizeof(attribs)/sizeof(GLint) - 2] = AGL_OFFSCREEN;
  }
  AGLPixelFormat pix;
  pix = aglCreatePixelFormat(attribs);
  ci->ctx = aglCreateContext(pix, NULL);
  if(ci->ctx == NULL) {
    T_ERROR("agl context creation error: %d", static_cast<int>(aglGetError()));
    throw std::runtime_error("agl context creation error.");
  }
  aglDestroyPixelFormat(pix);
}

TvkAGLContext::~TvkAGLContext() {
  aglDestroyContext(ci->ctx);
}

bool TvkAGLContext::isValid() const { return ci->ctx != NULL; }
bool TvkAGLContext::makeCurrent() {
  return aglSetCurrentContext(ci->ctx) == GL_TRUE;
}
bool TvkAGLContext::swapBuffers() { aglSwapBuffers(ci->ctx); return true; }

}
