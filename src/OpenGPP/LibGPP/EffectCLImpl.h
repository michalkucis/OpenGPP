#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include "EffectCL.h"
#include "EffectGLImpl.h"
#include "..\LibCLFFT\clFFT.h"

class ClBuffer2DFloat
{
	cl::Buffer m_buffer; 
	uint2 m_size;
	Ptr<cl::Context> m_context;
	Ptr<cl::CommandQueue> m_queue;

	cl::Kernel m_kernelZeroes;
	cl::Kernel m_kernelCopyToImage2DGL;
	cl::Kernel m_kernelCopyFromImage2DGL;
	cl::Kernel m_kernelDrawSolidRect;

public:
	ClBuffer2DFloat(uint2 resolution, Ptr<EffectCLObjectsFactory> factory, cl_mem_flags flags)
	{
		try
		{	
			m_context = factory->getCLGLContext();
			m_queue = factory->getCLCommandQueue();

			m_buffer = cl::Buffer(*m_context, flags | CL_MEM_READ_WRITE, sizeof(float)*resolution.getArea());
			m_size = resolution;

			m_kernelZeroes = createKernel(*(factory->getCLGLContext()),
				"kernels\\kernels.cl", "zeroesMemory");
			m_kernelCopyFromImage2DGL = createKernel(*(factory->getCLGLContext()), 
				"kernels\\kernels.cl", "copyFromImage2DGL");
			m_kernelCopyToImage2DGL = createKernel(*(factory->getCLGLContext()), 
				"kernels\\kernels.cl", "copyToImage2DGL");
			m_kernelDrawSolidRect = createKernel(*(factory->getCLGLContext()), 
				"kernels\\kernels.cl", "drawSolidRect");

		} catchCLError;
	}

	uint2 getSize()
	{
		return m_size;
	}

	operator cl::Buffer()
	{
		return m_buffer;
	}

	// requires explicit call of glFlush before zeroesMemory and clFlush after zeroesMemory (before any openGL call) 
	void zeroesMemory()
	{
		m_kernelZeroes.setArg(0,m_buffer);
		m_queue->enqueueNDRangeKernel(
			m_kernelZeroes,
			cl::NDRange(0,0),
			cl::NDRange(m_size.x, m_size.y));
	}

	void copyFromImage2DGL(cl::Image2DGL image, float mult = 1.0f)
	{		
		uint2 imageSize (image.getImageInfo<CL_IMAGE_WIDTH>(), 
			image.getImageInfo<CL_IMAGE_HEIGHT>());
		uint2 bufferSize (getSize());
		uint2 size = uint2::getMin(imageSize, bufferSize);

		m_kernelCopyFromImage2DGL.setArg(0, m_buffer);
		m_kernelCopyFromImage2DGL.setArg(1, sizeof(size), &size);
		m_kernelCopyFromImage2DGL.setArg(2, image);
		m_kernelCopyFromImage2DGL.setArg(3, mult);

		m_queue->enqueueNDRangeKernel(
			m_kernelCopyFromImage2DGL,
			cl::NDRange(0,0),
			cl::NDRange(size.x, size.y));
	}

	void copyToImage2DGL(cl::Image2DGL image, float mult = 1.0f)
	{		
		uint2 imageSize (image.getImageInfo<CL_IMAGE_WIDTH>(), 
			image.getImageInfo<CL_IMAGE_HEIGHT>());
		uint2 bufferSize (getSize());
		uint2 size = uint2::getMin(imageSize, bufferSize);

		m_kernelCopyToImage2DGL.setArg(0, m_buffer);
		m_kernelCopyToImage2DGL.setArg(1, size);
		m_kernelCopyToImage2DGL.setArg(2, image);
		m_kernelCopyToImage2DGL.setArg(3, mult);

		m_queue->enqueueNDRangeKernel(
			m_kernelCopyToImage2DGL,
			cl::NDRange(0,0),
			cl::NDRange(size.x, size.y));
	}

	void drawSolidRect(int2 begin, int2 end, float value)
	{
		m_kernelDrawSolidRect.setArg(0,m_buffer);
		m_kernelDrawSolidRect.setArg(1,begin);
		m_kernelDrawSolidRect.setArg(2,end);
		m_kernelDrawSolidRect.setArg(3,value);

		m_queue->enqueueNDRangeKernel(
			m_kernelDrawSolidRect,
			cl::NDRange(0,0),
			cl::NDRange(m_size.x, m_size.y));
	}
};

typedef void* clFFT_Plan;
enum clFFT_Dimension;
struct clFFT_Dim3;
enum clFFT_Direction;

class ClFFT
{
	clFFT_Plan* m_fftPlan;
	Ptr<cl::Context> m_context;
	Ptr<cl::CommandQueue> m_queue;
	uint3 m_size;

	void init (Ptr<cl::Context> context, Ptr<cl::CommandQueue> queue, clFFT_Dimension& dim, clFFT_Dim3& dim3);
public:
	ClFFT(Ptr<cl::Context> context, Ptr<cl::CommandQueue> queue, uint dim);
	ClFFT(Ptr<cl::Context> context, Ptr<cl::CommandQueue> queue, uint2 dim);
	ClFFT(Ptr<cl::Context> context, Ptr<cl::CommandQueue> queue, uint3 dim);
	~ClFFT();
	void perform(cl::Buffer real, cl::Buffer im, clFFT_Direction& dir);
};

