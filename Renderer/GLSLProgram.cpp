/**************************************************************************************************************

(c) 2004-05 by Jens Schneider, TUM.3D
  mailto:jens.schneider@in.tum.de
  Computer Graphics and Visualization Group
    Institute for Computer Science I15
  Technical University of Munich
  
**************************************************************************************************************/
#ifdef _WIN32
  #pragma warning( disable : 4996 ) // disable deprecated warning 
#endif

#include "GLSLProgram.h"
#include <windows.h>
#include "../Controller/MasterController.h"

#include "GLSLProgram.inl"

bool GLSLProgram::m_bGlewInitialized=false;    ///< GL Extension Wrangler (glew) is initialized on first instantiation

/**
 * Default Constructor.
 * Initializes glew on first instantiation. 
 * \param void
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 * \see Initialize()
 */
GLSLProgram::GLSLProgram(MasterController* pMasterController) :
  m_pMasterController(pMasterController),
  m_bInitialized(false),
  m_bEnabled(false),
  m_hProgram(0)
{
    Initialize();
}

/**
 * Copy Constructor.
 * Initializes glew on first instantiation. 
 * \param other - GLSLProgram object to initialize this with
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \remark this object will not be automatically enabled, even if other was.
 * \date Aug.2004
 * \see Initialize()
 */
GLSLProgram::GLSLProgram(const GLSLProgram &other) {

  m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::GLSLProgram(const GLSLProgram &other) - calling the copy constructor is _slow_\n");

  m_bInitialized = other.m_bInitialized;
  m_bEnabled     = false;  
  
  // Retrieve handles for all attached shaders
  GLint iNumAttachedShaders;
  glGetProgramiv(other.m_hProgram,GL_ATTACHED_SHADERS,&iNumAttachedShaders);
  if (iNumAttachedShaders==0) {
    m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::GLSLProgram(const GLSLProgram &other) - Error copying vertex shader.\n");
    m_bInitialized=false;
    return;
  }
  GLuint *hShader=new GLuint[iNumAttachedShaders];
  glGetAttachedShaders(other.m_hProgram,iNumAttachedShaders,&iNumAttachedShaders,hShader);

  // create a new program object
  m_hProgram=glCreateProgram();

  // Attach all shaders that were attached to other also to this
  for (int i=0; i<iNumAttachedShaders; i++) glAttachShader(m_hProgram,hShader[i]);
  
  // link the program together
  glLinkProgram(m_hProgram);

  // check for errors
  GLint iLinked;
  glGetProgramiv(m_hProgram,GL_LINK_STATUS,&iLinked);
        
  if (CheckGLError("GLSLProgram(const GLSLProgram &other)") || iLinked!=int(GL_TRUE)) {
    glDeleteProgram(m_hProgram);
    m_hProgram=0;
    m_bInitialized=false;
    return;
  }
  else {
    m_bInitialized=true;
  }
}



/**
 * Specialized Constructor.
 * Loads any combination of vertex and fragment shader from disk.
 * \param VSFile - name of the file containing the vertex shader
 * \param FSFile - name of the file containing the fragment shader
 * \param src - selects the source of vertex and fragment shader. Can be either GLSLPROGRAM_DISK or GLSLPROGRAM_STRING
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 * \see Initialize()
 * \see Load()
 * \see GLSLPROGRAM_SOURCE
 */
GLSLProgram::GLSLProgram(MasterController* pMasterController, const char *VSFile, const char *FSFile,GLSLPROGRAM_SOURCE src) :
  m_pMasterController(pMasterController),
  m_bInitialized(false),
  m_bEnabled(false),
  m_hProgram(0)
{
  Initialize();
  Load(VSFile,FSFile,src);
}



/**
 * Standard Destructor.
 * Cleans up the memory automatically.
 * \param void
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 */
GLSLProgram::~GLSLProgram() {
  glDeleteProgram(m_hProgram);
  m_hProgram=0;
}



/**
 * Returns the handle to this shader.
 * \param void
 * \return a const handle. If this is an invalid shader, the handle will be 0.
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Feb.2005
 */
GLSLProgram::operator GLuint(void) const {
  return m_hProgram;
}

/**
 * Initializes the class.
 * If GLSLProgram is initialized for the first time, initialize GLEW
 * \param void
 * \return void 
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 * \see m_bGlewInitialized
 */
void GLSLProgram::Initialize(void) {
  if (!m_bGlewInitialized) {
    if (GLEW_OK!=glewInit()) m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::Initialize() - GLEW initialization failed!\n");    
    if (GLEW_VERSION_2_0) m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","OpenGL 2.0 supported\n");
    else { // check for ARB extensions
      if (glewGetExtension("GL_ARB_shader_objects")) m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","ARB_shader_objects supported.\n");
      else {
        m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","ARB_shader_objects not supported!\n");
        exit(255);
      }
      if (glewGetExtension("GL_ARB_shading_language_100")) m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","ARB_shading_language_100 supported.\n");
      else {
        m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","ARB_shading_language_100 not supported!\n");
        exit(255);
      }
    }
    m_bGlewInitialized=true;
  }
#ifdef GLSL_DEBUG  // Anti-Joachim Tactics...
  else {
    if (glMultiTexCoord2f==NULL) m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::Initialize() - GLEW must be initialized. Set CShader::m_bGlewInitialized = false in Shader.cpp\n");
  }
#endif
}



/**
 * Loads vertex and fragment shader from disk/memory.
 * Loads any combination of vertex and fragment shader from disk or from a memory position.
 * Generates error/information messages to stdout during loading.
 * If nor successful the handle of the shader will be set to 0.
 * \param VSFile - name of the file containing the vertex shader
 * \param FSFile - name of the file containing the fragment shader
 * \param src - selects the source of vertex and fragment shader. Can be either GLSLPROGRAM_DISK or GLSLPROGRAM_STRING
 * \return void
 * \warning Uses glGetError()
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 * \see GLSLPROGRAM_SOURCE
 */
