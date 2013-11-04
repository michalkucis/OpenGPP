#pragma once

#include "Input.h"
#include "GeneratorOfSequenceFilenames.h"
#include <ImfArray.h>
#include <ImfRgba.h>

#pragma comment(lib, "Half.lib")
#pragma comment(lib, "IlmImf.lib")


namespace Imf
{
	class InputFile;
};

namespace cv
{
	class Mat;
	class VideoCapture;
};

class InputOpenCV: public Input
{
public:
	enum interp_t
	{
		NEAREST = 1,
		LINEAR,
		AREA,
		CUBIC,
		LANCZOS
	};
protected:
	int interpName (interp_t n);
	template <typename TEX_CLASS>
	TEX_CLASS* createTextureFromMat (cv::Mat mat, int texInternalFormat, int texFormat, interp_t interp = LINEAR, int2 texSize = int2(0,0));
};

class InputLoadFromSingleFileOpenCV: public InputOpenCV
{
protected:
	Ptr<GLTexture2D> m_texColor;
	string m_filename;
	int2 m_size;
	interp_t m_interp;
public:
	InputLoadFromSingleFileOpenCV (string filename, int2 newSize = int2(0,0), interp_t method = LINEAR)
	{
		m_filename = filename;
		m_size = newSize;
		m_interp = method;
		init();
	}
protected:
	void init ();
public:
	void next () {}
	Ptr<GLTexture2D> getProcessed ();
};

class InputLoadFromSingleFileOpenEXR: public InputOpenCV
{
protected:
	Ptr<GLTexture2D> m_texColor;
	Ptr<GLTexture2D> m_texDepth;
	string m_filename;
	int2 m_size;
	interp_t m_interp;
public:
	InputLoadFromSingleFileOpenEXR (string filename, int2 size = int2(0,0), interp_t interp = LINEAR)
	{
		m_filename = filename;
		m_size = size;
		m_interp = interp;
		init();
		initDepth();
	}
protected:
	void init ();
	void initDepth ();
	void readRgba (const string& filename, Imf::Array2D<Imf::Rgba> &pixels, int &width, int &height);
public:
	void next () {}
	Ptr<GLTexture2D> getProcessed ();
	Ptr<GLTexture2D> getProcessedDepth();
	bool isAvailableDepth()
	{
		return true;
	}
};

class InputLoadFromSingleFileWithEnvMapOpenEXR: public InputLoadFromSingleFileOpenEXR
{
protected:
	Ptr<GLTextureEnvMap> m_texEnvMap;
	string m_filenameEnvMap;
	int2 m_sizeEnvMap;
	interp_t m_interpEnvMap;
public:
	InputLoadFromSingleFileWithEnvMapOpenEXR(
		string filename, int2 size = int2(0,0), interp_t interp = LINEAR,
		string filenameEnvMap = "", int2 sizeEnvMap = int2(0,0), interp_t interpEnvMap = LINEAR):
		InputLoadFromSingleFileOpenEXR(filename, size, interp)
	{
		m_sizeEnvMap = sizeEnvMap;
		m_interpEnvMap = interpEnvMap;
		m_filenameEnvMap = filenameEnvMap;
		initEnvMap();
	}
protected:
	void initEnvMap();
public:
	Ptr<GLTextureEnvMap> getProcessedEnvMap();
	bool isAvailableEnvMap()
	{
		return true;
	}
};

template <typename Constructor = InputLoadFromSingleFileOpenCV>
class InputLoadFromSequence: public InputOpenCV
{
protected:
	GeneratorOfInputSequenceFilenames m_filenameGenerator;
	int2 m_size;
	interp_t m_interp;
	Ptr<GLTexture2D> m_texLoaded;
	Ptr<GLTexture2D> m_texDepthLoaded;
public:
	InputLoadFromSequence<Constructor>(GeneratorOfInputSequenceFilenames filenameGenerator, 
		int2 newSize = int2(0,0), interp_t method = LINEAR):
	m_filenameGenerator(filenameGenerator), m_size(newSize), m_interp(method)
	{
		next ();
	}

	void next ()
	{
		string filename = m_filenameGenerator.getFilename();
		m_filenameGenerator.next();
		Constructor loader (filename, m_size, m_interp);
		m_texLoaded = loader.getProcessed();
		if(loader.isAvailableDepth())
			m_texDepthLoaded = loader.getProcessedDepth();
		else
			m_texDepthLoaded = NULL;
	}

