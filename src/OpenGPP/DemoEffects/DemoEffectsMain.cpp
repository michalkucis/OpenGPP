#include <assert.h>
#include <iostream>
#include <vector>
#include "MemoryLeakDetector.h"
#include "Application.h"
#include "Error.h"

#include <windows.h>
#include <SDL.h>
#undef main
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "Ptr.h"
#include "Vector.h"
#include "Renderer.h"
#include "InputImplOpenCV.h"
#include "EffectImpl.h"
#include "EffectImplOpenCV.h"
#include "PostProcessor.h"
#include "EffectGLImpl.h"
#include "EffectCLImpl.h"
#include "SharedObjectsFactory.h"

class FuncFtoFDistortionTest: public FuncFtoF
{
	float m_e;
public:
	FuncFtoFDistortionTest (float f)
	{

		m_e = f;
	}
	float operator() (float x)
	{
		return powf(x,m_e);
	}
};

class FuncFtoFVignettingTest: public FuncFtoF
{
	float m_e;
public:
	FuncFtoFVignettingTest (float f)
	{
		m_e = f;
	}
	float operator() (float x)
	{
		return 1-powf(x,m_e);
	}
};

class EffectTestDOFImportance: public EffectDOFImportance
{
public:
	EffectTestDOFImportance (Ptr<SharedObjectsFactory> sof): EffectDOFImportance (sof, 5, 0.1f, 1024.0f/768)
	{

	}
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
	{
		static int nIt = 0;
		FuncFtoFCompCoC func (2.66f+(-2.0f+nIt/2.0f), 3, 48, 42, 0.01f, 10);
		if (nIt++ >= 10)
			nIt = 0;
		EffectDOFImportance::setFuncCoC(func);
		Ptr<GLTexture2D> texCoC = EffectDOFImportance::process (tex, depth, envMap);
		return texCoC;
	}
};

class EffectTestDOFRegular: public EffectDOFRegular
{
public:
	EffectTestDOFRegular( Ptr<SharedObjectsFactory> sof ): EffectDOFRegular (sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), 5, 0.1f, 1024.0f/768)
	{

	}
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
	{
		static int nIt = 0;
		FuncFtoFCompCoC func (2.66f+(-2.0f+nIt/2.0f), 3, 48, 42, 0.01f, 10);
		if (nIt++ >= 10)
			nIt = 0;
		EffectDOFRegular::setFuncCoC(func);
		Ptr<GLTexture2D> texCoC = EffectDOFRegular::process (tex, depth, envMap);
		return texCoC;
	}
};

class EffectTestDOFDistribution: public EffectCLDOFDistribution
{
public:
	EffectTestDOFDistribution(Ptr<SharedObjectsFactory> sof): EffectCLDOFDistribution(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), 1024.0f/768)
	{

	}
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
	{
		static int nIt = 0;
		FuncFtoFCompCoC func (2.66f+(-2.0f+nIt/2.0f), 3, 48, 42, 0.01f, 10);
		if (nIt++ >= 10)
			nIt = 0;
		EffectCLDOFDistribution::setFuncCoC(func);
		Ptr<GLTexture2D> texCoC = EffectCLDOFDistribution::process (tex, depth, envMap);
		return texCoC;
	}
};

#define SCREEN_REGION(X,Y) X*0.25f,Y*0.25f,0.25f,0.25f

class ApplicationPP: public Application
{
public:
	ApplicationPP (): Application(1280,720*4/3),
		m_ppWebcam(uint2(512,512)),
		m_ppWithDepth(uint2(512,512)),
		m_ppWithDepthAndEnvMap(uint2(512,512))
	{
		m_sof = new SharedObjectsFactory(uint2(1024,768));
	}
private:
	//Renderer m_renderer;

	Ptr<SharedObjectsFactory> m_sof;
	PostProcessor m_ppWebcam;
	PostProcessor m_ppWithDepth;
	PostProcessor m_ppWithDepthAndEnvMap;
	Ptr<Input> m_inputs[4];
public:
	void initData ()
	{
		m_ppWebcam.m_input = new InputLoadFromVideoDeviceOpenCV(0);
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(0,0))));
		Ptr<FuncFtoF> func = new FuncFtoFDistortionTest (0.9f);
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectDistortionGrid(m_sof, uint2(10),func)));
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(1,0))));

		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectLoadInput(m_ppWebcam.m_input)));
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectVignettingImageFile(m_sof, "vignettingMask.jpg", false)));
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(2,0))));

		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectLoadInput(m_ppWebcam.m_input)));
		Ptr<FuncFtoF> func2 = new FuncFtoFVignettingTest (1); 
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectVignettingImageFunc(m_sof, uint2(128,128), func2, false)));
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(3,0))));

		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectLoadInput(m_ppWebcam.m_input)));
		EffectChromaticAberration* effectChromAber = new EffectChromaticAberration (m_sof);
		effectChromAber->setGrid(0, uint2(10,10), new FuncFtoFDistortionTest (0.9f));
		effectChromAber->setGrid(2, uint2(10,10), new FuncFtoFDistortionTest (1.1f));
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(effectChromAber));
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(0,1))));

		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectLoadInput(m_ppWebcam.m_input)));
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectNoiseUniform (m_sof)));
		m_ppWebcam.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(1,1))));

