#pragma warning (disable: 4996)
#include "inputloader.h"
#include <opencv\cv.h>
#include <opencv\highgui.h>
#include <ffopencv\ffopencv.h>
#include <ImfArray.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <SDL.h>
#undef main
#include <list>
#include "xml.h"
#include "sdl.h"
#include "hdrimage.h"
#include "hdrimagefunctor.h"


class InputLoaderAbstract
{
protected:
	float* getChannel (string channel, HDRImage3f* p)
	{
		if (channel == "red")
			return p->getRedBuffer ();
		else if (channel == "green")
			return p->getGreenBuffer ();
		else if (channel == "blue")
			return p->getBlueBuffer ();
		else
			throw XMLexception ("","","", "Unknown channel name");
	}

	void loadColorStd (XMLinputFile& input, HDRImage3f& file, PtrHDRImage3f& color)
	{
		if (! color.get())
		{
			color = PtrHDRImage3f (new HDRImage3f (file.getSize()));
			color->zeroesBlock (uint2 (0,0), color->getSize()-uint2(1,1));
		}
		if (file.getSize() != color->getSize())
			throw XMLexception ("", "", "Invalid size of the input images");

		uint2 size = file.getSize();
		uint2 total = file.getTotalSize ();
		if (input.red != "")
		{
			float* p = getChannel (input.red, &file);
			for (uint y = 0; y < size.y; y++)
			{
				float* dst = color->getRedBuffer (uint2 (0, y));
				memcpy (dst, p + total.x*y, size.x*sizeof(float));
			}
		}

		if (input.green != "")
		{
			float* p = getChannel (input.green, &file);
			for (uint y = 0; y < size.y; y++)
			{
				float* dst = color->getGreenBuffer (uint2 (0, y));
				memcpy (dst, p + total.x*y, size.x*sizeof(float));
			}
		}

		if (input.blue != "")
		{
			float* p = getChannel (input.blue, &file);
			for (uint y = 0; y < size.y; y++)
			{
				float* dst = color->getBlueBuffer (uint2 (0, y));
				memcpy (dst, p + total.x*y, size.x*sizeof(float));
			}
		}
	}

	void loadDepthStd (XMLinputFile& input, HDRImage3f& file, PtrHDRImage1f& depth)
	{
		if (! depth.get())
		{
			depth = PtrHDRImage1f (new HDRImage1f (file.getSize()));
			depth->zeroesBlock (uint2 (0,0), depth->getSize()-uint2(1,1));
		}

		if (file.getSize() != depth->getSize())
			throw XMLexception ("", "", "Invalid size of the input images");

		uint2 size = file.getSize();
		uint2 total = file.getTotalSize ();
		if (input.depth != "")
		{
			float* p = getChannel (input.depth, &file);
			for (uint y = 0; y < size.y; y++)
			{
				float* dst = depth->getBuffer (uint2 (0, y));
				memcpy (dst, p + total.x*y, size.x*sizeof(float));
			}
		}
	}

	void loadEnvmapStd (XMLinputFile& input, HDRImage3f& file, PtrHDRImage3f& envmap)
	{
		if (! envmap.get())
		{
			envmap = PtrHDRImage3f (new HDRImage3f (file.getSize()));
			envmap->zeroesBlock (uint2 (0,0), envmap->getTotalSize()-uint2(1,1));
		}
		if (file.getSize() != envmap->getSize())
			throw XMLexception ("", "", "Invalid size of the input images");

		uint2 size = file.getSize();
		uint2 total = file.getTotalSize ();
		if (input.envred != "")
		{
			float* p = getChannel (input.envred, &file);
			for (uint y = 0; y < size.y; y++)
			{
				float* dst = envmap->getRedBuffer (uint2 (0, y));
				memcpy (dst, p + total.x*y, size.x*sizeof(float));
			}
		}

		if (input.envgreen != "")
		{
			float* p = getChannel (input.envgreen, &file);
			for (uint y = 0; y < size.y; y++)
			{
				float* dst = envmap->getGreenBuffer (uint2 (0, y));
				memcpy (dst, p + total.x*y, size.x*sizeof(float));
			}
		}

		if (input.envblue != "")
		{
			float* p = getChannel (input.envblue, &file);
			for (uint y = 0; y < size.y; y++)
			{
				float* dst = envmap->getBlueBuffer (uint2 (0, y));
				memcpy (dst, p + total.x*y, size.x*sizeof(float));
			}
		}
	}

