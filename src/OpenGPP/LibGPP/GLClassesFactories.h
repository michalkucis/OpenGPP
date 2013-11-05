#pragma once

#include "GLClasses.h"
#include "Factory.h"
#include <GL/glew.h>
#include <GL/gl.h>

class GLTexturesRGBFactory: public Factory<GLTexture2D> 
{
	int m_width, m_internalFormat, m_height;
	int m_format, m_type;
public:
	GLTexturesRGBFactory (uint2 resolution)
	{
		m_width = resolution.x;
		m_height = resolution.y;
		m_format = GL_RGB;
		m_internalFormat = GL_RGB32F;
	}
	int2 getTexture2DSize()
	{
		return int2(m_width, m_height);
	}
protected:
	Ptr<GLTexture2D> allocProduct ()
	{
		return new GLTexture2D (m_width, m_height, m_internalFormat, m_format);
	}
};

class GLTexturesRedFactory: public Factory<GLTexture2D> 
{
	int m_width, m_internalFormat, m_height;
	int m_format, m_type;
public:
	GLTexturesRedFactory (uint2 resolution)
	{
		m_width = resolution.x;
		m_height = resolution.y;
		m_format = GL_RED;
		m_internalFormat = GL_R32F;
	}
	int2 getTexture2DSize()
	{
		return int2(m_width, m_height);
	}
protected:
	Ptr<GLTexture2D> allocProduct ()
	{
		return new GLTexture2D (m_width, m_height, m_internalFormat, m_format);
	}
};
/*
class GLTexturesDepthFactory: public Factory<GLTexture2D>
{
	uint2 m_resolution;
public:
	GLTexturesDepthFactory (uint2 resolution): 
		m_resolution(resolution)
	{}
protected:
	Ptr<GLTexture2D> allocProduct ()
	{
		return new GLTextureDepth (m_resolution.x, m_resolution.y, GL_R32F, GL_RED);
	}
};*/

//template <int GLTYPE>
//class GLRenderbufferFactory: public Factory<GLRenderbuffer> 
//{
//	int m_width, m_height;
//	int m_format;
//public:
//	GLRenderbufferFactory (int2 resolution)
//	{
//		m_width = resolution.x;
//		m_height = resolution.y;
//	}
//protected:
//	Ptr<GLRenderbuffer> allocProduct ()
//	{
//		return new GLRenderbuffer (m_width, m_height, GLTYPE);
//	}
//};

class GLFramebufferFactory: public Factory<GLFramebuffer> 
{
protected:
	Ptr<GLFramebuffer> allocProduct ()
	{
		return new GLFramebuffer;
	}
};