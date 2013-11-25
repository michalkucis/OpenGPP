#pragma once

#include "Base.h"
#include "Effect.h"
#include "GLClasses.h"
#include "GeneratorOfSequenceFilenames.h"
#include <ImfRgba.h>

/*
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "opencv_ffmpeg210d.lib")
#pragma comment(lib, "ml210d.lib")
#pragma comment(lib, "cv210d.lib")
#pragma comment(lib, "cxcore210d.lib")
#pragma comment(lib, "cvaux210d.lib")
#pragma comment(lib, "highgui210d.lib")
*/

class EffectSaveToSingleFileOpenCV: public Effect
{
	string m_filename;
public:
	EffectSaveToSingleFileOpenCV (string filename)
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
	Ptr<GLTexture2D> process(Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
};

class EffectSaveToSingleFileOpenEXR: public Effect
{
	string m_filename;
public:
	EffectSaveToSingleFileOpenEXR (string filename)
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
	Ptr<GLTexture2D> process (Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
};

template <typename Constructor = InputLoadFromSingleFileOpenCV>
class EffectSaveToSequence: public Effect
{
	GeneratorOfOutputSequenceFilenames m_filenameGenerator;
	int2 m_size;
public:
	EffectSaveToSequence<Constructor>(GeneratorOfOutputSequenceFilenames filenameGenerator):
		m_filenameGenerator(filenameGenerator)
	{

	}
	Ptr<GLTexture2D> process (Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
	{
		string filename = m_filenameGenerator.getFilename();
		m_filenameGenerator.next();
		Constructor saver (filename);
		return saver.process(tex, depth, envMap);
	}
};

struct CvVideoWriter;

class EffectSaveToVideoFileOpenCV: public Effect
{
	string m_filename, m_fourcc;
	float m_fps;
	int m_repeatFrame;
	//Ptr<cv::VideoWriter> m_wr;
	CvVideoWriter* m_writer;
public:
	EffectSaveToVideoFileOpenCV (string filename, float fps = 30, int repeatFrame = 1, string fourCC = "DIVX");
	virtual ~EffectSaveToVideoFileOpenCV ();
	Ptr<GLTexture2D> process (Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
};