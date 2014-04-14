#pragma once

#include "OpenGPP.h"
#include "util.h"
#include <assert.h>
#include <sstream>
#include <xlocale>



struct StructConstMemory
{
	int2 resolution;
	int2 halfResolution;
	int bufferPitch;
};



class UserTransformation
{
protected:
	Ptr<cl::Buffer> m_bufferConstMemory;
	StructConstMemory m_hostConstMemory;

	Ptr<ClFacade> m_clFacade;

	Ptr<cl::CommandQueue> getQueue()
	{
		return m_clFacade->getQueue();
	}

	Ptr<cl::Context> getContext()
	{
		return m_clFacade->getContext();
	}

	cl::Buffer initConstMemory(int bufferPitch, int2 resolution)
	{
		m_hostConstMemory.bufferPitch = bufferPitch;
		m_hostConstMemory.resolution = resolution;
		m_hostConstMemory.halfResolution = resolution/2 + resolution%2;
		if (m_bufferConstMemory.isNull())
			m_bufferConstMemory = new cl::Buffer(*getContext(), CL_MEM_READ_ONLY, sizeof(StructConstMemory));
		getQueue()->enqueueWriteBuffer(*m_bufferConstMemory, false, 0, sizeof(StructConstMemory), &m_hostConstMemory);
		return *m_bufferConstMemory;
	}

public:
	UserTransformation(Ptr<ClFacade> facade)
	{
		m_clFacade = facade;
	}
	virtual void prepare(int2 resolution)=0;
	virtual void process()=0;
};



class UTloadDynamic1ChannelBufferFromDLMFile: public UserTransformation
{
protected:
	string m_filename;
	float* m_hostMemoryPixels;
	int2 m_resolution;

	int2 m_preparedResolution;
	float* m_memory;

public:
	UTloadDynamic1ChannelBufferFromDLMFile(Ptr<ClFacade> facade, string filename, int2 res):
	  UserTransformation(facade)
	{
		m_filename = filename;
		m_memory = NULL;
		m_preparedResolution = int2(0,0);
		m_resolution = res;
		m_hostMemoryPixels = new float[m_resolution.getArea()];
		readRedFromDLM(m_filename, m_hostMemoryPixels, m_resolution);
	}

	~UTloadDynamic1ChannelBufferFromDLMFile()
	{
		delete[](m_memory);
		delete[](m_hostMemoryPixels);
	}

	class spaceClass: public std::ctype<char>
	{
	public:
		spaceClass(): std::ctype<char>(getTable())
		{}
		static mask const* getTable()
		{
			static mask rc[table_size];
			rc[','] = std::ctype_base::space;
			rc['\n'] = std::ctype_base::space;
			rc[' '] = std::ctype_base::space;
			return rc;
		}

	};

	void readRedFromDLM (const string& filename, float* memory, int2 resolution)
	{
		std::ifstream ifs (filename);
		string line;
		ifs.imbue(std::locale(ifs.getloc(), new spaceClass()));
		
		for (int y = 0; y < resolution.y; y++)
			for(int x = 0; x < resolution.x; x++)
			{
				float fval;
				ifs >> fval; 
				memory[x+y*resolution.x] = fval;
			}
	}

	void prepare(int2 resolution)
	{
		if (m_preparedResolution == resolution)
			return;

		if (m_memory)
			delete[](m_memory);
		m_memory = new float[resolution.getArea()];

		for(int y = 0; y < resolution.y; y++)
			for(int copied = 0; copied < resolution.x; copied += m_resolution.x)
			{
				int nRowSrc = y % m_resolution.y;
				int nRowDst = y;

				int nColumnSrc = 0;
				int nColumnDst = copied;

				int nCountCols = std::min(resolution.x - copied, m_resolution.x);

				memcpy(
					m_memory + nColumnDst + nRowDst*resolution.y,
					m_hostMemoryPixels + nColumnSrc + nRowSrc*m_resolution.x,
					nCountCols*sizeof(float));
			}
	}

