#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv/cv.hpp>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "InputImplOpenCV.h"
#include "Error.h"
#include <ImfRgbaFile.h>
#include <ImfInputFile.h>
#include <ImathBox.h>

class Visitor2WriteUchar3BGRFromCvMat: public Visitor2WriteOnly<uchar3>
{
	cv::Mat m_mat;
public:
	Visitor2WriteUchar3BGRFromCvMat (cv::Mat& mat): m_mat(mat) {}
	uchar3 visitWrite (int2 n)
	{
		cv::Vec3b& matValue = m_mat.at<cv::Vec3b> (n.y, n.x);
		uchar3 u;
		u.z = matValue[0];
		u.y = matValue[1];
		u.x = matValue[2];
		return u;
	}
};

class Visitor2WriteFloat3FromCvMat: public Visitor2WriteOnly<float3>
{
	cv::Mat m_mat;
public:
	Visitor2WriteFloat3FromCvMat (cv::Mat& mat): m_mat(mat) {}
	float3 visitWrite (int2 n)
	{
		cv::Vec3f& matValue = m_mat.at<cv::Vec3f> (n.y, n.x);
		float3 u;
		u.x = matValue[0];
		u.y = matValue[1];
		u.z = matValue[2];
		return u;
	}
};

class Visitor2WriteFloatFromCvMat: public Visitor2WriteOnly<float>
{
	cv::Mat m_mat;
public:
	Visitor2WriteFloatFromCvMat (cv::Mat& mat): m_mat(mat) {}
	float visitWrite (int2 n)
	{
		float& matValue = m_mat.at<float> (n.y, n.x);
		return matValue;
	}
};

class Visitor2WriteFloat3FromEXR: public Visitor2WriteOnly<float3>
{
	Imf::Array2D<Imf::Rgba>& m_mat;
	int2 m_size;
public:
	Visitor2WriteFloat3FromEXR (Imf::Array2D<Imf::Rgba>& mat,int2 maxSize): m_mat(mat), m_size(maxSize) {}
	float3 visitWrite (int2 n)
	{
		Imf::Rgba& rgba = m_mat[n.y][n.x];
		float3 u;
		u.x = rgba.r;
		u.y = rgba.g;
		u.z = rgba.b;
		return u;
	}
};

class Visitor2WriteFloatFromEXR: public Visitor2WriteOnly<float>
{
	Imf::Array2D<float>& m_mat;
	int2 m_size;
public:
	Visitor2WriteFloatFromEXR (Imf::Array2D<float>& mat,int2 maxSize): m_mat(mat), m_size(maxSize) {}
	float visitWrite (int2 n)
	{
		float& alpha = m_mat[n.y][n.x];
		return alpha;
	}
};

int InputOpenCV::interpName( interp_t n )
{
	int cvInterp;
	switch (n)
	{
	case NEAREST:
		cvInterp = cv::INTER_NEAREST;
		break;
	case LINEAR:
		cvInterp = cv::INTER_LINEAR;
		break;
	case AREA:
		cvInterp = cv::INTER_CUBIC;
		break;
	case CUBIC:
		cvInterp = cv::INTER_AREA;
		break;
	case LANCZOS:
		cvInterp = cv::INTER_LANCZOS4;
		break;
	default:
		assert(0);
	}
	return cvInterp;
}


template <typename TEX_CLASS>
TEX_CLASS* InputOpenCV::createTextureFromMat( cv::Mat mat, int texInternalFormat, int texFormat, interp_t interp, int2 texSize )
{
	int width = mat.cols;
	int height = mat.rows;

	if (texSize.x%2)
		texSize.x--;
	if (texSize.y%2)
		texSize.y--;
	if (texSize.x && texSize.y)
	{
		cv::Mat matResized;
		cv::resize(mat,matResized, cv::Size(texSize.x,texSize.y), 0, 0, interpName(interp));
		mat = matResized;
		width = texSize.x;
		height = texSize.y;
	}
	if (width%2)
		width--;
	if (height%2)
		height--;

	TEX_CLASS* tex;
	if (texInternalFormat == GL_RGB)
	{
		Visitor2WriteUchar3BGRFromCvMat visitor(mat);
		tex = new TEX_CLASS(width, height, texInternalFormat, texFormat);
		tex->visitWriteOnly(&visitor);
	}
	else if (texInternalFormat == GL_RGB32F)
	{
		Visitor2WriteFloat3FromCvMat visitor(mat);
		tex = new TEX_CLASS(width, height, texInternalFormat, texFormat);
		tex->visitWriteOnly(&visitor);
	}
	else if (texInternalFormat == GL_DEPTH_COMPONENT32F)
	{
		Visitor2WriteFloatFromCvMat visitor(mat);
		tex = new TEX_CLASS(width, height, texInternalFormat, texFormat);
		tex->visitWriteOnly(&visitor);
	}
	else if (texInternalFormat == GL_R32F)
	{
		Visitor2WriteFloatFromCvMat visitor(mat);
		tex = new TEX_CLASS(width, height, texInternalFormat, texFormat);
		tex->visitWriteOnly(&visitor);
	}
	else
		assert(0);
	return tex;
}

