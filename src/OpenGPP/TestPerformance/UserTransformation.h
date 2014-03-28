#pragma once

#include <OpenGPP.h>

struct ConstMemory_t
{
	int2 textureResolution;
	int2 megaTextureResolution;
	int2 halfTextureResolution;
};

class SharedObjects
{
protected:
	cl::Image2DGL m_imIn, m_imOut;
public:
	SharedObjects(int2 maxResolution, cl::Image2DGL imIn, cl::Image2DGL imOut)
	{
		m_imIn = imIn;
		m_imOut = imOut;
	}
	cl::Image2DGL getSrcImage()
	{
		return m_imIn;
	}
	cl::Image2DGL getDstImage()
	{
		return m_imOut;
	}
	void switchData()
	{

	}

};

class UserTransformation
{
protected:
	Ptr<cl::Context> m_context;
	Ptr<cl::CommandQueue> m_queue;
	cl::Buffer m_constMemory;
	ConstMemory_t m_hostConstMem;
public:	
	UserTransformation(Ptr<EffectCLObjectsFactory> clFactory)
	{
		m_context = clFactory->getCLGLContext();
		m_queue = clFactory->getCLCommandQueue();

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	int ceilToWarpSizeMult(int n)
	{
		return 64*(n/64+(n%64)?1:0);
	}
	void copyImage(Ptr<SharedObjects> so, int2 resolution)
	{
		cl::Image2DGL src(so->getSrcImage()), dst(so->getDstImage());
		so->switchData();
		cl::size_t<3> zero;
		zero[0] = 0;
		zero[1] = 0;
		zero[2] = 0;
		cl::size_t<3> region;
		region[0] = resolution.x;
		region[1] = resolution.y;
		region[2] = 1;
		m_queue->enqueueCopyImage(src, dst, zero, zero, region);
	}
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	virtual void process(Ptr<SharedObjects> sharedObject, int2 resolution)=0;
};


class TransForwardHorizon2in1ImageToImage: public UserTransformation
{
	cl::Kernel m_kernel;
public:
	TransForwardHorizon2in1ImageToImage(Ptr<EffectCLObjectsFactory> clFactory):
	  UserTransformation(clFactory)
	  {
		  m_kernel = createKernel(*m_context, "kernels\\wperfCDF97User.cl", "horizonImage2Image");
	  }
	  void process(Ptr<SharedObjects> so, int2 resolution)
	  {
		  initKernelConstMemory(resolution);
		  if (resolution.x > 1)
		  {
			  m_kernel.setArg(0, so->getSrcImage());
			  m_kernel.setArg(1, so->getDstImage());
			  so->switchData();

			  int numWorkItemWidth = resolution.x;
			  int numGlobalItemsWidth = numWorkItemWidth;
			  int2 numWorkItems(numWorkItemWidth, 1);
			  int2 numGlobalItems(numGlobalItemsWidth, resolution.y);
			  //m_kernel.setArg(2, numWorkItemWidth*sizeof(float4), 0);
			  initKernelConstMemory(resolution);
			  //m_kernel.setArg(3, m_constMemory);
			  m_queue->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  //else
		  //	  copyImage (imSwapper, resolution);
	  }
};


class EffectUserTransforms: public EffectCL<EffectGL>
{
	Vector<Ptr<UserTransformation>> m_vecTransforms;
	Vector<int2> m_vecResolutions;
public:
	EffectUserTransforms(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectCL(factoryRGB, factoryRed, factory)
	{
		
	}

	EffectUserTransforms(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectCL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		
	}
public:
	void pushBackTransformation(Ptr<UserTransformation> trans, int2 resolution)
	{
		m_vecTransforms.pushBack(trans);
		m_vecResolutions.pushBack(resolution);
	}
	void performTransformation(Ptr<SharedObjects> so)
	{
		size_t s = m_vecResolutions.getSize();
		for (size_t i = 0; i < s; i++)
		{
			int2 res = m_vecResolutions[i];
			m_vecTransforms[i]->process(so, res);
		}
	}
public:
	void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult)
	{
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());	
		Ptr<SharedObjects> so = new SharedObjects(res, imageColor,imageResult);

		// transform:
		performTransformation(so);	

		cl::Image2DGL src(so->getSrcImage()), dst(so->getDstImage());

		//res = int2(imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());
		dst = imageResult;
		cl::size_t<3> zero;
		zero[0] = 0;
		zero[1] = 0;
		zero[2] = 0;
		cl::size_t<3> region;
		region[0] = res.x;
		region[1] = res.y;
		region[2] = 1;
		m_queue->enqueueCopyImage(src, dst, zero, zero, region);
	}

	Ptr<GLTexture2D> createTexTarget()
	{
		return m_factoryTexRed->createProduct();
	}
};
