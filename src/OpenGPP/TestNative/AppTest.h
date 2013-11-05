#pragma once

#include "TestingFramework.h"
#include "Application.h"
#include "Renderer.h"
#include "PostProcessor.h"
#include "InputImplOpenCV.h"
#include "EffectimplOpenCV.h"
#include "EffectImpl.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL.h>

class AppTest: public Application
{
public:
	AppTest() : Application(640,480)
	{

	}
protected:
	Renderer m_renderer;
	PostProcessor m_pp;
	int m_countRenders;
protected:
	virtual void initData()
	{
		m_renderer.init(200,200,GL_RGBA,GL_UNSIGNED_BYTE,false,GL_DEPTH_COMPONENT);
	}
	void render ()
	{
		m_renderer.beginRender();
		drawScreen();
		m_renderer.endRender();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, 640 , 480 , 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		m_pp.process();
		SDL_GL_SwapBuffers( );

		if (! --m_countRenders)
			SDL_Quit();
	}
	void clearData ()
	{

	}
public:
	void runSingleOpenCV ()
	{
		init();
		exec();
		clear();
	}
};