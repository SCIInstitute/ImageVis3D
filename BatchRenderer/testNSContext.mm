// clang++ testNSContext.mm -o testNSContext -framework Cocoa -framework OpenGL

// When building the batch renderer project with qmake, I found that the 
// OpenGL context was only usable with explicit context declaration 
// through CGMacros and ctx_obj. This file helps eliminate makefile
// variables.
// As it turns out, this is something related to the settings in qmake...

#include <iostream>
#import <OpenGL/OpenGL.h>
#import <AppKit/AppKit.h>

// Simple error reporting macros to help keep the sample code clean
#define REPORTGLERROR(task) { GLenum tGLErr = glGetError(); if (tGLErr != GL_NO_ERROR) { std::cout << "OpenGL error " << tGLErr << " while " << task << "\n"; } }
#define REPORT_ERROR_AND_EXIT(desc) { std::cout << desc << "\n"; return 1; }
#define NULL_ERROR_EXIT(test, desc) { if (!test) REPORT_ERROR_AND_EXIT(desc); }

int main (int argc, char * const argv[])
{
  NSOpenGLPixelBuffer*    pixBuf;
  NSOpenGLContext*        openGLContext;

  NSOpenGLPixelFormatAttribute  attributes[] = {
    NSOpenGLPFANoRecovery,
    NSOpenGLPFAAccelerated,
    NSOpenGLPFADepthSize, 24,
    (NSOpenGLPixelFormatAttribute) 0
  };

  NSOpenGLPixelFormat*    pixFormat = [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];

  // OpenGL context
  openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixFormat shareContext:nil];
  NULL_ERROR_EXIT(openGLContext, "Unable to create NSOpenGLContext");

  [openGLContext makeCurrentContext];

  // Test very basic calls to OpenGL
  const GLubyte* vendor     = glGetString(GL_VENDOR);
  const GLubyte* renderer   = glGetString(GL_RENDERER);
  const GLubyte* versionl   = glGetString(GL_VERSION);

  if (vendor && renderer && versionl)
    std::cerr << "OpenGL initialization. Running on a " << vendor << " " 
        << renderer << " with OpenGL version " << versionl << std::endl;
  else
    std::cerr << "Unable to obtain OpenGL strings." << std::endl;

  // Cleanup
  [openGLContext clearDrawable];
  [openGLContext release];

  return 0;
}