class ClMultiplyArrayComplex
{
	cl::Kernel m_kernel;
	Ptr<cl::CommandQueue> m_queue;
public:
	ClMultiplyArrayComplex (Ptr<EffectCLObjectsFactory> factory)
	{
		m_kernel = createKernel(*(factory->getCLGLContext()), 
			"kernels\\kernels.cl", "multiplyArrayComplex");
		m_queue = factory->getCLCommandQueue();
	}
	void multiply(cl::Buffer inout_re1, cl::Buffer inout_im1, cl::Buffer in_re2, cl::Buffer in_im2, int2 size)
	{
		m_kernel.setArg(0,inout_re1);
		m_kernel.setArg(1,inout_im1);
		m_kernel.setArg(2,in_re2);
		m_kernel.setArg(3,in_im2);

		m_queue->enqueueNDRangeKernel(
			m_kernel,
			cl::NDRange(0,0),
			cl::NDRange(size.x, size.y));
	}
};


struct ComputeDoFCoCStruct
{
	float inM, b1M, s;
};

class EffectCLComputeCoC: public EffectCL<EffectDOF>
{
	float m_multOut;
	FuncFtoFCompCoC m_funcComputeCoC;
	ComputeDoFCoCStruct m_memoryComputeCoC;
	cl::Buffer m_bufferComputeCoC;
	cl::Kernel m_kernel;
public:
	EffectCLComputeCoC(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, 
			float aspectRatio, Ptr<EffectCLObjectsFactory> factory, float multOut = 1.0f): 
	  EffectCL(factoryRGB, factoryRed, factory),
		  m_bufferComputeCoC(*m_context, CL_MEM_READ_ONLY, sizeof(ComputeDoFCoCStruct)),
		  m_funcComputeCoC(1.77f, 3, 48, 42, 0.01f),
		  m_multOut(multOut)
	  {
		  m_kernel = createKernel(*m_context, "kernels\\kernels.cl", "kernelComputeCoC");
	  }

	  void setFuncCoC (FuncFtoFCompCoC& func)
	  {
		  m_funcComputeCoC = func;
	  }

	  Ptr<GLTexture2D> createTexTarget()
	  {
		  return m_factoryTexRed->createProduct();
	  }

	  bool requireDepth()
	  {return true;}

	  void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, cl::Image2DGL& imageResult)
	  {
		  m_memoryComputeCoC.inM = m_funcComputeCoC.getInMult();
		  m_memoryComputeCoC.b1M = m_funcComputeCoC.getB1() * m_funcComputeCoC.getM();
		  m_memoryComputeCoC.s = m_funcComputeCoC.getS();
		  m_queue->enqueueWriteBuffer(m_bufferComputeCoC, CL_FALSE, 0, sizeof(ComputeDoFCoCStruct), &m_memoryComputeCoC);

		  m_kernel.setArg(0, imageColor);
		  m_kernel.setArg(1, imageDepth);
		  m_kernel.setArg(2, imageResult);
		  m_kernel.setArg(3, m_bufferComputeCoC);
		  m_kernel.setArg(4, m_multOut);
		  int width = imageResult.getImageInfo<CL_IMAGE_WIDTH>();
		  int height = imageResult.getImageInfo<CL_IMAGE_HEIGHT>();
		  m_queue->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(width,height));
	  }
};

struct ComputeDoFDiffusionStruct
{
	int offsets[16];
	int numActiveThreads[16];
	int numIter;
};





#define MAX_TEXTURE_SIZE (2048)
#define MAX_OFFSET (1024*8)
#define COUNT_GLOBAL_CHANNELS (6)
#define COUNT_GLOBAL_ITEMS (MAX_TEXTURE_SIZE*MAX_OFFSET*COUNT_GLOBAL_CHANNELS)

struct LineForABCRGB_t
{
	float arrA[MAX_OFFSET];
	float arrB[MAX_OFFSET];
	float arrC[MAX_OFFSET];
	float arrRed[MAX_OFFSET];
	float arrGreen[MAX_OFFSET];
	float arrBlue[MAX_OFFSET];
};

class EffectCLDOFDistributionHorizontal: public EffectCL<EffectDOF>
{
	FuncFtoFCompCoC m_funcComputeCoC;
	ComputeDoFCoCStruct m_memoryComputeCoC;
	cl::Buffer m_bufferComputeCoC;

	ComputeDoFDiffusionStruct m_memoryComputeDoFRow;
	cl::Buffer m_bufferComputeDofRow;

	//ComputeDoFDiffusionStruct m_memoryComputeDoFColumn;
	//cl::Buffer m_bufferComputeDofColumn;

	Ptr<cl::Buffer> m_bufferABCRGB;

	cl::Kernel m_kernel;

public:
	EffectCLDOFDistributionHorizontal(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed,
		float aspectRatio, Ptr<EffectCLObjectsFactory> factory, float multOut = 1.0f, Ptr<cl::Buffer> bufferABCRGB = NULL): 
		EffectCL(factoryRGB, factoryRed, factory),
		  m_bufferComputeCoC(*m_context, CL_MEM_READ_ONLY, sizeof(ComputeDoFCoCStruct)),
		  m_bufferComputeDofRow(*m_context, CL_MEM_READ_ONLY, sizeof(ComputeDoFDiffusionStruct)),
		  //m_bufferComputeDofColumn(m_context, CL_MEM_READ_ONLY, sizeof(ComputeDoFDiffusionStruct)),
		  m_funcComputeCoC(1.77f, 3, 48, 42, 0.01f)
	  {
		  m_kernel = createKernel (*m_context, "kernels\\dofDiffusion.cl", "diffusionHorizontal");
		  if (bufferABCRGB.isNull())
			  m_bufferABCRGB = new cl::Buffer(*m_context, CL_MEM_READ_WRITE, COUNT_GLOBAL_ITEMS*sizeof(float));
		  else
			  m_bufferABCRGB = bufferABCRGB;
	  }