	void load (PtrXMLinput input, PtrHDRImage3f& color, PtrHDRImage1f& depth, PtrHDRImage3f& envmap)
	{
		for (uint i = 0; i < input->files.size(); i++)
		{
			XMLinputFile file = (input->files)[i];
			bool isColor = (file.red!="" || file.green!="" || file.blue!="");
			bool isEnvMap = (file.envred!="" || file.envgreen!="" || file.envblue!="");

			if (file.type == XMLinputFile::TYPE_STANDARD)
			{
				HDRImage3f image;
				if (! image.loadFromFile (file.path.c_str()))
				{
					string desc = file.path + " is not found";
					throw XMLexception ("", "", "", desc);
				}
				if (isColor)
					loadColorStd (file, image, color);
				if (file.depth != "")
					loadDepthStd (file, image, depth);
				if (isEnvMap)
					loadEnvmapStd (file, image, envmap);
			}
			else if (file.type == XMLinputFile::TYPE_EXR)
			{
				loadExr (file, color, depth, envmap);
			}
		}

		if (! color.get())
			throw XMLexception ("", "", "", "Input color image is not defined");


		// multiply color image:
		HDRIFunctorMult functorMult (input->colorMult);
		color->applyFunctor (functorMult);

		// depth:
		if (depth.get())
		{
			HDRIFunctorDepth functor (input.get(), depth->getSize());
			depth->applyFunctor (functor);
		}
	}

	void copyExr (Imf::Array2D<float>& input, uint2 arrSize, uint i, PtrHDRImage3f& color, PtrHDRImage1f& depth, PtrHDRImage3f& envmap)
	{
		uint2 size;
		uint2 total;

		// init resolution:
		switch (i)
		{
		case 0:
		case 1:
		case 2:
			if (! color.get())
				color = PtrHDRImage3f (new HDRImage3f (arrSize));
			if (color->getSize() != arrSize)
				throw XMLexception ("","", "input image resolutions are not equal");
			size = color->getSize();
			total = color->getTotalSize();
			break;
		case 3:
			if (! depth.get())
				depth = PtrHDRImage1f (new HDRImage1f (arrSize));
			if (depth->getSize() != arrSize)
				throw XMLexception ("","", "input image resolutions are not equal");
			size = depth->getSize();
			total = depth->getTotalSize();
			break;
		case 4:
		case 5:
		case 6:
			if (! envmap.get())
				envmap = PtrHDRImage3f (new HDRImage3f (arrSize));
			if (envmap->getSize() != arrSize)
				throw XMLexception ("","", "input image resolutions are not equal");
			size = envmap->getSize();
			total = envmap->getTotalSize();
		}

		// get buffer:
		float* p;
		switch (i)
		{
		case 0:
			p = color->getRedBuffer ();
			break;
		case 1:
			p = color->getGreenBuffer ();
			break;
		case 2:
			p = color->getBlueBuffer ();
			break;
		case 3:
			p = depth->getBuffer ();
			break;
		case 4:
			p = envmap->getRedBuffer ();
			break;
		case 5:
			p = envmap->getGreenBuffer ();
			break;
		case 6:
			p = envmap->getBlueBuffer ();
			break;
		}

		// copy:
		for (uint y = 0; y < size.y; y++)
		{
			float* src = input[y];
			memcpy (p, src, size.x * sizeof(float));
			p += total.x;
		}
	}

