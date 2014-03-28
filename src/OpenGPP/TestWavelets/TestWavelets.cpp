#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv/cv.hpp>
#include <OpenGPP.h>

#include <Windows.h>
#include <fstream>



struct ConstMemory_t
{
	int2 textureResolution;
	int2 megaTextureResolution;
	int2 halfTextureResolution;
};


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



class EffectBaseWavelet: public EffectCL<EffectGL>
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
	


	int2 getLvlResolution (int2 lvl0, int lvl)
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
	EffectBaseWavelet(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory):
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








class EffectCDF53: public EffectBaseWavelet
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
	EffectCDF53(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBaseWavelet(factoryRGB, factoryRed, factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf53linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf53columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf53linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf53columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectCDF53(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBaseWavelet(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf53linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf53columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf53linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf53columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	int getOptCountWorkItems (int resolution)
	{
		int count = resolution/2 + resolution%2 + 2;
		int mask = 32-1;
		count = (count & (~mask)) + ((count&mask) ? 32 : 1);
		return count;
	}
public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		int countLvls = getUsefulCountLvls(res);
		for (int i = 0; i < countLvls; i++)
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
		for (int i = getUsefulCountLvls(res)-1; i >= 0; i--)
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



class EffectCDF97: public EffectBaseWavelet
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
	EffectCDF97(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBaseWavelet(factoryRGB, factoryRed, factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectCDF97(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBaseWavelet(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "cdf97columnsReconstruct");

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
		int countLvls = getUsefulCountLvls(res);
		for (int i = 0; i < countLvls; i++)
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
		for (int i = getUsefulCountLvls(res)-1; i >= 0; i--)
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



class EffectWCDF53: public EffectBaseWavelet
{
	cl::Kernel m_kernelLinesTransform;
	cl::Kernel m_kernelColumnsTransform;
	cl::Kernel m_kernelLinesReconstruct;
	cl::Kernel m_kernelColumnsReconstruct;

	Vector<cl::Image2DGL> m_vecImagesHorizontWeight;
	Vector<cl::Image2DGL> m_vecImagesVerticalWeight;

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
	int2 getImageResolution (cl::Image2DGL& im)
	{
		int2 res (im.getImageInfo<CL_IMAGE_WIDTH>(), im.getImageInfo<CL_IMAGE_HEIGHT>());
		return res;
	}
	void updateImagesWeight (int2 imageRes)
	{
		int countLvls = getUsefulCountLvls(imageRes);
		if (m_vecImagesHorizontWeight.getSize() != countLvls)
			m_vecImagesHorizontWeight.setSize(countLvls);
		if (m_vecImagesVerticalWeight.getSize() != countLvls)
			m_vecImagesVerticalWeight.setSize(countLvls);

		for (int i = 0; i < countLvls; i++)
		{
			int2 res = getLvlResolution(imageRes, i);
			cl::Image2DGL imDefault;
			cl::Image2DGL imWeight = m_vecImagesHorizontWeight[i];
			bool needInit = false;
			if (imDefault() == imWeight())
				needInit = true;
			else if (res.x != getImageResolution(imWeight).x || res.y != getImageResolution(imWeight).y)
				needInit = true;

			if (needInit)
			{
				if (imDefault() != imWeight())
				{
					std::vector<cl::Memory> vecMemory;
					vecMemory.push_back(imWeight);
					m_queue->enqueueReleaseGLObjects(&vecMemory);
				}
				GLTexture2D* tex = new GLTexture2D (res.x, res.y, GL_R32F, GL_RED);
				cl::Image2DGL image = getImage2DGL(*m_context, CL_MEM_READ_WRITE, tex);
				m_vecImagesHorizontWeight[i] = image;
				std::vector<cl::Memory> vecMemory;
				vecMemory.push_back(image);
				m_queue->enqueueAcquireGLObjects(&vecMemory);
			}
		}
		for (int i = 0; i < countLvls; i++)
		{
			int2 res = getLvlResolution(imageRes, i);
			cl::Image2DGL imDefault;
			cl::Image2DGL imWeight = m_vecImagesVerticalWeight[i];
			bool needInit = false;
			if (imDefault() == imWeight())
				needInit = true;
			else if (res.x != getImageResolution(imWeight).x || res.y != getImageResolution(imWeight).y)
				needInit = true;

			if (needInit)
			{
				if (imDefault() != imWeight())
				{
					std::vector<cl::Memory> vecMemory;
					vecMemory.push_back(imWeight);
					m_queue->enqueueReleaseGLObjects(&vecMemory);
				}
				GLTexture2D* tex = new GLTexture2D (res.x, res.y, GL_R32F, GL_RED);
				cl::Image2DGL image = getImage2DGL(*m_context, CL_MEM_READ_WRITE, tex);
				m_vecImagesVerticalWeight[i] = image;
				std::vector<cl::Memory> vecMemory;
				vecMemory.push_back(image);
				m_queue->enqueueAcquireGLObjects(&vecMemory);
			}
		}
	}
	EffectWCDF53(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBaseWavelet(factoryRGB, factoryRed, factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf53linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf53columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf53linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf53columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectWCDF53(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBaseWavelet(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf53linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf53columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf53linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf53columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}


	int getOptCountWorkItems (int resolution)
	{
		int count = resolution/2 + resolution%2 + 2;
		int mask = 32-1;
		count = (count & (~mask)) + ((count&mask) ? 32 : 1);
		return count;
	}


public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{
		int countLvls = getUsefulCountLvls(res);
		for (int i = 0; i < countLvls; i++)
		{
			int2 resLvl = getLvlResolution(res, i);

			if (resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesTransform);
				m_kernelLinesTransform.setArg(2, m_vecImagesHorizontWeight[i]);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesTransform.setArg(3, numWorkItemWidth*sizeof(float), 0);
				m_kernelLinesTransform.setArg(4, numWorkItemWidth*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesTransform.setArg(5, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsTransform);
				m_kernelColumnsTransform.setArg(2, m_vecImagesVerticalWeight[i]);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				m_kernelColumnsTransform.setArg(3, numWorkItemHeight*sizeof(float), 0);
				m_kernelColumnsTransform.setArg(4, numWorkItemHeight*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsTransform.setArg(5, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}

	void performReconstruction(ImageSwapper& imSwapper, int2 res)
	{
		for (int i = getUsefulCountLvls(res)-1; i >= 0; i--)
		{
			int2 resLvl (getLvlResolution(res, i));

			if ( resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsReconstruct);
				m_kernelColumnsReconstruct.setArg(2, m_vecImagesVerticalWeight[i]);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1,numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numWorkItemHeight);
				m_kernelColumnsReconstruct.setArg(3, numWorkItemHeight*sizeof(float), 0);
				m_kernelColumnsReconstruct.setArg(4, numWorkItemHeight*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsReconstruct.setArg(5, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if ( resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesReconstruct);
				m_kernelLinesReconstruct.setArg(2, m_vecImagesHorizontWeight[i]);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesReconstruct.setArg(3, numWorkItemWidth*sizeof(float), 0);
				m_kernelLinesReconstruct.setArg(4, numWorkItemWidth*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesReconstruct.setArg(5, m_constMemory);
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
		updateImagesWeight(res);

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



class EffectWCDF97: public EffectBaseWavelet
{
	cl::Kernel m_kernelLinesTransform;
	cl::Kernel m_kernelColumnsTransform;
	cl::Kernel m_kernelLinesReconstruct;
	cl::Kernel m_kernelColumnsReconstruct;

	Vector<cl::Image2DGL> m_vecImagesHorizontWeight;
	Vector<cl::Image2DGL> m_vecImagesVerticalWeight;

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
	int2 getImageResolution (cl::Image2DGL& im)
	{
		int2 res (im.getImageInfo<CL_IMAGE_WIDTH>(), im.getImageInfo<CL_IMAGE_HEIGHT>());
		return res;
	}
	void updateImagesWeight (int2 imageRes)
	{
		int countLvls = getUsefulCountLvls(imageRes);
		if (m_vecImagesHorizontWeight.getSize() != countLvls)
			m_vecImagesHorizontWeight.setSize(countLvls);
		if (m_vecImagesVerticalWeight.getSize() != countLvls)
			m_vecImagesVerticalWeight.setSize(countLvls);

		for (int i = 0; i < countLvls; i++)
		{
			int2 res = getLvlResolution(imageRes, i);
			cl::Image2DGL imDefault;
			cl::Image2DGL imWeight = m_vecImagesHorizontWeight[i];
			bool needInit = false;
			if (imDefault() == imWeight())
				needInit = true;
			else if (res.x != getImageResolution(imWeight).x || res.y != getImageResolution(imWeight).y)
				needInit = true;

			if (needInit)
			{
				if (imDefault() != imWeight())
				{
					std::vector<cl::Memory> vecMemory;
					vecMemory.push_back(imWeight);
					m_queue->enqueueReleaseGLObjects(&vecMemory);
				}
				GLTexture2D* tex = new GLTexture2D (res.x, res.y, GL_R32F, GL_RED);
				cl::Image2DGL image = getImage2DGL(*m_context, CL_MEM_READ_WRITE, tex);
				m_vecImagesHorizontWeight[i] = image;
				std::vector<cl::Memory> vecMemory;
				vecMemory.push_back(image);
				m_queue->enqueueAcquireGLObjects(&vecMemory);
			}
		}
		for (int i = 0; i < countLvls; i++)
		{
			int2 res = getLvlResolution(imageRes, i);
			cl::Image2DGL imDefault;
			cl::Image2DGL imWeight = m_vecImagesVerticalWeight[i];
			bool needInit = false;
			if (imDefault() == imWeight())
				needInit = true;
			else if (res.x != getImageResolution(imWeight).x || res.y != getImageResolution(imWeight).y)
				needInit = true;

			if (needInit)
			{
				if (imDefault() != imWeight())
				{
					std::vector<cl::Memory> vecMemory;
					vecMemory.push_back(imWeight);
					m_queue->enqueueReleaseGLObjects(&vecMemory);
				}
				GLTexture2D* tex = new GLTexture2D (res.x, res.y, GL_R32F, GL_RED);
				cl::Image2DGL image = getImage2DGL(*m_context, CL_MEM_READ_WRITE, tex);
				m_vecImagesVerticalWeight[i] = image;
				std::vector<cl::Memory> vecMemory;
				vecMemory.push_back(image);
				m_queue->enqueueAcquireGLObjects(&vecMemory);
			}
		}
	}
	EffectWCDF97(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBaseWavelet(factoryRGB, factoryRed, factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectWCDF97(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBaseWavelet(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf97linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf97columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf97linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsNaive.cl", "wcdf97columnsReconstruct");

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}


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
		int countLvls = getUsefulCountLvls(res);
		for (int i = 0; i < countLvls; i++)
		{
			int2 resLvl = getLvlResolution(res, i);

			if (resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesTransform);
				m_kernelLinesTransform.setArg(2, m_vecImagesHorizontWeight[i]);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesTransform.setArg(3, numWorkItemWidth*sizeof(float), 0);
				m_kernelLinesTransform.setArg(4, numWorkItemWidth*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesTransform.setArg(5, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if (resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsTransform);
				m_kernelColumnsTransform.setArg(2, m_vecImagesVerticalWeight[i]);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numGlobalItemsHeight);
				m_kernelColumnsTransform.setArg(3, numWorkItemHeight*sizeof(float), 0);
				m_kernelColumnsTransform.setArg(4, numWorkItemHeight*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsTransform.setArg(5, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}

	void performReconstruction(ImageSwapper& imSwapper, int2 res)
	{
		for (int i = getUsefulCountLvls(res)-1; i >= 0; i--)
		{
			int2 resLvl (getLvlResolution(res, i));

			if ( resLvl.y > 1)
			{
				imSwapper.setImages(m_kernelColumnsReconstruct);
				m_kernelColumnsReconstruct.setArg(2, m_vecImagesVerticalWeight[i]);

				int numWorkItemHeight = getOptCountWorkItems(resLvl.y);
				int numGlobalItemsHeight = numWorkItemHeight;
				int2 numWorkItems(1,numWorkItemHeight);
				int2 numGlobalItems(resLvl.x, numWorkItemHeight);
				m_kernelColumnsReconstruct.setArg(3, numWorkItemHeight*sizeof(float), 0);
				m_kernelColumnsReconstruct.setArg(4, numWorkItemHeight*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelColumnsReconstruct.setArg(5, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);

			if ( resLvl.x > 1)
			{
				imSwapper.setImages(m_kernelLinesReconstruct);
				m_kernelLinesReconstruct.setArg(2, m_vecImagesHorizontWeight[i]);

				int numWorkItemWidth = getOptCountWorkItems(resLvl.x);
				int numGlobalItemsWidth = numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, resLvl.y);
				m_kernelLinesReconstruct.setArg(3, numWorkItemWidth*sizeof(float), 0);
				m_kernelLinesReconstruct.setArg(4, numWorkItemWidth*sizeof(float), 0);
				initKernelConstMemory(resLvl);
				m_kernelLinesReconstruct.setArg(5, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
				copyImage (imSwapper, resLvl);
		}
	}

	virtual void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());
		updateImagesWeight(res);
		
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

template <class EFFECT_WAVELET>
class EffectToneMappingBasedOnWavelets: public EFFECT_WAVELET
{
	cl::Kernel m_kernelPrepareHDR, m_kernelPerformToneMapping, m_kernelCreateLDR;
public:
	EffectToneMappingBasedOnWavelets(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
		EFFECT_WAVELET(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelPrepareHDR = createKernel(*m_context, "kernels\\toneMapping.cl", "prepareHDR");
		m_kernelPerformToneMapping = createKernel(*m_context, "kernels\\toneMapping.cl", "performToneMapping");
		m_kernelCreateLDR = createKernel(*m_context, "kernels\\toneMapping.cl", "createLDR");
	}

protected:
	cl::Image2DGL m_imageU, m_imageV;
	void updateImageU (int2 res)
	{
		cl::Image2DGL imDefault;
		bool needInit = false;
		if (imDefault() == m_imageU())
			needInit = true;
		else if (res.x != getImageResolution(m_imageU).x || res.y != getImageResolution(m_imageU).y)
			needInit = true;

		if (needInit)
		{
			if (imDefault() != m_imageU())
			{
				std::vector<cl::Memory> vecMemory;
				vecMemory.push_back(m_imageU);
				m_queue->enqueueReleaseGLObjects(&vecMemory);
			}
			GLTexture2D* tex = new GLTexture2D (res.x, res.y, GL_R32F, GL_RED);
			m_imageU = getImage2DGL(*m_context, CL_MEM_READ_WRITE, tex);
			std::vector<cl::Memory> vecMemory;
			vecMemory.push_back(m_imageU);
			m_queue->enqueueAcquireGLObjects(&vecMemory);
		}
	}

	void updateImageV (int2 res)
	{
		cl::Image2DGL imDefault;
		bool needInit = false;
		if (imDefault() == m_imageV())
			needInit = true;
		else if (res.x != getImageResolution(m_imageV).x || res.y != getImageResolution(m_imageV).y)
			needInit = true;

		if (needInit)
		{
			if (imDefault() != m_imageV())
			{
				std::vector<cl::Memory> vecMemory;
				vecMemory.push_back(m_imageV);
				m_queue->enqueueReleaseGLObjects(&vecMemory);
			}
			GLTexture2D* tex = new GLTexture2D (res.x, res.y, GL_R32F, GL_RED);
			m_imageV = getImage2DGL(*m_context, CL_MEM_READ_WRITE, tex);
			std::vector<cl::Memory> vecMemory;
			vecMemory.push_back(m_imageV);
			m_queue->enqueueAcquireGLObjects(&vecMemory);
		}
	}
public:
	void performOperations(cl::Image2DGL& imageColor, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());
		updateImagesWeight(res);
		updateImageU(res);
		updateImageV(res);
		int2 numGlobalItems = res;

		// rgb to log yuv
		imSwapper.setImages(m_kernelPrepareHDR);
		m_kernelPrepareHDR.setArg(2, m_imageU);
		m_kernelPrepareHDR.setArg(3, m_imageV);
		m_queue->enqueueNDRangeKernel(m_kernelPrepareHDR, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y));

		// wcdf...
		performTransformation(imSwapper, res);

		// tone mapping
		imSwapper.setImages(m_kernelPerformToneMapping);
		m_queue->enqueueNDRangeKernel(m_kernelPerformToneMapping, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y));
		
		// reverse wcdf
		performReconstruction(imSwapper, res);
		
		// log yuv to rgb
		imSwapper.setImages(m_kernelCreateLDR, imageResult);
		m_kernelCreateLDR.setArg(2, m_imageU);
		m_kernelCreateLDR.setArg(3, m_imageV);
		m_queue->enqueueNDRangeKernel(m_kernelCreateLDR, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y));
	}

	Ptr<GLTexture2D> createTexTarget()
	{
		return m_factoryTexRGB->createProduct();
	}
};



class EffectWavelet: public EffectBaseWavelet
{
	cl::Kernel m_kernelTransformOneLevel;
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
	EffectWavelet(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectBaseWavelet(factoryRGB, factoryRed, factory)
	{
		m_kernelTransformOneLevel = createKernel(*m_context, "kernels\\waveletsCool.cl", "tranformOneLevel");
		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}

	EffectWavelet(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectBaseWavelet(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelTransformOneLevel = createKernel(*m_context, "kernels\\waveletsCool.cl", "tranformOneLevel");
		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	int2 getCountGlobalItems (int2 resolution, int2 numWorkItemsPerGroup)
	{
		int2 numEffectiveItems = numWorkItemsPerGroup - 4;
		int2 globalItems = resolution/2 + resolution%2;
		globalItems.x = globalItems.x/numEffectiveItems.x + (globalItems.x%numEffectiveItems.x?1:0);
		globalItems.y = globalItems.y/numEffectiveItems.y + (globalItems.y%numEffectiveItems.y?1:0);
		return globalItems * numWorkItemsPerGroup;
	}
public:
	void performTransformation(ImageSwapper& imSwapper, int2 res)
	{		
		int countLvls = getUsefulCountLvls(res);
		for (int i = 0; i < countLvls; i++)
		{
			int2 resLvl = getLvlResolution(res, i);

			int2 imageSize = res;
			int2 numWorkItemsPerGroup(32, 32);
			int2 numGlobalItems(getCountGlobalItems(res, numWorkItemsPerGroup));

			imSwapper.setImages(m_kernelTransformOneLevel);
			m_kernelTransformOneLevel.setArg(2, 16 /*sizeof(float2)*numWorkItemsPerGroup.getArea()*/);
			initKernelConstMemory(resLvl);
			m_kernelTransformOneLevel.setArg(3, m_constMemory);

			m_queue->enqueueNDRangeKernel(m_kernelTransformOneLevel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItemsPerGroup.x, numWorkItemsPerGroup.y));
		}
	}
	void performReconstruction(ImageSwapper& imSwapper, int2 res)
	{
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





class ApplicationWavelets: public Application
{
	Ptr<PostProcessor> m_pp;

public:
	ApplicationWavelets (): Application(1024, 768)
	{
	}
	void initData()
	{
		uint2 resolution = uint2(1036,777);
		m_pp = new PostProcessor(new SharedObjectsFactory(resolution));
		m_pp->m_input = new InputLoadFromSingleFileOpenEXR("exrChangeLightIntensity\\img_light1_lamp250_pos0.exr");
		//m_pp->m_vecEffects.pushBack(new EffectToneMappingBasedOnWavelets<EffectWCDF53>(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		m_pp->m_vecEffects.pushBack(new EffectCDF97(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
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
		ApplicationWavelets a; 
		a.run();
	} catch (Error& ex) {
		std::string desc = ex.getDesc();
		printf ("%s\n", desc.c_str());
		std::cout << "Wait until any button to be pressed...";
		std::cin.ignore(1);	
	}

	return 0;
}
