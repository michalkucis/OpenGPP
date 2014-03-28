#pragma once

#include <OpenGPP.h>

int getOptimalStride(int width)
{
	int alignment = 32;
	return (width / alignment + 1)*alignment;
}


class ClBuffer2D
{
	Ptr<cl::Context> m_context;
	Ptr<cl::CommandQueue> m_queue;
	Ptr<cl::Buffer> m_buffer;
	size_t m_stride;
	size_t m_allocatedLines;
	int2 m_size;
public:
	ClBuffer2D(Ptr<cl::Context> context, Ptr<cl::CommandQueue> queue, int2 resolution, int stride)
	{
		m_context = context;
		m_queue = queue;

		m_size = resolution;
		m_allocatedLines = getOptimalStride(m_size.y);
		m_stride = stride;

		m_buffer = new cl::Buffer (*context, CL_MEM_READ_WRITE, stride*m_allocatedLines*sizeof(float));
	}

	operator cl::Buffer ()
	{
		return *m_buffer;
	}
	cl::Buffer getClBuffer()
	{
		return (cl::Buffer) *this;
	}
	int2 getSize()
	{
		return m_size;
	}
	int getStride()
	{
		return m_stride;
	}
	int getAllocLines()
	{
		return m_allocatedLines;
	}
};


class ClFacade
{
protected:
	Ptr<cl::Context> m_context;
	Ptr<cl::CommandQueue> m_queue;
	Ptr<ClBuffer2D> m_buffer1, m_buffer2;
	Ptr<cl::Image2D> m_im1, m_im2;

	Ptr<ClBuffer2D> m_activeBuffer;
	Ptr<cl::Image2D> m_activeIm;
public:
	ClFacade(int2 maxResolution)
	{
		try{
		m_context = new cl::Context(::getCLGLContext());
		m_queue = new cl::CommandQueue(::getCLCommandQueue(*m_context));

		int2 bufferResolution (maxResolution);

		m_buffer1 = new ClBuffer2D(m_context, m_queue, bufferResolution, getOptimalStride(bufferResolution.x));
		m_buffer2 = new ClBuffer2D(m_context, m_queue, bufferResolution, getOptimalStride(bufferResolution.x));

		m_im1 = new cl::Image2D(
			*m_context, 
			CL_MEM_READ_WRITE,
			cl::ImageFormat(CL_INTENSITY, CL_FLOAT),
			maxResolution.x,
			maxResolution.y);
		m_im2 = new cl::Image2D(
			*m_context, 
			CL_MEM_READ_WRITE,
			cl::ImageFormat(CL_INTENSITY, CL_FLOAT),
			maxResolution.x,
			maxResolution.y);
		}catchCLError;
	}
	Ptr<cl::Context> getContext()
	{
		return m_context;
	}

	Ptr<cl::CommandQueue> getQueue()
	{
		return m_queue;
	}
	void setActiveBuffer(Ptr<ClBuffer2D> buffer)
	{
		m_activeIm = NULL;
		m_activeBuffer = buffer;
	}

	Ptr<ClBuffer2D> getActiveBuffer()
	{
		assert (m_activeIm.isNull());
		return m_activeBuffer;
	}

	void setActiveIm(Ptr<cl::Image2D> im)
	{
		m_activeBuffer = NULL;
		m_activeIm = im;
	}

	Ptr<cl::Image2D> getActiveIm()
	{
		assert(m_activeIm.isNull());
		return m_activeIm;
	}

	Ptr<cl::Image2D> getUnusedIm()
	{
		if(m_activeIm != m_im1)
			return m_im1;
		else
			return m_im2;
	}

	Ptr<ClBuffer2D> getUnusedBuffer()
	{
		if (m_activeBuffer != m_buffer1)
			return m_buffer1;
		else
			return m_buffer2;
	}
};