	void loadExr (XMLinputFile& input, PtrHDRImage3f& color, PtrHDRImage1f& depth, PtrHDRImage3f& envmap)
	{
		try
		{
			Imf::InputFile file (input.path.c_str());		
			Imath::Box2i dw = file.header().dataWindow();
			int width = dw.max.x - dw.min.x + 1;
			int height = dw.max.y - dw.min.y + 1;

			for (int i = 0; i < 7; i++)
			{
				HDRImage1f image;
				string s; 
				switch (i) {
				case 0: s=input.red; break;
				case 1: s=input.green; break;
				case 2: s=input.blue; break;
				case 3: s=input.depth; break;
				case 4: s=input.envred; break;
				case 5: s=input.envgreen; break;
				case 6: s=input.envblue; break;
				}
		
				//if (i >= 4)
				//	break;
				if (s == "")
					continue;
				Imf::Array2D<float> pixels (height, width);
				Imf::FrameBuffer frameBuffer;
				frameBuffer.insert (s.c_str(),
						Imf::Slice (Imf::FLOAT, 
								(char*) (&(pixels[0][0]) - dw.min.x - dw.min.y * width),
								sizeof (pixels[0][0]) * 1,
								/*sizeof (pixels[0][0]) * width)*/
								((char*) (pixels[1])) - ((char*) (pixels[0])) ));
		
				file.setFrameBuffer (frameBuffer);
				file.readPixels (dw.min.y, dw.max.y);
				uint2 arrSize (width, height);
				copyExr (pixels, arrSize, i, color, depth, envmap);
			}
		} catch (std::exception&)
		{
			string msg = input.path + string(" is invalid exr file");

			throw XMLexception ("","",msg.c_str());
		}
	}
	PtrHDRImage3f m_color, m_envmap;
	PtrHDRImage1f m_depth;
	int m_numIter;
	PtrXMLinput m_ptrXML;
public:
	InputLoaderAbstract (PtrXMLinput input)
	{
		m_numIter = 0;
		m_ptrXML = input;
	}
	virtual ~InputLoaderAbstract ()
	{
	}
	bool isSingleImageInput ()
	{
		return (m_ptrXML->type == XMLinput::SINGLE);
	}
	int getNumIteration ()
	{
		return m_numIter;
	}
	virtual PtrHDRImage3f getColor ()
	{
		return m_color;
	}
	virtual PtrHDRImage1f getDepth ()
	{
		if (! isDepth ())
			XMLexception ("", "depth is not loaded");
		return m_depth;
	}
	virtual PtrHDRImage3f getEnvmap ()
	{		
		if (! isEnvmap ())
			XMLexception ("", "environment map is not loaded");
		return m_envmap;
	}
	virtual bool isDepth ()
	{
		return (NULL != m_depth.get());
	}
	virtual bool isEnvmap ()
	{
		return (NULL != m_envmap.get());
	}
	virtual bool isEnd () = 0
	{
	}
	virtual void next ()
	{
		m_numIter++;
	}
};