void GLSLProgram::Load(const char *VSFile, const char *FSFile,GLSLPROGRAM_SOURCE src) {
  CheckGLError();

  // load
  GLuint hVS=0;
  GLuint hFS=0;
  bool bVSSuccess=true;  // fixed function pipeline is always working
  if (VSFile!=NULL) {
    m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","\nVERTEX SHADER:\n");
    hVS=LoadShader(VSFile,GL_VERTEX_SHADER,src);
    if (hVS!=0) m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","OK.\n");
    else {
      bVSSuccess=false;
      if (src==GLSLPROGRAM_DISK) {
        m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","ERROR IN: ");
        m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",VSFile);
        m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","\n");
      }
      else {
        m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","---------- ERROR -----------\n");
        int iPos=0;
        int iLine=1;
        char chLine[32];
        char *chVerbose=new char[strlen(VSFile)+1];
        memcpy(chVerbose,VSFile,strlen(VSFile)+1);
        for (unsigned int i=0; i<strlen(VSFile); i++) {
          if (chVerbose[i]=='\n') {
            chVerbose[i]='\0';
            sprintf(chLine,"(%.4i) ",iLine++);
            m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",chLine);
            m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",&chVerbose[iPos]);
            m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","\n");
            iPos=i+1;
          }
        }
        delete[] chVerbose;
      }
    }
  }
  bool bFSSuccess=true;  // fixed function pipeline is always working
  if (FSFile!=NULL) {
    m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","\nFRAGMENT SHADER:\n");
    hFS=LoadShader(FSFile,GL_FRAGMENT_SHADER,src);
    if (hFS!=0) m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","OK.\n");
    else {
      bFSSuccess=false;
      if (src==GLSLPROGRAM_DISK) {
        m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","ERROR IN: ");
        m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",FSFile);
        m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","\n");
      }
      else {
        m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","---------- ERROR -----------\n");
        int iPos=0;
        int iLine=1;
        char chLine[32];
        char *chVerbose=new char[strlen(FSFile)+1];
        memcpy(chVerbose,FSFile,strlen(FSFile)+1);
        for (unsigned int i=0; i<strlen(FSFile); i++) {
          if (chVerbose[i]=='\n') {
            chVerbose[i]='\0';
            sprintf(chLine,"(%.4i) ",iLine++);
            m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",chLine);
            m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",&chVerbose[iPos]);
            m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","\n");
            iPos=i+1;
          }
        }
        delete[] chVerbose;
      } 
    }    
  }
  
  // attach to program object
  m_hProgram=glCreateProgram();
  if (hVS) glAttachShader(m_hProgram,hVS); 
  if (hFS) glAttachShader(m_hProgram,hFS);

  // link the program together
  m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","\nPROGRAM OBJECT: \n");
  if (bVSSuccess && bFSSuccess) {
    glLinkProgram(m_hProgram);

    // check for errors
    GLint iLinked;
    glGetProgramiv(m_hProgram,GL_LINK_STATUS,&iLinked);
    WriteInfoLog(m_hProgram,true);
      
    // flag shaders such that they can be deleted when they get detached
    if (hVS) glDeleteShader(hVS);
    if (hFS) glDeleteShader(hFS);
    if (CheckGLError("Load()") || iLinked!=GLint(GL_TRUE)) {
      glDeleteProgram(m_hProgram);
      m_hProgram=0;
      m_bInitialized=false;
      return;
    }
    else {
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","OK.\n\n");
      m_bInitialized=true;
    }
  }
  else {
    if (hVS) glDeleteShader(hVS);
    if (hFS) glDeleteShader(hFS);
    glDeleteProgram(m_hProgram);
    m_hProgram=0;
    m_bInitialized=false;
    if (!bVSSuccess && !bFSSuccess) m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","Error in vertex and fragment shaders\n");
    else if (!bVSSuccess) m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","Error in vertex shader\n");
    else if (!bFSSuccess) m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","Error in fragment shader\n");
  }
}



/**
 * Writes errors/information messages to stdout.
 * Gets the InfoLogARB of hObject and messages it.
 * \param hObject - a handle to the object.
 * \param bProgram - if true, hObject is a program object, otherwise it is a shader object.
 * \return true: InfoLogARB non-empty and GLSLPROGRAM_STRICT defined OR only warning, false otherwise
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \see m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",)
 * \date Aug.2004
 */
bool GLSLProgram::WriteInfoLog(GLuint hObject, bool bProgram) {
  // Check for errors
  GLint iLength;
  if (bProgram)  glGetProgramiv(hObject,GL_INFO_LOG_LENGTH,&iLength);
  else      glGetShaderiv(hObject,GL_INFO_LOG_LENGTH,&iLength);

  GLboolean bAtMostWarnings=true;
  if (iLength>1) {    
    char *pcLogInfo=new char[iLength];
    if (bProgram) {
      glGetProgramInfoLog(hObject,iLength,&iLength,pcLogInfo);
      bAtMostWarnings=glIsProgram(hObject);
    }
    else {
      glGetShaderInfoLog(hObject,iLength,&iLength,pcLogInfo);
      bAtMostWarnings=glIsShader(hObject);
    }
    m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",pcLogInfo);
    m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","\n");
    delete[] pcLogInfo;  
#ifdef GLSLPROGRAM_STRICT
    return true;
#endif
  }
  return !bool(bAtMostWarnings==GL_TRUE); // error occured?  
}



/**
 * Loads a vertex or fragment shader.
 * Loads either a vertex or fragment shader and tries to compile it.
 * \param ShaderDesc - name of the file containing the shader
 * \param Type - either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
 * \param src - defines the source of the shader. Can be either GLSLPROGRAM_DISK or GLSLPROGRAM_STRING.
 * \return a handle to the compiled shader if successful, 0 otherwise
 * \warning uses glGetError()
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Mar.2005
 * \see GLSLPROGRAM_SOURCE
 */
