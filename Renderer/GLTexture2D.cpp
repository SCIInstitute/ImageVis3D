#include "GLTexture2D.h"


GLTexture2D::GLTexture2D(GLuint iSizeX, GLuint iSizeY, GLint internalformat, GLenum format, GLenum type, const GLvoid *pixels, 
						 GLint iMagFilter, GLint iMinFilter, GLint wrapX, GLint wrapY) :
	GLTexture(),
	m_iSizeX(iSizeX),
	m_iSizeY(iSizeY),
	m_internalformat(internalformat),
	m_format(format),
	m_type(type)
{
	glGenTextures(1, &m_iGLID);
	glBindTexture(GL_TEXTURE_2D, m_iGLID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapX);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapY);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, iMagFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, iMinFilter);
	glTexImage2D(GL_TEXTURE_2D, 0, m_internalformat, m_iSizeX, m_iSizeY, 0, m_format, m_type, pixels);
}

void GLTexture2D::SetData(const GLvoid *pixels) {
	glBindTexture(GL_TEXTURE_2D, m_iGLID);
	glTexImage2D(GL_TEXTURE_2D, 0, m_internalformat, m_iSizeX, m_iSizeY, 0, m_format, m_type, pixels);
}