//		m_inputs[3] = new InputLoadFromSingleFileOpenEXR("exrChangeLightIntensity\\img_light1_lamp0_pos0.exr");
		m_inputs[0] = new InputLoadFromSingleFileOpenEXR("exrChangeLightIntensity\\img_light1_lamp10_pos0.exr");
		//m_inputs[2] = new InputLoadFromSingleFileOpenEXR("exrChangeLightIntensity\\img_light1_lamp50_pos0.exr");
		m_inputs[1] = new InputLoadFromSingleFileOpenEXR("exrChangeLightIntensity\\img_light1_lamp250_pos0.exr");

		m_ppWithDepth.m_input = m_inputs[0];
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(2,1))));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectBrightnessAdapter(m_sof)));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(3,1))));

		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectLoadInput(m_inputs[1])));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectMotionBlurSimple(m_sof, float3(0,0,0), float2(10,10), 8)));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(0,2))));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectLoadInput(m_inputs[1])));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectMotionBlur2Phases(m_sof, float3(0,0,0), float2(10,10), 4, 4)));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(1,2))));

		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectLoadInput(m_inputs[1])));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectTestDOFRegular (m_sof)));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(2,2))));

		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectLoadInput(m_inputs[1])));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectTestDOFImportance(m_sof)));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(3,2))));

		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectLoadInput(m_inputs[1])));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectResizeImages(m_sof)));
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectTestDOFDistribution(m_sof)));	
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(3,3))));	
	
		Ptr<Input> inputWithEnvMap = new InputLoadFromSingleFileWithEnvMapOpenEXR(
			"exrMovingLight\\img_light1_lamp50_pos0_floor2.exr", int2(640,480), InputOpenCV::LINEAR,
			"exrMovingLight\\lpr_light1_lamp50_pos0_floor2.exr", int2(0,0), InputOpenCV::LINEAR);
		m_ppWithDepthAndEnvMap.m_input = inputWithEnvMap;

		m_ppWithDepthAndEnvMap.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(0,3))));	
		m_ppWithDepthAndEnvMap.m_vecEffects.pushBack(Ptr<Effect>(
			new EffectLensFlareStarFromSimple(m_sof,512,512, true, new EffectCLObjectsFactory)));
		m_ppWithDepthAndEnvMap.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(1,3))));	

		m_ppWithDepthAndEnvMap.m_vecEffects.pushBack(Ptr<Effect>(new EffectLoadInput(inputWithEnvMap)));	
		EffectLensFlareStarFromEnvMap* effect =	new EffectLensFlareStarFromEnvMap
			(m_sof, 512,512, true, new EffectCLObjectsFactory);
		effect->setLensFlareMult(5);
		m_ppWithDepthAndEnvMap.m_vecEffects.pushBack(Ptr<Effect>(effect));
		m_ppWithDepthAndEnvMap.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(SCREEN_REGION(2,3))));	
	}
	void render ()
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, 1 , 1 , 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		m_ppWebcam.process();

		static int numIndex = 0;
		static int numIter = 0;
		if ((numIter++ % 12) == 0)
			++numIndex %= 2;
		m_ppWithDepth.m_input = m_inputs[numIndex];
		m_ppWithDepth.process();
		m_ppWithDepthAndEnvMap.process();
		Application::render();
	}
	void clearData ()
	{
		m_ppWebcam.clear ();
		m_ppWithDepth.clear ();
		for (int i = 0; i < 4; i++)
			m_inputs[i] = Ptr<Input>();
		checkGLError();
	}
};


int main( int argc, char* argv[] )
{
	static MemoryLeakDetector mld;
	//mld.setBreakpoint(847);
	try 
	{
		ApplicationPP pp; 
		pp.run();
	} catch (Error& ex) {
		std::string desc = ex.getDesc();
		printf ("%s\n", desc.c_str());
		assert (0);
	}
    return 0;
}
