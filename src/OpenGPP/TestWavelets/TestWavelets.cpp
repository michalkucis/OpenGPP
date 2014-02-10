#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv/cv.hpp>
#include <OpenGPP.h>

#include <fstream>

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
	ApplicationWavelets (): Application(250, 250)
	{
	}
	void initData()
	{
		uint2 resolution = uint2(1036,777);
		m_pp = new PostProcessor(new SharedObjectsFactory(resolution));
		m_pp->m_input = new InputLoadFromSingleFileOpenEXR("exrChangeLightIntensity\\img_light1_lamp250_pos0.exr");
		m_pp->m_vecEffects.pushBack(new EffectWaveletsTransform(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		m_pp->m_vecEffects.pushBack(new EffectWaveletsReconstruct(new SharedObjectsFactory(resolution), new EffectCLObjectsFactory));
		m_pp->m_vecEffects.pushBack(new EffectRenderToScreen(0,0,1,1));
		m_pp->m_vecEffects.pushBack(new EffectCopyColorAndDepthMaps(new SharedObjectsFactory(resolution)));
		m_pp->m_vecEffects.pushBack(new EffectSaveToMatlabASCII("out.txt"));
	}
	void render()
	{
		m_pp->process();
		sendQuit();
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