	void process()
	{
		Ptr<ClBuffer2D> buffer = m_clFacade->getUnusedBuffer();
		m_clFacade->setActiveBuffer(buffer);
		m_clFacade->getQueue()->enqueueWriteBuffer(*buffer, false, 0, buffer->getSize().y*buffer->getStride()*sizeof(float), m_memory);
	}
};


class UTloadDynamic1ChannelBufferFromEXRFile: public UserTransformation
{
protected:
	string m_filename;
	Imf::Array2D<float> m_hostMemoryPixels;
	int m_width, m_height, m_stride;

	int2 m_preparedResolution;
	float* m_memory;

public:
	UTloadDynamic1ChannelBufferFromEXRFile(Ptr<ClFacade> facade, string filename):
	  UserTransformation(facade)
	  {
		  m_filename = filename;
		  m_memory = NULL;
		  m_preparedResolution = int2(0,0);
		  readRedFromEXR(m_filename, m_hostMemoryPixels, m_width, m_height, m_stride);
	  }

	  ~UTloadDynamic1ChannelBufferFromEXRFile()
	  {
		  delete[](m_memory);
	  }

	  void readRedFromEXR (const string& filename, Imf::Array2D<float> &pixels, int &width, int &height, int& stride)
	  {
		  Imf::InputFile file (filename.c_str());
		  Imath::Box2i dw = file.header().dataWindow();
		  width = dw.max.x - dw.min.x + 1;
		  height = dw.max.y - dw.min.y + 1;
		  stride = getOptimalStride(width);
		  pixels.resizeErase (height, stride);
		  Imf::FrameBuffer frameBuffer;
		  frameBuffer.insert ("R", // name
			  Imf::Slice (Imf::FLOAT, // type
			  (char *) (&pixels[0][0] - // base
			  dw.min.x -
			  dw.min.y * width),
			  sizeof (pixels[0][0]) * 1, // xStride
			  sizeof (pixels[0][0]) * stride,// yStride
			  1, 1, // x/y sampling
			  0.0)); // fillValue
		  file.setFrameBuffer (frameBuffer);
		  file.readPixels (dw.min.y, dw.max.y);
	  }

	  void prepare(int2 resolution)
	  {
		  if (m_preparedResolution == resolution)
			  return;

		  if (m_memory)
			  delete[](m_memory);
		  m_memory = new float[resolution.getArea()];

		  for(int y = 0; y < resolution.y; y++)
			  for(int copied = 0; copied < resolution.x; copied += m_width)
			  {
				  int nRowSrc = y % m_height;
				  int nRowDst = y;

				  int nColumnSrc = 0;
				  int nColumnDst = copied;

				  int nCountCols = std::min(resolution.x - copied, m_width);

				  memcpy(
					  m_memory + nColumnDst + nRowDst*resolution.y,
					  &m_hostMemoryPixels[0][0] + nColumnSrc + nRowSrc*m_stride,
					  nCountCols*sizeof(float));
			  }
	  }

	  void process()
	  {
		  Ptr<ClBuffer2D> buffer = m_clFacade->getUnusedBuffer();
		  m_clFacade->setActiveBuffer(buffer);
		  m_clFacade->getQueue()->enqueueWriteBuffer(*buffer, false, 0, buffer->getSize().y*buffer->getStride()*sizeof(float), m_memory);
	  }
};



