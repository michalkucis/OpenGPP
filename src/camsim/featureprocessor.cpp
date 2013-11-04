#include "featureprocessor.h"
#include "featurevignettingfunc.h"
#include "featurevignettingmask.h"
#include "featuredistortionwarp.h"
#include "featurechromabber.h"
#include "featureblur.h"
#include "featuredof.h"
#include "featurelensflaresimple.h"
#include "featuremotionblur.h"
#include "featureclamp.h"
#include "featurenoisesimple.h"
#include "featuretransformvalues.h"
#include "featurefnumadaptation.h"
#include "featuredoadaptation.h"
#include "adaptation.h"
#include "featureapertureadapt.h"

Feature* FeatureProcessor::getFeature (XMLparser* parser, XMLfeatures::feature_t& xmlfeature)
{
	Feature* feature = NULL;
	string featureName = xmlfeature.name;
	XMLfeatures::etype_t t = xmlfeature.type;
	switch (t)
	{
	case XMLfeatures::VIGNETTING_MASK:
		{
			PtrXMLvignettingMask xml = parser->parseVignettingMask(featureName);
			feature = new FeatureVignettingMask (xml->m_filepath);
			break;
		}
	case XMLfeatures::VIGNETTING_RADIAL:
		{
			PtrXMLvignettingRadial xml = parser->parseVignettingRadial(featureName);
			feature = new FeatureVignettingFunc (xml->m_func);
			break;
		}
	case XMLfeatures::DISTORTION_GRID:
		{
			PtrXMLdistortion xml = parser->parseDistortionGrid(featureName);
			PtrHDRImage1f gridX,gridY;
			xml->getGrid(gridX,gridY);
			feature = new FeatureDistortionWarp (gridX, gridY);
			break;
		}
	case XMLfeatures::DISTORTION_RADIAL:
		{
			PtrXMLdistortion xml = parser->parseDistortionRadial(featureName);
			PtrHDRImage1f gridX,gridY;
			xml->getGrid(gridX,gridY);
			feature = new FeatureDistortionWarp (gridX, gridY);
			break;
		}
	case XMLfeatures::CHROMABER_RADIAL:
		{
			PtrXMLchromAbberation xml = parser->parseChromaber(featureName);
			feature = new FeatureChromAbber (xml->getFuncRedCyan(), xml->getFuncBlueYellow(), xml->getSizeOfGrid());
			break;
		}
	case XMLfeatures::BLUR:
		{
			PtrXMLblur xml = parser->parseBlur(featureName);
			feature = new FeatureBlur (xml->getFunc());
			break;
		}
	case XMLfeatures::DEPTHOFFIELD:
		{
			PtrXMLdepthoffield xml = parser->parseDepthoffield(featureName);
			
			FeatureDOF::etype_t type;
			switch (xml->m_type)
			{
			case XMLdepthoffield::TYPE_DIFFUSION:
				type = FeatureDOF::DIFFUSION;
				break;
			case XMLdepthoffield::TYPE_GATHER:
				type = FeatureDOF::GATHER;
				break;
			}
			feature = new FeatureDOF (type, xml->m_focusDist, xml->m_apertureValue, xml->m_focalLen, xml->m_sensorSize);
			break;
		}
	case XMLfeatures::LENSFLARE_SIMPLE:
		{
			PtrXMLlensflareSimple xml = parser->parseLensflareSimple(featureName);
			feature = new FeatureLensFlareSimple (xml->m_sizeofenvmap, xml->m_envmapangle, xml->m_fovangle, 
				xml->m_starMult, xml->m_csMult1, xml->m_csMult2);
			break;
		}
	case XMLfeatures::MOTIONBLUR_PARAM:
		{
			PtrXMLmotionblurParam xml = parser->parseMotionblurParam(featureName);
			PtrFeatureMotionBlurFullscreenParam param = PtrFeatureMotionBlurFullscreenParam (new FeatureMotionBlurFullscreenParam);

			param->deltaCameraPos = xml->deltaCameraPos;
			param->deltaTarget = xml->deltaTarget;
			param->depthIncreases = float2 (sinf (xml->fovAngle.x), sinf (xml->fovAngle.y));
			param->targetDist = xml->targetDist;
			param->fovAngle = xml->fovAngle;
			feature = new FeatureMotionBlur (xml->numIterations, param);
			break;
		}
	case XMLfeatures::CLAMPVALUES:
		{
			PtrXMLclampValues xml = parser->parseClampValues(featureName);
			feature = new FeatureClamp (xml->m_maxValue);
			break;
		}
	case XMLfeatures::NOISE_SIMPLE:
		{
			PtrXMLnoiseSimple xml = parser->parseNoiseSimple(featureName);
			feature = new FeatureNoiseSimple (xml->m_funcSignalToNoise, xml->m_min, xml->m_max);
			break;
		}
	case XMLfeatures::TRANSFORMVALUES:
		{
			PtrXMLtransformValues xml = parser->parseTransformValues(featureName);
			feature = new FeatureTransformValues (xml->m_func);
			break;
		}
	case XMLfeatures::APERTUREADAPT:
		{
			PtrXMLapertureAdapt xml = parser->parseAdaptation(featureName);
			AdaptationParam param;
			xml->getAdaptationParam (param);
			//PtrAdaptation adaptation = PtrAdaptation (new Adaptation (xml->getFnum()));
			//adaptation->setParam (param);

			feature = new FeatureApertureAdapt (xml->getFnum(), param);
		}
	}
	assert (feature);
	return feature;
}