	  Ptr<cl::Buffer> getBufferABCRGB()
	  {
		  return m_bufferABCRGB;
	  }

	  void setFuncCoC (FuncFtoFCompCoC& func)
	  {
		  m_funcComputeCoC = func;
	  }

	  Ptr<GLTexture2D> createTexTarget()
	  {
		  return m_factoryTexRGB->createProduct();
	  }

	  bool requireDepth()
	  {return true;}

protected:
	void getVecSizes( Vector<int>& out_vecJ, int N )
	{
		out_vecJ.clear();
		out_vecJ.pushBack(N);

		int maxL = round(log2((float)N+1));
		for(int L = 1; L < maxL; L++)
		{
			int j = round(powf(2, (float)-L)*(N+1))-1;
			out_vecJ.pushBack(j);
		}
	}

	void getVecOffsets( Vector<int>& out_vecOffsets, int N, int alignment ) 
	{
		Vector<int> vecMaxJ;
		getVecSizes(vecMaxJ, N);

		int extraAppend = 2;

		out_vecOffsets.clear();
		int actualOffset = 0;//N + (alignment*N-N-extraAppend) % alignment;
		for(uint i = 0; i < vecMaxJ.getSize(); i++)
		{
			out_vecOffsets.pushBack(actualOffset);
			int offset = vecMaxJ[i];

			int smallestAcceptableOffset = offset + extraAppend;
			int extraAlignment = (alignment*N-smallestAcceptableOffset) % alignment;
			int bestOffset = smallestAcceptableOffset + extraAlignment;
			actualOffset += bestOffset;
		}
	}

	ComputeDoFDiffusionStruct getComputeDoFDiffusionStruct( int N, int alignment )
	{
		ComputeDoFDiffusionStruct compute;
		Vector<int> vecSizes;
		getVecSizes(vecSizes, N);
		Vector<int> vecOffsets;
		getVecOffsets(vecOffsets, N, alignment);
		if (vecSizes.getSize() > 20)
			error0(ERR_STRUCT, "Array is too small");

		compute.numIter = vecSizes.getSize();
		for (int i = 0; i < compute.numIter; i++)
		{
			compute.offsets[i] = vecOffsets[i];
			compute.numActiveThreads[i] = vecSizes[i];
		}
		return compute;
	}

public:
	void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult)
	{
		int widthColor = imageColor.getImageInfo<CL_IMAGE_WIDTH>();
		int heightColor = imageColor.getImageInfo<CL_IMAGE_HEIGHT>();

		int widthDepth = imageDepth.getImageInfo<CL_IMAGE_WIDTH>();
		int heightDepth = imageDepth.getImageInfo<CL_IMAGE_HEIGHT>();

		int width = imageResult.getImageInfo<CL_IMAGE_WIDTH>();
		int height = imageResult.getImageInfo<CL_IMAGE_HEIGHT>();

		if (widthColor != width 
			|| heightColor != height
			|| widthDepth != width
			|| heightDepth != height)
			error0(ERR_STRUCT, "Input images and output image have not the same sizes. This effect requires it.");

		m_memoryComputeCoC.inM = m_funcComputeCoC.getInMult();
		m_memoryComputeCoC.b1M = m_funcComputeCoC.getB1() * m_funcComputeCoC.getM();
		m_memoryComputeCoC.s = m_funcComputeCoC.getS();
		m_queue->enqueueWriteBuffer(m_bufferComputeCoC, CL_FALSE, 0, sizeof(ComputeDoFCoCStruct), &m_memoryComputeCoC);

		//int maxSize(max(width, height));
		int numWorkItemsPerGroup = width/2 + width%2;
		m_memoryComputeDoFRow = getComputeDoFDiffusionStruct(width, 32);
		m_queue->enqueueWriteBuffer(m_bufferComputeDofRow, CL_FALSE, 0, sizeof(ComputeDoFDiffusionStruct), &m_memoryComputeDoFRow);
		//m_memoryComputeDoFColumn = getComputeDoFDiffusionStruct(height, 32);
		//m_queue.enqueueWriteBuffer(m_bufferComputeDofColumn, CL_FALSE, 0, sizeof(ComputeDoFDiffusionStruct), &m_memoryComputeDoFColumn);

		m_kernel.setArg(0, imageColor);
		m_kernel.setArg(1, imageDepth);
		m_kernel.setArg(2, imageResult);
		m_kernel.setArg(3, m_bufferComputeCoC);
		m_kernel.setArg(4, m_bufferComputeDofRow);
		//kernel.setArg(5, m_bufferComputeDofColumn);
		m_kernel.setArg(5, *(m_bufferABCRGB.get()));

		m_queue->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), 
			cl::NDRange(numWorkItemsPerGroup, height), 
			cl::NDRange(numWorkItemsPerGroup, 1));

		////////////////////////////
		// TODO:Debug
		//m_queue.enqueueReadBuffer(m_bufferABCRGB, CL_TRUE, 0, sizeof(m_memoryABCRGB), &m_memoryABCRGB);
		////////////////////////////
	}
};

class EffectCLDOFDistributionVertical: public EffectCL<EffectDOF>
{
	FuncFtoFCompCoC m_funcComputeCoC;
	ComputeDoFCoCStruct m_memoryComputeCoC;
	cl::Buffer m_bufferComputeCoC;

