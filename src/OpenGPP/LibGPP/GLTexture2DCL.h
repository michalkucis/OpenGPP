#pragma once
// GLTexture2DCL is not debugged

/* NEED DEBUG:
#include <ImfRgbaFile.h>

class GLTexture2DCL
{
	Ptr<GLTexture2D> m_tex;
	Ptr<cl::CommandQueue> m_clQueue;
	Ptr<cl::Context> m_clContext;
public:
	GLTexture2DCL(Ptr<GLTexture2D> tex, Ptr<EffectCLObjectsFactory> factoryCL)
	{
		m_tex = tex;
		m_clQueue = factoryCL->getCLCommandQueue();
		m_clContext = factoryCL->getCLGLContext();
	}

	template<typename visitorType>
	void visitReadOnlyTemplate (Visitor2ReadOnly<visitorType>* v)
	{
		try {
			glFlush();
			cl::Image2DGL image = getImage2DGL(*m_clContext,CL_MEM_READ_ONLY|CL_MEM_USE_HOST_PTR, m_tex);
			std::vector<cl::Memory> vecGLMems;
			vecGLMems.push_back(image);
			m_clQueue->enqueueAcquireGLObjects(&vecGLMems);

			int width = m_tex->getResolution().x;
			int height = m_tex->getResolution().y;
			cl::size_t<3> org;
			org[0] = 0; 
			org[1] = 0;
			org[2] = 0;
			cl::size_t<3> region;
			region[0] = 1;
			region[1] = 1;
			region[2] = 1;
			visitorType* ptr = new visitorType[width*height];
			m_clQueue->enqueueReadImage(image, CL_TRUE, org, region, 
				width*sizeof(visitorType), 0, ptr);
			int i = 0;
			for(int y = 0; y < height; y++)
				for(int x = 0; x < width; x++)
				{
					visitorType* item = &(ptr[i]);
					v->visitRead(int2(x,y), *item);
					i++;
				}

			delete[](ptr);
			m_clQueue->enqueueReleaseGLObjects(&vecGLMems);
			m_clQueue->flush();
		} catchCLError;
	}

	void visitReadOnly (Visitor2ReadOnly<uchar3>* v)
	{
		visitReadOnlyTemplate<uchar3> (v);
	}

	void visitReadOnly (Visitor2ReadOnly<float3>* v)
	{
		visitReadOnlyTemplate<float3> (v);
	}

	void visitReadOnly (Visitor2ReadOnly<float>* v)
	{
		visitReadOnlyTemplate<float> (v);
	}
};

class Visitor2ReadOnlyImfRgba: public Visitor2ReadOnly<float3>
{
	Imf::Rgba* m_memory;
	int2 m_size;

public:
	Visitor2ReadOnlyImfRgba(Imf::Rgba* memory, int2 size)
	{
		m_memory = memory;
		m_size = size;
	}

	void visitRead (int2 n, const float3& rgb)
	{
		Imf::Rgba rgba;
		rgba.r = rgb.x;
		rgba.g = rgb.y;
		rgba.b = rgb.z;
		rgba.a = 1;
		m_memory[n.x + n.y*m_size.x] = rgba;
	}
};

class EffectSaveExrUsingOpenCL: public Effect
{
	string m_filename;
	Ptr<EffectCLObjectsFactory> m_factoryCL;
public:
	EffectSaveExrUsingOpenCL(string filename, Ptr<EffectCLObjectsFactory> factoryCL)
	{
		m_filename = filename;
		m_factoryCL = factoryCL;
	}
	void setFilename (string filename)
	{
		m_filename = filename;
	}
	string getFilename ()
	{
		return m_filename;
	}    
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
	{		
		glFinish();

		int width = tex->getResolution().x;
		int height = tex->getResolution().y;
		GLTexture2DCL texCL(tex, m_factoryCL);
		Imf::Rgba* ptrRGBA = new Imf::Rgba[width*height];
		Visitor2ReadOnlyImfRgba visitor(ptrRGBA, int2(width, height));
		texCL.visitReadOnly(&visitor);

		Imf::RgbaOutputFile file (m_filename.c_str(), width, height, Imf::WRITE_RGBA);      // 1
		file.setFrameBuffer (ptrRGBA, 1, width);                         // 2
		file.writePixels (height);                                      // 3

		delete [] (ptrRGBA);

		return tex;
	}
};
*/
