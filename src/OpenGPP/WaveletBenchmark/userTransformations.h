#pragma once

#include "OpenGPP.h"
#include "util.h"

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

	virtual void process()=0;
};



class UTloadPersistent1ChannelBufferFromFile: public UserTransformation
{
protected:
	string m_filename;
	Imf::Array2D<float> m_hostMemoryPixels;
	Ptr<ClBuffer2D> m_buffer;

public:
	UTloadPersistent1ChannelBufferFromFile(Ptr<ClFacade> facade, string filename):
		UserTransformation(facade)
	{
		m_filename = filename;
		int width;
		int height;
		int stride;
		readRedFromEXR(m_filename, m_hostMemoryPixels, width, height, stride);
		int2 resolution(width, height);
		m_buffer = new ClBuffer2D (facade->getContext(), facade->getQueue(), resolution, stride);
		facade->getQueue()->enqueueWriteBuffer(*m_buffer, false, 0, m_buffer->getSize().y*m_buffer->getStride()*sizeof(float), &m_hostMemoryPixels[0][0]);
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
	void process()
	{
		m_clFacade->setActiveBuffer(m_buffer);
	}
};


class UTloadDynamic1ChannelBufferFromFile: public UserTransformation
{
protected:
	string m_filename;
	Imf::Array2D<float> m_hostMemoryPixels;

public:
	UTloadDynamic1ChannelBufferFromFile(Ptr<ClFacade> facade, string filename):
	  UserTransformation(facade)
	  {
		  m_filename = filename;
		  int width;
		  int height;
		  int stride;
		  readRedFromEXR(m_filename, m_hostMemoryPixels, width, height, stride);
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
	void process()
	{
		Ptr<ClBuffer2D> buffer = m_clFacade->getUnusedBuffer();
		m_clFacade->setActiveBuffer(buffer);
		m_clFacade->getQueue()->enqueueWriteBuffer(*buffer, false, 0, buffer->getSize().y*buffer->getStride()*sizeof(float), &m_hostMemoryPixels[0][0]);
	}
};


class UTsaveClBufferToArray: public UserTransformation
{
public:
	UTsaveClBufferToArray(Ptr<ClFacade> facade):
		UserTransformation(facade)
	{

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

			//writeRedChannelToExrFile(m_filename, memBlock, buffer->getSize(), buffer->getStride());

			delete [] memBlock;

		} catchCLError
	}
};


class UTsaveBufferToEXRFile: public UserTransformation
{
protected:
	string m_filename;
	void writeRedChannelToExrFile(const string& filename, float* red, int2 resolution, int stride)
	{
		float* redReorganized = new float [resolution.getArea()];
		for(int y = 0; y < resolution.y; y++)
			memcpy(redReorganized + y*resolution.x, red + y*stride, sizeof(float)*resolution.x);
		Imf::Header header (resolution.x, resolution.y); // 1
		header.channels().insert ("R", Imf::Channel (Imf::FLOAT)); // 2

		Imf::OutputFile file (filename.c_str(), header); // 4
		Imf::FrameBuffer frameBuffer; // 5
		frameBuffer.insert ("R", // name // 6
			Imf::Slice (Imf::FLOAT, // type // 7
			(char *) redReorganized, // base // 8
			sizeof (*redReorganized) * 1, // xStride// 9
			sizeof (*redReorganized) * resolution.x)); // yStride// 10
		file.setFrameBuffer (frameBuffer); // 16
		file.writePixels (resolution.y); // 17

		delete [] redReorganized;
	}
public:
	UTsaveBufferToEXRFile(Ptr<ClFacade> facade, string filename):
		UserTransformation(facade)
	{
		m_filename = filename;
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
		
			writeRedChannelToExrFile(m_filename, memBlock, buffer->getSize(), buffer->getStride());
		
			delete [] memBlock;

		} catchCLError
	}
};

class UTsaveBufferToDLM: public UserTransformation
{
protected:
	string m_filename;
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



class UThorizonMemUsingPrivateMemory: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonMemUsingPrivateMemory(Ptr<ClFacade> facade, int2 resolution):
		UserTransformation(facade)
	{
		m_facade = facade;
		m_resolution = resolution;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "horizonUsingPrivateMemory");
	}
	int getOptCountWorkItems(int size)
	{
		return size/2 + size%2 + 4;
	}
	int getRequiredLocalMomeryItems(int size)
	{
		return getOptimalStride(size);
	}
	void process()
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
			m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		}
		catchCLError;
	}
};

