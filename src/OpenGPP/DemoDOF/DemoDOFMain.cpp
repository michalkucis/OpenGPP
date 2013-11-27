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
//#include "EffectTest.h"
#include "EffectGLImpl.h"
#include "EffectCLImpl.h"

class ApplicationDOF: public Application
{
public:
	ApplicationDOF (): Application(1024,768),
		m_ppWithDepth(uint2(1024,768))
	{
		m_dofCoCFunc = NULL;
		m_sof = new SharedObjectsFactory(uint2(1024, 768));
	}
private:
	PostProcessor m_ppWithDepth;
	Ptr<Input> m_inputs[4];
	EffectDOFCircleOfConfusionInternal* m_dofCoCFunc;
	Ptr<SharedObjectsFactory> m_sof;
public:
	void handleKeyDown( SDL_Keysym* keysym )
	{
		Vector<Ptr<Effect>>& vec = m_ppWithDepth.m_vecEffects;
		switch (keysym->sym)
		{
		case '1':
			vec.clear();
			vec.pushBack(Ptr<Effect>(new EffectRenderToScreen(0,0,1,1)));
			break; 
		case '2':
			vec.clear();
			vec.pushBack(Ptr<Effect>(new EffectRenderDepthToScreen(0,0,1,1)));
			break;
		case '3':
			vec.clear();
			vec.pushBack(Ptr<Effect>(new EffectDOFShowCoC(m_sof)));
			vec.pushBack(Ptr<Effect>(new EffectRenderToScreen(0,0,1,1)));
			break;
		case '4':
			vec.clear();
			vec.pushBack(Ptr<Effect>(new EffectDOFRegular(m_sof, 5,1,1024.0f/768.0f)));
			vec.pushBack(Ptr<Effect>(new EffectRenderToScreen(0,0,1,1)));
			break;
		case '5':
			vec.clear();
			vec.pushBack(Ptr<Effect>(new EffectDOFImportance(m_sof, 5,1,1024.0f/768.0f)));
			vec.pushBack(Ptr<Effect>(new EffectRenderToScreen(0,0,1,1)));
			break;
		case '6':
			vec.clear();
			vec.pushBack(Ptr<Effect>(new EffectCLDOFDistribution(m_sof, 1024.0f/768.0f)));
			vec.pushBack(Ptr<Effect>(new EffectRenderToScreen(0,0,1,1)));
			break;
		case SDLK_ESCAPE:
			SDL_Quit();
			break;
		}
	}
	void initData ()
	{
		m_inputs[0] = new InputLoadFromSingleFileOpenEXR("exrReposition\\img_light1_lamp50_pos0.exr", int2(1024,768));
		m_inputs[1] = new InputLoadFromSingleFileOpenEXR("exrReposition\\img_light1_lamp50_pos5.exr", int2(1024,768));
		m_inputs[2] = new InputLoadFromSingleFileOpenEXR("exrReposition\\img_light1_lamp50_pos10.exr", int2(1024,768));
		m_inputs[3] = new InputLoadFromSingleFileOpenEXR("exrReposition\\img_light1_lamp50_pos15.exr", int2(1024,768));

		m_ppWithDepth.m_input = m_inputs[0];
		m_ppWithDepth.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(0,0,1,1)));
	}
	void render ()
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, 1 , 1 , 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		{
			Ptr<Effect> ptrEffect1st = m_ppWithDepth.m_vecEffects[0];
			Effect* pEffect = ptrEffect1st.get();
			assert(pEffect);
			EffectDOFShowCoC* pEffectShowCoC = dynamic_cast<EffectDOFShowCoC*> (pEffect);
			float mult = 1.0f;
			if (pEffectShowCoC)
				mult = 100.0f;

			static int nIt = 0;
			FuncFtoFCompCoC func (2.66f+(-2.0f+nIt/20.0f), 3, 48, 42, 0.01f, mult);
			if (nIt++ >= 100)
				nIt = 0;
			EffectDOF* pEffectDOF = dynamic_cast<EffectDOF*> (pEffect); 
			if (pEffectDOF)
				pEffectDOF->setFuncCoC(func);
		}
		{
			static int numIndex = 0;
			static int numIter = 0;
			if ((numIter++ % 100) == 0)
				++numIndex %= 4;
			//m_ppWithDepth.m_input = m_inputs[numIndex];

		}
		m_ppWithDepth.process();
		Application::render();
	}
	void clearData ()
	{
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
		ApplicationDOF pp; 
		pp.run();
	} catch (Error& ex) {
		std::string desc = ex.getDesc();
		printf ("%s\n", desc.c_str());

		std::cout << "Press any key to continue . . .";
		std::cin.ignore();
		std::cin.get();
	}
	return 0;
}