	ComputeDoFDiffusionStruct m_memoryComputeDoFColumn;
	cl::Buffer m_bufferComputeDofColumn;

	Ptr<cl::Buffer> m_bufferABCRGB;

	cl::Kernel m_kernel;

public:
	EffectCLDOFDistributionVertical(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, 
			float aspectRatio, Ptr<EffectCLObjectsFactory> factory, float multOut = 1.0f, Ptr<cl::Buffer> bufferABCRGB = NULL): 
	  EffectCL(factoryRGB, factoryRed, factory),
		  m_bufferComputeCoC(*m_context, CL_MEM_READ_ONLY, sizeof(ComputeDoFCoCStruct)),
		  //m_bufferComputeDofRow(m_context, CL_MEM_READ_ONLY, sizeof(ComputeDoFDiffusionStruct)),
		  m_bufferComputeDofColumn(*m_context, CL_MEM_READ_ONLY, sizeof(ComputeDoFDiffusionStruct)),
		  m_funcComputeCoC(1.77f, 3, 48, 42, 0.01f)
	  {
		  m_kernel = createKernel(*m_context, "kernels\\dofDiffusion.cl", "diffusionVertical");
		  if (bufferABCRGB.isNull())
			  m_bufferABCRGB = new cl::Buffer(*m_context, CL_MEM_READ_WRITE, COUNT_GLOBAL_ITEMS*sizeof(float));
		  else
			  m_bufferABCRGB = bufferABCRGB;
	  }

	  Ptr<cl::Buffer> getBufferABCRGB()
	  {
		  return m_bufferABCRGB;
	  }

	  void setFuncCoC (FuncFtoFCompCoC& func)
	  {
		  m_funcComputeCoC = func;
	  }

	  Ptr<GLTexture2D> createTexTarget()
	  {
		  return m_factoryTexRGB->createProduct();
	  }

	  bool requireDepth()
	  {return true;}

protected:
	void getVecSizes( Vector<int>& out_vecJ, int N )
	{
		out_vecJ.clear();
		out_vecJ.pushBack(N);

		int maxL = round(log2((float)N+1));
		for(int L = 1; L < maxL; L++)
		{
			int j = round(powf(2, (float)-L)*(N+1))-1;
			out_vecJ.pushBack(j);
		}
	}

	void getVecOffsets( Vector<int>& out_vecOffsets, int N, int alignment ) 
	{
		Vector<int> vecMaxJ;
		getVecSizes(vecMaxJ, N);

		int extraAppend = 2;

		out_vecOffsets.clear();
		int actualOffset = 0;//N + (alignment*N-N-extraAppend) % alignment;
		for(uint i = 0; i < vecMaxJ.getSize(); i++)
		{
			out_vecOffsets.pushBack(actualOffset);
			int offset = vecMaxJ[i];

			int smallestAcceptableOffset = offset + extraAppend;
			int extraAlignment = (alignment*N-smallestAcceptableOffset) % alignment;
			int bestOffset = smallestAcceptableOffset + extraAlignment;
			actualOffset += bestOffset;
		}
	}

	ComputeDoFDiffusionStruct getComputeDoFDiffusionStruct( int N, int alignment )
	{
		ComputeDoFDiffusionStruct compute;
		Vector<int> vecSizes;
		getVecSizes(vecSizes, N);
		Vector<int> vecOffsets;
		getVecOffsets(vecOffsets, N, alignment);
		if (vecSizes.getSize() > 20)
			error0(ERR_STRUCT, "Array is too small");

		compute.numIter = vecSizes.getSize();
		for (int i = 0; i < compute.numIter; i++)
		{
			compute.offsets[i] = vecOffsets[i];
			compute.numActiveThreads[i] = vecSizes[i];
		}
		return compute;
	}

public:
	void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult)
	{
		int widthColor = imageColor.getImageInfo<CL_IMAGE_WIDTH>();
		int heightColor = imageColor.getImageInfo<CL_IMAGE_HEIGHT>();

		int widthDepth = imageDepth.getImageInfo<CL_IMAGE_WIDTH>();
		int heightDepth = imageDepth.getImageInfo<CL_IMAGE_HEIGHT>();

		int width = imageResult.getImageInfo<CL_IMAGE_WIDTH>();
		int height = imageResult.getImageInfo<CL_IMAGE_HEIGHT>();

		if (widthColor != width 
			|| heightColor != height
			|| widthDepth != width
			|| heightDepth != height)
			error0(ERR_STRUCT, "Input images and output image have not the same sizes. This effect requires it.");

		m_memoryComputeCoC.inM = m_funcComputeCoC.getInMult();
		m_memoryComputeCoC.b1M = m_funcComputeCoC.getB1() * m_funcComputeCoC.getM();
		m_memoryComputeCoC.s = m_funcComputeCoC.getS();
		m_queue->enqueueWriteBuffer(m_bufferComputeCoC, CL_FALSE, 0, sizeof(ComputeDoFCoCStruct), &m_memoryComputeCoC);

		//int maxSize(max(width, height));
		int numWorkItemsPerGroup = height/2 + height%2;
		//m_memoryComputeDoFRow = getComputeDoFDiffusionStruct(width, 32);
		//m_queue.enqueueWriteBuffer(m_bufferComputeDofRow, CL_FALSE, 0, sizeof(ComputeDoFDiffusionStruct), &m_memoryComputeDoFRow);
		m_memoryComputeDoFColumn = getComputeDoFDiffusionStruct(height, 32);
		m_queue->enqueueWriteBuffer(m_bufferComputeDofColumn, CL_FALSE, 0, sizeof(ComputeDoFDiffusionStruct), &m_memoryComputeDoFColumn);

		m_kernel.setArg(0, imageColor);
		m_kernel.setArg(1, imageDepth);
		m_kernel.setArg(2, imageResult);
		m_kernel.setArg(3, m_bufferComputeCoC);
		//kernel.setArg(4, m_bufferComputeDofRow);
		m_kernel.setArg(4, m_bufferComputeDofColumn);
		m_kernel.setArg(5, *(m_bufferABCRGB.get()));

		m_queue->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), 
			cl::NDRange(numWorkItemsPerGroup, width), 
			cl::NDRange(numWorkItemsPerGroup, 1));

		////////////////////////////
		// TODO:Debug
		//m_queue.enqueueReadBuffer(m_bufferABCRGB, CL_TRUE, 0, sizeof(m_memoryABCRGB), &m_memoryABCRGB);
		////////////////////////////
	}
};

