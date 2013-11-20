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
	Ptr<PostProcessor> m_pp;

public:
	AppTest(): Application(250, 250)
	{
	}
	void initData()
	{
		m_pp = new PostProcessor();
		m_pp->m_input = new InputLoadFromSingleFileOpenEXR("exrChangeLightIntensity\\img_light1_lamp0_pos0.exr");
		m_pp->m_vecEffects.pushBack(new EffectRenderToScreen(0,0,1,1));
	}
	void render()
	{
		m_pp->process();
	}
	void clearData ()
	{		
		m_pp->clear ();
	}
};