Ptr<GLTexture2D> InputLoadFromSingleFileOpenCV::getProcessed()
{
	return m_texColor;
}

void InputLoadFromSingleFileOpenCV::init()
{
	if (! m_texColor.isNull())
		error(ERR_STRUCT);

	cv::Mat mat = cvLoadImage(m_filename.c_str());
	if (mat.data==NULL)
		error1(ERR_STRUCT, "File '%s' cannot be opened", m_filename.c_str());
	m_texColor = createTextureFromMat<GLTexture2D>(mat, GL_RGB, GL_RGB, m_interp, m_size);
}

void InputLoadFromSingleFileOpenEXR::readRgba(const string& filename, Imf::Array2D<Imf::Rgba> &pixels, int &width, int &height )
{
	Imf::RgbaInputFile file (filename.c_str());
	Imath::Box2i dw = file.dataWindow();
	width = dw.max.x - dw.min.x + 1;
	height = dw.max.y - dw.min.y + 1;
	pixels.resizeErase (height, width);
	file.setFrameBuffer (&pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);
	file.readPixels(dw.min.y, dw.max.y);
}

void InputLoadFromSingleFileOpenEXR::init()
{
	if (! m_texColor.isNull())
		error(ERR_STRUCT);

	try
	{
		int width, height;
		Imf::Array2D<Imf::Rgba> arrRgba;
		readRgba(m_filename, arrRgba, width, height);

		if (m_size.x && m_size.y)
		{
			cv::Mat mat(height, width, CV_32FC3);
			for (int y = 0; y < height; y++)
				for (int x = 0; x < width; x++)
				{
					Imf::Rgba& rgba = arrRgba[y][x];
					cv::Vec3f& v3f = mat.at<cv::Vec3f>(y,x);
					v3f[0] = rgba.r;
					v3f[1] = rgba.g;
					v3f[2] = rgba.b;
				}
			m_texColor = createTextureFromMat<GLTexture2D>(mat,GL_RGB32F,GL_RGB,LINEAR,m_size);
		}
		else
		{
			m_texColor = new GLTexture2D(width,height,GL_RGB32F,GL_RGB);
			Visitor2WriteFloat3FromEXR visitor(arrRgba, int2(width,height));
			m_texColor->visitWriteOnly(&visitor);
		}
	}
	catch (Error& error)
	{
		throw error;
	}
	catch (std::exception&)
	{
		string msg = string("Image '") + m_filename + string("' cannot be opened");
		error0 (ERR_STRUCT, msg);
	}
}

void InputLoadFromSingleFileOpenEXR::initDepth ()
{
	if (! m_texDepth.isNull())
		error(ERR_STRUCT);

	try
	{
		Imf::Array2D<float> arrPixels;
		int width, height;
		{
			Imf::InputFile file (m_filename.c_str());		
			Imath::Box2i dw = file.header().dataWindow();
			Imf::FrameBuffer frameBuffer;
			width = dw.max.x - dw.min.x + 1;
			height = dw.max.y - dw.min.y + 1;
			arrPixels.resizeErase (height, width);

			frameBuffer.insert ("Z",
				Imf::Slice (Imf::FLOAT, 
				(char*) (&(arrPixels[0][0]) - dw.min.x - dw.min.y * width),
				sizeof (arrPixels[0][0]) * 1,
				sizeof (arrPixels[0][0]) * width,
				1,
				1,
				0.0));

			file.setFrameBuffer (frameBuffer);
			file.readPixels (dw.min.y, dw.max.y);
		}

		if (m_size.x && m_size.y)
		{
			cv::Mat mat(height, width, CV_32F);
			for (int y = 0; y < height; y++)
				for (int x = 0; x < width; x++)
				{
					float& value = arrPixels[y][x];
					float& out = mat.at<float>(y,x);
					out = value;
				}
			m_texDepth = createTextureFromMat<GLTexture2D>(mat,GL_R32F,GL_RED,LINEAR,m_size);
		}
		else
		{
			m_texDepth = new GLTexture2D(width,height,GL_R32F, GL_RED);
			Visitor2WriteFloatFromEXR visitor(arrPixels, int2(width,height));
			m_texDepth->visitWriteOnly(&visitor);
		}
	} catch (Error& error)
	{
		throw error;
	}
	catch (std::exception&)
	{
		string msg = string("Image '") + m_filename + string("' cannot be opened");
		error0 (ERR_STRUCT, msg);
	}
}

