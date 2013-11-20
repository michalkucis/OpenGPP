#include "EffectCLImpl.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <SDL.h>
#undef main
#include "EffectGL.h"
#include <fstream>
#include "Application.h"
#include "PostProcessor.h"
#include "InputImplOpenCV.h"
#include "EffectImpl.h"
#include "EffectGLImpl.h"
#include "EffectImplOpenCV.h"
#include "MemoryLeakDetector.h"





/*
//#include <boost/thread.hpp> 
void workerFunc(Ptr<cl::Event> event)
{
	printf("workerFunc: the thread is starting...\n");
	event->wait();
	printf("workerFunc: the marker is accepted..\n");
}

Ptr<cl::Event> g_ptrEvent;

#include <windows.h>
DWORD WINAPI winapiWorkerFunc(LPVOID lpParameter)
{
	Ptr<cl::Event> ptrEvent = g_ptrEvent;
	workerFunc(ptrEvent);
	return 0;
}*/



/*
class EffectTestOpenCLAndOpenGL: public EffectGL
{
	Ptr<cl::Context> m_context;
	Ptr<cl::CommandQueue> m_queue;
	cl::Kernel m_kernel;
public:
	EffectTestOpenCLAndOpenGL(Ptr<EffectCLObjectsFactory> factory)
	{
		m_context = factory->getCLGLContext();
		m_queue = factory->getCLCommandQueue();
		m_kernel = createKernel(*m_context, 
			"kernels\\kernels.cl", "test");
	}
	Ptr<GLTexture2D> process (Ptr<GLTexture2D> texColor, Ptr<GLTexture2D> texDepth, Ptr<GLTextureEnvMap> envMap)
	{
		try 
		{
			Ptr<GLTexture2D> texRes = m_factoryTexRGB->createProduct();

			glFlush();
			cl::Image2DGL imageColor = getImage2DGL(*m_context, CL_MEM_READ_WRITE, texColor);
			//cl::Image2DGL imageRes = getImage2DGL(*m_context, CL_MEM_READ_WRITE, texRes);
			std::vector<cl::Memory> vecGLMems;	
			vecGLMems.push_back(imageColor);
			//vecGLMems.push_back(imageRes);
			m_queue->enqueueAcquireGLObjects(&vecGLMems);

			//m_kernel.setArg(0, imageRes);
			m_kernel.setArg(0, imageColor);
			m_queue->enqueueNDRangeKernel(
				m_kernel,
				cl::NDRange(0,0),
				cl::NDRange(texColor->getResolution().x, texColor->getResolution().y));

			m_queue->enqueueReleaseGLObjects(&vecGLMems);
			m_queue->flush();

			return texColor;
		}catchCLError;
	}
};

class EffectTest: public EffectTestOpenCLAndOpenGL
{
public:
	EffectTest(): EffectTestOpenCLAndOpenGL(new EffectCLObjectsFactory)
	{}
};*/




class EffectTest: public EffectLensFlareStarFromEnvMap
{
public:
	EffectTest(): EffectLensFlareStarFromEnvMap(new GLTexturesRGBFactory(uint2(512,512)), new GLTexturesRedFactory(uint2(512,512)),
		1024,true,new EffectCLObjectsFactory)
	{}
};




class ApplicationCL: public Application
{
	int m_countRenders;
public:
	ApplicationCL (): Application(1024,768)
	{
		m_countRenders = 100;
	}
private:
	PostProcessor m_pp;
public:
	void handleKeyDown( SDL_keysym* keysym )
	{
	}
	void initData ()
	{
		m_pp.m_input = new InputLoadFromSingleFileWithEnvMapOpenEXR(
			"exrReposition\\img_light1_lamp50_pos0.exr", int2(1024,768), InputOpenCV::LINEAR,
			"exrReposition\\lpr_light1_lamp50_pos0.exr", int2(0,0), InputOpenCV::LINEAR);

		//m_pp.m_input = new InputLoadFromSingleFileOpenEXR("exrReposition\\img_light1_lamp50_pos0.exr");
		//m_pp.m_vecEffects.pushBack(Ptr<Effect>(new EffectTest()));
		
		m_pp.m_vecEffects.pushBack(Ptr<Effect>(new EffectCopyColorAndDepthMaps()));
		m_pp.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(0,0,1,1)));
		m_pp.m_vecEffects.pushBack(Ptr<Effect>(new EffectSaveToVideoFileOpenCV("out.avi")));
		//m_pp.m_vecEffects.pushBack(Ptr<Effect>(new EffectSaveToSingleFileOpenEXR("out.exr")));
	
	}
	void render ()
	{
		m_pp.process();
		Application::render();

		//if (0 == --m_countRenders)
			//SDL_Quit();
	}
	void clearData ()
	{
		m_pp.clear ();
		checkGLError();
	}
};



int main( int argc, char* argv[] )
{
	//int N = 721;
	//int sum = 0;
	//for (int L = 1; L <= round(log2(N+1))-1; L++)
	//{
	//	int upper = round(powf(2,-L)*(N+1))-2;
	//	std::cout << "L=" << int2str(L) 
	//		<< "\tj_min=" << int2str(0)
	//		<< "\tj_max=" << int2str(upper)
	//		<< "\tcount=" << int2str(upper+1)
	//		<< "\tsum=" << int2str(sum += upper+1)
	//		<< std::endl;
	//}
		//for (int j = 0; j < powf(2,-L)*(N+1)-1; j++)
			//std::cout << "L=" << int2str(L) << "\tj=" << int2str(j) << std::endl;
	//for (int i = 0; i < 129; i++)
	//	printf("%d = %d\n", i, getOversizeExp2(i));
	static MemoryLeakDetector mld;
	//mld.setBreakpoint(847);
	try 
	{
		ApplicationCL pp; 
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