class EffectCLDOFDistribution: public EffectDOF
{
	Ptr<EffectCLDOFDistributionVertical> m_effectVertical;
	Ptr<EffectCLDOFDistributionHorizontal> m_effectHorizontal;
public:
	EffectCLDOFDistribution(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, 
		float aspectRatio, Ptr<EffectCLObjectsFactory> clFactory = NULL):
		EffectDOF(factoryRGB, factoryRed)
	{
		if (clFactory.isNull())
			clFactory = new EffectCLObjectsFactory;
		m_effectVertical = new EffectCLDOFDistributionVertical(factoryRGB, factoryRed, aspectRatio, clFactory);
		m_effectHorizontal = new EffectCLDOFDistributionHorizontal(factoryRGB, factoryRed, aspectRatio, clFactory, 1.0f, m_effectVertical->getBufferABCRGB());
	}
	Ptr<GLTexture2D> process (Ptr<GLTexture2D> color, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
	{
		Ptr<GLTexture2D> tex = m_effectVertical->process(color, depth, envMap);
		return m_effectHorizontal->process(tex,depth, envMap);
	}
	void setFuncCoC (FuncFtoFCompCoC& func)
	{
		m_effectVertical->setFuncCoC(func);
		m_effectHorizontal->setFuncCoC(func);
	}
};


template<int MAT_WIDTH, int MAT_HEIGHT>
class EffectShowStar: public EffectCL<EffectGL>
{
	Ptr<cl::CommandQueue> m_queue;
	ClBuffer2DFloat m_bufferRe;
	ClBuffer2DFloat m_bufferIm;
	ClBuffer2DFloat m_kernelRe;
	ClBuffer2DFloat m_kernelIm;
	ClMultiplyArrayComplex m_mult;
	ClFFT m_fft;
	cl::Kernel m_kernelStar;
public:
	EffectShowStar(Ptr<EffectCLObjectsFactory> factory): 
	  EffectCL<EffectGL> (factory),
		  m_bufferRe(uint2(MAT_WIDTH, MAT_HEIGHT), factory, CL_MEM_READ_WRITE),
		  m_bufferIm(uint2(MAT_WIDTH, MAT_HEIGHT), factory, CL_MEM_READ_WRITE),
		  m_kernelRe(uint2(MAT_WIDTH, MAT_HEIGHT), factory, CL_MEM_READ_WRITE),
		  m_kernelIm(uint2(MAT_WIDTH, MAT_HEIGHT), factory, CL_MEM_READ_WRITE),
		  m_fft(factory->getCLGLContext(), factory->getCLCommandQueue(), uint2(MAT_WIDTH,MAT_HEIGHT)),
		  m_mult(factory)
	  {
		  m_queue = factory->getCLCommandQueue();
		  m_kernelStar = createKernel(*m_context, "kernels\\kernels.cl", "star");
	  }
protected:
	void initStar(ClBuffer2DFloat& buffer)
	{
		uint2 bufferSize = buffer.getSize();
		uint2 numWorkItems = bufferSize;

		m_kernelStar.setArg(0, (cl::Buffer)buffer);
		m_kernelStar.setArg(1, bufferSize);
		float mult = 1024.0f*1024 / buffer.getSize().getArea();
		m_kernelStar.setArg(2, mult);
		m_queue->enqueueNDRangeKernel(
			m_kernelStar,
			cl::NDRange(0,0),
			cl::NDRange(numWorkItems.x, numWorkItems.y));
	}
public:
	void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, cl::Image2DGL& imageResult)
	{
		int radius = 1;
		m_bufferRe.drawSolidRect(int2(1024,1024)/4, int2(1024,1024)/4+int2(radius), 1.0f/radius/radius);
		m_bufferIm.zeroesMemory();
		m_fft.perform(m_bufferRe, m_bufferIm, clFFT_Forward);
		initStar(m_kernelRe);
		m_kernelIm.zeroesMemory();
		m_fft.perform(m_kernelRe, m_kernelIm, clFFT_Forward);
		m_mult.multiply(m_bufferRe, m_bufferIm, m_kernelRe, m_kernelIm, int2(MAT_WIDTH, MAT_HEIGHT));

		m_fft.perform(m_bufferRe, m_bufferIm, clFFT_Inverse);
		m_bufferRe.copyToImage2DGL(imageResult, 1.0f/MAT_WIDTH/MAT_HEIGHT);
	}
	Ptr<GLTexture2D> createTexTarget()
	{
		return m_factoryTexRGB->createProduct();
	}
};

