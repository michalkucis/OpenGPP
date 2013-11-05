#include "outputsaver.h"

#include <opencv\ml.h>
#include <opencv\cxcore.hpp>
#include <opencv\highgui.h>
#include <opencv\highgui.hpp>
#include <ffopencv\ffopencv.h>

#include <ImfRgbaFile.h>

class OutputSaverAbstract
{
protected:
	PtrXMLoutput m_xmloutput;
	bool m_end;
	int m_seqActual;

	void saveFixed (PtrHDRImage3f image, string filename)
	{
		cv::Mat mat (image->getSize().y, image->getSize().x, CV_32FC3);
		uint2 size = image->getSize();
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
			{
				float3 inpix = image->getPixel (uint2(x,y)) * image->getMult();
				float3& outpix = mat.at<float3> (y, x);

				outpix.x = inpix.z;
				outpix.y = inpix.y;
				outpix.z = inpix.x;
				outpix *= 255;
			}
		cv::imwrite (filename, mat);
	}
	void saveExr (PtrHDRImage3f image, string filename)
	{
		uint2 size = image->getSize();
		Imf::RgbaOutputFile file (filename.c_str(), size.x, size.y, Imf::WRITE_RGBA);
		Imf::Rgba* arrRgba = new Imf::Rgba [size.getArea()];
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
			{
				float3 inpix = image->getPixel (uint2(x,y)) * image->getMult();
				Imf::Rgba& rgba = arrRgba [x + y*size.x];

				rgba.r = inpix.x;
				rgba.g = inpix.y;
				rgba.b = inpix.z;
				rgba.a = 1;
			}
		file.setFrameBuffer (arrRgba, 1, size.x);
		file.writePixels (size.y);
	}
public:
	OutputSaverAbstract (string filename, string outputName)
	{		
		m_end = false;
		XMLparser parser (filename);
		m_xmloutput = parser.parseOutput(outputName);
		
		m_seqActual = m_xmloutput->m_isSequence ? m_xmloutput->m_sequence.start : 0;
	}
	virtual ~OutputSaverAbstract () { }
	virtual bool isWndVisibled ()
	{
		return m_xmloutput->m_wndVisibled;
	}
	virtual int2 getWndSize ()
	{
		return m_xmloutput->m_wndSize;
	}
	virtual void next ()
	{
	}
	virtual void save (PtrHDRImage3f image) = 0;
	virtual bool isEnd ()
	{
		return m_end;
	}

	PtrXMLoutput getXMLoutput() 
	{
		return m_xmloutput;
	}
};

class OutputSaverImage: public OutputSaverAbstract
{
public:
	OutputSaverImage (string filename, string outputName): 
		OutputSaverAbstract (filename, outputName)
	{	
	}
	void save (PtrHDRImage3f image)
	{
		if (isEnd ())
			return;

		if (m_xmloutput->m_imageType == XMLoutput::FIXED)
			saveFixed (image,  m_xmloutput->m_singleImageFilename);
		else if (m_xmloutput->m_imageType == XMLoutput::EXR)
			saveExr (image,  m_xmloutput->m_singleImageFilename);
			
		m_end = true;
	}
};

class OutputSaverSequence: public OutputSaverAbstract
{
public:
	OutputSaverSequence (string filename, string outputName): 
		OutputSaverAbstract (filename, outputName)
	{	
	}
	void save (PtrHDRImage3f image)
	{
		if (isEnd ())
			return;

		char buffer [1024];
		sprintf (buffer, m_xmloutput->m_sequence.filename.c_str(), m_seqActual);
		string filename = buffer;
		if (m_xmloutput->m_imageType == XMLoutput::FIXED)
			saveFixed (image, filename);
		else if (m_xmloutput->m_imageType == XMLoutput::EXR)
			saveExr (image, filename);

		m_end = m_end || m_seqActual == m_xmloutput->m_sequence.end;
		m_seqActual += m_xmloutput->m_sequence.step;
	}
};

class OutputSaverVideoOpencv: public OutputSaverAbstract
{
	//boost::shared_ptr<cv::VideoWriter> m_vw;
	string m_filename, m_fourcc;
	float m_fps;
	int m_repeatFrame;

	CvVideoWriter* m_wr;
public:
	OutputSaverVideoOpencv (string filename, string outputName): 
		OutputSaverAbstract (filename, outputName)
	{	
		XMLparser parser (filename);
		PtrXMLoutput output (parser.parseOutput(outputName));
		m_filename = output->m_videoFilename;
		m_fourcc = output->m_videoFourcc;
		m_fps = output->m_videoFPS;
		m_repeatFrame = output->m_videoRepeatOneFrame;

		m_wr = NULL;
	}
	virtual ~OutputSaverVideoOpencv ()
	{
		cvReleaseVideoWriter_FFMPEG (&m_wr);
	}
	void save (PtrHDRImage3f image)
	{
		cv::Size size = image->getSize().getCvSize();
		if (size.height % 2)
			size.height--;
		if (size.width % 2)
			size.width--;
		if (! m_wr)
		{
			const char* sz4cc = m_fourcc.c_str();
			try 
			{
				m_wr = cvCreateVideoWriter_FFMPEG (m_filename.c_str(),
					CV_FOURCC (sz4cc[0], sz4cc[1], sz4cc[2], sz4cc[3]), m_fps, size, true);
			} catch (cv::Exception& ex)
			{
				throw XMLexceptionInvalid ("","",ex.msg);
			}
		}
		IplImage* iplimage = cvCreateImage (size, IPL_DEPTH_8U, 3);
		for (int y = 0; y < size.height; y++)
			for (int x = 0; x < size.width; x++)
			{
				uchar3& out = CV_IMAGE_ELEM (iplimage, uchar3, y, x);
				float3& in = image->getPixel (uint2 (x, y)) * image->getMult();
				out.x = (uchar) (clamp(in.z) * 255);
				out.y = (uchar) (clamp(in.y) * 255);
				out.z = (uchar) (clamp(in.x) * 255);
				//out.x = 0;
				//out.y = 0;
				//out.z = 0;			
			
			}
		for (int i = 0; i < m_repeatFrame; i++)
			cvWriteFrame_FFMPEG (m_wr, iplimage);
		cvReleaseImage (&iplimage);
	}
};

OutputSaver::OutputSaver (std::string filename, std::string outputName)
{
	XMLparser parser (filename);
	PtrXMLoutput output (parser.parseOutput(outputName));
	if (output->m_isSingleImageOutput)
		m_delegator = new OutputSaverImage (filename, outputName);
	else if (output->m_isSequence)
		m_delegator = new OutputSaverSequence (filename, outputName);
	else if (output->m_isVideo && output->m_videoLib=="opencv")
		m_delegator = new OutputSaverVideoOpencv (filename, outputName);
	else
		assert (0);
}
OutputSaver::~OutputSaver ()
{
	delete m_delegator;
}
void OutputSaver::save (PtrHDRImage3f image)
{
	m_delegator->save (image);
}
bool OutputSaver::isWndVisibled()
{
	return m_delegator->isWndVisibled();
}
int2 OutputSaver::getWndSize ()
{
	return m_delegator->getWndSize ();
}
bool OutputSaver::isEnd ()
{
	return m_delegator->isEnd ();
}

PtrXMLoutput OutputSaver::getXMLoutput()
{
	return m_delegator->getXMLoutput ();
}


