
#include <math.h>
#include <boost/shared_ptr.hpp>
#include <list>
#include <ImfArray.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <SDL.h> 
#undef main

#include "xml.h"
#include "type.h"
#include "sdl.h"
#include "hdrimage.h"
#include <algorithm>
#include "func.h"
#include "feature.h"
#include "featurestacker.h"
#include "distortionwarpfunctor.h"
#include "initgrid.h"
#include "featuredistortionwarp.h"
#include "featurechromabber.h"
#include "featurelensflare.h"
#include "featurenoise.h"
#include "featuredof.h"
#include "featurefuziness.h"
#include "featureirrtosignal.h"
#include "featureclamp.h"
#include "featuredoadaptation.h"
#include "featurefnumadaptation.h"
#include "featuremotionblur.h"
#include "hdrimagefunctor.h"
#include "featurevignettingfunc.h"
#include "featurevignettingmask.h"

//#define SCENE_DISABLED
//#define USE_DOF_GATHER


uint2 g_posMouse (200,200);
/*float g_intensityMult = 1.0f;
const float g_initMult = 1.0f;
const float g_initMultLight = 300.0f;
*/





namespace sdl
{
void onInit ()
{
}


void onDestroy ()
{
}

void onWindowResized (int width, int height)
{
}

bool g_arrEnabledIsConst;
bool g_arrEnabled [10] = {false,false,false,false,false,false,false,true,false,false};//{true, true, true, true, true, true, true, true, true, true};

void onKeyDown (SDLKey key, Uint16 mod)
{
	if (key >= SDLK_0 && key <= SDLK_9)
	{
		int index = key - SDLK_0;
		if (! g_arrEnabledIsConst)
			g_arrEnabled[index] ^= true;
	}
}


void onKeyUp (SDLKey key, Uint16 mod)
{
}


void onMouseMove (unsigned x, unsigned y, int xrel, int yrel, Uint8 buttons)
{
	if (buttons == SDL_BUTTON (SDL_BUTTON_LEFT))
		g_posMouse = uint2 (x,y);
}


void onMouseDown (Uint8 button, unsigned x, unsigned y)
{
	if (button == SDL_BUTTON (SDL_BUTTON_LEFT))
		g_posMouse = uint2 (x,y);
}


void onMouseUp (Uint8 button, unsigned x, unsigned y)
{
}













float* getChannel (string channel, HDRImage3f* p)
{
	if (channel == "red")
		return p->getRedBuffer ();
	else if (channel == "green")
		return p->getGreenBuffer ();
	else if (channel == "blue")
		return p->getBlueBuffer ();
	else
		throw XMLexception ("","","", "Uknown channel name");
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

void loadColorStd (XMLinputFile& input, HDRImage3f& file, PtrHDRImage3f& color)
{
	if (! color.get())
	{
		color = PtrHDRImage3f (new HDRImage3f (file.getSize()));
		color->zeroesBlock (uint2 (0,0), color->getTotalSize()-uint2(1,1));
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

using namespace Imf;

void _copyExr (Array2D<float>& input, uint2 arrSize, uint i, PtrHDRImage3f& color, PtrHDRImage1f& depth, PtrHDRImage3f& envmap)
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
	InputFile file (input.path.c_str());		
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
		/*s = "R";*/
		Array2D<float> pixels (height, width);
		FrameBuffer frameBuffer;
		frameBuffer.insert (s.c_str(),
				Slice (Imf::FLOAT, 
						(char*) (&(pixels[0][0]) - dw.min.x - dw.min.y * width),
						sizeof (pixels[0][0]) * 1,
						/*sizeof (pixels[0][0]) * width)*/
						((char*) (pixels[1])) - ((char*) (pixels[0])) ));
		
		file.setFrameBuffer (frameBuffer);
		file.readPixels (dw.min.y, dw.max.y);
		uint2 arrSize (width, height);
		_copyExr (pixels, arrSize, i, color, depth, envmap);
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

	if (! depth.get())
		throw XMLexception ("", "", "", "Depth map is not defined");

	if (! envmap.get())
		throw XMLexception ("", "", "", "Environmental map is not defined");

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




PtrFeature imperfDoF, 
	imperfFuziness,
	imperfMotionBlur,
	imperfDistortion,
	imperfChromabber,
	imperfLensflare,
	imperfAdaptDo,
	imperfAdaptGet,
	imperfVignetting,
	imperfIrrToSignal,
	imperfNoise,
	imperfCFA;


void initEffectLensflare (XMLparser& parser, PtrHDRImage1f& ptrdepth, PtrHDRImage3f& ptrcubeEnvironment)
{
	PtrXMLlensflare lensflare = parser.getCamera()->getLensflare();
	g_arrEnabled[3] = (lensflare->m_enabled);
	{
		imperfLensflare = PtrFeature (new FeatureLensFlare (ptrcubeEnvironment, ptrdepth->getSize(), 
			lensflare->m_sizeofenvmap, lensflare->m_envmapangle, lensflare->m_fovangle, lensflare->m_starMult, 
			lensflare->m_csMult1, lensflare->m_csMult2));
	}
}

void initEffects (XMLparser& parser, PtrHDRImage1f& ptrdepth, PtrHDRImage3f& ptrcubeEnvironment)
{
	PtrXMLfuziness fuziness = parser.getCamera()->getFuziness();
	g_arrEnabled[2] = (fuziness->isEnabled());
	{
		imperfFuziness = (PtrFeature (new FeatureFuziness (fuziness->getFunc())));
		//stacker.add (imperf);
	}
	PtrXMLmotionblur motionblur = parser.getCamera()->getMotionblur();
	g_arrEnabled[8] = (motionblur->isEnabledParameters ());
	{
		PtrFeatureMotionBlurFullscreenParam param = PtrFeatureMotionBlurFullscreenParam (new FeatureMotionBlurFullscreenParam);
			
		param->deltaCameraPos = motionblur->deltaCameraPos;
		param->deltaTarget = motionblur->deltaTarget;
		param->depth = ptrdepth;
		param->depthIncreases = float2 (sinf (motionblur->fovAngle.x), sinf (motionblur->fovAngle.y));
		param->targetDist = motionblur->targetDist;
		param->fovAngle = motionblur->fovAngle;
		imperfMotionBlur =  (PtrFeature (new FeatureMotionBlur (motionblur->numIterations, param)));
	}


	PtrXMLgeometricDistortion distortion = parser.getCamera()->getDistortion();
		
	g_arrEnabled[9] = (distortion->isEnabled ());
	{
		PtrHDRImage1f gridX = PtrHDRImage1f (new HDRImage1f);
		PtrHDRImage1f gridY = PtrHDRImage1f (new HDRImage1f);
		distortion->getGrid (gridX, gridY);
		if (distortion->getType() == XMLgeometricDistortion::TYPE_NET)
			imperfDistortion = PtrFeature (new FeatureDistortionWarp (gridX, gridY));
		//stacker.add (imperf);
	}
	PtrXMLchromAbberation chromabber = parser.getCamera()->getChromAbberation ();
	g_arrEnabled[0] = (chromabber->isEnabled ());
	{
		imperfChromabber = PtrFeature (new FeatureChromAbber 
				(chromabber->getFuncRedCyan(), chromabber->getFuncBlueYellow(), chromabber->getSizeOfGrid(), ptrdepth->getSize().get<float>()));
		//stacker.add (imperf);
	}

	PtrXMLaperture aperture = parser.getCamera()->getAperture();
	PtrAdaptation adaptation = PtrAdaptation (new Adaptation (aperture->getFnum()));
	{
		g_arrEnabled[4] = (aperture->isEnabledBrightness());
		{
			imperfAdaptDo = (PtrFeature (new FeatureDoAdaptation (adaptation)));
			//stacker.add (imperf1);
		}
		if (aperture->getType() == XMLaperture::TYPE_ADAPT)
		{
			AdaptationParam param;
			aperture->getAdaptationParam (param);
			adaptation->setParam (param);
			boost::shared_ptr<FeatureFnumAdaptation<false>> feature = 
					boost::shared_ptr<FeatureFnumAdaptation<false>> (new FeatureFnumAdaptation<false> (adaptation));
			imperfAdaptGet = (feature);
			//stacker.add (imperf2);
		}
	}
	PtrXMLvignetting vignetting = parser.getCamera()->getVignetting();
	g_arrEnabled[7] = (vignetting->isEnabled ());
	{
		if (vignetting->getType() == XMLvignetting::TYPE_REGULAR)
			imperfVignetting = (PtrFeature (new FeatureVignettingFunc (vignetting->getRegFunc(), ptrdepth->getSize().get<float>())));
		else
			imperfVignetting = (PtrFeature (new FeatureVignettingMask (vignetting->getIrregFilepath().c_str(), ptrdepth->getSize())));
	}

	PtrFuncFtoF func = parser.getCamera()->m_funcIrrToSignal; 
	imperfIrrToSignal =  (PtrFeature (new FeatureIrrToSignal (func)));
	//stacker.add (imperf);

	{
		PtrXMLdepthoffield dof = parser.getCamera()->getDepthOfField ();
		g_arrEnabled[1] = (dof->m_enabled);

		float apert;
		if (! dof->m_apertureIsFixed)
			apert = adaptation->getFnum();
		else
			apert = dof->m_apertureValue;
		PtrFuncRadiusOfCoC func = PtrFuncRadiusOfCoC (new FuncFocusedToDist (dof->m_focusDist, apert, dof->m_focalLen, dof->m_sensorSize, ptrdepth->getSize().x));
		if (XMLdepthoffield::TYPE_DIFFUSION == dof->m_type)
			imperfDoF = PtrFeature (new FeatureDOF (ptrdepth, createDiffusionDOF (func, dof->m_numIterations)));
		else
			imperfDoF = PtrFeature (new FeatureDOF (ptrdepth, createLinGatherDOF (func)));
		
	}

	g_arrEnabled[5] = (parser.getCamera()->m_isNoiseEnabled);
	{
		PtrFuncFtoF func = parser.getCamera()->m_funcSignalToNoise; 
		float fmin = parser.getCamera()->m_noiseMin;
		float fmax = parser.getCamera()->m_noiseMax;
		imperfNoise =  (PtrFeature (new FeatureNoise (func, fmin, fmax)));
	}
	PtrXMLcolorfilterarray cfa = parser.getCamera()->getColorFilterArray();
	g_arrEnabled[6] = (cfa->isEnabled());
	{			
		FeatureColorFilterArrayParam red, green, blue;
		cfa->getRed (red);
		cfa->getGreen (green);
		cfa->getBlue (blue);
		imperfCFA = (PtrFeature (new FeatureColorFilterArray (red, green, blue)));
		//stacker.add (feature);
	}
}


int g_width(640), g_height(480);


class FileIter
{
	PtrXMLoutputFile m_file;
	uint m_iter;
public:
	FileIter (PtrXMLoutputFile file)
	{
		m_file = file;
		m_iter = 0;
	}

	bool isFileOutput ()
	{
		if (m_iter < m_file->itBegin)
			return false;
		else if (m_iter >= m_file->itBegin + m_file->itCount * m_file->itStep)
			return false;
		return true;
	}

	string getFilename ()
	{
		uint num = m_iter - m_file->itBegin;
		
		std::stringstream ss;
		ss << m_file->filePrefix;
		{
			char* num = new char [m_file->fileNumDigits + 1];
			uint it = m_iter - m_file->itBegin;
			sprintf (num, "%0*u", m_file->fileNumDigits, it);
			ss << num;

			delete [] (num);
		}
		ss << m_file->fileSuffix;
		return ss.str();
	}

	bool iterate ()
	{
		m_iter++;
		return isEnd ();
	}
	
	bool isEnd ()
	{
		return m_iter >= m_file->itBegin + m_file->itCount * m_file->itStep;
	}
	
};

void onRedrawCore (SDL_Surface* screen, XMLparser& parser)
{
	static PtrHDRImage3f ptrlight;
	static PtrHDRImage3f ptrcolor;
	static PtrHDRImage1f ptrdepth;
	static PtrHDRImage3f ptrenvmap;
	
	PtrXMLvideoExt videoExt = parser.getInput()->videoExt;
	if (! videoExt.get())
	{
		static bool bInit = false;
		if (! bInit)
		{ // init lens:
			bInit = true;
			load (parser.getInput (), ptrcolor, ptrdepth, ptrenvmap);
			initEffects (parser, ptrdepth, ptrenvmap);
			PtrXMLoutput out = parser.getOutput();
/*			if ((out->type == XMLoutput::TYPE_DEMO) && out->getDemo()->lightEnabled)
			{
				ptrlight = PtrHDRImage3f (new HDRImage3f);
				ptrlight->loadFromFile (out->getDemo()->lightFilepath.c_str());
				HDRIFunctorMult mult (out->getDemo()->lightMult);
				ptrlight->applyFunctor (mult);
			}
*/		}
	}
/*	else
	{
		static int numActual = videoExt->start;
		static int delayCounter = 0;
		if (! delayCounter++) // <- new input
		{
			parser.getInput()->clone ();
			load (parser.getInput (), ptrcolor, ptrdepth, ptrenvmap);
			initEffects (parser, ptrdepth, ptrenvmap);
			PtrXMLoutput out = parser.getOutput();
		}

		if (delayCounter == videoExt->delay)
		{
			delayCounter = 0;
			if (numActual++ == videoExt->end)
				numActual = videoExt->start;
		}
	}
*/	
	static bool bInitLensflare = false;

	if (!bInitLensflare && g_arrEnabled[3])
	{
		bInitLensflare = true;
		initEffectLensflare (parser, ptrdepth, ptrenvmap);
	}

	static PtrHDRImage3f ptrinput = PtrHDRImage3f (new HDRImage3f (ptrcolor->getSize()));
	static PtrHDRImage3f ptroutput;
	{
		ptrinput->zeroesBlock (uint2 (0,0), ptrinput->getSize()-uint2(1,1));
		ptrinput->setMult (ptrcolor->getMult());
		copyHDRImage (ptrinput.get(), ptrcolor.get(), uint2(0,0), uint2(0,0), ptrcolor->getSize());
	
/*		if (ptrlight.get())
		{
			uint2 dstOrg = g_posMouse - ptrlight->getSize()/2;
			uint2 srcOrg (0,0);
			uint2 size (ptrlight->getSize());
			copyHDRImage (ptrinput.get(), ptrlight.get(), dstOrg, srcOrg, ptrinput->getSize());
		}*/
	}
	FeatureStacker stacker;
#define IFIMP(N,IMP) if (g_arrEnabled[N]) stacker.add (IMP);
#define CASE(FEATURE) case(XMLcamera::FEATURE):
	std::vector<XMLcamera::efeature>& vec = parser.getCamera()->m_vecFeatures;
	for (uint i = 0; i < vec.size(); i++)
	{
		switch (vec[i])
		{
		CASE(DOF) IFIMP (1, imperfDoF); break;
		CASE(BLUR) IFIMP (2, imperfFuziness); break;
		CASE(MOTION) IFIMP (8, imperfMotionBlur); break;
		CASE(LENSFLARE) IFIMP (3, imperfLensflare); break;
		CASE(DIST) IFIMP (9, imperfDistortion); break;
		CASE(CHROMABBER) IFIMP (0, imperfChromabber); break;
		CASE(ADAPTDO) IFIMP (4, imperfAdaptDo); break;
		CASE(ADAPTGET) IFIMP (4, imperfAdaptGet); break;
		CASE(VIGNETTING) IFIMP (7, imperfVignetting); break; 
		CASE(CLAMP) stacker.add (PtrFeature (new FeatureClamp (16))); break;
		CASE(IRRTOSIGNAL) stacker.add (imperfIrrToSignal); break;
		CASE(NOISE) IFIMP (5, imperfNoise); break;
		CASE(CFA) IFIMP (6, imperfCFA);
		}

	}
	//stacker.add (imperfVignetting);
#undef IFIMP
	//printf ("lens flare: %d\n", g_arrEnabled[3]);

	stacker.show (ptrinput, ptroutput);

	//ptroutput = ptrinput;
	{
		PtrHDRImage3f hdriWnd;
		//if ()
			ptroutput->getSubImage (hdriWnd, uint2(g_width, g_height), float2(0,0), float2(0,0));
		//else

		static SDL_Surface* surface = NULL;
		surface = hdriWnd->getSDLSurface (ptroutput->getMult(), surface, uint2(g_width, g_height));
		SDL_BlitSurface (surface, NULL, screen, NULL);
	}
	if (parser.getOutput()->type == XMLoutput::TYPE_FILE)
	{
		static SDL_Surface* surface = NULL;
		surface = ptroutput->getSDLSurface (ptroutput->getMult(), surface, uint2(-1, -1));

		static FileIter iter (parser.getOutput()->getFile());
		if (iter.isFileOutput ())
		{
			string filename = iter.getFilename ();
			SDL_SaveBMP (surface, filename.c_str());
		}
		iter.iterate ();
		if (iter.isEnd ())
			sendQuit ();
	}
}

const char* g_xmlfile;
const char* g_cameraName;
const char* g_cameraVersion;
const char* g_input;
const char* g_output;

void onRedraw (SDL_Surface* screen)
{
	static XMLparser parser (g_xmlfile, g_cameraName, g_cameraVersion, g_input, g_output);
	//if (parser.getOutput()->type == XMLoutput::TYPE_DEMO)
	onRedrawCore (screen, parser);
		
}



}

int main (int argc, const char** argv)
{
	using std::cout;
	using std::endl;
	try
	{
		/////////////////////////////////
		if (argc >= 6)
		{
			sdl::g_xmlfile = argv[1];
			sdl::g_cameraName = argv[2];
			sdl::g_cameraVersion = argv[3];
			sdl::g_input = argv[4];
			sdl::g_output = argv[5];
		}
		else
		{
			cout << "ERROR: missing arguments" << endl
				<< "argument 1 - path to xml-configuration file" << endl
				<< "argument 2 - camera name" << endl
				<< "argument 3 - camera version" << endl
				<< "argument 4 - input" << endl
				<< "argument 5 - output" << endl
				<< endl
				<< "Example of demo:" << endl
				<< "  imperf config.xml IOS400D aperture anno1440mult demo" << endl
				<< "Example of creating files:" << endl
				<< "  imperf config.xml IOS400D distortion canteen screenshot" << endl
				<< "  imperf config.xml IOS400D motionblur wien0 screenshot" << endl
				<< "  imperf config.xml IOS400D lensflare wien1 screenshot" << endl
				<< "  imperf config.xml IOS400D dofdiffusion wien1 screenshot" << endl
				<< "  imperf config.xml IOS400D dofgather wien2 screenshot" << endl;
			return 0;
		}
		{
			XMLparser parser (sdl::g_xmlfile, sdl::g_cameraName, sdl::g_cameraVersion, sdl::g_input, sdl::g_output);
			PtrXMLlensflare lensflare = parser.getCamera()->getLensflare();
			sdl::g_arrEnabled[3] = (lensflare->m_enabled);
			
			PtrXMLoutput output = parser.getOutput();
			if (output->type == XMLoutput::TYPE_DEMO)
			{
				PtrXMLoutputDemo demo = output->getDemo();
				sdl::g_width = demo->wndSize.x;
				sdl::g_height = demo->wndSize.y;
				sdl::g_arrEnabledIsConst = false;
			}
			else if (output->type == XMLoutput::TYPE_FILE)
			{
				PtrXMLoutputFile file = output->getFile();
				sdl::g_width = file->wndSize.x;
				sdl::g_height = file->wndSize.y;
				sdl::g_arrEnabledIsConst = true;
			}
		}
		sdl::init (sdl::g_width, sdl::g_height);
		sdl::mainLoop ();
		sdl::destroy ();
	} catch (XMLexception& ex)
	{
		std::cerr << "ERROR in xml: " << ex.get() << std::endl;
	}
	return 0;
}