class InputLoaderSingle: public InputLoaderAbstract
{
public:
	InputLoaderSingle (PtrXMLinput input):
		InputLoaderAbstract (input)
	{
		PtrXMLinput xmlinput = input->clone();
		xmlinput->type = XMLinput::SINGLE;
		for (uint i = 0; i < xmlinput->files.size(); i++)
		{
			XMLinputFile& file = xmlinput->files[i];
			char buffer [1024];
			sprintf (buffer, file.path.c_str(), xmlinput->sequence.start);
			file.path = buffer;
		}
		load (xmlinput, m_color, m_depth, m_envmap);
	}
	bool isEnd ()
	{
		return false;
	}
};
class InputLoaderSequence: public InputLoaderAbstract
{
public:
	InputLoaderSequence (PtrXMLinput input):
		InputLoaderAbstract (input)
	{
		PtrXMLinput xmlinput = input->clone();
		xmlinput->type = XMLinput::SINGLE;
		for (uint i = 0; i < xmlinput->files.size(); i++)
		{
			XMLinputFile& file = xmlinput->files[i];
			char buffer [1024];
			sprintf (buffer, file.path.c_str(), xmlinput->sequence.start);
			file.path = buffer;
		}
		load (xmlinput, m_color, m_depth, m_envmap);
	}
	bool isEnd ()
	{
		int nElemsEachCycle = (m_ptrXML->sequence.end - m_ptrXML->sequence.start+1) / m_ptrXML->sequence.step;
		int nElems = nElemsEachCycle * m_ptrXML->sequence.repeat;
		return m_numIter >= nElems;
	}
	void next ()
	{
		m_numIter++;
		if (isEnd())
			return;
		int nElemsEachCycle = (m_ptrXML->sequence.end - m_ptrXML->sequence.start+1) / m_ptrXML->sequence.step;
		int nElems = nElemsEachCycle * m_ptrXML->sequence.repeat;
		int num = m_ptrXML->sequence.start + m_ptrXML->sequence.step*(m_numIter % nElemsEachCycle);

		PtrXMLinput xmlinput = m_ptrXML->clone();
		xmlinput->type = XMLinput::SINGLE;
		for (uint i = 0; i < xmlinput->files.size(); i++)
		{
			XMLinputFile& file = xmlinput->files[i];
			char buffer [1024];
			sprintf (buffer, file.path.c_str(), num);
			file.path = buffer;
		}

		load (xmlinput, m_color, m_depth, m_envmap);
	}
};
class InputLoaderList: public InputLoaderAbstract
{
	std::list<PtrXMLinput> m_listXMLinputs;
	PtrInputLoader m_inputloaderFromList;
public:
	InputLoaderList (PtrXMLinput input):
		InputLoaderAbstract (input)
	{
		for (uint i = 0; i < input->listRefs.size(); i++)
			m_listXMLinputs.push_back (input->listRefs[i]);

		next();
	}
	PtrHDRImage3f getColor ()
	{
		return m_inputloaderFromList->getColor();
	}
	PtrHDRImage1f getDepth ()
	{
		return m_inputloaderFromList->getDepth();
	}
	PtrHDRImage3f getEnvmap ()
	{		
		return m_inputloaderFromList->getEnvmap();
	}
	bool isDepth ()
	{
		return (!m_inputloaderFromList->isEnd()) && m_inputloaderFromList->isDepth();
	}
	bool isEnvmap ()
	{
		return (!m_inputloaderFromList->isEnd()) && m_inputloaderFromList->isEnvmap();
	}
	bool isEnd ()
	{
		return (m_listXMLinputs.empty()) && (m_inputloaderFromList.get() == NULL);
	}
	void next ()
	{
		m_inputloaderFromList = sharedNull<InputLoader>();
		while (! isEnd())
		{
			if ((!m_inputloaderFromList.get()) || m_inputloaderFromList->isEnd() 
				|| (m_inputloaderFromList->isSingleImageInput() && m_inputloaderFromList->getNumIteration()!=0))
			{
				PtrXMLinput front = m_listXMLinputs.front();
				m_listXMLinputs.pop_front ();
				m_inputloaderFromList = sharedNew<InputLoader> (front);
			}
			else
			{
				m_inputloaderFromList->next();
				if (!m_inputloaderFromList->isEnd())
					return;
			}
		}
	}
};
//class InputLoaderVideoOpencv: public InputLoaderAbstract
//{
//	cv::VideoCapture m_capture;
//	float m_colorMult;
//	bool m_end;
//	void loadColorImage ()
//	{
//		m_end = m_end || ! m_capture.grab ();
//		if (m_end)
//			return;
//		
//		cv::Mat mat;
//		m_capture.retrieve (mat);
//		cv::Size cvsize = mat.size();
//		uint2 size = uint2(cvsize.width, cvsize.height);
//		m_color = sharedNew<HDRImage3f> (size);
//		for (uint n = 0; n < 3; n++)
//		{
//			m_capture.retrieve (mat, n);
//			for (uint y = 0; y < size.y; y++)
//				for (uint x = 0; x < size.x; x++)
//				{
//					uchar u = mat.at<uchar> (y, x);
//					float* f = m_color->getBuffer(uint2(x,y), n);
//					*f = ((float)u) * m_colorMult / 255.0f;
//				}
//		}
//	}
//public:
//	InputLoaderVideoOpencv (PtrXMLinput input):
//		InputLoaderAbstract (input)
//	{
//		if (! m_capture.open (input->videoColorPath))
//			throw XMLexceptionInvalid ("","", "input(video)\\color path");
//		m_end = false;
//		m_colorMult = input->videoColorMult;
//		loadColorImage ();
//	}
//	void next ()
//	{
//		loadColorImage ();
//	}
//	bool isEnd ()
//	{
//		return m_end;
//	}
//};
class InputLoaderVideoOpencv: public InputLoaderAbstract
{
	CvCapture* m_capture;
	float m_colorMult;
	bool m_end;
	void loadColorImage ()
	{
		if (m_end)
			return;
		IplImage* img = 0; 
		if(!cvGrabFrame_FFMPEG(m_capture))
		{
			m_end = true;
			return;
		}

		img=cvRetrieveFrame_FFMPEG(m_capture, 1);
		
		uint2 size (img->width, img->height);
		if (! m_color.get())
			m_color = sharedNew<HDRImage3f> (size);
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
			{
				uchar3& in = CV_IMAGE_ELEM (img, uchar3, y, x);
				float3 out;
				out.x = ((uchar) in.z) / 255.0f;
				out.y = ((uchar) in.y) / 255.0f;
				out.z = ((uchar) in.x) / 255.0f;

				m_color->setPixel (uint2 (x, y), out);
			}
	}
public:
	InputLoaderVideoOpencv (PtrXMLinput input):
		InputLoaderAbstract (input)
	{
		m_capture = cvCreateFileCapture_FFMPEG (input->videoColorPath.c_str());
		if (! m_capture)
			throw XMLexceptionInvalid ("","", "input(video)\\color path");
		m_end = false;
		m_colorMult = input->videoColorMult;
		loadColorImage ();
	}
	~InputLoaderVideoOpencv ()
	{
		cvReleaseCapture_FFMPEG (&m_capture);
	}
	void next ()
	{
		loadColorImage ();
	}
	bool isEnd ()
	{
		return m_end;
	}
};


