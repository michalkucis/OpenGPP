#pragma once

#include "OpenGPP.h"

int getOptimalStride(int width)
{
	int alignment = 32;
	return (width / alignment + (width%alignment ? 1 : 0)) * alignment;
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

	Ptr<ClBuffer2D> m_activeBuffer;
public:
	ClFacade(int2 maxResolution)
	{
		try{
			m_context = new cl::Context(::getCLGLContext());
			m_queue = new cl::CommandQueue(::getCLCommandQueue(*m_context));

			int2 bufferResolution (maxResolution);

			m_buffer1 = new ClBuffer2D(m_context, m_queue, bufferResolution, getOptimalStride(bufferResolution.x));
			m_buffer2 = new ClBuffer2D(m_context, m_queue, bufferResolution, getOptimalStride(bufferResolution.x));
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
		m_activeBuffer = buffer;
	}

	Ptr<ClBuffer2D> getActiveBuffer()
	{
		return m_activeBuffer;
	}

	Ptr<ClBuffer2D> getUnusedBuffer()
	{
		if (m_activeBuffer != m_buffer1)
			return m_buffer1;
		else
			return m_buffer2;
	}

	void setResolution(int2 res)
	{
		if (res != m_buffer1->getSize())
			m_buffer1 = new ClBuffer2D(m_context, m_queue, res, getOptimalStride(res.x));

		if (res != m_buffer2->getSize())
			m_buffer2 = new ClBuffer2D(m_context, m_queue, res, getOptimalStride(res.x));
	}
};