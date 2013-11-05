#pragma once

#include "Ptr.h"
#include "CLWrapper.h"
#include "GLClassesFactories.h"

#pragma comment(lib, "OpenCL.lib")


class EffectCLObjectsFactory
{
	Ptr<cl::Context> m_context;
	Ptr<cl::CommandQueue> m_queue;
public:
	EffectCLObjectsFactory()
	{
		m_context = new cl::Context(::getCLGLContext());
		m_queue = new cl::CommandQueue(::getCLCommandQueue(*m_context));
	}
	Ptr<cl::Context> getCLGLContext()
	{
		return m_context;
	}
	Ptr<cl::CommandQueue> getCLCommandQueue()
	{
		return m_queue;
	}
};

template<typename EFFECT_GL_PARENT>
class EffectCL: public EFFECT_GL_PARENT
{
protected:
	Ptr<cl::Context> m_context;
	Ptr<cl::CommandQueue> m_queue;

	virtual void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult) = 0;
public:
	EffectCL (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> clFactory):
		EFFECT_GL_PARENT(factoryRGB, factoryRed)
	{
		try 
		{
			m_context = clFactory->getCLGLContext();
			m_queue = clFactory->getCLCommandQueue ();
		}catchCLError;
	}

	~EffectCL()
	{
		m_queue->enqueueBarrier();
	}
protected:
	virtual Ptr<GLTexture2D> createTexTarget() = 0;
public:
	Ptr<GLTexture2D> process (Ptr<GLTexture2D> texColor, Ptr<GLTexture2D> texDepth, Ptr<GLTextureEnvMap> texEnvMap)
	{
		try 
		{
			Ptr<GLTexture2D> texRes = createTexTarget();

			glFlush();
			cl::Image2DGL imageColor = getImage2DGL(*m_context, CL_MEM_READ_ONLY, texColor);
			cl::Image2DGL imageDepth;
			if (requireDepth())
				imageDepth = getImage2DGL(*m_context, CL_MEM_READ_ONLY, texDepth);
			cl::Image2DGL imageRes = getImage2DGL(*m_context, CL_MEM_WRITE_ONLY, texRes);
			std::vector<cl::Memory> vecGLMems;
			vecGLMems.push_back(imageColor);
			if (requireDepth())
				vecGLMems.push_back(imageDepth);
			vecGLMems.push_back(imageRes);
			m_queue->enqueueAcquireGLObjects(&vecGLMems);

			runKernel(imageColor, imageDepth, texEnvMap, imageRes);

			m_queue->enqueueReleaseGLObjects(&vecGLMems);
			m_queue->flush();
			
			return texRes;
		} catchCLError;
	}
};