//class InputLoaderVideoFfmpeg: public InputLoaderAbstract
//{
//	float m_colorMult;
//	bool m_end;
//
//public:
//	void pgm_save (unsigned char* buf, int wrap, int xsize, int ysize, const char* filename)
//	{
//		FILE* f = fopen (filename, "w");
//		fprintf (f, "P5\n%d %d\n %d\n", xsize, ysize*3, 255);
//		for (int i = 0; i < ysize; i++)
//			fwrite (buf + i * wrap*3, 1, wrap*3, f);
//		fclose (f);
//	}
//void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
//  FILE *pFile;
//  char szFilename[32];
//  int  y;
//  
//  // Open file
//  sprintf(szFilename, "frame%d.ppm", iFrame);
//  pFile=fopen(szFilename, "wb");
//  if(pFile==NULL)
//    return;
//  
//  // Write header
//  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
//  
//  // Write pixel data
//  for(y=0; y<height; y++)
//    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
//   
//  // Close file
//  fclose(pFile);
//}
//	InputLoaderVideoFfmpeg (PtrXMLinput input):
//		InputLoaderAbstract (input)
//	{
//		m_end = false;
//		m_colorMult = input->videoColorMult;
//
//		static bool s_initialized = false;
//		if (! s_initialized)
//		{
//			av_register_all();
//			avcodec_register_all();
//			s_initialized = true;
//		}
//
//		AVFormatContext* pFormatCtx = avformat_alloc_context();
//		avformat_open_input(&pFormatCtx, input->videoColorPath.c_str(), NULL, NULL);
//		if (0 > avformat_find_stream_info(pFormatCtx, NULL))
//			throw XMLexceptionInvalid("","","Could't find stream information");
//
//		int nVideoStream = -1;
//		for (uint i = 0; i < pFormatCtx->nb_streams; i++)
//		{
//			AVCodecContext* enc = pFormatCtx->streams[i]->codec;
//			if (enc->codec_type == AVMEDIA_TYPE_VIDEO)
//			{
//				nVideoStream = i;
//				break;
//			}
//		}
//		if (nVideoStream == -1)
//			throw XMLexceptionInvalid("","","Could't find video stream");
//
//		AVCodecContext* pCodecCtx = pFormatCtx->streams[nVideoStream]->codec;
//		AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
//		if (0 > avcodec_open(pCodecCtx, pCodec))
//			throw XMLexceptionInvalid("","","Video codec cannot be opened");
//		
//		int ret;
//		while (true) 
//		{
//			AVPacket* pkt = new AVPacket;
//			ret = av_read_frame(pFormatCtx, pkt);
//			if (ret < 0) 
//			{
//				delete pkt;
//				break;
//			}
//			if (pkt->stream_index == nVideoStream) 
//			{
//				AVFrame* frame = avcodec_alloc_frame ();
//				AVFrame* frameRGB = avcodec_alloc_frame ();
//				int got_picture = 1;
//				int len = avcodec_decode_video2(pCodecCtx, frame, &got_picture, pkt); 
//				if (len < 0)
//					throw XMLexceptionInvalid ("","","Error while decoding frame");
//				if (got_picture)
//				{
//					uint dstX = pCodecCtx->width;
//					uint dstY = pCodecCtx->height;
//					
//
//					static int num = 0;
//					SaveFrame (frameRGB, frameRGB->width, frameRGB->height, num++);
///*					int numBytes=avpicture_get_size(PIX_FMT_RGB24, dstX, dstY);
//					uint8_t *buffer = (uint8_t*) av_malloc(numBytes*sizeof(uint8_t));
//					avpicture_fill ((AVPicture*) frameRGB, buffer, PIX_FMT_RGB24, dstX, dstY);
//					av_free (buffer);
//					
//
//					SwsContext* imgConvertCtx = sws_getContext (dstX, dstY, pCodecCtx->pix_fmt, 
//						dstX, dstY, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
//					if (imgConvertCtx == NULL)
//						throw XMLexceptionInvalid ("", "", "Could not initialize conversion context.");
//
//					sws_scale (imgConvertCtx, frame->data, frame->linesize, 0, pCodecCtx->height, frameRGB->data, frameRGB->linesize);
//					sws_freeContext (imgConvertCtx);
//					static int num = 0;
//					SaveFrame (frameRGB, frameRGB->width, frameRGB->height, num++);
//					/*uchar* data = frame->data[0];
//					int linesize = frame->linesize[0];
//					int2 size (pCodecCtx->width, pCodecCtx->height);
//
//					static std::string filename = ".pgm";
//					filename = std::string("a") + filename; 
//					pgm_save(data, linesize, size.x, size.y, filename.c_str());
//				*/
//				}
//				
//				avcodec_free_frame (&frame);
//				avcodec_free_frame (&frameRGB);
//			} 
//
//			av_free_packet(pkt);
//			delete pkt;
//		}
//		//avcodec_close(pCodec);
//		avformat_free_context(pFormatCtx);
//
//	}
//	~InputLoaderVideoFfmpeg ()
//	{
//	}
//	void next ()
//	{
//		//loadColorImage ();
//	}
//	bool isEnd ()
//	{
//		return m_end;
//	}
//};


