#include <opencv/highgui.h>
//#include <ffopencv\ffopencv.h>

#pragma comment(lib, "cv210d.lib")
#pragma comment(lib, "cvaux210d.lib")
#pragma comment(lib, "cvhaartraining210d.lib")
#pragma comment(lib, "cxcore210d.lib")
#pragma comment(lib, "cxts210d.lib")
#pragma comment(lib, "highgui210d.lib")
#pragma comment(lib, "ml210d.lib") 

#include <windows.h>
#include <gl/GL.h>
#include "EffectImplOpenCV.h"
#include "Error.h"
#include <ImathBox.h>
#include <ImfRgbaFile.h>
//#include "OpenCV242Hack_ffmpeg_api.hpp"

class Visitor2ReadUchar3RGBToCvMat: public Visitor2ReadOnly<uchar3>
{
	cv::Mat m_mat;
public:
	Visitor2ReadUchar3RGBToCvMat (cv::Mat& mat): m_mat(mat) {}
	void visitRead (int2 n, const uchar3& texValue)
	{
		//uint2 matSize (m_mat.size[0], m_mat.size[1]);
		cv::Vec3b& matValue = m_mat.at<cv::Vec3b> (n.y,n.x);

		matValue[0] = texValue.z;
		matValue[1] = texValue.y;
		matValue[2] = texValue.x;
	}
};

class Visitor2ReadFloat3FromEXR: public Visitor2ReadOnly<float3>
{
	Imf::Rgba* m_arr;
	int2 m_size;
public:
	Visitor2ReadFloat3FromEXR (Imf::Rgba* arr,int2 maxSize): m_arr(arr), m_size(maxSize) {}
	void visitRead (int2 n, const float3& texValue)
	{
		Imf::Rgba& rgba = m_arr[n.y*m_size.x + n.x];
		
		rgba.r = texValue.x;
		rgba.g = texValue.y;
		rgba.b = texValue.z;
		rgba.a = 1;
	}
};

Ptr<GLTexture2D> EffectSaveToSingleFileOpenCV::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	glFinish();
	int width = tex->getResolution().x;
	int height = tex->getResolution().y;

	cv::Mat mat (height, width, CV_8UC3);
	Visitor2ReadUchar3RGBToCvMat visitor(mat);
	tex->visitReadOnly(&visitor);

	//cv::imwrite (m_filename, mat);
	try {
		cv::imwrite (m_filename, mat);
	} catch (cv::Exception& ex) {
		printf("");
	}
	return tex;
}

void writeRgba2 (string fileName,
	const Imf::Rgba *pixels,
	int width,
	int height)
{
}

Ptr<GLTexture2D> EffectSaveToSingleFileOpenEXR::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{		
	//printf("main: EffectSaveToSingleFileOpenEXR is waiting to OpenGL...\n");
	glFinish();
	//printf("main: EffectSaveToSingleFileOpenEXR is starting...\n");
	int width = tex->getResolution().x;
	int height = tex->getResolution().y;

	Imf::Rgba *pixels = new Imf::Rgba[width*height];
	Visitor2ReadFloat3FromEXR visitor(pixels, int2(width, height));
	tex->visitReadOnly(&visitor);

	Imf::RgbaOutputFile file (m_filename.c_str(), width, height, Imf::WRITE_RGBA);      // 1
	file.setFrameBuffer (pixels, 1, width);                         // 2
	file.writePixels (height);                                      // 3

	delete [] (pixels);
	//printf("main: EffectSaveToSingleFileOpenEXR is ending...\n");

	return tex;
}

EffectSaveToVideoFileOpenCV::EffectSaveToVideoFileOpenCV( string filename, float fps, int repeatFrame, string fourCC /*= ""*/ )
{
	m_filename = filename;
	m_fourcc = fourCC;
	m_fps = fps;
	m_repeatFrame = repeatFrame;
	m_writer = NULL;
}

EffectSaveToVideoFileOpenCV::~EffectSaveToVideoFileOpenCV()
{
	//if(m_writer)
	//	cvReleaseVideoWriter_FFMPEG(&m_writer);
}

Ptr<GLTexture2D> EffectSaveToVideoFileOpenCV::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	//try 
	//{
	//	glFinish();
	//
	//	int width = tex->getResolution().x;
	//	int height = tex->getResolution().y;
	//
	//	cv::Mat mat (height, width, CV_8UC3);
	//	Visitor2ReadUchar3RGBToCvMat visitor(mat);
	//	tex->visitReadOnly(&visitor);
	//
	//	if (width % 2)
	//		width--;
	//	if (height % 2)
	//		height--;
	//	if (! m_writer)
	//	{
	//		int fourCC = CV_FOURCC(m_fourcc[0], m_fourcc[1], m_fourcc[2], m_fourcc[3]);
	//		cv::Size frameSize (width, height);
	//		m_writer = cvCreateVideoWriter_FFMPEG(m_filename.c_str(),
	//			fourCC, m_fps, frameSize, true);
	//		if (! m_writer)
	//			error0 (ERR_OPENCV, "Video cannot be created");
	//	}
	//
	//	CvSize sizeFrame = cvSize(width, height);
	//	IplImage* iplimage = cvCreateImage (sizeFrame, IPL_DEPTH_8U, 3);
	//	for (int y = 0; y < height; y++)
	//		for (int x = 0; x < width; x++)
	//		{
	//			uchar3& out = CV_IMAGE_ELEM (iplimage, uchar3, y, x);
	//			cv::Vec3b& in = mat.at<cv::Vec3b> (y, x);
	//			out.x = (uchar) in[0];
	//			out.y = (uchar) in[1];
	//			out.z = (uchar) in[2];
	//			//out.x = 0;
	//			//out.y = 0;
	//			//out.z = 0;
	//		}
	//	for (int i = 0; i < m_repeatFrame; i++)
	//		cvWriteFrame_FFMPEG (m_writer, iplimage);
	//	cvReleaseImage(&iplimage);
	//} catch (cv::Exception& ex) {
	//	error1(ERR_OPENCV, "OpenCV error: %s", ex.what());
	//}
	
	return tex;
}