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
  \file    WGLContext.cpp
  \author  Tom Fogal
           SCI Institute
           University of Utah
  \brief   Establishes an OpenGL context on windows.
*/
#ifdef DETECTED_OS_WINDOWS
# include <windows.h>
# include <GL/glew.h>
# include <GL/wglew.h>
#else
# include <GL/glxew.h>
#endif

#include "WGLContext.h"
#include <sstream>

namespace tuvok
{

struct winfo
{
  HDC deviceContext;
  HGLRC renderingContext;
  HWND window;
};

static void outputLastError()
{
  DWORD lastError = GetLastError();
  LPVOID msgBuffer;
  FormatMessageW(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    lastError,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR)&msgBuffer,
    0, NULL
  );
  T_ERROR("Win32 error: %s", std::string((LPSTR)msgBuffer));
  LocalFree(msgBuffer);
}

WGLContext::WGLContext(uint32_t w, uint32_t h, uint8_t color_bits,
                             uint8_t depth_bits, uint8_t stencil_bits,
                             bool double_buffer, bool visible) :
  wi(new struct winfo())
{
  wi->deviceContext = NULL;
  wi->renderingContext = NULL;
  wi->window = NULL;
  if(w == 0 || h == 0 ||
     !(color_bits==8 || color_bits==16 || color_bits==24 || color_bits==32) ||
     !(depth_bits==8 || depth_bits==16 || depth_bits==24 || depth_bits==32 ||
       depth_bits==0) ||
     !(stencil_bits == 0 || stencil_bits == 8))
  {
    T_ERROR("Invalid parameters passed to constructor.");
    throw NoAvailableContext();
  }

  wi->window = CreateWindowExW(WS_EX_TOOLWINDOW, L"Static", L"GLContextWindow",
                               WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN ,
                               0, 0, w, h, 0, 0,
                               GetModuleHandle(NULL), 0);
  if (!wi->window)
  {
    outputLastError();
    return;
  }
  if(visible)
  {
    ShowWindow(wi->window, SW_SHOW);
  } 
  else
  {
    ShowWindow(wi->window, SW_HIDE);
  }

  wi->deviceContext = GetDC(wi->window);
  if(!wi->deviceContext)
  {
    // GetDC doesn't SetLastError, but this still works anyway...
    outputLastError();
    DestroyWindow(wi->window);
    throw NoAvailableContext();
  }

  PIXELFORMATDESCRIPTOR pfd;
  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_TYPE_RGBA;
  if (double_buffer) pfd.dwFlags |= PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = color_bits;
  pfd.cRedBits = 0;
  pfd.cRedShift = 0;
  pfd.cGreenBits = 0;
  pfd.cGreenShift = 0;
  pfd.cBlueBits = 0;
  pfd.cBlueShift = 0;
  pfd.cAlphaBits = 0;
  pfd.cAlphaShift = 0;
  pfd.cAccumBits = 0;
  pfd.cAccumRedBits = 0;
  pfd.cAccumGreenBits = 0;
  pfd.cAccumBlueBits = 0;
  pfd.cAccumAlphaBits = 0;
  pfd.cDepthBits = depth_bits;
  pfd.cStencilBits = stencil_bits;
  pfd.cAuxBuffers = 0;
  pfd.iLayerType = PFD_MAIN_PLANE;
  pfd.bReserved = 0;
  pfd.dwLayerMask = 0;
  pfd.dwVisibleMask = 0;
  pfd.dwDamageMask = 0;

  int pixelFormat = ChoosePixelFormat(wi->deviceContext, &pfd);

  if (!pixelFormat)
  {
    outputLastError();
    ReleaseDC(wi->window, wi->deviceContext);
    DestroyWindow(wi->window);
    throw NoAvailableContext();
  }

  PIXELFORMATDESCRIPTOR pfdResult;
  DescribePixelFormat(wi->deviceContext, pixelFormat,
                      sizeof(PIXELFORMATDESCRIPTOR), &pfdResult);

  if (!(pfdResult.dwFlags & PFD_SUPPORT_OPENGL))
  {
    T_ERROR("No OpenGL support.");
    ReleaseDC(wi->window, wi->deviceContext);
    DestroyWindow(wi->window);
    throw NoAvailableContext();
  }

  if (double_buffer && !(pfdResult.dwFlags & PFD_DOUBLEBUFFER))
  {
    WARNING("No double buffer support!");
  }

  std::ostringstream ss;
  if (pfdResult.cColorBits != color_bits) 
  {
    ss << "Color bits requested: " << color_bits << ", actual color bits: "
       << pfdResult.cColorBits << "\n";
  }
  if (pfdResult.cDepthBits != depth_bits) 
  {
    ss << "Depth bits requested " << depth_bits << ", actual depth bits: "
       << pfdResult.cDepthBits << "\n";
  }
  if (pfdResult.cStencilBits != stencil_bits) 
  {
    ss << "Stencil bits requested " << stencil_bits << ", actual stencil bits:"
       << pfdResult.cStencilBits << "\n";
  }
  MESSAGE("%s", ss.str().c_str());

  if (!SetPixelFormat(wi->deviceContext, pixelFormat, &pfd)) 
  {
    outputLastError();
    ReleaseDC(wi->window, wi->deviceContext);
    DestroyWindow(wi->window);
    throw NoAvailableContext();
  }

  wi->renderingContext = wglCreateContext(wi->deviceContext);

  if (!wi->renderingContext) 
  {
    outputLastError();
    ReleaseDC(wi->window, wi->deviceContext);
    DestroyWindow(wi->window);
    throw NoAvailableContext();
  }
  makeCurrent();
}

WGLContext::~WGLContext()
{
  wglDeleteContext(wi->renderingContext);
  ReleaseDC(wi->window, wi->deviceContext);
  DestroyWindow(wi->window);
}

bool WGLContext::isValid() const
{
  // Object would not be created otherwise (throw in constructor)
  return true;
}

bool WGLContext::makeCurrent()
{
  if(!wglMakeCurrent(wi->deviceContext, wi->renderingContext)) 
  {
    outputLastError();
    return false;
  }
  return true;
}

bool WGLContext::swapBuffers()
{
  if(!isValid()) { return false; }

  if(!SwapBuffers(wi->deviceContext)) 
  {
    outputLastError();
    return false;
  }
  return true;
}

} /* tuvok namespace */