Ptr<GLTexture2D> InputLoadFromSingleFileOpenEXR::getProcessed()
{
	return m_texColor;
}

Ptr<GLTexture2D> InputLoadFromSingleFileOpenEXR::getProcessedDepth()
{
	return m_texDepth;
}

void InputLoadFromSingleFileWithEnvMapOpenEXR::initEnvMap()
{
	if (! m_texEnvMap.isNull())
		error(ERR_STRUCT);

	try
	{
		int width, height;
		Imf::Array2D<Imf::Rgba> arrRgba;
		readRgba(m_filenameEnvMap, arrRgba, width, height);

		if (m_size.x && m_size.y)
		{
			cv::Mat mat(height, width, CV_32FC3);
			for (int y = 0; y < height; y++)
				for (int x = 0; x < width; x++)
				{
					Imf::Rgba& rgba = arrRgba[y][x];
					cv::Vec3f& v3f = mat.at<cv::Vec3f>(y,x);
					v3f[0] = rgba.r;
					v3f[1] = rgba.g;
					v3f[2] = rgba.b;
				}
				m_texEnvMap = createTextureFromMat<GLTextureEnvMap>(mat,GL_RGB32F,GL_RGB,LINEAR,m_size);
		}
		else
		{
			m_texEnvMap = new GLTextureEnvMap(width,height,GL_RGB32F,GL_RGB);
			Visitor2WriteFloat3FromEXR visitor(arrRgba, int2(width,height));
			m_texEnvMap->visitWriteOnly(&visitor);
		}
	}
	catch (Error& error)
	{
		throw error;
	}
	catch (std::exception&)
	{
		string msg = string("Image '") + m_filename + string("' cannot be opened");
		error0 (ERR_STRUCT, msg);
	}
}

Ptr<GLTextureEnvMap> InputLoadFromSingleFileWithEnvMapOpenEXR::getProcessedEnvMap()
{
	return m_texEnvMap;
}

void InputLoadFromVideoFileOpenCV::init()
{	
	m_capture = new cv::VideoCapture (m_filename);

	if (! m_capture->isOpened())
		error1(ERR_STRUCT, "Video '%s' cannot be opened", m_filename.c_str());

	next();
}

Ptr<GLTexture2D> InputLoadFromVideoFileOpenCV::getProcessed()
{
	return m_texLoaded;
}

void InputLoadFromVideoFileOpenCV::next()
{
	cv::Mat mat;
	*m_capture >> mat;
	if (mat.data == NULL)
	{
		init();
		*m_capture >> mat;
	}
	m_texLoaded = createTextureFromMat<GLTexture2D>(mat, GL_RGB, GL_RGB, m_interp, m_size);
}

void InputLoadFromVideoDeviceOpenCV::init()
{	
	m_capture = new cv::VideoCapture (m_device);

	if (! m_capture->isOpened())
		error1(ERR_STRUCT, "Video device '%d' cannot be opened", m_device);
}

Ptr<GLTexture2D> InputLoadFromVideoDeviceOpenCV::getProcessed()
{
	return m_texLoaded;
}

void InputLoadFromVideoDeviceOpenCV::next()
{
	cv::Mat mat;
	*m_capture >> mat;
	if (mat.data == NULL)
	{
		init();
		*m_capture >> mat;
	}
	m_texLoaded = createTextureFromMat<GLTexture2D>(mat, GL_RGB, GL_RGB, m_interp, m_size);
}
