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
#include "StdTuvokDefines.h"
#include <stdexcept>
#include <GL/glxew.h>

#include "glx-context.h"
#include "Controller/Controller.h"

namespace tuvok {

struct xinfo {
  Display *display;
  XVisualInfo *visual;
  Window win;
  GLXContext ctx;
  Colormap cmap;
};

static struct xinfo x_connect(uint32_t, uint32_t, bool, bool);
static XVisualInfo* find_visual(Display*, bool);
static void glx_init(Display*, XVisualInfo*, GLXContext&);

TvkGLXContext::TvkGLXContext(uint32_t w, uint32_t h, uint8_t,
                             uint8_t, uint8_t,
                             bool double_buffer,
                             bool visible) :
  xi(new struct xinfo())
{
#ifndef NDEBUG
  // Enable X debugging.
  _Xdebug = 1;
#endif
  // if you *really* require a specific value... just hack this class
  // to your liking.  You probably want to add those parameters to x_connect
  // and use them there.
  WARNING("Ignoring color, depth, stencil bits.  For many applications, it "
          "is better to let the GLX library choose the \"best\" visual.");
  *this->xi = x_connect(w, h, double_buffer, visible);
  glx_init(xi->display, xi->visual, xi->ctx);
  this->makeCurrent();
  MESSAGE("Current context: %p", glXGetCurrentContext());
}

TvkGLXContext::~TvkGLXContext()
{
  glXDestroyContext(xi->display, xi->ctx);
  XDestroyWindow(xi->display, xi->win);
  XFree(xi->visual);
  XCloseDisplay(xi->display);
  xi.reset();
}

bool TvkGLXContext::isValid() const
{
  return this->xi->display != NULL && this->xi->ctx != NULL;
}

bool TvkGLXContext::makeCurrent()
{
  if(glXMakeCurrent(this->xi->display, this->xi->win, this->xi->ctx) != True) {
    T_ERROR("Could not make context current!");
    return false;
  }
  return true;
}

bool TvkGLXContext::swapBuffers()
{
  glXSwapBuffers(this->xi->display, this->xi->win);
  // SwapBuffers generates an X error if it fails.
  return true;
}

static struct xinfo
x_connect(uint32_t width, uint32_t height, bool dbl_buffer, bool visible)
{
  struct xinfo rv;

  rv.display = XOpenDisplay(NULL);
  if(rv.display == NULL) {
    T_ERROR("Could not connect to display: '%s'!", XDisplayName(NULL));
    throw NoAvailableContext();
  }
  XSynchronize(rv.display, True);

  rv.visual = find_visual(rv.display, dbl_buffer);

  Window parent = RootWindow(rv.display, rv.visual->screen);

  XSetWindowAttributes xw_attr;
  xw_attr.override_redirect = False;
  xw_attr.background_pixel = 0;
  xw_attr.colormap = XCreateColormap(rv.display, parent, rv.visual->visual,
                                     AllocNone);
  xw_attr.event_mask = StructureNotifyMask | ExposureMask;

  // X will create BadValue exceptions if these aren't true... much easier to
  // debug via asserts.
  assert(rv.visual->depth > 0);
  assert(width > 0);
  assert(height > 0);
  rv.win = XCreateWindow(rv.display, parent, 0,0, width,height, 0,
                         rv.visual->depth,
                         InputOutput, rv.visual->visual,
                         CWBackPixel | CWBorderPixel | CWColormap |
                         CWOverrideRedirect | CWEventMask,
                         &xw_attr);
  XStoreName(rv.display, rv.win, "Tuvok");
  if(visible) {
    if(XMapRaised(rv.display, rv.win) != 0) {
      T_ERROR("Could not map window!");
      throw std::runtime_error("could not map window");
    }
  }
  XSync(rv.display, False);

  return rv;
}

static void
glx_init(Display *disp, XVisualInfo *visual, GLXContext& ctx)
{
  if(!glXQueryExtension(disp, NULL, NULL)) {
    T_ERROR("Display does not support glX.");
    return;
  }

  ctx = glXCreateContext(disp, visual, 0, GL_TRUE);
  if(!ctx) {
    T_ERROR("glX Context creation failed.");
  }
}

static XVisualInfo *
find_visual(Display *d, bool double_buffered)
{
    XVisualInfo *ret_v;
    // GLX_USE_GL is basically a no-op, so this provides a convenient
    // way of specifying double buffering or not.
    int att_buf = double_buffered ? GLX_DOUBLEBUFFER : GLX_USE_GL;
    int attr[] = {
      GLX_RGBA,
      att_buf,
      GLX_RED_SIZE,         5,
      GLX_GREEN_SIZE,       6,
      GLX_BLUE_SIZE,        5,
      GLX_ALPHA_SIZE,       8,
      GLX_DEPTH_SIZE,       8,
      GLX_ACCUM_RED_SIZE,   0,
      GLX_ACCUM_GREEN_SIZE, 0,
      GLX_ACCUM_BLUE_SIZE,  0,
      None
    };

    ret_v = glXChooseVisual(d, DefaultScreen(d), attr);
    MESSAGE("ChooseVisual got us %p", (const void*)ret_v);
    return ret_v;
}

} // namespace tuvok