GLuint GLSLProgram::LoadShader(const char *ShaderDesc,GLenum Type,GLSLPROGRAM_SOURCE src) {
  // assert right type
  assert(Type==GL_VERTEX_SHADER || Type==GL_FRAGMENT_SHADER);

  CheckGLError();

  unsigned long lFileSize;
  char *pcShader;
  FILE *fptr;

  // Load and compile vertex shader
  switch(src) {
    case GLSLPROGRAM_DISK:
      fptr=fopen(ShaderDesc,"rb");
      if (!fptr) {
        m_pMasterController->DebugOut()->Error("GLSLProgram::LoadShader","GLSLProgram::LoadShader() - File %s not found!",ShaderDesc);
        return 0;
      }
      if (fseek(fptr,0,SEEK_END)) {
        fclose(fptr);
        m_pMasterController->DebugOut()->Error("GLSLProgram::LoadShader","Error reading file %s.",ShaderDesc);
        return 0;
      }
      lFileSize=ftell(fptr)/sizeof(char);
      fseek(fptr,0,SEEK_SET);
      pcShader=new char[lFileSize+1];
      pcShader[lFileSize]='\0';
      if (lFileSize!=fread(pcShader,sizeof(char),lFileSize,fptr)) {
        fclose(fptr);
        delete[] pcShader;
        m_pMasterController->DebugOut()->Error("GLSLProgram::LoadShader","Error reading file %s.",ShaderDesc);
        return 0;
      }
      fclose(fptr);
      break;
    case GLSLPROGRAM_STRING:
      pcShader=(char*)ShaderDesc;
      lFileSize=long(strlen(pcShader));
      break;
    default:
      m_pMasterController->DebugOut()->Error("GLSLProgram::LoadShader","Unknown source");
      return 0;
      break;
  }
  GLuint hShader=glCreateShader(Type);  
  glShaderSource(hShader,1,(const char**)&pcShader,NULL);  // upload null-terminated shader
  glCompileShader(hShader);
  if (pcShader!=ShaderDesc) delete[] pcShader;

  // Check for compile status
  GLint iCompiled;
  glGetShaderiv(hShader,GL_COMPILE_STATUS,&iCompiled);

  // Check for errors
  bool bError=false;
  if (WriteInfoLog(hShader,false)) {
    glDeleteShader(hShader);
    bError=true;
  }

  if (CheckGLError("LoadProgram()") || iCompiled!=GLint(GL_TRUE)) {
    glDeleteShader(hShader);
    bError=true;
  }

  if (bError) return 0;
  return hShader;
}



/**
 * Enables the program for rendering.
 * Generates error messages if something went wrong (i.e. program not initialized etc.)
 * \param void
 * \return void
 * \warning uses glGetError()
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 */
void GLSLProgram::Enable(void) {
  if (m_bInitialized) {
    CheckGLError();
    glUseProgram(m_hProgram);
    if (!CheckGLError("Enable()")) m_bEnabled=true;
  }
  else m_pMasterController->DebugOut()->Error("GLSLProgram::Enable","No program loaded!\n");
}



/**
 * Disables the program for rendering.
 * Generates error messages if something went wrong (i.e. program not initialized etc.)
 * \param void
 * \return void
 * \warning uses glGetError()
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 */
void GLSLProgram::Disable(void) {
  if (m_bInitialized) {
    CheckGLError();
    glUseProgram(0);
    if (!CheckGLError("Disable()")) m_bEnabled=false;
  }
  else m_pMasterController->DebugOut()->Error("GLSLProgram::Disable","No program loaded!\n");
}



/**
 * Checks and handles glErrors.
 * This routine is verbose when run with GLSL_DEBUG defined, only.
 * If called with NULL as parameter, queries glGetError() and returns true if glGetError() did not return GL_NO_ERROR.
 * If called with a non-NULL parameter, queries glGetError() and concatenates pcError and the verbosed glError.
 * If in debug mode, the error is output to stderr, otherwise it is silently ignored.
 * \param pcError first part of an error message. May be NULL.
 * \param pcAdditional additional part of error message. May be NULL.
 * \return bool specifying if an error occured (true) or not (false)
 * \warning uses glGetError()
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 */
bool GLSLProgram::CheckGLError(const char *pcError, const char *pcAdditional) const{
#ifdef GLSL_DEBUG
  if (pcError==NULL) {  // Simply check for error, true if an error occured.
#endif

    return (glGetError()!=GL_NO_ERROR);

#ifdef GLSL_DEBUG
  }
  else {  // print out error
    GLenum iError=glGetError();
    char *pcMessage;
    if (pcAdditional) {
      size_t len=16+strlen(pcError)+(pcAdditional ? strlen(pcAdditional) : 0);
      pcMessage=new char[len];
      sprintf(pcMessage,pcError,pcAdditional);
    }
    else pcMessage=(char*)pcError;

    char *output=new char[strlen(pcMessage)+128];
    switch (iError) {
      case GL_NO_ERROR:
        if (pcMessage!=pcError) delete[] pcMessage;
        return false;
        break;
      case GL_INVALID_ENUM:       sprintf(output,"%s - %s\n",pcMessage,"GL_INVALID_ENUM");    break;
      case GL_INVALID_VALUE:      sprintf(output,"%s - %s\n",pcMessage,"GL_INVALID_VALUE");    break;
      case GL_INVALID_OPERATION:  sprintf(output,"%s - %s\n",pcMessage,"GL_INVALID_OPERATION");  break;
      case GL_STACK_OVERFLOW:     sprintf(output,"%s - %s\n",pcMessage,"GL_STACK_OVERFLOW");  break;
      case GL_STACK_UNDERFLOW:    sprintf(output,"%s - %s\n",pcMessage,"GL_STACK_UNDERFLOW");  break;
      case GL_OUT_OF_MEMORY:      sprintf(output,"%s - %s\n",pcMessage,"GL_OUT_OF_MEMORY");    break;
      default:                    sprintf(output,"%s - unknown GL_ERROR\n",pcError);      break;    
    }
    if (pcMessage!=pcError) delete[] pcMessage;
    delete[] output;

    // display the error.
    m_pMasterController->DebugOut()->Error("GLSLProgram::CheckGLError",output);

    return true;  
  }
#endif
}