class EffectLensFlareStarFromSimple: public EffectCL<EffectGL>
{
protected:
	bool m_dynamic;
	int m_height, m_width;
	Ptr<cl::CommandQueue> m_queue;
	Ptr<ClBuffer2DFloat> m_bufferRe;
	Ptr<ClBuffer2DFloat> m_bufferIm;
	Ptr<ClBuffer2DFloat> m_kernelRe;
	Ptr<ClBuffer2DFloat> m_kernelIm;
	Ptr<EffectCLObjectsFactory> m_factoryCLObjects;
	ClMultiplyArrayComplex m_mult;
	ClFFT m_fft;
	cl::Kernel m_kernelStar;
	cl::Kernel m_kernelFillBuffer;
	cl::Kernel m_kernelFillImage;
	float m_lensFlareMult;
public:
	EffectLensFlareStarFromSimple(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed,
		int width, int height, bool dynamic, Ptr<EffectCLObjectsFactory> factory): 
		EffectCL<EffectGL> (factoryRGB, factoryRed, factory),
			m_fft(factory->getCLGLContext(), factory->getCLCommandQueue(), uint2(width,height)),
			m_mult(factory),
			m_lensFlareMult(1)
	{
		m_width = width;
		m_height = height;
		m_dynamic = dynamic;
		m_queue = factory->getCLCommandQueue();
		m_kernelStar = createKernel(*m_context, "kernels\\kernels.cl", "star");
		m_factoryCLObjects = factory;
		if(!m_dynamic)
		{  
			allocBuffers(factory);
			initStar(m_kernelRe);
			m_kernelIm->zeroesMemory();
			clFFT_Direction dir = clFFT_Forward;
			m_fft.perform(*m_kernelRe, *m_kernelIm, dir);
		}
		m_kernelFillBuffer = createKernel(*m_context, "kernels\\kernels.cl", "fillBufferFromImage");
		m_kernelFillImage = createKernel(*m_context, "kernels\\kernels.cl", "fillImageFromBufferAndImage");
	}

	void setLensFlareMult(float mult)
	{
		m_lensFlareMult = mult;
	}

protected:
	void allocBuffers (Ptr<EffectCLObjectsFactory> factory)
	{
		m_bufferRe = new ClBuffer2DFloat(uint2(m_width, m_height), factory, CL_MEM_READ_WRITE);
		m_bufferIm = new ClBuffer2DFloat(uint2(m_width, m_height), factory, CL_MEM_READ_WRITE);
		m_kernelRe = new ClBuffer2DFloat(uint2(m_width, m_height), factory, CL_MEM_READ_WRITE);
		m_kernelIm = new ClBuffer2DFloat(uint2(m_width, m_height), factory, CL_MEM_READ_WRITE);
	}

	void freeBuffers()
	{
		m_bufferRe = NULL;
		m_bufferIm = NULL;
		m_kernelRe = NULL;
		m_kernelIm = NULL;
	}

	void initStar(Ptr<ClBuffer2DFloat> buffer)
	{
		float starMult = 0.0001f;

		buffer->zeroesMemory();
		buffer->drawSolidRect(int2(0,0),int2(0,0),1.0f);
		uint2 bufferSize = buffer->getSize();
		uint2 numWorkItems = bufferSize;

		m_kernelStar.setArg(0, (cl::Buffer)*buffer);
		m_kernelStar.setArg(1, bufferSize);
		float mult = 1024.0f*1024 / buffer->getSize().getArea();
		m_kernelStar.setArg(2, mult * starMult);
		m_queue->enqueueNDRangeKernel(
			m_kernelStar,
			cl::NDRange(0,0),
			cl::NDRange(numWorkItems.x, numWorkItems.y));
	}

	void simpleFillBufferFromImage(cl::Image2DGL imageColor, Ptr<ClBuffer2DFloat> bufferRe)
	{
		m_bufferRe->zeroesMemory();

		int width = imageColor.getImageInfo<CL_IMAGE_WIDTH>();
		int height = imageColor.getImageInfo<CL_IMAGE_HEIGHT>();

		uint2 imageResolution (width, height);
		float2 normRes = imageResolution.get2<float2>();
		normRes /= (float) imageResolution.getMaxValue();
		float2 halfSize (m_bufferRe->getSize().get2<float2>()/2);
		uint2 numWorkItems = (halfSize * normRes).get2<uint2>();

		m_kernelFillBuffer.setArg(0, (cl::Buffer)*bufferRe);
		m_kernelFillBuffer.setArg(1, m_bufferRe->getSize());
		m_kernelFillBuffer.setArg(2, imageColor);
		m_queue->enqueueNDRangeKernel(
			m_kernelFillBuffer,
			cl::NDRange(0,0),
			cl::NDRange(numWorkItems.x, numWorkItems.y));
	}
	
