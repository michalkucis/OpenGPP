#include "Renderer.h"
#include "Error.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>


void Renderer::init( int width, int height, uint internalColorFormat, uint colorFormat, bool depthExist, uint depthFormat )
{
	*m_initialized = true;
	m_width = width;
	m_height = height;
	m_color = new GLTexture2D (m_width,m_height,internalColorFormat,colorFormat);
	if (depthExist)
		m_depth = new GLTexture2D (m_width,m_height,depthFormat, GL_DEPTH_COMPONENT);
	m_framebuffer = new GLFramebuffer (m_color, m_depth);
}

void Renderer::beginRender()
{
	if (! *m_initialized)
		error0(ERR_STRUCT,"Renderer is not initialized");
	m_framebuffer->bind();
	checkGLError ();
}

void Renderer::endRender()
{
	if (! *m_initialized)
		error0(ERR_STRUCT,"Renderer is not initialized");
	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();
	//glOrtho(0, m_width , m_height , 0, -1, 1);
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	//glBindTexture(GL_TEXTURE_2D, (GLuint) *m_texColor);
	m_framebuffer->unbind();
}

//void Renderer::drawColor( int x, int y, int sx, int sy )
//{
//	if (! *m_initialized)
//		error0(ERR_STRUCT,"Renderer is not initialized");
//	glEnable(GL_TEXTURE_2D);
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, *m_texColor);
//	//glViewport(0, 0, m_texColor->getResolution().x, m_texColor->getResolution().y);
//	glBegin(GL_QUADS);
//	glTexCoord2f(0,1.0f);   glVertex2f(0,0);
//	glTexCoord2f(0,0);   glVertex2f(0,(float)m_height);
//	glTexCoord2f(1.0f,0);   glVertex2f((float)m_width,(float)m_height);
//	glTexCoord2f(1.0f,1.0f);   glVertex2f((float)m_width,0);
//	glEnd();
//	glBindTexture(GL_TEXTURE_2D, 0);
//	checkGLError ();
//}

Ptr<Input> Renderer::createInput()
{
	if (! *m_initialized)
		error0(ERR_STRUCT,"Renderer is not initialized");

//	return new InputRendered (m_initialized, m_texColor, m_rbDepth);
	Ptr<Input> ptr = Ptr<Input>(new InputRenderedColor (m_initialized, m_color));
	return ptr;
}
