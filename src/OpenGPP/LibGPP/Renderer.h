#pragma once

#include "GLclasses.h"
#include "InputImpl.h"



class Renderer
{
	uint m_width, m_height;
	Ptr<bool> m_initialized;
	Ptr<GLFramebuffer> m_framebuffer;
	Ptr<GLTexture2D> m_color;
	Ptr<GLTexture2D> m_depth;
public:
	//Renderer ()
	//{
	//	m_initialized = new bool(false);
	//}

	Renderer(int width, int height, uint internalColorFormat, uint colorFormat, bool depthExist, uint depthFormat)
	{
		m_initialized = new bool(false);
		init(width, height, internalColorFormat, colorFormat, depthExist, depthFormat);
	}

	void clear ()
	{
		m_initialized = new bool(false);
		m_framebuffer = Ptr<GLFramebuffer> ();
		m_color = Ptr<GLTexture2D> ();
		m_depth = Ptr<GLTexture2D> ();
	}
	void init (int width, int height, uint internalColorFormat, uint colorFormat, bool depthExist, uint depthFormat);
	void beginRender ();
	void endRender ();
	//void drawColor(int x, int y, int sx, int sy);
	Ptr<Input> createInput ();
};