InputLoader::InputLoader (PtrXMLinput input)
{
	if (input->type == XMLinput::SINGLE)
		m_il = new InputLoaderSingle (input);
	else if (input->type == XMLinput::SEQUENCE)
		m_il = new InputLoaderSequence (input);
	else if (input->type == XMLinput::LIST)
		m_il = new InputLoaderList (input);
	else if (input->type == XMLinput::VIDEO && input->videoLib == "opencv")
		m_il = new InputLoaderVideoOpencv (input);
	//else if (input->type == XMLinput::VIDEO && input->videoLib == "ffmpeg")
	//	m_il = new InputLoaderVideoFfmpeg (input);	
	else
		assert (0);
}
InputLoader::InputLoader (std::string filename, std::string inputName)
{
	XMLparser parser (filename);
	PtrXMLinput input (parser.parseInput(inputName));
	if (input->type == XMLinput::SINGLE)
		m_il = new InputLoaderSingle (input);
	else if (input->type == XMLinput::SEQUENCE)
		m_il = new InputLoaderSequence (input);
	else if (input->type == XMLinput::LIST)
		m_il = new InputLoaderList (input);
	else if (input->type == XMLinput::VIDEO && input->videoLib == "opencv")
		m_il = new InputLoaderVideoOpencv (input);
	//else if (input->type == XMLinput::VIDEO && input->videoLib == "ffmpeg")
	//	m_il = new InputLoaderVideoFfmpeg (input);
	else
		assert (0);
}

InputLoader::~InputLoader ()
{
	delete m_il;
}
bool InputLoader::isSingleImageInput ()
{
	return m_il->isSingleImageInput ();
}
int InputLoader::getNumIteration ()
{
	return m_il->getNumIteration ();
}
PtrHDRImage3f InputLoader::getColor ()
{
	return m_il->getColor();
}
PtrHDRImage1f InputLoader::getDepth ()
{
	return m_il->getDepth();
}
PtrHDRImage3f InputLoader::getEnvmap ()
{		
	return m_il->getEnvmap ();
}
bool InputLoader::isDepth ()
{
	return m_il->isDepth();
}
bool InputLoader::isEnvmap ()
{
	return m_il->isEnvmap();
}
bool InputLoader::isEnd ()
{
	return m_il->isEnd();
}
void InputLoader::next ()
{
	m_il->next();
}