class UThorizonMemUsingLocalMemory: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonMemUsingLocalMemory(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "copyGlobalLocalGlobalAsync");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2 + 4;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
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
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};


class UThorizon2in1Buffer: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizon2in1Buffer(Ptr<ClFacade> facade, int2 resolution):
		UserTransformation(facade)
	{
		m_facade = facade;
		m_resolution = resolution;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "horizon2in1Buffer");
	}
	int getOptCountWorkItems(int size)
	{
		return size/2 + size%2;
	}
	int getRequiredLocalMomeryItems(int size)
	{
		return getOptimalStride(size);
	}
	void process()
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
			m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		}
		catchCLError;
	}
};


class UTvertical2in1Buffer: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UTvertical2in1Buffer(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	{
		m_facade = facade;
		m_resolution = resolution;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "vertical2in1Buffer");
	}
	int getOptCountWorkItems(int size)
	{
		return size/2 + size%2;
	}
	int getRequiredLocalMomeryItems(int size)
	{
		return getOptimalStride(size);
	}
	void process()
	{
		try {
			Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
			m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
			Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
			m_kernel.setArg(1, bufferResult->getClBuffer());

			m_facade->setActiveBuffer(bufferResult);


			int numWorkItemHeight = getOptCountWorkItems(m_resolution.y);
			int2 numWorkItems(1, numWorkItemHeight);
			int2 numGlobalItems(m_resolution.x, numWorkItemHeight);
			cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
			m_kernel.setArg(2, bufferConstMemory);
			//initKernelConstMemory(resLvl);
			m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.y), NULL);
			getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		}
		catchCLError;
	}
};

class UTvertical2in1BufferAsync: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UTvertical2in1BufferAsync(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	{
		m_facade = facade;
		m_resolution = resolution;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "vertical2in1BufferAsync");
	}
	int getOptCountWorkItems(int size)
	{
		return size/2 + size%2;
	}
	int getRequiredLocalMomeryItems(int size)
	{
		return getOptimalStride(size);
	}
	void process()
	{
		try {
			Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
			m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
			Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
			m_kernel.setArg(1, bufferResult->getClBuffer());

			m_facade->setActiveBuffer(bufferResult);

			int numWorkItemHeight = getOptCountWorkItems(m_resolution.y);
			int2 numWorkItems(1, numWorkItemHeight);
			int2 numGlobalItems(m_resolution.x, numWorkItemHeight);
			cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
			m_kernel.setArg(2, bufferConstMemory);
			//initKernelConstMemory(resLvl);
			m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.y), NULL);
			getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		}
		catchCLError;
	}
};


class UTcopyGlobalLocalGlobal: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UTcopyGlobalLocalGlobal(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "copyGlobalLocalGlobal");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
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
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};


class UTcopyGlobalLocalLine: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UTcopyGlobalLocalLine(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "copyGlobalLocalGlobal");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
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
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};


class UTcopyGlobalLocalColumn: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UTcopyGlobalLocalColumn(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "copyGlobalLocalColumn");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
	  {
		  try {
			  Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
			  m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
			  Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
			  m_kernel.setArg(1, bufferResult->getClBuffer());

			  m_facade->setActiveBuffer(bufferResult);


			  int numWorkItemHeight = getOptCountWorkItems(m_resolution.y);
			  int2 numWorkItems(1, numWorkItemHeight);
			  int2 numGlobalItems(m_resolution.x, numWorkItemHeight);
			  cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
			  m_kernel.setArg(2, bufferConstMemory);
			  //initKernelConstMemory(resLvl);
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.y), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};

