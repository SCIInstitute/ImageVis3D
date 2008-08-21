/**************************************************************************************************************

(c) 2004-05 by Jens Schneider, TUM.3D
  mailto:jens.schneider@in.tum.de
  Computer Graphics and Visualization Group
    Institute for Computer Science I15
  Technical University of Munich

(c) 2007-2008 by Jens Krueger, SCI
  mailto:jens@sci.utah.edu

  - - - H I S T O R Y - - -
  
  Nov.15  If GLSLPROGRAM_STRICT is defined, treat compiler warnings as errors. This was default 
          for v1.0b. If left undefined, warnings are written to stdout, but ignored as long as the 
      result is a valid program object.
  Apr.07  renamed to GLSLProgram. Support for OpenGL 2.0.
  Apr.04  work-around for nVidia ß-driver bug with ARB_shader_objects
  Mar.05  added support for GLSLProgram
  Mar.05  added support for booleans 
  Mar.05  added support for matrices, including implicit casts from bool and int
  Mar.05  added support for programs in strings as opposed to programs in files
  Mar.05  improved mplicit casting of SetUniformVector().
      This is only enabled when GLSL_ALLOW_IMPLICIT_CASTS is defined, otherwise SetUniform behaves 
      strict, i.e. no uploading of ints to float uniforms.
  Mar.05  all text output is now done by the method message(). By default, text is send to stderr. 
      If you want to change this behaviour, change GLSLProgram::message() in Shader.inl
  
  Feb.05  added operator GLuint. This allows a GLSLProgram object to be cast into its respective GL handle.

  Aug.04  hey, it runs

**************************************************************************************************************/


#ifndef GLSLPROGRAM_H
#define GLSLPROGRAM_H

#define GLSL_ALLOW_IMPLICIT_CASTS ///< if enabled, SetUniformVector allows for implicit casting, which can cost performance
#define GLSLPROGRAM_STRICT        ///< if enabled, GLSL-compiler warnings are treated as errors

#ifdef _DEBUG
  #define GLSL_DEBUG  ///< switches on debugging output - can be changed per-class.
#endif
#ifdef _WIN32
  #ifdef GLSL_DEBUG
    #pragma message("    [GLSLProgram.h] DEBUG ON.\n")
  #else
    #pragma message("    [GLSLProgram.h] DEBUG OFF.\n")
  #endif
#endif

/**
 * Used to specify sources for shaders.
 * There are two different sources so far, either GLSLPROGRAM_DISK, which means that
 * a shader is specified by a C-string containing a file name, or GLSLPROGRAM_STRING,
 * in which case the shader is specified directly as sourcecode in a C-string.
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date March 2004
 */
typedef enum {
  GLSLPROGRAM_DISK=0,
  GLSLPROGRAM_STRING
} GLSLPROGRAM_SOURCE;

#include "GLInclude.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

class MasterController;

/** 
 * Wrapper for handling OpenGL 2.0 conformant program objects.
 * \class GLSLProgram
 * \version 1.0c
 * \warning include _before_ you include anything like gl.h, glut.h etc.
 * \warning requires the GL Extension Wrangler (glew) library.
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date November 2005
 */
class GLSLProgram {
public:
  GLSLProgram(MasterController* pMasterController);                                             ///< Standard Constructor.
  GLSLProgram(const GLSLProgram &other);                                                        ///< Copy Constructor
  GLSLProgram(MasterController* pMasterController, const char *VSFile, const char *FSFile,GLSLPROGRAM_SOURCE src=GLSLPROGRAM_DISK);  ///< Constructor. Loads any combination of vertex (VPFile) and fragment (FPFile) shaders either from disk or from a C-string.
  ~GLSLProgram();                                                                               ///< Destructor. Automatic clean-up.
  void Load(const char *VSFile, const char *FSFile,GLSLPROGRAM_SOURCE src=GLSLPROGRAM_DISK);    ///< Loads any combination of vertex (VPFile) and fragment (FPFile) shaders either from disk or from a C-string.
  void Enable(void);                                                                            ///< Enables this shader for rendering.
  void Disable(void);                                                                           ///< Disables this shader for rendering (using fixed function pipeline again).
  operator GLuint(void) const;                                                                  ///< Returns the handle of this shader.
  
  inline bool IsValid(void);    ///< returns true if this program is valid

  inline void SetUniformVector(const char *name,float x=0.0f, float y=0.0f, float z=0.0f, float w=0.0f) const;  ///< Sets an uniform parameter.
  inline void SetUniformVector(const char *name,int x=0, int y=0, int z=0, int w=0) const;            ///< Sets an uniform parameter.
  inline void SetUniformVector(const char *name,bool x=false, bool y=false,  bool z=false, bool w=false) const;  ///< Sets an uniform parameter.
  inline void SetUniformVector(const char *name,const float *v) const;    ///< Sets an uniform parameter.  
  inline void SetUniformVector(const char *name,const int *i) const;    ///< Sets an uniform parameter.  
  inline void SetUniformVector(const char *name,const bool *b) const;    ///< Sets an uniform parameter.  
  
  inline void SetUniformMatrix(const char *name,const float *m,bool bTranspose=false) const;    ///< Sets an uniform matrix. Matrices are always float.

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
  inline void SetUniformMatrix(const char *name,const int *m,bool bTranspose=false) const;    ///< Sets an uniform matrix. Matrices are always float.
  inline void SetUniformMatrix(const char *name,const bool *m,bool bTranspose=false) const;    ///< Sets an uniform matrix. Matrices are always float.
#endif

  inline void SetUniformArray(const char *name,const float *a) const;    ///< Sets an uniform array. User has to take care that a is large enough.
  inline void SetUniformArray(const char *name,const int   *a) const;    ///< Sets an uniform array. User has to take care that a is large enough.
  inline void SetUniformArray(const char *name,const bool  *a) const;    ///< Sets an uniform array. User has to take care that a is large enough.

private:
  void    Initialize(void);
  GLuint    LoadShader(const char*,GLenum,GLSLPROGRAM_SOURCE src);
  bool    WriteInfoLog(GLuint,bool);
  bool        CheckGLError(const char *pcError=NULL,const char *pcAdditional=NULL) const;
  bool    m_bInitialized;
  bool    m_bEnabled;
  GLuint    m_hProgram;  
  static bool m_bGlewInitialized;

  MasterController*  m_pMasterController;

};

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
#undef GLSL_ALLOW_IMPLICIT_CASTS
#endif

#endif // GLSLPROGRAM_H