class UTsaveClBufferToArray: public UserTransformation
{
	int2 m_resolution;
	float* m_memory;
public:
	UTsaveClBufferToArray(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	  {
		  m_resolution = int2(0,0);
		  m_memory = NULL;
	  }
	~UTsaveClBufferToArray()
	{
		if (m_memory)
			delete[](m_memory);
	}

	  void prepare(int2 resolution)
	  {
		  if (m_resolution == resolution)
			  return;

		  if (m_memory)
			  delete[](m_memory);

		  m_resolution = resolution;
		  m_memory = new float[resolution.getArea()];
	  }

	  void process()
	  {
		  try{
			  Ptr<ClBuffer2D> buffer = m_clFacade->getActiveBuffer();
			  Ptr<cl::CommandQueue> queue = m_clFacade->getQueue();
			  cl::Event event;
			  queue->enqueueReadBuffer(
				  *buffer,
				  CL_FALSE, 
				  0, 
				  buffer->getStride()*buffer->getSize().y*sizeof(float), 
				  m_memory,
				  NULL,
				  &event);

			  //  Blah blah blah
			  event.wait();
		  } catchCLError
	  }
};


class UTsaveBufferToDLM: public UserTransformation
{
protected:
	string m_filename;
	int2 m_resolution;
	void writeRedChannelToDLM(const string& filename, float* red, int2 resolution, int stride)
	{
		std::ofstream fs(filename);
		for (int y = 0; y < resolution.y; y++)
		{
			for (int x = 0; x < resolution.x; x++)
			{
				if (x!=0)
					fs << ",";
				fs << red[x + y*stride];
			}
			fs << std::endl;
		}
	}

public:
	UTsaveBufferToDLM(Ptr<ClFacade> facade, string filename):
	  UserTransformation(facade)
	{
		m_filename = filename;
	}
	void prepare(int2 resolution)
	{
		m_resolution = resolution;
	}
	void process()
	{
		try{
			Ptr<ClBuffer2D> buffer = m_clFacade->getActiveBuffer();
			Ptr<cl::CommandQueue> queue = m_clFacade->getQueue();
			float* memBlock = new float [buffer->getStride()*buffer->getSize().y];
			cl::Event event;
			queue->enqueueReadBuffer(
				*buffer,
				CL_FALSE, 
				0, 
				buffer->getStride()*buffer->getSize().y*sizeof(float), 
				memBlock,
				NULL,
				&event);

			//  Blah blah blah
			event.wait();

			writeRedChannelToDLM(m_filename, memBlock, buffer->getSize(), buffer->getStride());

			delete [] memBlock;

		} catchCLError
	}
};


class UThorizonLiftingSync: public UserTransformation
{
	cl::Kernel m_kernel, m_kernelWide;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonLiftingSync(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	{
		m_facade = facade;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonLiftingSync");
		m_kernelWide = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonLiftingSyncWide");
	}
	int getOptCountWorkItems(int size)
	{
		return size/2 + size%2;
	}
	int getRequiredLocalMomeryItems(int size)
	{
		return getOptimalStride(size);
	}
	void prepare(int2 resolution)
	{
		m_resolution = resolution;
	}
	void process()
	{
		if (m_resolution.x <= 2048)
		{
			try {
				Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				m_kernel.setArg(1, bufferResult->getClBuffer());
				m_facade->setActiveBuffer(bufferResult);


				int numWorkItemWidth = getOptCountWorkItems(m_resolution.x);
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numWorkItemWidth, m_resolution.y);
				cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				m_kernel.setArg(2, bufferConstMemory);
				//initKernelConstMemory(resLvl);
				m_kernel.setArg(3, sizeof(cl_float) * m_resolution.x, NULL);
				getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			} catchCLError;
		}
		else
		{
			Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
			m_kernelWide.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
			Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
			m_kernelWide.setArg(1, bufferResult->getClBuffer());
			m_facade->setActiveBuffer(bufferResult);


			int numWorkItemWidth = m_resolution.x/((m_resolution.x/2048) + (m_resolution.x%2048?1:0))/2;
			int2 numWorkItems(numWorkItemWidth, 1);
			int2 numGlobalItems(numWorkItemWidth, m_resolution.y);
			cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
			m_kernelWide.setArg(2, bufferConstMemory);
			//initKernelConstMemory(resLvl);
			m_kernelWide.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			m_kernelWide.setArg(4, m_resolution.x/numWorkItemWidth/2);
			getQueue()->enqueueNDRangeKernel(m_kernelWide, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		}
	}
};

class UThorizonLiftingAsync: public UserTransformation
{
	cl::Kernel m_kernel;
	cl::Kernel m_kernelWide;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonLiftingAsync(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	{
		m_facade = facade;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonLiftingAsync");
		m_kernelWide = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonLiftingAsyncWide");
	}
	int getOptCountWorkItems(int size)
	{
		return size/2 + size%2;
	}
	int getRequiredLocalMomeryItems(int size)
	{
		return size + 16;
	}
	void prepare(int2 resolution)
	{
		m_resolution = resolution;
	}
	void process()
	{
		if (m_resolution.x <= 2048)
		{
			try 
			{
				  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				  m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				  m_kernel.setArg(1, bufferResult->getClBuffer());

				  m_facade->setActiveBuffer(bufferResult);


				  int numWorkItemWidth = getOptCountWorkItems(m_resolution.x);
				  int2 numWorkItems(numWorkItemWidth, 1);
				  int2 numGlobalItems(numWorkItemWidth, m_resolution.y);
				  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				  m_kernel.setArg(2, bufferConstMemory);
				  //initKernelConstMemory(resLvl);
				  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  } catchCLError;
		}
		else
		{			
			try 
			{
				Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				m_kernelWide.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				m_kernelWide.setArg(1, bufferResult->getClBuffer());
				m_facade->setActiveBuffer(bufferResult);


				int numWorkItemWidth = m_resolution.x/((m_resolution.x/2048) + (m_resolution.x%2048?1:0))/2;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numWorkItemWidth, m_resolution.y);
				cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				m_kernelWide.setArg(2, bufferConstMemory);
				m_kernelWide.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				m_kernelWide.setArg(4, m_resolution.x/numWorkItemWidth/2);
				getQueue()->enqueueNDRangeKernel(m_kernelWide, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			} catchCLError;
		}
	 }
};


class UThorizonConvolutionAsync: public UserTransformation
{
	cl::Kernel m_kernel;
	cl::Kernel m_kernelWide;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonConvolutionAsync(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonConvolutionAsync");
		  m_kernelWide = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonConvolutionAsyncWide");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return size + 16;
	  }
	  void prepare(int2 resolution)
	  {
		  m_resolution = resolution;
	  }
	  void process()
	  {
		  if (m_resolution.x <= 2048)
		  {
			  try 
			  {
				  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				  m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				  m_kernel.setArg(1, bufferResult->getClBuffer());

				  m_facade->setActiveBuffer(bufferResult);


				  int numWorkItemWidth = getOptCountWorkItems(m_resolution.x);
				  int2 numWorkItems(numWorkItemWidth, 1);
				  int2 numGlobalItems(numWorkItemWidth, m_resolution.y);
				  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				  m_kernel.setArg(2, bufferConstMemory);
				  //initKernelConstMemory(resLvl);
				  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			  } catchCLError;
		  }
		  else
		  {			
			  try 
			  {
				  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				  m_kernelWide.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				  m_kernelWide.setArg(1, bufferResult->getClBuffer());
				  m_facade->setActiveBuffer(bufferResult);


				  int numWorkItemWidth = m_resolution.x/((m_resolution.x/2048) + (m_resolution.x%2048?1:0))/2;
				  int2 numWorkItems(numWorkItemWidth, 1);
				  int2 numGlobalItems(numWorkItemWidth, m_resolution.y);
				  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				  m_kernelWide.setArg(2, bufferConstMemory);
				  m_kernelWide.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				  m_kernelWide.setArg(4, m_resolution.x/numWorkItemWidth/2);
				  getQueue()->enqueueNDRangeKernel(m_kernelWide, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			  } catchCLError;
		  }
	  }
};



class UThorizonConvolutionAsync2: public UserTransformation
{
	cl::Kernel m_kernel;
	cl::Kernel m_kernelWide;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonConvolutionAsync2(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonConvolutionAsync2");
		  m_kernelWide = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonConvolutionAsyncWide2");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return 2*(size + 16);
	  }
	  void prepare(int2 resolution)
	  {
		  m_resolution = resolution;
	  }
	  void process()
	  {
		  if (m_resolution.x <= 2048)
		  {
			  try 
			  {
				  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				  m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				  m_kernel.setArg(1, bufferResult->getClBuffer());

				  m_facade->setActiveBuffer(bufferResult);


				  int numWorkItemWidth = getOptCountWorkItems(m_resolution.x);
				  int2 numWorkItems(numWorkItemWidth, 1);
				  int2 numGlobalItems(numWorkItemWidth, m_resolution.y/2);
				  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				  m_kernel.setArg(2, bufferConstMemory);
				  //initKernelConstMemory(resLvl);
				  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			  } catchCLError;
		  }
		  else
		  {			
			  try 
			  {
				  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				  m_kernelWide.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				  m_kernelWide.setArg(1, bufferResult->getClBuffer());
				  m_facade->setActiveBuffer(bufferResult);


				  int numWorkItemWidth = m_resolution.x/((m_resolution.x/2048) + (m_resolution.x%2048?1:0))/2;
				  int2 numWorkItems(numWorkItemWidth, 1);
				  int2 numGlobalItems(numWorkItemWidth, m_resolution.y/2);
				  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				  m_kernelWide.setArg(2, bufferConstMemory);
				  m_kernelWide.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				  m_kernelWide.setArg(4, m_resolution.x/numWorkItemWidth/2);
				  getQueue()->enqueueNDRangeKernel(m_kernelWide, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			  } catchCLError;
		  }
	  }
};

class UThorizonConvolutionAsync4: public UserTransformation
{
	cl::Kernel m_kernel;
	cl::Kernel m_kernelWide;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonConvolutionAsync4(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonConvolutionAsync4");
		  m_kernelWide = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonConvolutionAsyncWide4");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return 4*(size + 16);
	  }
	  void prepare(int2 resolution)
	  {
		  m_resolution = resolution;
	  }
	  void process()
	  {
		  if (m_resolution.x <= 2048)
		  {
			  try 
			  {
				  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				  m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				  m_kernel.setArg(1, bufferResult->getClBuffer());

				  m_facade->setActiveBuffer(bufferResult);


				  int numWorkItemWidth = getOptCountWorkItems(m_resolution.x);
				  int2 numWorkItems(numWorkItemWidth, 1);
				  int2 numGlobalItems(numWorkItemWidth, m_resolution.y/4);
				  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				  m_kernel.setArg(2, bufferConstMemory);
				  //initKernelConstMemory(resLvl);
				  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			  } catchCLError;
		  }
		  else
		  {			
			  try 
			  {
				  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				  m_kernelWide.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				  m_kernelWide.setArg(1, bufferResult->getClBuffer());
				  m_facade->setActiveBuffer(bufferResult);


				  int numWorkItemWidth = m_resolution.x/((m_resolution.x/2048) + (m_resolution.x%2048?1:0))/2;
				  int2 numWorkItems(numWorkItemWidth, 1);
				  int2 numGlobalItems(numWorkItemWidth, m_resolution.y/4);
				  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				  m_kernelWide.setArg(2, bufferConstMemory);
				  m_kernelWide.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				  m_kernelWide.setArg(4, m_resolution.x/numWorkItemWidth/2);
				  getQueue()->enqueueNDRangeKernel(m_kernelWide, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			  } catchCLError;
		  }
	  }
};

class UTverticalConvolutionAsync: public UserTransformation
{
	cl::Kernel m_kernel;
	cl::Kernel m_kernelWide;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UTverticalConvolutionAsync(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "verticalConvolutionAsync");
		  m_kernelWide = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "verticalConvolutionAsyncWide");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return size + 16;
	  }
	  void prepare(int2 resolution)
	  {
		  m_resolution = resolution;
	  }
	  void process()
	  {
		  if (m_resolution.x <= 2048)
		  {
			  try 
			  {
				  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				  m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				  m_kernel.setArg(1, bufferResult->getClBuffer());

				  m_facade->setActiveBuffer(bufferResult);


				  int numWorkItemHeight = getOptCountWorkItems(m_resolution.y);
				  int2 numWorkItems(numWorkItemHeight, 1);
				  int2 numGlobalItems(numWorkItemHeight, m_resolution.x);
				  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				  m_kernel.setArg(2, bufferConstMemory);
				  //initKernelConstMemory(resLvl);
				  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			  } catchCLError;
		  }
		  else
		  {			
			  try 
			  {
				  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				  m_kernelWide.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				  m_kernelWide.setArg(1, bufferResult->getClBuffer());
				  m_facade->setActiveBuffer(bufferResult);


				  int numWorkItemHeight = m_resolution.y/((m_resolution.y/2048) + (m_resolution.y%2048?1:0))/2;
				  int2 numWorkItems(numWorkItemHeight, 1);
				  int2 numGlobalItems(numWorkItemHeight, m_resolution.x);
				  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				  m_kernelWide.setArg(2, bufferConstMemory);
				  m_kernelWide.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				  m_kernelWide.setArg(4, m_resolution.x/numWorkItemHeight/2);
				  getQueue()->enqueueNDRangeKernel(m_kernelWide, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			  } catchCLError;
		  }
	  }
};



class UThorizonConvolutionAsyncTransposed: public UserTransformation
{
	cl::Kernel m_kernel;
	cl::Kernel m_kernelWide;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonConvolutionAsyncTransposed(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	{
		m_facade = facade;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonConvolutionAsyncTransposed");
		m_kernelWide = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "horizonConvolutionAsyncWideTransposed");
	}
	int getOptCountWorkItems(int size)
	{
		return size/2 + size%2;
	}
	int getRequiredLocalMomeryItems(int size)
	{
		return size + 16;
	}
	void prepare(int2 resolution)
	{
		m_resolution = resolution;
	}
	void process()
	{
		if (m_resolution.x <= 2048)
		{
			try 
			{
				Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				m_kernel.setArg(1, bufferResult->getClBuffer());

				m_facade->setActiveBuffer(bufferResult);


				int numWorkItemWidth = getOptCountWorkItems(m_resolution.x);
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numWorkItemWidth, m_resolution.y);
				cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				m_kernel.setArg(2, bufferConstMemory);
				//initKernelConstMemory(resLvl);
				m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			} catchCLError;
		}
		else
		{			
			try 
			{
				Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
				m_kernelWide.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
				Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
				m_kernelWide.setArg(1, bufferResult->getClBuffer());
				m_facade->setActiveBuffer(bufferResult);


				int numWorkItemWidth = m_resolution.x/((m_resolution.x/2048) + (m_resolution.x%2048?1:0))/2;
				int2 numWorkItems(numWorkItemWidth, 1);
				int2 numGlobalItems(numWorkItemWidth, m_resolution.y);
				cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
				m_kernelWide.setArg(2, bufferConstMemory);
				m_kernelWide.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
				m_kernelWide.setArg(4, m_resolution.x/numWorkItemWidth/2);
				getQueue()->enqueueNDRangeKernel(m_kernelWide, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
			} catchCLError;
		}
	}
};



class UTverticalOverlappedTiles: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UTverticalOverlappedTiles(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	{
		m_facade = facade;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "verticalOverlappedTiles");
	}
	int getRequiredLocalMomeryItems(int2 size)
	{
		return size.getArea()*2 + 32*8;
	}
	void prepare(int2 resolution)
	{
		m_resolution = resolution;
	}
	void process()
	{
		try 
		{
			Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
			m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
			Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
			m_kernel.setArg(1, bufferResult->getClBuffer());

			m_facade->setActiveBuffer(bufferResult);

			int2 numWorkItems(32, 32);
			int2 numGlobalItems(m_resolution.x, m_resolution.y/2);
			cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
			m_kernel.setArg(2, bufferConstMemory);
			//initKernelConstMemory(resLvl);
			m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(numWorkItems), NULL);
			getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		} catchCLError;
	}
};




class UTverticalSlidingWindow: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UTverticalSlidingWindow(Ptr<ClFacade> facade):
		UserTransformation(facade)
	{
		m_facade = facade;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "verticalSlidingWindow");
	}
	int getRequiredLocalMomeryItems(int2 size)
	{
		return size.getArea()*2*2;
	}
	void prepare(int2 resolution)
	{
		m_resolution = resolution;
	}
	void process()
	{
		try 
		{
			Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
			m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
			Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
			m_kernel.setArg(1, bufferResult->getClBuffer());

			m_facade->setActiveBuffer(bufferResult);

			int2 numWorkItems(32, 32);
			int2 numGlobalItems(m_resolution.x, 32);
			cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
			m_kernel.setArg(2, bufferConstMemory);
			//initKernelConstMemory(resLvl);
			m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(numWorkItems), NULL);
			getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		} catchCLError;
	}
};





