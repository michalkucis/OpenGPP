#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv/cv.hpp>
#include <OpenGPP.h>
#include "UserTransformation.h"

#include <Windows.h>
#include <fstream>

//struct ConstMemory_t
//{
//	int2 textureResolution;
//	int2 megaTextureResolution;
//	int2 halfTextureResolution;
//};


class ImageSwapper
{
	int n_nUsedImage;
	cl::Image2DGL m_imagesSwap[2];
	cl::Image2DGL m_imageSrc;
public:
	ImageSwapper(cl::Image2DGL sourceIm, cl::Image2DGL* imageSwap)
	{
		m_imageSrc = sourceIm;
		n_nUsedImage = -1;
		m_imagesSwap[0] = imageSwap[0];		
		m_imagesSwap[1] = imageSwap[1];
	}

	void getImages(cl::Image2DGL& src, cl::Image2DGL& dst)
	{
		if (n_nUsedImage==-1)
		{
			src = m_imageSrc;
			dst = m_imagesSwap[0];
			n_nUsedImage = 0;
		}
		else if(n_nUsedImage==0)
		{
			src = m_imagesSwap[0];
			dst = m_imagesSwap[1];
			n_nUsedImage = 1;
		}
		else if(n_nUsedImage==1)
		{
			src = m_imagesSwap[1];
			dst = m_imagesSwap[0];
			n_nUsedImage = 0;
		}
	}
	void swap()
	{
		if (n_nUsedImage==-1)
		{
			n_nUsedImage = 0;
		}
		else if(n_nUsedImage==0)
		{
			n_nUsedImage = 1;
		}
		else if(n_nUsedImage==1)
		{
			n_nUsedImage = 0;
		}
	}
	void setImages(cl::Kernel& kernel)
	{
		cl::Image2DGL src, dst;
		getImages(src, dst);
		kernel.setArg(0, src);
		kernel.setArg(1, dst);
	}

	void setImages(cl::Kernel& kernel, cl::Image2DGL imTarget)
	{
		cl::Image2DGL src, dst;
		getImages(src, dst);
		kernel.setArg(0, src);
		kernel.setArg(1, imTarget);
	}
};



class EffectBase: public EffectCL<EffectGL>
{
private:
	void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult)
	{
		error0(ERR_STRUCT, "DON'T CALL ME BABY!!!");
	}

protected:
	virtual void performTransformation(ImageSwapper& imSwapper, int2 resolution)=0;
	virtual void performReconstruction(ImageSwapper& imSwapper, int2 resolution)=0;
	virtual void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])=0;



	virtual int2 getLvlResolution (int2 lvl0, int lvl)
	{
		int width = lvl0.x;
		int height = lvl0.y;
		for(int i = 0; i < lvl && (width > 1 || height > 1); i++)
		{
			height = height / 2 + height % 2;
			width = width / 2 + width % 2;
		}
		int2 res (width, height);
		return res;
	}

	int getUsefulCountLvls (int2 lvl0)
	{
		for (int lvl = 0; true; lvl++)
		{
			int2 res = getLvlResolution(lvl0, lvl);
			if (res.x == 1 && res.y == 1)
				return lvl;
		}
	}

	void copyImage(ImageSwapper& imSwapper, int2 resolution)
	{
		cl::Image2DGL src, dst;
		imSwapper.getImages(src,dst);
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


public:
	EffectBase(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory):
	  EffectCL(factoryRGB, factoryRed, factory)
	  {

	  }
	  Ptr<GLTexture2D> m_texRes;
	  Ptr<GLTexture2D> m_texTmp1;
	  Ptr<GLTexture2D> m_texTmp2;
	  cl::Image2DGL m_imageTmp[2];
	  cl::Image2DGL m_imageRes;


	  cl::Image2DGL m_imageColor;

	  virtual Ptr<GLTexture2D> process (Ptr<GLTexture2D> texColor, Ptr<GLTexture2D> texDepth, Ptr<GLTextureEnvMap> texEnvMap)
	  {
		  try 
		  {
			  static bool bInit = false;
			  if (! bInit)
			  {
				  bInit = true;
				  m_texRes = createTexTarget();
				  m_texTmp1 = m_factoryTexRed->createProduct();
				  m_texTmp2 = m_factoryTexRed->createProduct();
				  m_imageTmp[0] = getImage2DGL(*m_context, CL_MEM_READ_WRITE, m_texTmp1);
				  m_imageTmp[1] = getImage2DGL(*m_context, CL_MEM_READ_WRITE, m_texTmp2);
				  m_imageRes = getImage2DGL(*m_context, CL_MEM_WRITE_ONLY, m_texRes);

				  m_imageColor = getImage2DGL(*m_context, CL_MEM_READ_ONLY, texColor);
			  }

			  glFlush();

			  std::vector<cl::Memory> vecGLMems;
			  vecGLMems.push_back(m_imageColor);
			  for (int i = 0; i < 2; i++)
				  vecGLMems.push_back(m_imageTmp[i]);

			  vecGLMems.push_back(m_imageRes);
			  m_queue->enqueueAcquireGLObjects(&vecGLMems);
			  performOperations(m_imageColor, m_imageRes, m_imageTmp);

			  m_queue->enqueueReleaseGLObjects(&vecGLMems);
			  m_queue->flush();

			  return m_texRes;
		  } catchCLError;

	  }
};


