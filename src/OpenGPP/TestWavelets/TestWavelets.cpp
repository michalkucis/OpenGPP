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





class EffectWaveletsCool: public EffectCL<EffectGL>
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

		m_constMemory = cl::Buffer(*m_context, CL_MEM_READ_ONLY, sizeof(m_hostConstMem), &m_hostConstMem);
		m_queue->enqueueWriteBuffer(m_constMemory, false, 0, sizeof(m_hostConstMem), &m_hostConstMem);
	}
	EffectWaveletsCool(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	EffectCL(factoryRGB, factoryRed, factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsCool.cl", "linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsCool.cl", "columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsCool.cl", "linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsCool.cl", "columnsReconstruct");
	}

	EffectWaveletsCool(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectCL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsCool.cl", "linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsCool.cl", "columnsTransform");
		m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsCool.cl", "linesReconstruct");
		m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsCool.cl", "columnsReconstruct");
	}
protected:
	int ceilToPower2(int x)
	{
		x -= 1;
		int power = 0;
		for(; x; x >>= 1, power++);
		return 1 << power;
	}
	void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult)
	{
		error0(ERR_STRUCT, "DON'T CALL ME BABY!!!");
	}
public:
	Ptr<GLTexture2D> texRes;
	Ptr<GLTexture2D> texTmp1;
	Ptr<GLTexture2D> texTmp2;

	Ptr<GLTexture2D> process (Ptr<GLTexture2D> texColor, Ptr<GLTexture2D> texDepth, Ptr<GLTextureEnvMap> texEnvMap)
	{
		try 
		{
			static bool bInit = false;
			if (! bInit)
			{
				bInit = true;
				texRes = createTexTarget();
				texTmp1 = m_factoryTexRGB->createProduct();
				texTmp2 = m_factoryTexRGB->createProduct();
			}

			glFlush();
			cl::Image2DGL imageColor = getImage2DGL(*m_context, CL_MEM_READ_ONLY, texColor);
			cl::Image2DGL imageDepth;
			if (requireDepth())
				imageDepth = getImage2DGL(*m_context, CL_MEM_READ_ONLY, texDepth);
			cl::Image2DGL imageRes = getImage2DGL(*m_context, CL_MEM_WRITE_ONLY, texRes);
			cl::Image2DGL imageTmp[2] = {
				getImage2DGL(*m_context, CL_MEM_READ_WRITE, texTmp1),
				getImage2DGL(*m_context, CL_MEM_READ_WRITE, texTmp2)};

				std::vector<cl::Memory> vecGLMems;
				vecGLMems.push_back(imageColor);
				if (requireDepth())
					vecGLMems.push_back(imageDepth);
				for (int i = 0; i < 2; i++)
					vecGLMems.push_back(imageTmp[i]);

				vecGLMems.push_back(imageRes);
				m_queue->enqueueAcquireGLObjects(&vecGLMems);

				runKernels(imageColor, imageDepth, texEnvMap, imageRes, imageTmp);

				m_queue->enqueueReleaseGLObjects(&vecGLMems);
				m_queue->flush();

				return texRes;
		} catchCLError;
	}

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

	int getOptCountWorkItems (int resolution)
	{
		int count = resolution/2 + resolution%2 + 2;
		int mask = 32-1;
		count = (count & (~mask)) + ((count&mask) ? 32 : 1);
		return count;
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
	void runKernels(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{	
		ImageSwapper imSwapper (imageColor, imageTmp);
		int2 res (imageResult.getImageInfo<CL_IMAGE_WIDTH>(), imageResult.getImageInfo<CL_IMAGE_HEIGHT>());
		
		// transform:
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



		// reconstruct:
		for (int i = getUsefulCountLvls(res)-1; i >= 0; i--)
		//for (int i = 4; i >= 0; i--)
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
		

		static int start = GetTickCount();
		static int nFrames = -1;
		nFrames++;
		int actual = GetTickCount();
		printf("Time: %f\n", 1.0f/((actual-start)/1000.0f/(float)nFrames));
	}

	Ptr<GLTexture2D> createTexTarget()
	{
		return m_factoryTexRGB->createProduct();
	}

};



class EffectWaveletsTransform: public EffectCL<EffectGL>
{
	cl::Kernel m_kernel;
public:
	EffectWaveletsTransform(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	  EffectCL(factoryRGB, factoryRed, factory)
	  {
		  m_kernel = createKernel(*m_context, "kernels\\wavelets.cl", "transformLines");
	  }

	  EffectWaveletsTransform(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	  EffectCL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	  {
		  m_kernel = createKernel(*m_context, "kernels\\wavelets.cl", "transformLines");
	  }

	  void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult)
	  {
		  m_kernel.setArg(0, imageColor);
		  m_kernel.setArg(1, imageResult);
		  int imageWidth = imageResult.getImageInfo<CL_IMAGE_WIDTH>();
		  int imageHeight = imageResult.getImageInfo<CL_IMAGE_HEIGHT>();
		  int numWorkItemWidth = 16;
		  int numGlobalItemsWidth = (int) ceil((ceil(((double)imageWidth)/2))/(numWorkItemWidth-4))*numWorkItemWidth;
		  int2 numWorkItems(numWorkItemWidth, 1);
		  int2 numGlobalItems(numGlobalItemsWidth, imageHeight);
		  m_kernel.setArg(2, numWorkItemWidth*sizeof(float4), 0);
		  m_queue->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
	  }

	  Ptr<GLTexture2D> createTexTarget()
	  {
		  return m_factoryTexRGB->createProduct();
	  }

};


class EffectWaveletsReconstruct: public EffectCL<EffectGL>
{
	cl::Kernel m_kernel;
public:
	EffectWaveletsReconstruct(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<EffectCLObjectsFactory> factory): 
	  EffectCL(factoryRGB, factoryRed, factory)
	  {
		  m_kernel = createKernel(*m_context, "kernels\\wavelets.cl", "reconstructLines");
	  }

	  EffectWaveletsReconstruct(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	  EffectCL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	  {
		  m_kernel = createKernel(*m_context, "kernels\\wavelets.cl", "reconstructLines");
	  }

	  void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult)
	  {
		  m_kernel.setArg(0, imageColor);
		  m_kernel.setArg(1, imageResult);
		  int imageWidth = imageResult.getImageInfo<CL_IMAGE_WIDTH>();
		  int imageHeight = imageResult.getImageInfo<CL_IMAGE_HEIGHT>();
		  int numWorkItemWidth = 16;
		  int numGlobalItemsWidth = (int) ceil((ceil(((double)imageWidth)/2))/(numWorkItemWidth-2))*numWorkItemWidth;
		  int2 numWorkItems(numWorkItemWidth, 1);
		  int2 numGlobalItems(numGlobalItemsWidth, imageHeight);
		  m_kernel.setArg(2, numWorkItemWidth*sizeof(float4), 0);
		  m_queue->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
	  }

	  Ptr<GLTexture2D> createTexTarget()
	  {
		  return m_factoryTexRGB->createProduct();
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

		Visitor2ReadRGBToSaveMatlabASCII visitor("matlab.txt");
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
		m_pp->m_vecEffects.pushBack(new EffectWaveletsCool(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		m_pp->m_vecEffects.pushBack(new EffectRenderToScreen(0,0,1,1));
		m_pp->m_vecEffects.pushBack(new EffectCopyColorAndDepthMaps(new SharedObjectsFactory(resolution)));
		//m_pp->m_vecEffects.pushBack(new EffectSaveToMatlabASCII("out.txt"));
	}
	void render()
	{
		m_pp->process();
		//sendQuit();
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
