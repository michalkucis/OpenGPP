#include "EffectImpl.h"
#include "Error.h"

#include <windows.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "EffectGLImpl.h"

Ptr<GLTexture2D> EffectRenderToScreen::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	tex->bind();
	//glViewport(m_org.x, m_org.y, m_size.x, m_size.y);
	float minX = (float)m_org.x;
	float minY = (float)m_org.y;
	float maxX = (float)m_org.x + m_size.x;
	float maxY = (float)m_org.y + m_size.y;
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f,0.0f);	glVertex2f(minX,minY);
	glTexCoord2f(0.0f,1.0f);	glVertex2f(minX,maxY);
	glTexCoord2f(1.0f,1.0f);	glVertex2f(maxX,maxY);
	glTexCoord2f(1.0f,0.0f);	glVertex2f(maxX,minY);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	checkGLError ();
	return tex;
}

Ptr<GLTexture2D> EffectRenderDepthToScreen::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	depth->bind();
	float minX = (float)m_org.x;
	float minY = (float)m_org.y;
	float maxX = (float)m_org.x + m_size.x;
	float maxY = (float)m_org.y + m_size.y;
	m_program.use();

	m_program.setFloat3("mult", float3(0.001f,0.001f,0.001f));
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f,0.0f);	glVertex2f(minX,minY);
	glTexCoord2f(0.0f,1.0f);	glVertex2f(minX,maxY);
	glTexCoord2f(1.0f,1.0f);	glVertex2f(maxX,maxY);
	glTexCoord2f(1.0f,0.0f);	glVertex2f(maxX,minY);
	glEnd();
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	checkGLError ();
	return tex;
}