	void fillImageFromBufferAndImage(cl::Image2DGL imageRes, cl::Image2DGL imageColor, Ptr<ClBuffer2DFloat> bufferRe, float resMult)
	{			
		int width = imageRes.getImageInfo<CL_IMAGE_WIDTH>();
		int height = imageRes.getImageInfo<CL_IMAGE_HEIGHT>();
		uint2 numWorkItems (width, height);

		uint2 imageResolution (width, height);
		float2 normRes = imageResolution.get2<float2>();
		normRes /= (float) imageResolution.getMaxValue();
		float2 halfSize (bufferRe->getSize().get2<float2>()/2);
		uint2 effectiveBufferSize = (halfSize * normRes).get2<uint2>();

		m_kernelFillImage.setArg(0, imageRes);
		m_kernelFillImage.setArg(1, imageColor);
		m_kernelFillImage.setArg(2, (cl::Buffer)*bufferRe);
		m_kernelFillImage.setArg(3, bufferRe->getSize());
		m_kernelFillImage.setArg(4, effectiveBufferSize);
		m_kernelFillImage.setArg(5, resMult);
		m_queue->enqueueNDRangeKernel(
			m_kernelFillImage,
			cl::NDRange(0,0),
			cl::NDRange(numWorkItems.x, numWorkItems.y));
	}

public:
	virtual void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult)
	{
		if(m_dynamic)
		{  
			allocBuffers(m_factoryCLObjects);
			initStar(m_kernelRe);
			m_kernelIm->zeroesMemory();
			clFFT_Direction dir = clFFT_Forward;
			m_fft.perform(*m_kernelRe, *m_kernelIm, dir);
		}
		
		// fill buffer with grayscale 
		simpleFillBufferFromImage(imageColor, m_bufferRe);

		//////////////////////////////////////////////////////////////////////////////
		float resMult = 1.0f / (m_bufferRe->getSize().getArea());
		{
			m_bufferIm->zeroesMemory();
			clFFT_Direction dirForward = clFFT_Forward;
			m_fft.perform(*m_bufferRe, *m_bufferIm, dirForward);
			m_mult.multiply(*m_bufferRe, *m_bufferIm, *m_kernelRe, *m_kernelIm, int2(m_width, m_height));
			clFFT_Direction dirInverse = clFFT_Inverse;
			m_fft.perform(*m_bufferRe, *m_bufferIm, dirInverse);	
		}
		resMult *= m_lensFlareMult;
		//////////////////////////////////////////////////////////////////////////////
		fillImageFromBufferAndImage(imageResult, imageColor, m_bufferRe, resMult);

		if(m_dynamic)
			freeBuffers();
	}

	Ptr<GLTexture2D> createTexTarget()
	{
		return m_factoryTexRGB->createProduct();
	}
};

class EffectLensFlareStarFromEnvMap: public EffectLensFlareStarFromSimple
{
protected:
	GLProgram m_programVignetting;
	Ptr<GLTexture2D> m_texVignetting;

	Ptr<GLTexture2D> m_texEnvMap;
	cl::Image2DGL m_imageEnvMap;

	cl::Kernel m_kernelFillImage2;
	float2 m_FoV;