	Ptr<GLTexture2D> getProcessed ()
	{
		return m_texLoaded;
	}

	bool isAvailableDepth()
	{
		return ! m_texDepthLoaded.isNull();
	}

	Ptr<GLTexture2D> getProcessedDepth ()
	{
		if(m_texDepthLoaded.isNull())
			error0(ERR_STRUCT, "Depth map is not available");
		return m_texDepthLoaded;
	}
};

template <typename Constructor = InputLoadFromSingleFileWithEnvMapOpenEXR>
class InputLoadFromSequenceWithEnvMap: public InputOpenCV
{
protected:
	GeneratorOfInputSequenceFilenames m_colorFilenameGenerator;
	int2 m_colorSize;
	interp_t m_colorInterp;
	GeneratorOfInputSequenceFilenames m_envMapFilenameGenerator;
	int2 m_envMapSize;
	interp_t m_envMapInterp;
	Ptr<GLTexture2D> m_texColorLoaded;
	Ptr<GLTexture2D> m_texDepthLoaded;
	Ptr<GLTextureEnvMap> m_texEnvMapLoaded;
public:
	InputLoadFromSequenceWithEnvMap<Constructor>(
		GeneratorOfInputSequenceFilenames colorFilenameGenerator, 
		int2 colorSize, interp_t colorMethod,
		GeneratorOfInputSequenceFilenames envMapFilenameGenerator,
		int2 envMapSize, interp_t envMapMethod):
	m_colorFilenameGenerator(colorFilenameGenerator), m_colorSize(colorSize), m_colorInterp(colorMethod),
	m_envMapFilenameGenerator(envMapFilenameGenerator), m_envMapSize(envMapSize), m_envMapInterp(envMapMethod)
	{
		next ();
	}

	void next ()
	{
		{
			string filename = m_colorFilenameGenerator.getFilename();
			m_colorFilenameGenerator.next();
			Constructor loader (filename, m_colorSize, m_colorInterp);
			m_texColorLoaded = loader.getProcessed();
			if(loader.isAvailableDepth())
				m_texDepthLoaded = loader.getProcessedDepth();
			else
				m_texDepthLoaded = NULL;
		}
		{
			string filename = m_envMapFilenameGenerator.getFilename();
			m_envMapFilenameGenerator.next();
			Constructor loader (filename, m_envMapSize, m_envMapInterp);
			m_texEnvMapLoaded = loader.getProcessedEnvMap();
		}
	}

	Ptr<GLTexture2D> getProcessed()
	{
		return m_texColorLoaded;
	}

	bool isAvailableDepth()
	{
		return ! m_texDepthLoaded.isNull();
	}

	Ptr<GLTexture2D> getProcessedDepth()
	{
		if(m_texDepthLoaded.isNull())
			error0(ERR_STRUCT, "Depth map is not available");
		return m_texDepthLoaded;
	}
	
	bool isAvailableEnvMap()
	{
		return ! m_texEnvMapLoaded.isNull();
	}

	Ptr<GLTextureEnvMap> getProcessedEnvMap()
	{
		return m_texEnvMapLoaded;
	}
};


class InputLoadFromVideoFileOpenCV: public InputOpenCV
{
	string m_filename;
	int2 m_size;
	interp_t m_interp;
	Ptr<cv::VideoCapture> m_capture;
	Ptr<GLTexture2D> m_texLoaded;
public:
	InputLoadFromVideoFileOpenCV (string filename, int2 newSize = int2(0,0), interp_t method = LINEAR)
	{
		m_filename = filename;
		m_size = newSize;
		m_interp = method;
		init();
		next();
	}
protected:
	void init ();
public:
	void next ();
	Ptr<GLTexture2D> getProcessed ();
};

class InputLoadFromVideoDeviceOpenCV: public InputOpenCV
{
	int m_device;
	int2 m_size;
	interp_t m_interp;
	Ptr<cv::VideoCapture> m_capture;
	Ptr<GLTexture2D> m_texLoaded;
public:
	InputLoadFromVideoDeviceOpenCV (int device, int2 newSize = int2(0,0), interp_t method = LINEAR)
	{
		m_device = device;
		m_size = newSize;
		m_interp = method;
		init();
		next();
	}
protected:
	void init ();
public:
	void next ();
	Ptr<GLTexture2D> getProcessed ();
};