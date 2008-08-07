#include "GLTexture.h"
#include <cassert>
	
GLTexture::~GLTexture() {
	// TODO: I think we cannot call glDeleteTextures here as we are not guaranteed to be in the right context, ned to check this thougth
	assert(m_iGLID == (unsigned int)(-1));
}

void GLTexture::Delete() {
	glDeleteTextures(1,&m_iGLID); 
	m_iGLID = (unsigned int)(-1);
}