class UTtranspose: public UserTransformation
{
	cl::Kernel m_kernel;
	Ptr<ClFacade> m_facade;
public:
	UTtranspose(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "transpose");
	  }
	  void prepare(int2 resolution)
	  {
	  }	  
	  void process()
	  {
		  try {
			  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
			  m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
			  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
			  m_kernel.setArg(1, bufferResult->getClBuffer());

			  m_facade->setActiveBuffer(bufferResult);

			  int2 numWorkItems(32, 32);
			  int2 numGlobalItems(bufferIn->getStride(), bufferIn->getAllocLines());
			  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
			  m_kernel.setArg(2, bufferConstMemory);
			  m_kernel.setArg(3, sizeof(cl_float) * 1024, NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};


class UTtranspose2x2: public UserTransformation
{
	cl::Kernel m_kernel;
	Ptr<ClFacade> m_facade;
public:
	UTtranspose2x2(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "transpose2x2");
	  }
	  void prepare(int2 resolution)
	  {
	  }	  
	  void process()
	  {
		  try {
			  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
			  m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
			  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
			  m_kernel.setArg(1, bufferResult->getClBuffer());

			  m_facade->setActiveBuffer(bufferResult);

			  int2 numWorkItems(32, 32);
			  int2 numGlobalItems(bufferIn->getStride()/2, bufferIn->getAllocLines()/2);
			  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
			  m_kernel.setArg(2, bufferConstMemory);
			  m_kernel.setArg(3, sizeof(cl_float) * 1024 * 4, NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};




class UTcopyUsingLocalMemory: public UserTransformation
{
	cl::Kernel m_kernel;
	Ptr<ClFacade> m_facade;
public:
	UTcopyUsingLocalMemory(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97UserFinal.cl", "copyGlobalLocalGlobal");
	  }
	  void prepare(int2 resolution)
	  {
	  }	  
	  void process()
	  {
		  try {
			  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
			  m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
			  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
			  m_kernel.setArg(1, bufferResult->getClBuffer());

			  m_facade->setActiveBuffer(bufferResult);

			  int2 numWorkItems(std::min(bufferIn->getStride()/2,1024), 1);
			  int2 numGlobalItems(bufferIn->getStride()/2, bufferIn->getAllocLines());
			  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
			  m_kernel.setArg(2, bufferConstMemory);
			  m_kernel.setArg(3, sizeof(cl_float) * 1024 * 2, NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};
