/**************************************************************************************************************

(c) 2004-06 by Jens Schneider, TUM.3D
	mailto:jens.schneider@in.tum.de
	Computer Graphics and Visualization Group
    Institute for Computer Science I15
	Technical University of Munich
	
**************************************************************************************************************/
#ifndef GLFBOTEX_H_
#define GLFBOTEX_H_

#include <assert.h>
#include <stdlib.h>
#include "GLInclude.h"

class MasterController;

class GLFBOTex {
public:
	GLFBOTex(MasterController* pMasterController, GLenum minfilter, GLenum magfilter, GLenum wrapmode, GLsizei width, GLsizei height, GLenum intformat, bool bHaveDepth=false, int iNumBuffers=1);
	virtual ~GLFBOTex(void);
	inline virtual void Write(GLenum target=GL_COLOR_ATTACHMENT0_EXT,int iBuffer=0);
	inline virtual void Read(GLenum texunit,int iBuffer=0);
	inline virtual void FinishWrite(int iBuffer=0);
	inline virtual void FinishRead(int iBuffer=0);
	inline virtual operator GLuint(void) { return m_hTexture[0]; }
	inline virtual operator GLuint*(void) { return m_hTexture; }
private:
	bool			CheckFBO(const char* method);
	void			initFBO(void);
	void			initTextures(GLenum minfilter, GLenum magfilter, GLenum wrapmode, GLsizei width, GLsizei height, GLenum intformat);
  MasterController    *m_pMasterController;
	GLuint			        *m_hTexture;
	GLuint			        m_hDepthBuffer;
	static GLuint	      m_hFBO;
	static bool		      m_bInitialized;
	static int		      m_iCount;
	GLenum			        *m_LastTexUnit;
	int				          m_iNumBuffers;
	GLenum			        *m_LastAttachment;
};


// ************************************************************************************************************************************************************

class VBOTex {
public:
	VBOTex(MasterController* pMasterController, GLsizei width, GLsizei height, bool bHaveDepth=false, int iNumBuffers=1);
	virtual ~VBOTex(void);
	inline virtual void CopyToVBO(int iBuffer=0);
	inline virtual void Write(GLenum target=GL_COLOR_ATTACHMENT0_EXT, int iBuffer=0);
	inline virtual void FinishWrite(int iBuffer=0);
	inline virtual void ReadTex(GLenum texunit, int iBuffer=0);
	inline virtual void FinishReadTex(int iBuffer=0);
	inline virtual void Read(void);
	inline virtual void FinishRead(void);
	inline virtual operator GLuint(void) { return m_hPBO; }	
private:
  MasterController  *m_pMasterController;
	GLFBOTex          *m_hGLFBOTex;
	GLuint		        m_hPBO;
	GLsizei		        m_iWidth;
	GLsizei		        m_iHeight;
	GLenum		        *m_LastAttachment;
	static int	      m_iCount;
	static bool	      m_bInitialized;
	static bool	      m_bPBOSupported;
	int			          m_iNumBuffers;
};


#endif  // GLFBOTEX_H_
