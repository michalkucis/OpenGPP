#pragma once

#include "Effect.h"
#include "GLClasses.h"
#include "GLClassesFactories.h"

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glu32.lib")

class EffectGL: public Effect
{
protected:
	Ptr<GLTexturesRGBFactory> m_factoryTexRGB;
	Ptr<GLTexturesRedFactory> m_factoryTexRed;
	//Ptr<GLTexturesDepthFactory> m_factoryTexDepth;
	Ptr<GLFramebufferFactory> m_factoryFramebuffers;
public:
	EffectGL (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed)
	{				 
		m_factoryTexRGB = factoryRGB; //new GLTexturesRGBFactory(uint2(256,256));
		m_factoryTexRed = factoryRed; //new GLTexturesRedFactory(uint2(256,256));
		m_factoryFramebuffers = new GLFramebufferFactory;
		//m_factoryTexDepth = new GLTexturesDepthFactory(uint2(1024,768));
	}
	void drawQuad ()
	{
		int2 res = m_factoryTexRGB->getTexture2DSize();
		float2 fres = res.get2<float2>();
		GLint params[4];
		glGetIntegerv (GL_VIEWPORT, params);
		glViewport(0,0,res.x,res.y);
		fres = float2((float)params[2], (float)params[3]);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f,1.0f);	glVertex2f(0,0);
		glTexCoord2f(0.0f,0.0f);	glVertex2f(0,1);
		glTexCoord2f(1.0f,0.0f);	glVertex2f(1,1);
		glTexCoord2f(1.0f,1.0f);	glVertex2f(1,0);
		glEnd();
		glViewport(params[0],params[1],params[2],params[3]);
	}
	Ptr<GLTexture2D> loadTextureFromFile (string filename);
	virtual Ptr<GLTexture2D> process (Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)=0;
	virtual ~EffectGL () {}
};