class EffectAccessOneThreadTwoPixelsBorders: public EffectBase
{
	cl::Kernel m_kernelLinesTransform;
	cl::Kernel m_kernelColumnsTransform;
	cl::Kernel m_kernelLinesReconstruct;
	cl::Kernel m_kernelColumnsReconstruct;

	cl::Buffer m_constMemory;
	ConstMemory_t m_hostConstMem;
public:
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectAccessOneThreadTwoPixelsBorders(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(factoryRGB, factoryRed, factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectAccessOneThreadTwoPixelsBorders(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

public:
	int getOptCountWorkItems (int resolution)
	{
		int count = resolution/2 + resolution%2 + 4;
		int mask = 32-1;
		count = (count & (~mask)) + ((count&mask) ? 32 : 1);
		return count;
	}

public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl = getLvlResolution(res, i);
			if (resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesTransform);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesTransform.setArg(2, numWorkItemWidth*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesTransform.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsTransform);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				m_kernelColumnsTransform.setArg(2, numWorkItemHeight*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsTransform.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void performReconstruction(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl (getLvlResolution(res, i));

			if ( resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsReconstruct);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1,numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numWorkItemHeight);
				m_kernelColumnsReconstruct.setArg(2, numWorkItemHeight*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if ( resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesReconstruct);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesReconstruct.setArg(2, numWorkItemWidth*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());

		// transform:
		performTransformation(imSwapper, res);

		// reconstruct:
		performReconstruction(imSwapper, res);		

		cl::Image2DGL src, dst;
		imSwapper.getImages(src, dst);

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

class EffectAccessOneThreadTwoPixelsBorders2: public EffectBase
{
	cl::Kernel m_kernelLinesTransform;
	cl::Kernel m_kernelColumnsTransform;
	cl::Kernel m_kernelLinesReconstruct;
	cl::Kernel m_kernelColumnsReconstruct;

	cl::Buffer m_constMemory;
	ConstMemory_t m_hostConstMem;
public:
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectAccessOneThreadTwoPixelsBorders2(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(factoryRGB, factoryRed, factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders2.cl", "cdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectAccessOneThreadTwoPixelsBorders2(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders2.cl", "cdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

public:
	int getOptCountWorkItems (int resolution)
	{
		int count = resolution/2 + resolution%2 + 4;
		int mask = 32-1;
		count = (count & (~mask)) + ((count&mask) ? 32 : 1);
		return count;
	}

public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl = getLvlResolution(res, i);
			if (resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesTransform);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesTransform.setArg(2, numWorkItemWidth*sizeof(float)*2, 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesTransform.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsTransform);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				m_kernelColumnsTransform.setArg(2, numWorkItemHeight*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsTransform.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void performReconstruction(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl (getLvlResolution(res, i));

			if ( resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsReconstruct);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1,numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numWorkItemHeight);
				m_kernelColumnsReconstruct.setArg(2, numWorkItemHeight*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if ( resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesReconstruct);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesReconstruct.setArg(2, numWorkItemWidth*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());

		// transform:
		performTransformation(imSwapper, res);

		// reconstruct:
		performReconstruction(imSwapper, res);		

		cl::Image2DGL src, dst;
		imSwapper.getImages(src, dst);

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





class EffectAccessOneThreadSquareBorders: public EffectBase
{
	cl::Kernel m_kernelTransform;
	cl::Kernel m_kernelLinesReconstruct;
	cl::Kernel m_kernelColumnsReconstruct;

	cl::Buffer m_constMemory;
	ConstMemory_t m_hostConstMem;
public:
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectAccessOneThreadSquareBorders(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(factoryRGB, factoryRed, factory)
	{
		m_kernelTransform = createKernel(*m_context, "kernels\\wperfCDF97SquareBorders.cl", "cdf97transform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectAccessOneThreadSquareBorders(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelTransform = createKernel(*m_context, "kernels\\wperfCDF97SquareBorders.cl", "cdf97transform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

public:
	int getOptCountWorkItems (int resolution)
	{
		int count = resolution/2 + resolution%2 + 4;
		int mask = 32-1;
		count = (count & (~mask)) + ((count&mask) ? 32 : 1);
		return count;
	}

	int2 getOptGlobalItems(int2 resolution, int2 groupItems)
	{
		int2 grid = groupItems*2 - 8;
		int2 workGroups = resolution/grid;
		workGroups.x += (resolution.x/grid.x)?1:0;
		workGroups.y += (resolution.y/grid.y)?1:0;
		return workGroups * groupItems;
	}
public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl = getLvlResolution(res, i);

			imSwapper.setImages(m_kernelTransform);

			int2 numWorkItems(32, 32);
			int2 numGlobalItems(getOptGlobalItems(resLvl, numWorkItems));
			m_kernelTransform.setArg(2, numWorkItems.getArea()*sizeof(float)*2, 0);
			initKernelConstMemory(resLvl);
			m_kernelTransform.setArg(3, m_constMemory);
			m_queue->enqueueNDRangeKernel(m_kernelTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		}
	}
	void performReconstruction(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl (getLvlResolution(res, i));

			if ( resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsReconstruct);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1,numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numWorkItemHeight);
				m_kernelColumnsReconstruct.setArg(2, numWorkItemHeight*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if ( resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesReconstruct);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesReconstruct.setArg(2, numWorkItemWidth*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());

		// transform:
		performTransformation(imSwapper, res);

		// reconstruct:
		performReconstruction(imSwapper, res);		

		cl::Image2DGL src, dst;
		imSwapper.getImages(src, dst);

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

/*
class EffectAccessOneThreadTwoPixels: public EffectBase
{
	cl::Kernel m_kernelLinesTransform;
	cl::Kernel m_kernelColumnsTransform;
	cl::Kernel m_kernelLinesReconstruct;
	cl::Kernel m_kernelColumnsReconstruct;


	cl::Buffer m_constMemory;
	ConstMemory_t m_hostConstMem;
public:
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectAccessOneThreadTwoPixels(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(factoryRGB, factoryRed, factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixels.cl", "cdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixels.cl", "cdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixels.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixels.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectAccessOneThreadTwoPixels(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixels.cl", "cdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\wperfCDF97TwoPixels.cl", "cdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixels.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixels.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

public:
	int getOptCountWorkItems (int resolution)
	{
		int count = resolution/2 + resolution%2;
		int mask = 32-1;
		count = (count & (~mask)) + ((count&mask) ? 32 : 1);
		return count;
	}

public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl = getLvlResolution(res, i);
			if (resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesTransform);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesTransform.setArg(2, (numWorkItemWidth+12)*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesTransform.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsTransform);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				m_kernelColumnsTransform.setArg(2, (numWorkItemHeight+12)*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsTransform.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void performReconstruction(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl (getLvlResolution(res, i));
			if ( resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsReconstruct);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1,numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numWorkItemHeight);
				m_kernelColumnsReconstruct.setArg(2, numWorkItemHeight*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
				
			if ( resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesReconstruct);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesReconstruct.setArg(2, numWorkItemWidth*sizeof(float4), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
			copyImage (imSwapper, resLvl);
		}
	}
	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());

		// transform:
		performTransformation(imSwapper, res);

		// reconstruct:
		performReconstruction(imSwapper, res);		

		cl::Image2DGL src, dst;
		imSwapper.getImages(src, dst);

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
*/


class EffectAccessOneThreadFourPixelsBorders: public EffectBase
{
	cl::Kernel m_kernelLinesTransform;
	cl::Kernel m_kernelColumnsTransform;
	cl::Kernel m_kernelLinesReconstruct;
	cl::Kernel m_kernelColumnsReconstruct;


	cl::Buffer m_constMemory;
	ConstMemory_t m_hostConstMem;
public:
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectAccessOneThreadFourPixelsBorders(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(factoryRGB, factoryRed, factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\wperfCDF97FourPixelsBorders.cl", "cdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\wperfCDF97FourPixelsBorders.cl", "cdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectAccessOneThreadFourPixelsBorders(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\wperfCDF97FourPixelsBorders.cl", "cdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\wperfCDF97FourPixelsBorders.cl", "cdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\wperfCDF97TwoPixelsBorders.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

public:
	int getOptCountWorkItems (int resolution)
	{
		resolution += 8;
		int count = resolution/4 + (resolution%4?1:0);
		return count;
	}

public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl = getLvlResolution(res, i);
			if (resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesTransform);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesTransform.setArg(2, (numWorkItemWidth*sizeof(float)+12), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesTransform.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsTransform);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				m_kernelColumnsTransform.setArg(2, (numWorkItemHeight+12)*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsTransform.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void performReconstruction(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl (getLvlResolution(res, i));

			if ( resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsReconstruct);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y)*2;
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1,numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numWorkItemHeight);
				m_kernelColumnsReconstruct.setArg(2, (numWorkItemHeight+12)*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
				
			if ( resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesReconstruct);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x)*2;
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesReconstruct.setArg(2, (12+numWorkItemWidth)*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());

		// transform:
		performTransformation(imSwapper, res);

		// reconstruct:
		performReconstruction(imSwapper, res);		

		cl::Image2DGL src, dst;
		imSwapper.getImages(src, dst);

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


class EffectCopy1to1Image: public EffectBase
{
	cl::Kernel m_kernel1;
	cl::Kernel m_kernel2;


	cl::Buffer m_constMemory;
	ConstMemory_t m_hostConstMem;
	
	int2 getLvlResolution (int2 lvl0, int lvl)
	{
		lvl0 = int2(std::min(lvl0.x,1024), std::min(lvl0.y,1024));
		return EffectBase::getLvlResolution(lvl0, lvl);
	}
public:
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectCopy1to1Image(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(factoryRGB, factoryRed, factory)
	{
		m_kernel1 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy1in1Image");
		m_kernel2 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy1in1Image");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectCopy1to1Image(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernel1 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy1in1Image");
		m_kernel2 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy1in1Image");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl = getLvlResolution(res, i);
			if (resLvl.x > 1)
			{
				imSwapper.setImages(m_kernel1);

				int numWorkItemWidth = resLvl.x;
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel1, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.x > 1)
			{
				imSwapper.setImages(m_kernel1);

				int numWorkItemWidth = resLvl.x;
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel1, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				imSwapper.setImages(m_kernel2);

				int numWorkItemHeight = resLvl.y;
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel2, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void EffectBase::performReconstruction(ImageSwapper &,int2)
	{

	}
	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());

		// transform:
		performTransformation(imSwapper, res);	

		cl::Image2DGL src, dst;
		imSwapper.getImages(src, dst);

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




class EffectCopy2to1Image: public EffectBase
{
	cl::Kernel m_kernel1;
	cl::Kernel m_kernel2;


	cl::Buffer m_constMemory;
	ConstMemory_t m_hostConstMem;

	int2 getLvlResolution (int2 lvl0, int lvl)
	{
		lvl0 = int2(std::min(lvl0.x,1024), std::min(lvl0.y,1024));
		return EffectBase::getLvlResolution(lvl0, lvl);
	}
public:
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectCopy2to1Image(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(factoryRGB, factoryRed, factory)
	{
		m_kernel1 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Image1");
		m_kernel2 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Image2");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectCopy2to1Image(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernel1 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Image1");
		m_kernel2 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Image2");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		int i = 0;
		{
			int2 resLvl = getLvlResolution(res, i);
			if (resLvl.x > 1)
			{
				imSwapper.setImages(m_kernel1);

				int numWorkItemWidth = resLvl.x/2 + resLvl.x%2;
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel1, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.x > 1)
			{
				imSwapper.setImages(m_kernel1);

				int numWorkItemWidth = resLvl.x/2 + resLvl.x%2;
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel1, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				imSwapper.setImages(m_kernel2);

				int numWorkItemHeight = resLvl.y/2 + resLvl.y%2;
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel2, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void EffectBase::performReconstruction(ImageSwapper &,int2)
	{

	}
	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());

		// transform:
		performTransformation(imSwapper, res);	

		cl::Image2DGL src, dst;
		imSwapper.getImages(src, dst);

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


class EffectCopy1to1Buffer: public EffectBase
{
	cl::Kernel m_kernel1;
	cl::Kernel m_kernel2;
	cl::Kernel m_kernel3;

	cl::Buffer m_constMemory;
	cl::Buffer m_buffer1, m_buffer2;
	ConstMemory_t m_hostConstMem;

	int2 getLvlResolution (int2 lvl0, int lvl)
	{
		lvl0 = int2(std::min(lvl0.x,1024), std::min(lvl0.y,1024));
		return EffectBase::getLvlResolution(lvl0, lvl);
	}
public:
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectCopy1to1Buffer(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(factoryRGB, factoryRed, factory)
	{
		m_kernel1 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy1in1Buffer1");
		m_kernel2 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy1in1Buffer2");
		m_kernel3 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy1in1Buffer3");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
		m_buffer1 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
		m_buffer2 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
	}

	EffectCopy1to1Buffer(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernel1 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy1in1Buffer1");
		m_kernel2 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy1in1Buffer2");
		m_kernel3 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy1in1Buffer3");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
		m_buffer1 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
		m_buffer2 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
	}

public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		cl::Image2DGL imIn, imOut;
		imSwapper.getImages(imIn, imOut);
		
		int i = 0;
		{
			int2 resLvl = getLvlResolution(res, i);
			if (resLvl.x > 1)
			{
				m_kernel1.setArg(0, imIn);
				m_kernel1.setArg(1, m_buffer1);
				m_kernel1.setArg(2, 1024);

				int numWorkItemWidth = resLvl.x;
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel1, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.x > 1)
			{
				m_kernel2.setArg(0, m_buffer1);
				m_kernel2.setArg(1, m_buffer2);
				m_kernel2.setArg(2, 1024);

				int numWorkItemWidth = resLvl.x;
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel2, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				m_kernel3.setArg(0, m_buffer2);
				m_kernel3.setArg(1, imOut);
				m_kernel3.setArg(2, 1024);

				int numWorkItemHeight = resLvl.y;
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel3, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}
	void EffectBase::performReconstruction(ImageSwapper &,int2)
	{

	}
	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());

		// transform:
		performTransformation(imSwapper, res);	

		cl::Image2DGL src, dst;
		imSwapper.getImages(src, dst);

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


class EffectCopy2to1Buffer: public EffectBase
{
	cl::Kernel m_kernel1;
	cl::Kernel m_kernel2;
	cl::Kernel m_kernel3;
	cl::Kernel m_kernel4;

	cl::Buffer m_constMemory;
	cl::Buffer m_buffer1, m_buffer2;
	ConstMemory_t m_hostConstMem;

	int2 getLvlResolution (int2 lvl0, int lvl)
	{
		lvl0 = int2(std::min(lvl0.x,1024), std::min(lvl0.y,1024));
		return EffectBase::getLvlResolution(lvl0, lvl);
	}
public:
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectCopy2to1Buffer(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(factoryRGB, factoryRed, factory)
	{
		m_kernel1 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Buffer1");
		m_kernel2 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Buffer2");
		m_kernel3 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Buffer3");
		m_kernel4 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Buffer4");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
		m_buffer1 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
		m_buffer2 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
	}

	EffectCopy2to1Buffer(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernel1 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Buffer1");
		m_kernel2 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Buffer2");
		m_kernel3 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Buffer3");
		m_kernel4 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy2in1Buffer4");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
		m_buffer1 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
		m_buffer2 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
	}

public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		cl::Image2DGL imIn, imOut;
		imSwapper.getImages(imIn, imOut);
		//imSwapper.swap();

		int i = 0;
		{
			int2 resLvl = getLvlResolution(res, i);
			if (resLvl.x > 1)
			{
				m_kernel1.setArg(0, imIn);
				m_kernel1.setArg(1, m_buffer1);
				m_kernel1.setArg(2, 1024);

				int numWorkItemWidth = resLvl.x/2+resLvl.x%2;
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel1, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.x > 1)
			{
				m_kernel2.setArg(0, m_buffer1);
				m_kernel2.setArg(1, m_buffer2);
				m_kernel2.setArg(2, 1024);

				int numWorkItemWidth = resLvl.x/2+resLvl.x%2;
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel2, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				m_kernel3.setArg(0, m_buffer2);
				m_kernel3.setArg(1, m_buffer1);
				m_kernel3.setArg(2, 1024);

				int numWorkItemHeight = resLvl.y/2+resLvl.y%2;
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel3, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);


			if (resLvl.y > 1)
			{
				m_kernel4.setArg(0, m_buffer1);
				m_kernel4.setArg(1, imOut);
				m_kernel4.setArg(2, 1024);

				int numWorkItemHeight = resLvl.y/2+resLvl.y%2;
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel4, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);							
		}
	}
	void EffectBase::performReconstruction(ImageSwapper &,int2)
	{

	}

	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());

		// transform:
		performTransformation(imSwapper, res);	

		cl::Image2DGL src, dst;
		imSwapper.getImages(src, dst);

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


class EffectCopy4to1Buffer: public EffectBase
{
	cl::Kernel m_kernel1;
	cl::Kernel m_kernel2;
	cl::Kernel m_kernel3;

	cl::Buffer m_constMemory;
	cl::Buffer m_buffer1, m_buffer2;
	ConstMemory_t m_hostConstMem;

	int2 getLvlResolution (int2 lvl0, int lvl)
	{
		lvl0 = int2(std::min(lvl0.x,1024), std::min(lvl0.y,1024));
		return EffectBase::getLvlResolution(lvl0, lvl);
	}
public:
	void initKernelConstMemory(int2 imageRes)
	{
		m_hostConstMem.textureResolution = imageRes;
		m_hostConstMem.megaTextureResolution = imageRes*2-2;
		m_hostConstMem.halfTextureResolution = imageRes/2+imageRes%2;

		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectCopy4to1Buffer(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(factoryRGB, factoryRed, factory)
	{
		m_kernel1 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy4in1Buffer1");
		m_kernel2 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy4in1Buffer2");
		m_kernel3 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy4in1Buffer3");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
		m_buffer1 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
		m_buffer2 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
	}

	EffectCopy4to1Buffer(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBase(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernel1 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy4in1Buffer1");
		m_kernel2 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy4in1Buffer2");
		m_kernel3 = createKernel(*m_context, "kernels\\wperfCopyImage.cl", "copy4in1Buffer3");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
		m_buffer1 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
		m_buffer2 = cl::Buffer(*m_context, CL_MEM_READ_WRITE, 1024*1024*sizeof(float), NULL);
	}

public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		cl::Image2DGL imIn, imOut;
		imSwapper.getImages(imIn, imOut);
		//imSwapper.swap();

		int i = 0;
		{
			int2 resLvl = getLvlResolution(res, i);
			if (resLvl.x > 1)
			{
				m_kernel1.setArg(0, imIn);
				m_kernel1.setArg(1, m_buffer1);
				m_kernel1.setArg(2, 1024);

				//int numWorkItemWidth = resLvl.x/2+resLvl.x%2;
				//int numWorkItemHeight = resLvl.y/2+resLvl.y%2;
				//int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(32, 32);
				int2 numGlobalItems(resLvl/2+resLvl%2);
				numGlobalItems = (numGlobalItems/numWorkItems+1)*numWorkItems;
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel1, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.x > 1)
			{
				m_kernel2.setArg(0, m_buffer1);
				m_kernel2.setArg(1, m_buffer2);
				m_kernel2.setArg(2, 1024);

				int2 numWorkItems(32, 32);
				int2 numGlobalItems(resLvl/2+resLvl%2);
				numGlobalItems = (numGlobalItems/numWorkItems+1)*numWorkItems;
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel2, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				m_kernel3.setArg(0, m_buffer2);
				m_kernel3.setArg(1, imOut);
				m_kernel3.setArg(2, 1024);

				int2 numWorkItems(32, 32);
				int2 numGlobalItems(resLvl/2+resLvl%2);
				numGlobalItems = (numGlobalItems/numWorkItems+1)*numWorkItems;
				initKernelConstMemory(resLvl);
				m_queue->enqueueNDRangeKernel(m_kernel3, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);							
		}
	}
	void EffectBase::performReconstruction(ImageSwapper &,int2)
	{

	}

	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());

		// transform:
		performTransformation(imSwapper, res);	

		cl::Image2DGL src, dst;
		imSwapper.getImages(src, dst);

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


//class SharedObjects
//{
//	ImageSwapper& m_imSwapper;
//	cl::Buffer m_buffer1, m_buffer2;
//	int m_nUsedBuffer;
//public:
//	SharedObjects (ImageSwapper& imSwapper, int2 resolutionBuffer):
//		m_imSwapper(imSwapper)
//	{
//		m_nUsedBuffer = -1;
//	}
//
//	ImageSwapper& getImageSwapper ()
//	{
//		return m_imSwapper;
//	}
//	cl::Buffer& getSrcBuffer()
//	{
//		if (m_nUsedBuffer == -1)
//			assert(0);
//		else if (m_nUsedBuffer == 0)
//			return m_buffer1;
//		else if (m_nUsedBuffer == 1)
//			return m_buffer2;
//	}
//	cl::Buffer& getDstBuffer()
//	{
//		if (m_nUsedBuffer == -1)
//			return m_buffer1;
//		else if (m_nUsedBuffer == 0)
//			return m_buffer2;
//		else if (m_nUsedBuffer == 1)
//			return m_buffer1;
//
//	}
//	void exchangeBuffers()
//	{
//		if (m_nUsedBuffer == -1)
//			m_nUsedBuffer = 0;
//		else if (m_nUsedBuffer == 0)
//			m_nUsedBuffer = 1;
//		else if (m_nUsedBuffer == 1)
//			m_nUsedBuffer = 0;
//	}
//};
//
//
//
//



class Visitor2ReadRGBToSaveMatlabASCII: public Visitor2ReadOnly<float3>
{
	std::ofstream fs;
public:
	Visitor2ReadRGBToSaveMatlabASCII (string filename)
	{
		fs.open(filename);
	}
	void visitRead (int2 n, const float3& rgb)
	{
		float red = rgb.x;
		if (n.x == 0)
			fs << "\n";
		else 
			fs << ", ";
		fs << red;
	}
};
class EffectSaveToMatlabASCII: public Effect
{
	string m_filename;
public:
	EffectSaveToMatlabASCII (string filename)
	{
		m_filename = filename;
	}
	void setFilename (string filename)
	{
		m_filename = filename;
	}
	string getFilename ()
	{
		return m_filename;
	}
	Ptr<GLTexture2D> process(Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
	{
		glFinish();
		int width = tex->getResolution().x;
		int height = tex->getResolution().y;

		Visitor2ReadRGBToSaveMatlabASCII visitor(m_filename);
		tex->visitReadOnly(&visitor);
		return tex;
	}
};


class ApplicationPerformance: public Application
{
	uint2 m_resolutionPP;
	Ptr<PostProcessor> m_pp;
	EffectUserTransforms* m_effect;
public:
	ApplicationPerformance (): Application(1024, 768)
	{
	}
	void runKernel(cl::Image2DGL &,cl::Image2DGL &,Ptr<GLTextureEnvMap>,cl::Image2DGL &)
	{
	}
	void initData()
	{
		m_resolutionPP = uint2(1037,777);
		m_effect = new EffectUserTransforms(new SharedObjectsFactory(m_resolutionPP), new EffectCLObjectsFactory);
		m_effect->pushBackTransformation(new TransForwardHorizon2in1ImageToImage(new EffectCLObjectsFactory), m_resolutionPP.get2<int2>());

		m_pp = new PostProcessor(new SharedObjectsFactory(m_resolutionPP));
		m_pp->m_input = new InputLoadFromSingleFileOpenEXR("exrChangeLightIntensity\\img_light1_lamp250_pos0.exr");
		//m_pp->m_vecEffects.pushBack(new EffectAccessOneThreadSquareBorders(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		//m_pp->m_vecEffects.pushBack(new EffectAccessOneThreadFourPixelsBorders(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		//m_pp->m_vecEffects.pushBack(new EffectAccessOneThreadSquareBorders(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		//m_pp->m_vecEffects.pushBack(new EffectAccessOneThreadTwoPixelsBorders2(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		//m_pp->m_vecEffects.pushBack(new EffectAccessOneThreadTwoPixelsBorders(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		//m_pp->m_vecEffects.pushBack(new EffectCopy4to1Buffer(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		m_pp->m_vecEffects.pushBack(m_effect);
		//m_pp->m_vecEffects.pushBack(new EffectCopy2to1Buffer(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		//m_pp->m_vecEffects.pushBack(new EffectCopy1to1Buffer(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		//m_pp->m_vecEffects.pushBack(new EffectCopy1to1Image(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		//m_pp->m_vecEffects.pushBack(new EffectCopy2to1Image(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		m_pp->m_vecEffects.pushBack(new EffectRenderToScreen(0,0,1,1));
		//m_pp->m_vecEffects.pushBack(new EffectCopyColorAndDepthMaps(new SharedObjectsFactory(resolution)));
		//m_pp->m_vecEffects.pushBack(new EffectSaveToMatlabASCII("out.txt"));
	}
	void render()
	{
		m_pp->process();
		//sendQuit();

		static int start = GetTickCount();
		static int nFrames = -1;
		nFrames++;
		int actual = GetTickCount();
		printf("Time: %f\n", 1.0f/((actual-start)/1000.0f/(float)nFrames));
	}
	void clearData ()
	{		
		m_pp->clear ();
	}
};



int main( int argc, char* argv[] )
{
	try
	{	
		ApplicationPerformance a; 
		a.run();
	} catch (Error& ex) {
		std::string desc = ex.getDesc();
		printf ("%s\n", desc.c_str());
		std::cout << "Wait until any button to be pressed...";
		std::cin.ignore(1);	
	}

	return 0;
}
