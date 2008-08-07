#include "GLTexture1D.h"


GLTexture1D::GLTexture1D(GLuint iSize, GLint internalformat, GLenum format, GLenum type, const GLvoid *pixels, 
						 GLint iMagFilter, GLint iMinFilter, GLint wrap) :
	GLTexture(),
	m_iSize(iSize),
	m_internalformat(internalformat),
	m_format(format),
	m_type(type)
{
	glGenTextures(1, &m_iGLID);
	glBindTexture(GL_TEXTURE_1D, m_iGLID);

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, iMagFilter);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, iMinFilter);
	glTexImage1D(GL_TEXTURE_1D, 0, m_internalformat, m_iSize, 0, m_format, m_type, pixels);
}

void GLTexture1D::SetData(const GLvoid *pixels) {
	glBindTexture(GL_TEXTURE_1D, m_iGLID);
	glTexImage1D(GL_TEXTURE_1D, 0, m_internalformat, m_iSize, 0, m_format, m_type, pixels);
}