	void drawEnvMap (Ptr<GLTexture2D> outEnvMap, Ptr<GLTextureEnvMap> inEnvMap, float xFov, float yFov)
	{
		float x[4];
		for (int i = 0; i < 4; i++)
			x[i] = i / 3.0f;

		float y[3];
		for (int i = 0; i < 3; i++)
			y[i] = i / 2.0f;

		Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
		framebufferTarget->setColor(outEnvMap);
		framebufferTarget->bind();

		glActiveTexture(GL_TEXTURE0);
		inEnvMap->bind();
		glActiveTexture(GL_TEXTURE1);
		m_texVignetting->bind();

		//m_program.use();
		int2 res = m_factoryTexRGB->getTexture2DSize();
		float2 fres = res.get2<float2>();
		GLint viewportParams[4];
		glGetIntegerv (GL_VIEWPORT, viewportParams);
		glViewport(0,0,res.x,res.y);

		float2 offset;
		offset.x = ((float)M_PI/4) / (xFov/2);
		offset.y = ((float)M_PI/4) / (yFov/2);
		offset /= 2;

		float2 fsize = (float2(0.5f) - offset) * 2;
		//fres = float2((float)params[2], (float)params[3]);

		m_programVignetting.use();
		m_programVignetting.setInt("tex",0);
		m_programVignetting.setInt("texVignetting",1);

		//glDisable(GL_CULL_FACE);
		// center:
		glBegin(GL_QUADS);
		glTexCoord2f(x[0],y[2]);	glVertex2f(offset.x,  offset.y);
		glTexCoord2f(x[0],y[1]);	glVertex2f(offset.x,  1-offset.y);
		glTexCoord2f(x[1],y[1]);	glVertex2f(1-offset.x,1-offset.y);
		glTexCoord2f(x[1],y[2]);	glVertex2f(1-offset.x,offset.y);

		// left:
		glTexCoord2f(x[0],y[0]);	glVertex2f(offset.x-fsize.x,  offset.y-fsize.y);
		glTexCoord2f(x[1],y[0]);	glVertex2f(offset.x-fsize.x,  1-offset.y+fsize.y);
		glTexCoord2f(x[1],y[1]);	glVertex2f(offset.x,  1-offset.y);
		glTexCoord2f(x[0],y[1]);	glVertex2f(offset.x,  offset.y);

		// right:
		glTexCoord2f(x[3],y[1]);	glVertex2f(offset.x+fsize.x,  offset.y);
		glTexCoord2f(x[2],y[1]);	glVertex2f(offset.x+fsize.x,  1-offset.y);
		glTexCoord2f(x[2],y[0]);	glVertex2f(1-offset.x+fsize.x,1-offset.y+fsize.y);
		glTexCoord2f(x[3],y[0]);	glVertex2f(1-offset.x+fsize.x,offset.y-fsize.y);

		// top:
		glTexCoord2f(x[2],y[2]);	glVertex2f(offset.x,  offset.y+fsize.y);
		glTexCoord2f(x[2],y[1]);	glVertex2f(offset.x-fsize.x,  1-offset.y+fsize.y);
		glTexCoord2f(x[3],y[1]);	glVertex2f(1-offset.x+fsize.x,1-offset.y+fsize.y);
		glTexCoord2f(x[3],y[2]);	glVertex2f(1-offset.x,offset.y+fsize.y);

		// bottom:
		glTexCoord2f(x[2],y[0]);	glVertex2f(offset.x-fsize.y,  offset.y-fsize.y);
		glTexCoord2f(x[2],y[1]);	glVertex2f(offset.x,  1-offset.y-fsize.y);
		glTexCoord2f(x[1],y[1]);	glVertex2f(1-offset.x,1-offset.y-fsize.y);
		glTexCoord2f(x[1],y[0]);	glVertex2f(1-offset.x+fsize.y,offset.y-fsize.y);
		glEnd();

		glUseProgram(0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		checkGLError();
		framebufferTarget->unbind();

		glViewport(viewportParams[0],viewportParams[1],viewportParams[2],viewportParams[3]);
	}

	void loadTextures()
	{
		m_texVignetting = loadTextureFromFile("lensflaremask.jpg");
		m_texEnvMap = new GLTexture2D(m_width, m_height, GL_R32F, GL_RED);
		m_imageEnvMap = getImage2DGL(*m_context, CL_MEM_READ_WRITE, m_texEnvMap);
	}

	void unloadTextures()
	{
		m_imageEnvMap = NULL;
		m_texEnvMap = NULL;
		m_texVignetting = NULL;
	}

public:
	EffectLensFlareStarFromEnvMap (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, int width, int height, bool dynamic,
				Ptr<EffectCLObjectsFactory> factory): 
		m_programVignetting("shaders\\lensFlareStarVignetting.vs", "shaders\\lensFlareStarVignetting.fs"),
		EffectLensFlareStarFromSimple(factoryRGB, factoryRed, width, height, dynamic, factory)
	{
		m_kernelFillImage2 = createKernel(*m_context, "kernels\\kernels.cl", "fillImageFromCutBufferAndImage");
		if(!m_dynamic)
		{
			loadTextures();
		}
		m_FoV = float2((float)M_PI / 2, (float)M_PI / 2 * 480 / 640);
	}

	~EffectLensFlareStarFromEnvMap()
	{
		//m_imageEnvMap = NULL;
	}

	void fillImageFromCutBufferAndImage(cl::Image2DGL imageRes, cl::Image2DGL imageColor, ClBuffer2DFloat& bufferRe, float resMult, float2 cutBufferOrg, float2 cutBufferSize)
	{	
		int width = imageRes.getImageInfo<CL_IMAGE_WIDTH>();
		int height = imageRes.getImageInfo<CL_IMAGE_HEIGHT>();
		uint2 numWorkItems (width, height);

		uint2 imageResolution (width, height);
		float2 normRes = imageResolution.get2<float2>();
		normRes /= (float) imageResolution.getMaxValue();
		float2 halfSize (bufferRe.getSize().get2<float2>()/2);
		uint2 effectiveBufferSize = (halfSize * normRes).get2<uint2>();

		m_kernelFillImage2.setArg(0, imageRes);
		m_kernelFillImage2.setArg(1, imageColor);
		m_kernelFillImage2.setArg(2, (cl::Buffer)bufferRe);
		m_kernelFillImage2.setArg(3, bufferRe.getSize());
		m_kernelFillImage2.setArg(4, effectiveBufferSize);
		m_kernelFillImage2.setArg(5, cutBufferOrg);
		m_kernelFillImage2.setArg(6, cutBufferSize);
		m_kernelFillImage2.setArg(7, resMult);
		m_queue->enqueueNDRangeKernel(
			m_kernelFillImage2,
			cl::NDRange(0,0),
			cl::NDRange(numWorkItems.x, numWorkItems.y));
	}

	void runKernel(cl::Image2DGL& imageColor, cl::Image2DGL& imageDepth, Ptr<GLTextureEnvMap> envMap, cl::Image2DGL& imageResult)
	{
		if(m_dynamic)
		{
			allocBuffers(m_factoryCLObjects);
			initStar(m_kernelRe);
			m_kernelIm->zeroesMemory();
			clFFT_Direction dir = clFFT_Forward;
			m_fft.perform(*m_kernelRe, *m_kernelIm, dir);
			loadTextures();
		}

		m_queue->flush();
		
		drawEnvMap(m_texEnvMap, envMap, (float) M_PI, (float) M_PI);

		glFlush();

		std::vector<cl::Memory> vecGLMems;
		vecGLMems.push_back(m_imageEnvMap);
		m_queue->enqueueAcquireGLObjects(&vecGLMems);
		simpleFillBufferFromImage(m_imageEnvMap, m_bufferRe);
		m_queue->enqueueReleaseGLObjects(&vecGLMems);

		float resMult = 1.0f / (m_bufferRe->getSize().getArea());
		{
			m_bufferIm->zeroesMemory();
			clFFT_Direction dirForward = clFFT_Forward;
			m_fft.perform(*m_bufferRe, *m_bufferIm, dirForward);
			m_mult.multiply(*m_bufferRe, *m_bufferIm, *m_kernelRe, *m_kernelIm, int2(m_width, m_height));
			clFFT_Direction dirInverse = clFFT_Inverse;
			m_fft.perform(*m_bufferRe, *m_bufferIm, dirInverse);	
		}
		resMult *= m_lensFlareMult;
		
		float2 bufferSize(m_FoV / float2((float)M_PI));
		float2 bufferOrg(float2(1.0f) - bufferSize/2);
		fillImageFromCutBufferAndImage(imageResult, imageColor, *m_bufferRe, resMult, bufferOrg, bufferSize);

		if(m_dynamic)
		{
			unloadTextures();
			freeBuffers();
		}
	}

	bool requireEnvMap()
	{
		return true;
	}
};