/**************************************************************************************************************

(c) 2004-05 by Jens Schneider, TUM.3D
  mailto:jens.schneider@in.tum.de
  Computer Graphics and Visualization Group
    Institute for Computer Science I15
  Technical University of Munich
  
**************************************************************************************************************/

/**
 * Returns true if this program is valid. 
 * \param void
 * \return true if this program was initialized properly
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Jun.2005
 */
inline bool GLSLProgram::IsValid(void) {
  return m_bInitialized;
}

/**
 * Sets an uniform vector parameter.
 * \warning uses glGetError();
 * \param name - name of the parameter
 * \param x,y,z,w - up to four float components of the vector to set.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 */
inline void GLSLProgram::SetUniformVector(const char *name,float x, float y, float z, float w) const{
  assert(m_bEnabled);
  CheckGLError();
  
  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocation(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformVector(%s,float,...) [getting adress]",name)) return;

  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformVector","Error getting address for %s.",name);
    return;  
  }

  glGetActiveUniform(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformVector(%s,float,...) [getting type]",name)) return;  

  switch (iType) {
    case GL_FLOAT:            glUniform1f(iLocation,x); break;
    case GL_FLOAT_VEC2:          glUniform2f(iLocation,x,y); break;
    case GL_FLOAT_VEC3:          glUniform3f(iLocation,x,y,z); break;
    case GL_FLOAT_VEC4:          glUniform4f(iLocation,x,y,z,w); break;

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
    case GL_INT:
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D: 
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_RECT_ARB:
    case GL_SAMPLER_2D_RECT_SHADOW_ARB:  glUniform1i(iLocation,int(x)); break;

    case GL_INT_VEC2:          glUniform2i(iLocation,int(x),int(y)); break;
    case GL_INT_VEC3:          glUniform3i(iLocation,int(x),int(y),int(z)); break;
    case GL_INT_VEC4:          glUniform4i(iLocation,int(x),int(y),int(z),int(w)); break;
    case GL_BOOL:              glUniform1f(iLocation,x); break;
    case GL_BOOL_VEC2:          glUniform2f(iLocation,x,y); break;
    case GL_BOOL_VEC3:          glUniform3f(iLocation,x,y,z); break;
    case GL_BOOL_VEC4:          glUniform4f(iLocation,x,y,z,w); break;
#endif

    default: 
      m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformVector","Unknown type for %s.",name);
      break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformVector(%s,float,...)",name);
#endif
}



/**
 * Sets an uniform vector parameter.
 * \warning uses glGetError();
 * \param name - name of the parameter
 * \param x,y,z,w - up to four bool components of the vector to set.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Mar.2005
 */
inline void GLSLProgram::SetUniformVector(const char *name,bool x, bool y, bool z, bool w) const {
  assert(m_bEnabled);
  CheckGLError();
  
  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocation(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformVector(%s,bool,...) [getting adress]",name)) return;

  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformVector","Error getting address for %s.",name);
    return;  
  }

  glGetActiveUniform(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformVector(%s,bool,...) [getting type]",name)) return;  

  switch (iType) {
    case GL_BOOL:            glUniform1i(iLocation,(x ? 1 : 0)); break;
    case GL_BOOL_VEC2:          glUniform2i(iLocation,(x ? 1 : 0),(y ? 1 : 0)); break;
    case GL_BOOL_VEC3:          glUniform3i(iLocation,(x ? 1 : 0),(y ? 1 : 0),(z ? 1 : 0)); break;
    case GL_BOOL_VEC4:          glUniform4i(iLocation,(x ? 1 : 0),(y ? 1 : 0),(z ? 1 : 0),(w ? 1 : 0)); break;

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
    case GL_FLOAT:            glUniform1f(iLocation,(x ? 1.0f : 0.0f)); break;
    case GL_FLOAT_VEC2:          glUniform2f(iLocation,(x ? 1.0f : 0.0f),(y ? 1.0f : 0.0f)); break;
    case GL_FLOAT_VEC3:          glUniform3f(iLocation,(x ? 1.0f : 0.0f),(y ? 1.0f : 0.0f),(z ? 1.0f : 0.0f)); break;
    case GL_FLOAT_VEC4:          glUniform4f(iLocation,(x ? 1.0f : 0.0f),(y ? 1.0f : 0.0f),(z ? 1.0f : 0.0f),(w ? 1.0f : 0.0f)); break;
    case GL_INT:            glUniform1i(iLocation,(x ? 1 : 0)); break;
    case GL_INT_VEC2:          glUniform2i(iLocation,(x ? 1 : 0),(y ? 1 : 0)); break;
    case GL_INT_VEC3:          glUniform3i(iLocation,(x ? 1 : 0),(y ? 1 : 0),(z ? 1 : 0)); break;
    case GL_INT_VEC4:          glUniform4i(iLocation,(x ? 1 : 0),(y ? 1 : 0),(z ? 1 : 0),(w ? 1 : 0)); break;
#endif

    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformVector(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",bool,...) - Unknown type\n"); 
      break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformVector(%s,bool,...)",name);
#endif
}



/**
 * Sets an uniform vector parameter.
 * \warning uses glGetError();
 * \param name - name of the parameter
 * \param x,y,z,w - four int components of the vector to set.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 */
inline void GLSLProgram::SetUniformVector(const char *name,int x,int y,int z,int w) const {
  assert(m_bEnabled);
  CheckGLError();

  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocationARB(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformVector(%s,int,...) [getting adress]" )) return;
  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformVector","Error getting address for %s.",name);
    return;  
  }


  glGetActiveUniform(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformVector(%s,int,...) [getting type]",name)) return;

  switch (iType) {
    case GL_INT:
    case GL_SAMPLER_1D: 
    case GL_SAMPLER_2D: 
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_RECT_ARB:
    case GL_SAMPLER_2D_RECT_SHADOW_ARB:  glUniform1i(iLocation,x); break;

    case GL_INT_VEC2:          glUniform2i(iLocation,x,y); break;
    case GL_INT_VEC3:          glUniform3i(iLocation,x,y,z); break;
    case GL_INT_VEC4:          glUniform4i(iLocation,x,y,z,w); break;

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
    case GL_BOOL:            glUniform1i(iLocation,x); break;
    case GL_BOOL_VEC2:          glUniform2i(iLocation,x,y); break;
    case GL_BOOL_VEC3:          glUniform3i(iLocation,x,y,z); break;
    case GL_BOOL_VEC4:          glUniform4i(iLocation,x,y,z,w); break;
    case GL_FLOAT:            glUniform1f(iLocation,float(x)); break;
    case GL_FLOAT_VEC2:          glUniform2f(iLocation,float(x),float(y)); break;
    case GL_FLOAT_VEC3:          glUniform3f(iLocation,float(x),float(y),float(z)); break;
    case GL_FLOAT_VEC4:          glUniform4f(iLocation,float(x),float(y),float(z),float(w)); break;
#endif

    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformVector(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",int,...) - Unknown type\n"); 
      break;
  }  
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformVector(%s,int,...)",name);
#endif
}



/**
 * Sets an uniform vector parameter.
 * \warning uses glGetError();
 * \param name - name of the parameter
 * \param v - a float vector containing up to four elements.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 */
inline void GLSLProgram::SetUniformVector(const char *name,const float *v) const {  
  assert(m_bEnabled);
  CheckGLError();

  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocation(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformVector(%s,float*) [getting adress]",name)) return;  

  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformVector","Error getting address for %s.",name);
    return;  
  }


  glGetActiveUniform(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformVector(%s,float*) [getting type]" ,name)) return;

  switch (iType) {
    case GL_FLOAT:            glUniform1fv(iLocation,1,v); break;
    case GL_FLOAT_VEC2:          glUniform2fv(iLocation,1,v); break;
    case GL_FLOAT_VEC3:          glUniform3fv(iLocation,1,v); break;
    case GL_FLOAT_VEC4:          glUniform4fv(iLocation,1,v); break;

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
    case GL_INT:
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D: 
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_RECT_ARB:
    case GL_SAMPLER_2D_RECT_SHADOW_ARB:  glUniform1i(iLocation,int(v[0])); break;

    case GL_INT_VEC2:          glUniform2i(iLocation,int(v[0]),int(v[1])); break;
    case GL_INT_VEC3:          glUniform3i(iLocation,int(v[0]),int(v[1]),int(v[2])); break;
    case GL_INT_VEC4:          glUniform4i(iLocation,int(v[0]),int(v[1]),int(v[2]),int(v[3])); break;
    case GL_BOOL:            glUniform1fv(iLocation,1,v); break;
    case GL_BOOL_VEC2:          glUniform2fv(iLocation,1,v); break;
    case GL_BOOL_VEC3:          glUniform3fv(iLocation,1,v); break;
    case GL_BOOL_VEC4:          glUniform4fv(iLocation,1,v); break;
#endif

    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformVector(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",float*) - Unknown type\n"); 
      break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformVector(%s,float*)",name);
#endif
}



/**
 * Sets an uniform vector parameter.
 * \warning uses glGetError();
 * \param name - name of the parameter
 * \param i - an int vector containing up to 4 elements.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Aug.2004
 */
inline void GLSLProgram::SetUniformVector(const char *name,const int *i) const {
  assert(m_bEnabled);
  CheckGLError();

  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocation(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformVector(%s,int*) [getting adress]",name)) return;
  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformVector","Error getting address for %s.",name);
    return;  
  }

  glGetActiveUniform(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformVector(%s,int*) [getting type]",name)) return;

  switch (iType) {
    case GL_INT:
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D: 
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_RECT_ARB:
    case GL_SAMPLER_2D_RECT_SHADOW_ARB:  glUniform1i(iLocation,i[0]); break;

    case GL_INT_VEC2:          glUniform2iv(iLocation,1,i); break;
    case GL_INT_VEC3:          glUniform3iv(iLocation,1,i); break;
    case GL_INT_VEC4:          glUniform4iv(iLocation,1,i); break;
#ifdef GLSL_ALLOW_IMPLICIT_CASTS
    case GL_BOOL:            glUniform1iv(iLocation,1,i); break;
    case GL_BOOL_VEC2:          glUniform2iv(iLocation,1,i); break;
    case GL_BOOL_VEC3:          glUniform3iv(iLocation,1,i); break;
    case GL_BOOL_VEC4:          glUniform4iv(iLocation,1,i); break;
    case GL_FLOAT:            glUniform1f(iLocation,float(i[0])); break;
    case GL_FLOAT_VEC2:          glUniform2f(iLocation,float(i[0]),float(i[1])); break;
    case GL_FLOAT_VEC3:          glUniform3f(iLocation,float(i[0]),float(i[1]),float(i[2])); break;
    case GL_FLOAT_VEC4:          glUniform4f(iLocation,float(i[0]),float(i[1]),float(i[2]),float(i[3])); break;
#endif
    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformVector(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",int*) - Unknown type\n");
      break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformVector(%s,int*)",name);
#endif
}



/**
 * Sets an uniform vector parameter.
 * \warning uses glGetError();
 * \param name - name of the parameter
 * \param b - a bool vector containing up to 4 elements.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Mar.2005
 */
inline void GLSLProgram::SetUniformVector(const char *name,const bool *b) const {
  assert(m_bEnabled);
  CheckGLError();

  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocation(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformVector(%s,bool*) [getting adress]",name)) return;
  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformVector","Error getting address for %s.",name);
    return;  
  }

  glGetActiveUniform(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformVector(%s,bool*) [getting type]",name)) return;

  switch (iType) {
    case GL_BOOL:            glUniform1i(iLocation,(b[0] ? 1 : 0)); break;
    case GL_BOOL_VEC2:          glUniform2i(iLocation,(b[0] ? 1 : 0),(b[1] ? 1 : 0)); break;
    case GL_BOOL_VEC3:          glUniform3i(iLocation,(b[0] ? 1 : 0),(b[1] ? 1 : 0),(b[2] ? 1 : 0)); break;
    case GL_BOOL_VEC4:          glUniform4i(iLocation,(b[0] ? 1 : 0),(b[1] ? 1 : 0),(b[2] ? 1 : 0),(b[3] ? 1 : 0)); break;
#ifdef GLSL_ALLOW_IMPLICIT_CASTS
    case GL_INT:            glUniform1i(iLocation,(b[0] ? 1 : 0)); break;
    case GL_INT_VEC2:          glUniform2i(iLocation,(b[0] ? 1 : 0),(b[1] ? 1 : 0)); break;
    case GL_INT_VEC3:          glUniform3i(iLocation,(b[0] ? 1 : 0),(b[1] ? 1 : 0),(b[2] ? 1 : 0)); break;
    case GL_INT_VEC4:          glUniform4i(iLocation,(b[0] ? 1 : 0),(b[1] ? 1 : 0),(b[2] ? 1 : 0),(b[3] ? 1 : 0)); break;
    case GL_FLOAT:            glUniform1f(iLocation,(b[0] ? 1.0f : 0.0f)); break;
    case GL_FLOAT_VEC2:          glUniform2f(iLocation,(b[0] ? 1.0f : 0.0f),(b[1] ? 1.0f : 0.0f)); break;
    case GL_FLOAT_VEC3:          glUniform3f(iLocation,(b[0] ? 1.0f : 0.0f),(b[1] ? 1.0f : 0.0f),(b[2] ? 1.0f : 0.0f)); break;
    case GL_FLOAT_VEC4:          glUniform4f(iLocation,(b[0] ? 1.0f : 0.0f),(b[1] ? 1.0f : 0.0f),(b[2] ? 1.0f : 0.0f),(b[3] ? 1.0f : 0.0f)); break;
#endif
    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformVector(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",bool*) - Unknown type\n");
      break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformVector(%s,bool*)",name);
#endif
}



/**
 * Sets an uniform matrix.
 * Matrices are always of type float.
 * \warning uses glGetError();
 * \param name - name of the parameter
 * \param m - a float array containing up to 16 floats for the matrix.
 * \param bTranspose - if true, the matrix will be transposed before uploading.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Mar.2005
 */
inline void GLSLProgram::SetUniformMatrix(const char *name,const float *m,bool bTranspose) const {
  assert(m_bEnabled);
  CheckGLError();

  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocationARB(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformMatrix(%s,float*,bool) [getting adress]",name)) return;
  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformMatrix","Error getting address for %s.",name);
    return;
  }

  glGetActiveUniformARB(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformMatrix(%s,float*,bool) [getting type]",name)) return;

  switch (iType) {
    case GL_FLOAT_MAT2:          glUniformMatrix2fv(iLocation,1,bTranspose,m); break;
    case GL_FLOAT_MAT3:          glUniformMatrix3fv(iLocation,1,bTranspose,m); break;
    case GL_FLOAT_MAT4:          glUniformMatrix4fv(iLocation,1,bTranspose,m); break;
    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformMatrix(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",float*,bool) - Unknown type\n");
    break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformMatrix(%s,float*,bool)",name);
#endif
}



#ifdef GLSL_ALLOW_IMPLICIT_CASTS

/**
 * Sets an uniform matrix.
 * Matrices are always of type float.
 * \warning uses glGetError();
 * \remark only available if GLSL_ALLOW_IMPLICIT_CASTS is defined.
 * \param name - name of the parameter
 * \param m - an int array containing up to 16 ints for the matrix. Ints are converted to float before uploading.
 * \param bTranspose - if true, the matrix will be transposed before uploading.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Mar.2005
 */
inline void GLSLProgram::SetUniformMatrix(const char *name,const int *m, bool bTranspose) const {
  assert(m_bEnabled);
  CheckGLError();

  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocation(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformMatrix(%s,int*,bool) [getting adress]",name)) return;
  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformMatrix","Error getting address for %s.",name);
    return;
  }

  glGetActiveUniform(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformMatrix(%s,float*,bool) [getting type]",name)) return;

  float M[16];
  switch (iType) {
    case GL_FLOAT_MAT2:
      for (unsigned int ui=0; ui<4; ui++) M[ui]=float(m[ui]);
      glUniformMatrix2fv(iLocation,1,bTranspose,M); 
      break;
    case GL_FLOAT_MAT3:
      for (unsigned int ui=0; ui<9; ui++) M[ui]=float(m[ui]);
      glUniformMatrix3fv(iLocation,1,bTranspose,M); 
      break;
    case GL_FLOAT_MAT4:
      for (unsigned int ui=0; ui<16; ui++) M[ui]=float(m[ui]);
      glUniformMatrix4fv(iLocation,1,bTranspose,M); 
      break;
    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformMatrix(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",int*,bool) - Unknown type\n"); 
      break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformMatrix(%s,int*,bool)",name);
#endif
}



/**
 * Sets an uniform matrix.
 * Matrices are always of type float.
 * \warning uses glGetError();
 * \remark only available if GLSL_ALLOW_IMPLICIT_CASTS is defined.
 * \param name - name of the parameter
 * \param m - an int array containing up to 16 ints for the matrix. Ints are converted to float before uploading.
 * \param bTranspose - if true, the matrix will be transposed before uploading.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Mar.2005
 */
inline void GLSLProgram::SetUniformMatrix(const char *name,const bool *m, bool bTranspose) const {
  assert(m_bEnabled);
  CheckGLError();

  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocationARB(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformMatrix(%s,int*,bool) [getting adress]",name)) return;
  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformMatrix","Error getting address for %s.",name);
    return;
  }

  glGetActiveUniformARB(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformMatrix(%s,float*,bool) [getting type]",name)) return;

  float M[16];
  switch (iType) {
    case GL_FLOAT_MAT2:
      for (unsigned int ui=0; ui<4; ui++) M[ui]=(m[ui] ? 1.0f : 0.0f);
      glUniformMatrix2fv(iLocation,1,bTranspose,M); 
      break;
    case GL_FLOAT_MAT3:
      for (unsigned int ui=0; ui<9; ui++) M[ui]=(m[ui] ? 1.0f : 0.0f);
      glUniformMatrix3fv(iLocation,1,bTranspose,M); 
      break;
    case GL_FLOAT_MAT4:
      for (unsigned int ui=0; ui<16; ui++) M[ui]=(m[ui] ? 1.0f : 0.0f);
      glUniformMatrix4fv(iLocation,1,bTranspose,M); 
      break;
    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformMatrix(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",bool*,bool) - Unknown type\n"); 
      break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformMatrix(%s,int*,bool)",name);
#endif
}

#endif // GLSL_ALLOW_IMPLICIT_CASTS



/**
 * Sets an uniform array.
 * Sets the entire array at once. Single positions can still be set using the other SetUniform*() methods.
 * \warning uses glGetError();
 * \param name - name of the parameter
 * \param a - a float array containing enough floats to fill the entire uniform array.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Mar.2005
 */
inline void GLSLProgram::SetUniformArray(const char *name,const float *a) const {
  assert(m_bEnabled);
  CheckGLError();

  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocation(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformArray(%s,float*) [getting adress]",name)) return;
  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformArray","Error getting address for %s.",name);
    return;  
  }

  glGetActiveUniform(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformArray(%s,float*) [getting type]",name)) return;

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
  int *iArray;
#endif

  switch (iType) {
    case GL_FLOAT:            glUniform1fv(iLocation,iSize,a); break;
    case GL_FLOAT_VEC2:          glUniform2fv(iLocation,iSize,a); break;
    case GL_FLOAT_VEC3:          glUniform3fv(iLocation,iSize,a); break;
    case GL_FLOAT_VEC4:          glUniform4fv(iLocation,iSize,a); break;

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
    case GL_BOOL:            glUniform1fv(iLocation,iSize,a); break;
    case GL_BOOL_VEC2:          glUniform2fv(iLocation,iSize,a); break;
    case GL_BOOL_VEC3:          glUniform3fv(iLocation,iSize,a); break;
    case GL_BOOL_VEC4:          glUniform4fv(iLocation,iSize,a); break;
    
    case GL_INT:
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D: 
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_RECT_ARB:
    case GL_SAMPLER_2D_RECT_SHADOW_ARB:    
      iArray=new int[iSize];
      for (int i=0; i<iSize; i++) iArray[i]=int(a[i]);
      glUniform1iv(iLocation,iSize,iArray); 
      delete[] iArray;
      break;

    case GL_INT_VEC2:
      iArray=new int[2*iSize];
      for (int i=0; i<2*iSize; i++) iArray[i]=int(a[i]);
      glUniform2iv(iLocation,iSize,iArray); 
      delete[] iArray;
      break;
    case GL_INT_VEC3:
      iArray=new int[3*iSize];
      for (int i=0; i<3*iSize; i++) iArray[i]=int(a[i]);
      glUniform3iv(iLocation,iSize,iArray);
      delete[] iArray;
      break;
    case GL_INT_VEC4:
      iArray=new int[4*iSize];
      for (int i=0; i<4*iSize; i++) iArray[i]=int(a[i]);
      glUniform4iv(iLocation,iSize,iArray);
      delete[] iArray;
      break;
#endif

    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformArray(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",float*) - Unknown type\n");
      break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformArray(%s,float*)",name);
#endif
}



/**
 * Sets an uniform array.
 * Sets the entire array at once. Single positions can still be set using the other SetUniform*() methods.
 * \warning uses glGetError();
 * \param name - name of the parameter
 * \param a - an int array containing enough floats to fill the entire uniform array.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Mar.2005
 */
inline void GLSLProgram::SetUniformArray(const char *name,const int   *a) const {
  assert(m_bEnabled);
  CheckGLError();

  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocation(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformArray(%s,int*) [getting adress]",name)) return;
  if(iLocation==-1) {
    m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformArray","Error getting address for %s.",name);
    return;  
  }

  glGetActiveUniform(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformArray(%s,int*) [getting type]",name)) return;

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
  float *fArray;
#endif

  switch (iType) {
    case GL_INT:
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D: 
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_2D_RECT_ARB:
    case GL_SAMPLER_2D_RECT_SHADOW_ARB:  glUniform1iv(iLocation,iSize,a); break;
    case GL_INT_VEC2:          glUniform2iv(iLocation,iSize,a); break;
    case GL_INT_VEC3:          glUniform3iv(iLocation,iSize,a); break;
    case GL_INT_VEC4:          glUniform4iv(iLocation,iSize,a); break;

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
    case GL_BOOL:            glUniform1iv(iLocation,iSize,a); break;
    case GL_BOOL_VEC2:          glUniform2iv(iLocation,iSize,a); break;
    case GL_BOOL_VEC3:          glUniform3iv(iLocation,iSize,a); break;
    case GL_BOOL_VEC4:          glUniform4iv(iLocation,iSize,a); break;
    
    case GL_FLOAT:
      fArray=new float[iSize];
      for (int i=0; i<iSize; i++) fArray[i]=float(a[i]);
      glUniform1fv(iLocation,iSize,fArray); 
      delete[] fArray;
      break;
    case GL_FLOAT_VEC2:
      fArray=new float[2*iSize];
      for (int i=0; i<2*iSize; i++) fArray[i]=float(a[i]);
      glUniform2fv(iLocation,iSize,fArray); 
      delete[] fArray;
      break;
    case GL_FLOAT_VEC3:
      fArray=new float[3*iSize];
      for (int i=0; i<3*iSize; i++) fArray[i]=float(a[i]);
      glUniform3fv(iLocation,iSize,fArray);
      delete[] fArray;
      break;
    case GL_FLOAT_VEC4:
      fArray=new float[4*iSize];
      for (int i=0; i<4*iSize; i++) fArray[i]=float(a[i]);
      glUniform4fv(iLocation,iSize,fArray);
      delete[] fArray;
      break;
#endif

    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformArray(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",int*) - Unknown type\n");
      break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformArray(%s,int*)",name);
#endif
}



/**
 * Sets an uniform array.
 * Sets the entire array at once. Single positions can still be set using the other SetUniform*() methods.
 * \warning uses glGetError();
 * \param name - name of the parameter
 * \param a - a bool array containing enough floats to fill the entire uniform array.
 * \return void
 * \author <a href="mailto:jens.schneider@in.tum.de">Jens Schneider</a>
 * \date Mar.2005
 */
inline void GLSLProgram::SetUniformArray(const char *name,const bool  *a) const {
  assert(m_bEnabled);
  CheckGLError();

  GLint iSize;
  GLenum iType;
  GLint iLocation;
  iLocation=glGetUniformLocation(m_hProgram,name); // Position of uniform var

  if (CheckGLError("SetUniformArray(%s,bool*) [getting adress]",name)) return;
  if(iLocation==-1) {
   m_pMasterController->DebugOut()->Error("GLSLProgram::SetUniformArray","Error getting address for %s.",name);
    return;  
  }

  glGetActiveUniform(m_hProgram,iLocation,0,NULL,&iSize,&iType,NULL);

  if (CheckGLError("SetUniformArray(%s,bool*) [getting type]",name)) return;

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
  float *fArray;  
#endif
  int   *iArray;
  switch (iType) {
    case GL_BOOL:
      iArray=new int[iSize];
      for (int i=0; i<iSize; i++) iArray[i]=(a[i] ? 1 : 0);
      glUniform1iv(iLocation,iSize,iArray); 
      delete[] iArray;
      break;
    case GL_BOOL_VEC2:
      iArray=new int[2*iSize];
      for (int i=0; i<2*iSize; i++) iArray[i]=(a[i] ? 1 : 0);
      glUniform2iv(iLocation,iSize,iArray); 
      delete[] iArray;
      break;
    case GL_BOOL_VEC3:
      iArray=new int[3*iSize];
      for (int i=0; i<3*iSize; i++) iArray[i]=(a[i] ? 1 : 0);
      glUniform3iv(iLocation,iSize,iArray); 
      delete[] iArray;
      break;
    case GL_BOOL_VEC4:
      iArray=new int[4*iSize];
      for (int i=0; i<4*iSize; i++) iArray[i]=(a[i] ? 1 : 0);
      glUniform4iv(iLocation,iSize,iArray); 
      delete[] iArray;
      break;

#ifdef GLSL_ALLOW_IMPLICIT_CASTS
    case GL_INT:
      iArray=new int[iSize];
      for (int i=0; i<iSize; i++) iArray[i]=(a[i] ? 1 : 0);
      glUniform1iv(iLocation,iSize,iArray); 
      delete[] iArray;
      break;
    case GL_INT_VEC2:
      iArray=new int[2*iSize];
      for (int i=0; i<2*iSize; i++) iArray[i]=(a[i] ? 1 : 0);
      glUniform2iv(iLocation,iSize,iArray); 
      delete[] iArray;
      break;
    case GL_INT_VEC3:
      iArray=new int[3*iSize];
      for (int i=0; i<3*iSize; i++) iArray[i]=(a[i] ? 1 : 0);
      glUniform3iv(iLocation,iSize,iArray); 
      delete[] iArray;
      break;
    case GL_INT_VEC4:
      iArray=new int[4*iSize];
      for (int i=0; i<4*iSize; i++) iArray[i]=(a[i] ? 1 : 0);
      glUniform4iv(iLocation,iSize,iArray); 
      delete[] iArray;
      break;
    case GL_FLOAT:
      fArray=new float[iSize];
      for (int i=0; i<iSize; i++) fArray[i]=(a[i] ? 1.0f : 0.0f);
      glUniform1fv(iLocation,iSize,fArray); 
      delete[] fArray;
      break;
    case GL_FLOAT_VEC2:
      fArray=new float[2*iSize];
      for (int i=0; i<2*iSize; i++) fArray[i]=(a[i] ? 1.0f : 0.0f);
      glUniform2fv(iLocation,iSize,fArray); 
      delete[] fArray;
      break;
    case GL_FLOAT_VEC3:
      fArray=new float[3*iSize];
      for (int i=0; i<3*iSize; i++) fArray[i]=(a[i] ? 1.0f : 0.0f);
      glUniform3fv(iLocation,iSize,fArray);
      delete[] fArray;
      break;
    case GL_FLOAT_VEC4:
      fArray=new float[4*iSize];
      for (int i=0; i<4*iSize; i++) fArray[i]=(a[i] ? 1.0f : 0.0f);
      glUniform4fv(iLocation,iSize,fArray);
      delete[] fArray;
      break;
#endif

    default: 
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO","GLSLProgram::SetUniformArray(");
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",name);
      m_pMasterController->DebugOut()->Message("GLSLProgram::TODO",",bool*) - Unknown type\n");
      break;
  }
#ifdef GLSL_DEBUG
  CheckGLError("SetUniformArray(%s,bool*)",name);
#endif
}