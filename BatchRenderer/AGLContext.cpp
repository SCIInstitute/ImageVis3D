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
#include <AGL/agl.h>
#include <AGL/aglContext.h>
#include <AGL/aglRenderers.h>
#include "AGLContext.h"

namespace tuvok 
{

struct AGLContextInfo
{
  AGLContext ctx;
};

AGLBatchContext::AGLBatchContext(uint32_t, uint32_t, uint8_t color_bits,
                                 uint8_t depth_bits, uint8_t stencil_bits,
                                 bool double_buffer, bool visible) :
  mContextInfo(new AGLContextInfo())
{
  /*AGLRendererInfo ri = aglQueryRendererInfo( NULL, 0 );
  if ( ri != NULL )
  {
    unsigned int ii=0; 
    while( ri != NULL )
    {
      GLint vram, tram, accel, id;
      GLint mm = 1024*1024;
      aglDescribeRenderer( ri, AGL_RENDERER_ID, &id);
      aglDescribeRenderer( ri, AGL_ACCELERATED, &accel);
      aglDescribeRenderer( ri, AGL_VIDEO_MEMORY, &vram);
      aglDescribeRenderer( ri, AGL_TEXTURE_MEMORY, &tram);
      std::cout << "renderer " << ii << ": " << "id: " << id << " "<< "hardware: " << accel << " " << std::endl;
      std::cout << "\t"<< "vram (Mb): " << vram/mm << " "<< "tram (Mb): " << tram/mm << std::endl;
      ri = aglNextRendererInfo( ri ); 
      ii++;
    }
  }
  else
  {
    GLenum error = aglGetError();
    std::cout << "error: " << error << std::endl;
  }
  */

  mContextInfo->ctx = nullptr;
 
  /*
  // setup the pixelformatGLint 
  GLint attribs[] ={AGL_RGBA,AGL_DOUBLEBUFFER,AGL_ACCELERATED,AGL_NO_RECOVERY,AGL_DEPTH_SIZE, 16,AGL_NONE};
  const int width = 451;
  const int height = 123;
  AGLPixelFormat pixelFormat;
  AGLPbuffer pbuffer;
  AGLContext context, pbContext;
  long virtualScreen;
  CGOpenGLDisplayMask display2 =CGDisplayIDToOpenGLDisplayMask( CGMainDisplayID() );
  GDHandle display = GetMainDevice();
  Rect windowRect = { 100, 100, 100 + width, 100 + height }; 
  WindowRef window;
  // pbuffer pixelformat and context setup and creation
  printf( "%d\n", display2 );
  pixelFormat = aglChoosePixelFormat( display2, 1, attribs );
  pbContext = aglCreateContext( pixelFormat, NULL );
  mContextInfo->ctx = aglDestroyPixelFormat( pixelFormat );
  */

  GLint attribs[] =
  {
    AGL_BUFFER_SIZE, color_bits,
    AGL_RGBA,
    AGL_DEPTH_SIZE, depth_bits,
    AGL_STENCIL_SIZE, stencil_bits,
    AGL_NONE, // space for potential AGL_DOUBLEBUFFER
    AGL_NONE, // space for potential AGL_OFFSCREEN
    AGL_NONE,
  };

  if(double_buffer && !visible)
  {
    attribs[sizeof(attribs)/sizeof(GLint) - 2] = AGL_DOUBLEBUFFER;
    attribs[sizeof(attribs)/sizeof(GLint) - 1] = AGL_OFFSCREEN;
  }
  else if(double_buffer)
  {
    attribs[sizeof(attribs)/sizeof(GLint) - 2] = AGL_DOUBLEBUFFER;
  }
  else if(!visible)
  {
    attribs[sizeof(attribs)/sizeof(GLint) - 2] = AGL_OFFSCREEN;
  }

  AGLPixelFormat pix;
  pix = aglChoosePixelFormat(NULL, 0, attribs);
  if (pix == NULL)
  {
    std::cerr << "Cannot find appropriate pixel format." << static_cast<int>(aglGetError()) << std::endl;
    throw std::runtime_error("agl context creation error.");
  }

  std::cerr << "agl error: " << static_cast<int>(aglGetError());
  mContextInfo->ctx = aglCreateContext(pix, NULL);
  std::cerr << "Created AGL context" << std::endl;
  std::cerr << "agl error: " << static_cast<int>(aglGetError());
  if(mContextInfo->ctx == NULL)
  {
    std::cerr << "agl context creation error: " << static_cast<int>(aglGetError());
    throw std::runtime_error("agl context creation error.");
  }
  if (aglSetCurrentContext(NULL) == GL_TRUE)
  {
    const GLubyte* vendor     = glGetString(GL_VENDOR);
    const GLubyte* renderer   = glGetString(GL_RENDERER);
    const GLubyte* versionl   = glGetString(GL_VERSION);

    std::cerr << "OpenGL initialization. Running on a " << vendor << " " 
        << renderer << " with OpenGL version " << versionl << std::endl;
  }
  aglDestroyPixelFormat(pix);
}

AGLBatchContext::~AGLBatchContext() 
{
  aglDestroyContext(mContextInfo->ctx);
}

bool AGLBatchContext::isValid() const 
{ 
  return mContextInfo->ctx != NULL; 
}

bool AGLBatchContext::makeCurrent() 
{
  return aglSetCurrentContext(mContextInfo->ctx) == GL_TRUE;
}

bool AGLBatchContext::swapBuffers() 
{
  aglSwapBuffers(mContextInfo->ctx); return true; 
}

}