void FeatureProcessor::init (PtrXMLfeatures features, XMLparser* parser)
{
	for (uint i = 0; i < features->vecFeatures.size(); i++)
	{
		Feature* feature = getFeature(parser, features->vecFeatures[i]);

		m_vecFeatures.push_back(feature);
	}
}


PtrHDRImage3f FeatureProcessor::Process (PtrHDRImage3f color, PtrHDRImage1f depth, PtrHDRImage3f envmap)
{
		
	PtrHDRImage3f hdri3f = color;
	color->setMult (1.0f);
	PtrHDRImage3c hdri3c;
	enum state_t {STATE_HDRI3F, STATE_HDRI3C}  state = STATE_HDRI3F;
	for (uint i = 0; i < m_vecFeatures.size(); i++)
	{
		Feature* feature = m_vecFeatures[i];
		feature->setDepthMap(depth);
		feature->setEnvMap(envmap);
		if (state == STATE_HDRI3F)
		{
			switch (feature->getType())
			{
			case Feature::FEATURE_3F_TO_3F:
				{
					PtrHDRImage3f out;
					feature->featureFtoF (hdri3f, out);
					hdri3f = out;
					break;
				}					
			case Feature::FEATURE_3F_TO_3C:
				feature->featureFtoC (hdri3f, hdri3c);
				state = STATE_HDRI3C;
				break;
			case Feature::FEATURE_3C_TO_3F:
				hdri3f->clearAppendArea();					
				hdri3c = hdri3f->computeFFT();
				feature->featureCtoF (hdri3c, hdri3f);
				break;
			case Feature::FEATURE_3C_TO_3C:
				{
					hdri3f->clearAppendArea();
					hdri3c = hdri3f->computeFFT();
					PtrHDRImage3c hdriOut;
					feature->featureCtoC (hdri3c, hdriOut);
					hdri3c = hdriOut;
					state = STATE_HDRI3C;
					break;
				}
			}
		}
		else if (state == STATE_HDRI3C)
		{
			switch (feature->getType())
			{
			case Feature::FEATURE_3F_TO_3F:
				{
					PtrHDRImage3f in = hdri3c->computeRFFT ();
					in->clearAppendArea ();
					PtrHDRImage3f out;
					feature->featureFtoF (in, out);
					hdri3f = out;
					state = STATE_HDRI3F;
					break;
				}					
			case Feature::FEATURE_3F_TO_3C:
				{
					PtrHDRImage3f in = hdri3c->computeRFFT ();
					in->clearAppendArea ();
					PtrHDRImage3c out;
					feature->featureFtoC (in, out);
					hdri3c = out;
					state = STATE_HDRI3C;
					break;
				}
			case Feature::FEATURE_3C_TO_3F:
				{
					PtrHDRImage3c in = hdri3c;
					PtrHDRImage3f out;
					feature->featureCtoF (in, out);
					hdri3f = out;
					state = STATE_HDRI3F;
					break;
				}
			case Feature::FEATURE_3C_TO_3C:
				{
					PtrHDRImage3c in = hdri3c;
					PtrHDRImage3c out;
					feature->featureCtoC (in, out);
					hdri3c = out;
					state = STATE_HDRI3C;
					break;
				}
			}
		}
	}
	PtrHDRImage3f out_hdri;
	if (state == STATE_HDRI3F)
		out_hdri = hdri3f;
	else if (state == STATE_HDRI3C)
		out_hdri = hdri3c->computeRFFT ();
	out_hdri->clearAppendArea ();
	return out_hdri;
}

PtrFeatureProcessor getDefaultFeatureProcessor ()
{
	return sharedNew<FeatureProcessor> ();
}