class UTcopyGlobalLocalTide: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UTcopyGlobalLocalTide(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	{
		m_facade = facade;
		m_resolution = resolution;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "copyGlobalLocalTide");
	}
	int getOptCountWorkItems(int size)
	{
		return size/2 + size%2;
	}
	int getRequiredLocalMomeryItems(int size)
	{
		return getOptimalStride(size);
	}
	int2 roundGlobalItems(int2 val, int2 workItems)
	{
		int2 v (workItems.x*(val.x / workItems.x + (val.y % workItems.x?1:0)),
			workItems.y*(val.y / workItems.y + (val.y % workItems.y?1:0)));
		return v;
	}
	void process()
	{
		try {
			Ptr<ClBuffer2D> bufferIn = m_facade->getActiveBuffer();
			m_kernel.setArg(0, m_facade->getActiveBuffer()->getClBuffer());
			Ptr<ClBuffer2D> bufferResult = m_facade->getUnusedBuffer();
			m_kernel.setArg(1, bufferResult->getClBuffer());

			m_facade->setActiveBuffer(bufferResult);


			int numWorkItemHeight = getOptCountWorkItems(m_resolution.y);
			int2 numWorkItems(32, 16);
			int2 numGlobalItems(m_resolution.x, numWorkItemHeight);
			numGlobalItems = roundGlobalItems(numGlobalItems, numWorkItems);
			cl::Buffer bufferConstMemory = initConstMemory(bufferResult->getStride(), bufferResult->getSize());
			m_kernel.setArg(2, bufferConstMemory);
			//initKernelConstMemory(resLvl);
			m_kernel.setArg(3, sizeof(cl_float) * 1024, NULL);
			getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		}
		catchCLError;
	}
};

class UTcopyGlobalLocalGlobalAsync: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UTcopyGlobalLocalGlobalAsync(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "copyGlobalLocalGlobalAsync");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
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
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};




class UThorizonLiftingSync: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonLiftingSync(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "horizonLiftingSync");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
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
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};


class UThorizonLiftingSync2: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonLiftingSync2(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "horizonLiftingSync2");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
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
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};


class UThorizonLiftingSync3: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonLiftingSync3(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "horizonLiftingSync3");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2 + 4;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size) + 32;
	  }
	  void process()
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
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};



class UThorizonLiftingAsync: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonLiftingAsync(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "horizonLiftingAsync");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
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
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};


class UThorizonConvolutionAsync: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonConvolutionAsync(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "horizonConvolutionAsync");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
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
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};


class UThorizonConvolutionAsyncOpt: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonConvolutionAsyncOpt(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "horizonConvolutionAsyncOpt");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
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
			  m_kernel.setArg(3, sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};


class UTtransposeInsideTiles: public UserTransformation
{
	cl::Kernel m_kernel;
	Ptr<ClFacade> m_facade;
public:
	UTtransposeInsideTiles(Ptr<ClFacade> facade):
	  UserTransformation(facade)
	{
		m_facade = facade;
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "transposeInTiles");
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
			m_kernel.setArg(2, sizeof(cl_float) * 32 * 32, NULL);
			getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		}
		catchCLError;
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
		m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "transposeInTiles");
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
			m_kernel.setArg(2, sizeof(cl_float) * 32 * 32, NULL);
			getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		}
		catchCLError;
	}
};

/*
class UThorizonConvolutionAsyncOpt4Lines: public UserTransformation
{
	cl::Kernel m_kernel;
	int2 m_resolution;
	Ptr<ClFacade> m_facade;
public:
	UThorizonConvolutionAsyncOpt4Lines(Ptr<ClFacade> facade, int2 resolution):
	  UserTransformation(facade)
	  {
		  m_facade = facade;
		  m_resolution = resolution;
		  m_kernel = createKernel(*m_facade->getContext(), "kernels\\wperfCDF97User.cl", "horizonConvolutionAsyncOpt4Lines");
	  }
	  int getOptCountWorkItems(int size)
	  {
		  return size/2 + size%2;
	  }
	  int getRequiredLocalMomeryItems(int size)
	  {
		  return getOptimalStride(size);
	  }
	  void process()
	  {
		  try {
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
			  m_kernel.setArg(3, 4*sizeof(cl_float) * getRequiredLocalMomeryItems(m_resolution.x), NULL);
			  getQueue()->enqueueNDRangeKernel(m_kernel, cl::NDRange (0,0), cl::NDRange(numGlobalItems.x, numGlobalItems.y), cl::NDRange(numWorkItems.x, numWorkItems.y));
		  }
		  catchCLError;
	  }
};*/