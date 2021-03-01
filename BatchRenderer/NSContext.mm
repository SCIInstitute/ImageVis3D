/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
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

/// \author James Hughes
/// \date   December 2012

#include "NSContext.h"

#include <iostream>
#import <OpenGL/OpenGL.h>
#import <AppKit/AppKit.h>

namespace tuvok {

// Simple error reporting macros
#define REPORTGLERROR(task) { GLenum tGLErr = glGetError(); if (tGLErr != GL_NO_ERROR) { std::cout << "OpenGL error " << tGLErr << " while " << task << "\n"; } }
#define REPORT_ERROR_AND_EXIT(desc) { std::cout << desc << "\n"; return; }
#define NULL_ERROR_EXIT(test, desc) { if (!test) REPORT_ERROR_AND_EXIT(desc); }

struct NSContextInfo
{
  NSOpenGLContext*        openGLContext;
};

//------------------------------------------------------------------------------
NSContext::NSContext(
    uint32_t, uint32_t, uint8_t colorBits,
    uint8_t depthBits, uint8_t,
    bool doubleBuffer, bool) :
    mCI(new NSContextInfo)
{
  // On screen doesn't matter, we will be using an FBO anyways.
  NSOpenGLPixelFormatAttribute  attributes[] =
  {
    NSOpenGLPFAAccelerated,           // 'on screen' accelerated.
    NSOpenGLPFANoRecovery,            // Do not fall back to sw.
    NSOpenGLPFAColorSize, colorBits,
    NSOpenGLPFADepthSize, depthBits,
    NSOpenGLPFAAlphaSize, 8,
    doubleBuffer ? NSOpenGLPFADoubleBuffer : (NSOpenGLPixelFormatAttribute)0, 
    (NSOpenGLPixelFormatAttribute) 0
  };

  NSOpenGLPixelFormat*    pixFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];

  // Create the OpenGL context to render with (with color and depth buffers)
  mCI->openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixFormat shareContext:nil];
  if (mCI->openGLContext == NULL)
  {
    std::cerr << "Unable to create NSOpenGLContext.\n";
    throw std::runtime_error("NS context error.");
  }
}

//------------------------------------------------------------------------------
NSContext::~NSContext()
{
  [mCI->openGLContext clearDrawable];
  [mCI->openGLContext release];
}

//------------------------------------------------------------------------------
bool NSContext::isValid() const
{
  return (mCI->openGLContext != NULL); 
}

//------------------------------------------------------------------------------
bool NSContext::makeCurrent()
{
  [mCI->openGLContext makeCurrentContext];
  return [NSOpenGLContext currentContext] == mCI->openGLContext;
}

//------------------------------------------------------------------------------
bool NSContext::swapBuffers()
{
  return false;
}

} // end namespace tuvok
