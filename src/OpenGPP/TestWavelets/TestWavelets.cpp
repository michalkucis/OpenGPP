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


class EffectWaveletsCool: public EffectCL<EffectGL>
{
	cl::Kernel m_kernelLinesTransform;
	cl::Kernel m_kernelColumnsTransform;
	cl::Kernel m_kernelLinesReconstruct;
	cl::Kernel m_kernelColumnsReconstruct;
	cl::Kernel m_kernelCopy;
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
		//m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsCool.cl", "linesReconstruct");
		//m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsCool.cl", "columnsReconstruct");
	}

	EffectWaveletsCool(Ptr<SharedObjectsFactory> sof, Ptr<EffectCLObjectsFactory> factory): 
	EffectCL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), factory)
	{
		m_kernelLinesTransform = createKernel(*m_context, "kernels\\waveletsCool.cl", "linesTransform");
		m_kernelColumnsTransform = createKernel(*m_context, "kernels\\waveletsCool.cl", "columnsTransform");
		//m_kernelLinesReconstruct = createKernel(*m_context, "kernels\\waveletsCool.cl", "linesReconstruct");
		//m_kernelColumnsReconstruct = createKernel(*m_context, "kernels\\waveletsCool.cl", "columnsReconstruct");
	}

	int ceilToPower2(int x)
	{
		x -= 1;
		int power = 0;
		for(; x; x >>= 1, power++);
		return 1 << power;
	}

	Ptr<GLTexture2D> process (Ptr<GLTexture2D> texColor, Ptr<GLTexture2D> texDepth, Ptr<GLTextureEnvMap> texEnvMap)
	{
		try 
		{
			Ptr<GLTexture2D> texRes = createTexTarget();
			Ptr<GLTexture2D> texTmp1 = m_factoryTexRGB->createProduct();
			Ptr<GLTexture2D> texTmp2 = m_factoryTexRGB->createProduct();

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

	void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult)
	{
		error0(ERR_STRUCT, "DON'T CALL ME!!!");
	}
	void runKernels(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult, cl::Image2DGL imageTmp[2])
	{		
		int width = imageResult.getImageInfo<CL_IMAGE_WIDTH>();
		int height = imageResult.getImageInfo<CL_IMAGE_HEIGHT>();

		//width = width/2+width%2;
		//height = height/2+height%2;
		
		// transform:
		int nValidTmp=-1;
		int n = 100;
		while (width > 1 && height > 1)
		{
			if (! n--)
				break;
			if (width > 1)
			{
				if (nValidTmp==-1)
				{
					m_kernelLinesTransform.setArg(0, imageColor);
					m_kernelLinesTransform.setArg(1, imageTmp[0]);
					nValidTmp = 0;
				}
				else if(nValidTmp==0)
				{
					m_kernelLinesTransform.setArg(0, imageTmp[0]);
					m_kernelLinesTransform.setArg(1, imageTmp[1]);
					nValidTmp = 1;
				}
				else if(nValidTmp==1)
				{
					m_kernelLinesTransform.setArg(0, imageTmp[1]);
					m_kernelLinesTransform.setArg(1, imageTmp[0]);
					nValidTmp = 0;
				}
				int imageWidth = width;
				int imageHeight = height;
				int2 imageRes (width, height);
				int numWorkItemWidth = ceilToPower2(std::min(1024,width/2+width%2+2));
				int numGlobalItemsWidth = (int) ceil((ceil(((double)imageWidth)/2))/(numWorkItemWidth-2))*numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, imageHeight);
				m_kernelLinesTransform.setArg(2, numWorkItemWidth*sizeof(float4), 0);
				initKernelConstMemory(imageRes);
				m_kernelLinesTransform.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
			{
				cl::Image2DGL src, dst;
				if (nValidTmp==-1)
				{
					src = imageColor;
					dst = imageTmp[0];
					nValidTmp = 0;
				}
				else if(nValidTmp==0)
				{
					src = imageTmp[0];
					dst = imageTmp[1];
					nValidTmp = 1;
				}
				else if(nValidTmp==1)
				{
					src = imageTmp[1];
					dst = imageTmp[0];
					nValidTmp = 0;
				}
				cl::size_t<3> zero;
				zero[0] = 0;
				zero[1] = 0;
				zero[2] = 0;
				cl::size_t<3> region;
				region[0] = width;
				region[1] = height;
				region[2] = 1;
				m_queue->enqueueCopyImage(src, dst, zero, zero, region);
			}


			if (height > 1)
			{
				if (nValidTmp==-1)
				{
					m_kernelColumnsTransform.setArg(0, imageColor);
					m_kernelColumnsTransform.setArg(1, imageTmp[0]);
					nValidTmp = 0;
				}
				else if(nValidTmp==0)
				{
					m_kernelColumnsTransform.setArg(0, imageTmp[0]);
					m_kernelColumnsTransform.setArg(1, imageTmp[1]);
					nValidTmp = 1;
				}
				else if(nValidTmp==1)
				{
					m_kernelColumnsTransform.setArg(0, imageTmp[1]);
					m_kernelColumnsTransform.setArg(1, imageTmp[0]);
					nValidTmp = 0;
				}
				int imageWidth = width;
				int imageHeight = height;
				int2 imageRes (width, height);

				int numWorkItemHeight = ceilToPower2(std::min(1024,height));
				int numGlobalItemsHeight = (int) ceil((ceil(((double)imageHeight)/2))/(numWorkItemHeight-2))*numWorkItemHeight;
				int2 numWorkItems(1, numWorkItemHeight);
				int2 numGlobalItems(imageWidth, numGlobalItemsHeight);
				m_kernelColumnsTransform.setArg(2, numWorkItemHeight*sizeof(float4), 0);
				initKernelConstMemory(imageRes);
				m_kernelColumnsTransform.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelColumnsTransform, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
			else
			{
				cl::Image2DGL src, dst;
				if (nValidTmp==-1)
				{
					src = imageColor;
					dst = imageTmp[0];
					nValidTmp = 0;
				}
				else if(nValidTmp==0)
				{
					src = imageTmp[0];
					dst = imageTmp[1];
					nValidTmp = 1;
				}
				else if(nValidTmp==1)
				{
					src = imageTmp[1];
					dst = imageTmp[0];
					nValidTmp = 0;
				}
				cl::size_t<3> zero;
				zero[0] = 0;
				zero[1] = 0;
				zero[2] = 0;
				cl::size_t<3> region;
				region[0] = width;
				region[1] = height;
				region[2] = 1;
				m_queue->enqueueCopyImage(src, dst, zero, zero, region);
			}
			height = height / 2 + height % 2;
			width = width / 2 + width % 2;
		}

		cl::Image2DGL src, dst;
		if (nValidTmp==-1)
		{
			src = imageColor;
			dst = imageResult;
			nValidTmp = 0;
		}
		else if(nValidTmp==0)
		{
			src = imageTmp[0];
			dst = imageResult;
			nValidTmp = 1;
		}
		else if(nValidTmp==1)
		{
			src = imageTmp[1];
			dst = imageResult;
			nValidTmp = 0;
		}
		width = imageResult.getImageInfo<CL_IMAGE_WIDTH>();
		height = imageResult.getImageInfo<CL_IMAGE_HEIGHT>();
		cl::size_t<3> zero;
		zero[0] = 0;
		zero[1] = 0;
		zero[2] = 0;
		cl::size_t<3> region;
		region[0] = width;
		region[1] = height;
		region[2] = 1;
		m_queue->enqueueCopyImage(src, dst, zero, zero, region);
		
/*		Vector<int> vecUsedWidth;
		Vector<int> vecUsedHeight;
		width = imageResult.getImageInfo<CL_IMAGE_WIDTH>();
		int height = imageResult.getImageInfo<CL_IMAGE_HEIGHT>();
		while (width > 1 || height > 1)
		{
			height = height / 2 + height % 2;
			width = width / 2 + width % 2;
			vecUsedWidth.pushBack(width);
			vecUsedHeight.pushBack(height);
		}

		m_kernelLinesReconstruct.setArg(0, imageColor);
		m_kernelLinesReconstruct.setArg(1, imageResult);

		// reconstruct:
		for (int i = vecUsedWidth.getSize()-1; i >= 0; i--)
		{
			int width = vecUsedWidth[i];
			int height = vecUsedHeight[i];

			if (width > 1)
			{
				int imageWidth = width;
				int imageHeight = imageResult.getImageInfo<CL_IMAGE_HEIGHT>();
				int numWorkItemWidth = ceilToPower2(std::min(1024,width));
				int numGlobalItemsWidth = (int) ceil((ceil(((double)imageWidth)/2))/(numWorkItemWidth-2))*numWorkItemWidth;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numGlobalItemsWidth, imageHeight);
				m_kernelLinesReconstruct.setArg(2, numWorkItemWidth*sizeof(float4), 0);
				int2 imageRes (width, imageResult.getImageInfo<CL_IMAGE_HEIGHT>());
				initKernelConstMemory(imageRes);
				m_kernelLinesReconstruct.setArg(3, m_constMemory);
				m_queue->enqueueNDRangeKernel(m_kernelLinesReconstruct, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			}
		}
*/	
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
