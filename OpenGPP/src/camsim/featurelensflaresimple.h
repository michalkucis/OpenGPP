/////////////////////////////////////////////////////////////////////////////////
//
//  
//  
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "feature.h"
#include "featurestaticfft.h"
#include "featurelensflare_stacker.h"
#include "featurelensflare_star.h"
#include "featurelensflare_centralsymmetry.h"
#include "featurevignettingmask.h"
#include "hdrimagefunctor.h"
#include "featurelensflare_glow1.h"
#include "featurelensflare_glow2.h"






class FeatureLensFlareSimple: public Feature
{
	class FuncCS: public FuncFtoF3
	{
		float m_m1, m_m2;
	public:
		FuncCS (float m1, float m2)
		{
			m_m1 = m1;
			m_m2 = m2;
		}
		float3 operator () (float i2)
		{
			float3 m1 (m_m1); 
			return m1 * expf(i2*m_m2);
		}
	};
	
	const float pi;

	uint getNumFace (float2 coord)
	{
		uint nFace = 0;
		if (coord.y < 0.5f)
		{
			coord.y *= 2;
			if (coord.x < 1.0f/3)
			{
				nFace = 1;
			}
			else if (coord.x < 2.0f/3)
			{
				nFace = 2;
			} 
			else
			{
				nFace = 3;
			}			
		}
		else
		{
			coord.y = coord.y * 2 - 1.0f;
			if (coord.x < 1.0f/3)
			{	nFace = 4;
			}
			else if (coord.x < 2.0f/3)
			{
				nFace = 5;
			}
			else
			{
				nFace = 6;
			}			
		}
		return nFace;
	}

	float2 getFaceCoord (float2 coord)
	{
		uint nFace = 0;
		if (coord.y < 0.5f)
		{
			coord.y *= 2;
			if (coord.x < 1.0f/3)
			{
				coord.x *= 3;
				nFace = 1;
			}
			else if (coord.x < 2.0f/3)
			{
				coord.x = 3 * coord.x - 1;
				nFace = 2;
			}
			else
			{
				coord.x = 3 * coord.x - 2;
				nFace = 3;
			}			
		}
		else
		{
			coord.y = coord.y * 2 - 1.0f;
			if (coord.x < 1.0f/3)
			{
				coord.x *= 3;
				nFace = 4;
			}
			else if (coord.x < 2.0f/3)
			{
				coord.x = 3 * coord.x - 1;
				nFace = 5;
			}
			else
			{
				coord.x = 3 * coord.x - 2;
				nFace = 6;
			}			
		}
		return coord;
	}
	float2 getFaceAngles (float2 facecoord)
	{
		facecoord = facecoord * 2 - 1;
		float2 subtense = float2(1, 1) + (facecoord * facecoord);
		subtense = float2 (sqrtf(subtense.x), sqrtf(subtense.y));
		float2 sinf2 = float2 (facecoord.x/subtense.x, facecoord.y/subtense.y);
		float2 angles = float2 (asinf(sinf2.x), asinf(sinf2.y));
		return angles;
	}
	float2 getEnvAngles (uint nFace, float2 faceangles)
	{
		float xAngle;
		switch (nFace)
		{
		case 1:
			xAngle = - (pi/2) + faceangles.y;
			break;
		case 3:
			xAngle = (pi/2) - faceangles.y;
			break;
		case 2:
			xAngle = - faceangles.x;
			break;
		case 6:
			xAngle = faceangles.x;
			break;
		case 4:
			xAngle = faceangles.x;
			break;
		case 5:
			xAngle = - (pi) - faceangles.y;	
			if (xAngle < - pi)
				xAngle += 2*pi;
			break;
		}
		float yAngle;
		switch (nFace)
		{
		case 1:
			yAngle = - faceangles.x;
			break;
		case 3:
			yAngle = + faceangles.x;
			break;
		case 2:
			yAngle = pi/2 - faceangles.y;
			break;
		case 6:
			yAngle = -pi/2 + faceangles.y;
			break;
		case 4:
			yAngle = faceangles.y;
			break;
		case 5:
			yAngle = - (pi) - faceangles.x;	
			if (yAngle < - pi)
				yAngle += 2*pi;
			break;
		}
		return float2 (xAngle, yAngle);
	}
	float2 coordEnv2My (float2 coord)
	{
		uint nFace = getNumFace (coord);
		coord = getFaceCoord (coord);
		float2 faceangles = getFaceAngles (coord);
		float2 envangles = getEnvAngles (nFace, faceangles);
		switch (nFace)
		{
		case 1:
		case 3:
			envangles.y *= 1+((-faceangles.y + pi*1/4)/(pi/4));
			break;
		case 2:
		case 6:
			envangles.x *= 1+((-faceangles.y + pi*1/4)/(pi/4));
			break;
			//envangles.x *= (2.0f - coord.y);
		}

		float coef = pi/2;
		
		coord = (float2(1,1)+(envangles/coef))/2;
		return coord;
	}
	PtrHDRImage3f m_result;
	
	PtrHDRImage3f getPlaneEnvmap (PtrHDRImage3f cubeenvmap, uint2 envsize)
	{
		PtrHDRImage3f envmap;
		float2 envsizef (envsize.get<float>());
		uint2 cubesize (cubeenvmap->getSize());
		envmap = PtrHDRImage3f (new HDRImage3f (envsize, envsize));
		envmap->clearAppendArea ();
		float weight = 800*800.0f /10/ cubeenvmap->getSize().getArea () * 6;

		float2 invenvsize = float2(1,1)/envsize.get<float>();
		float2 invcubesize = float2(1,1)/cubesize.get<float>();
		for (uint y = 0; y < cubesize.y; y++)
			for (uint x = 0; x < cubesize.x; x++)
			{
				float3 color = cubeenvmap->getPixel (uint2(x,y));
				float2 cubecoord = float2 ((float)x,(float)y) / cubesize.get<float>() + invcubesize/2.0f;
				float2 mycoord = coordEnv2My (cubecoord);
				
				// point interpolation:
				uint2 outcoord = (mycoord*envsizef+invenvsize).get<uint> ();
				if (outcoord.x < 0 || outcoord.x >= envsize.x)
					continue;
				else if (outcoord.y < 0 || outcoord.y >= envsize.y)
					continue;
				{
					float2 coord = getFaceCoord (cubecoord);
					float x = coord.x;
					float y = coord.y;
					float localw = weight * 1/(1 + (x-0.5f)*(x-0.5f) + (y-0.5f)*(y-0.5f)); 
					envmap->setPixel (outcoord, color + envmap->getPixel(outcoord)*localw);
				}
			}
		return envmap;
	}
	PtrHDRImage1f getPlaneMask (uint2 cubesize,  uint2 envsize)
	{
		PtrHDRImage1f mask = PtrHDRImage1f (new HDRImage1f (envsize));
		float2 envsizef (envsize.get<float>());
		
		float2 invenvsize = float2(1,1)/envsize.get<float>();
		float2 invcubesize = float2(1,1)/cubesize.get<float>();
		for (uint y = 0; y < cubesize.y; y++)
			for (uint x = 0; x < cubesize.x; x++)
			{
				//float3 color = cubeenvmap->getPixel (uint2(x,y));
				float2 cubecoord = float2 ((float)x,(float)y) / cubesize.get<float>() + invcubesize/2.0f;
				float2 mycoord = coordEnv2My (cubecoord);
				float value = 1.f;
				// point interpolation:
				uint2 outcoord = (mycoord*envsizef+invenvsize).get<uint> ();
				if (outcoord.x < 0 || outcoord.x >= envsize.x)
					continue;
				else if (outcoord.y < 0 || outcoord.y >= envsize.y)
					continue;
				{
					//float2 coord = getFaceCoord (cubecoord);
					mask->setPixel (outcoord, value + mask->getPixel(outcoord));
				}
			}
		return mask;
	}

	class HDRIFunctorComputeMissingValues: public HDRImageFunctor
	{
		PtrHDRImage3f m_values;
		PtrHDRImage1f m_mask;
		uint2 m_size;
		typedef float* (HDRImage3f:: *FuncGet) (uint2);
	
		bool isInside (int2 pos)
		{
			if (pos.x < 0 || pos.y < 0 || pos.x >= (int)m_size.x || pos.y >= (int)m_size.y)
				return false;
			return true;
		}

	public:
		HDRIFunctorComputeMissingValues (PtrHDRImage3f values, PtrHDRImage1f mask)
		{
			m_values = values;
			m_mask = mask;
			m_size = m_values->getSize();
		}
		float getValue (uint x, uint y, FuncGet func)
		{
			float mask = m_mask->getPixel (x, y);
			if (mask != 0)
				return *(m_values.get()->*func) (uint2(x,y));

			// find {x-, x+, y-, y+} closest values
			float weights [8] = {0,0,0,0,0,0,0,0};
			float values [8];
			float sumweights = 0;
			// find weights and values:
			for (uint i = 0; i < 8; i++)
			{
				int2 offset;
				float weightmult = 1.0f;
				switch (i)
				{
				case 0:	offset = int2 (-1,-1); weightmult = 0.5f; break;
				case 1:	offset = int2 (1,-1); weightmult = 0.5f; break;
				case 2:	offset = int2 (0,-1); break;
				case 3:	offset = int2 (-1,0); break;
				case 4:	offset = int2 (1,0); break;
				case 5:	offset = int2 (0,1); break;		
				case 6:	offset = int2 (1,1); weightmult = 0.5f; break;
				case 7:	offset = int2 (-1,1); weightmult = 0.5f; break;		
				}

				int2 pos (x,y);
				pos += offset;
				for (uint j = 1; true; j = j << 1)
				{
					if (!isInside (pos))
						break;
					if (m_mask->getPixel(pos.x, pos.y))
					{
						if (j == 0)
							break;
						weights[i] = 1/j * weightmult;
						values[i] = *(m_values.get()->*func) (uint2(pos.x,pos.y));
						sumweights += weights[i];
						break;
					}
					pos += offset;
				}				
			}
			float ret = 0;
			for (uint i = 0; i < 8; i++)
				ret += values[i] * weights[i] / sumweights;
			return ret;
		}
		float getRed (uint x, uint y, float red)
		{
			FuncGet func = &(HDRImage3f::getRedBuffer);
			return getValue (x, y, func);
		}

		float getGreen (uint x, uint y, float green)
		{
			FuncGet func = &(HDRImage3f::getGreenBuffer);
			return getValue (x, y, func);
		}

		float getBlue (uint x, uint y, float blue)
		{
			FuncGet func = &(HDRImage3f::getBlueBuffer);
			return getValue (x, y, func);
		}
	};
	PtrHDRImage3f m_envmap;
	float m_csMult1, m_csMult2, m_starMult;
	uint2 m_sizeOfResult;
	float2 m_envangles, m_fowAngles;
public:
	FeatureLensFlareSimple (uint2 sizeOfResult, float2 envangles, float2 fowAngles, 
			float starMult, float csMult1, float csMult2):
		pi (3.14159f)
	{
		m_csMult1 = csMult1;
		m_csMult2 = csMult2;
		m_starMult = starMult;
		m_sizeOfResult = sizeOfResult;
		m_envangles = envangles;
		m_fowAngles = fowAngles;
	}
	void setEnvMap (PtrHDRImage3f envmap) 
	{
		m_envmap = envmap;
	}
	type getType ()
	{ 
		return FEATURE_3F_TO_3F;
	}
	void computeResult (uint2 resultSize)
	{
		m_result = getPlaneEnvmap (m_envmap, resultSize);

		// slower but better version:
		PtrHDRImage1f mask = getPlaneMask (m_envmap->getSize(), resultSize);
		HDRIFunctorApplyFunc functor0 (PtrFuncFtoF (new FuncFtoFinv));
		mask->applyFunctor (functor0);
		HDRIFunctorMultByHDRI1f functor1 (mask);
		m_result->applyFunctor (functor1);


		PtrHDRImage3f tmp = m_result;
		m_result = PtrHDRImage3f (new HDRImage3f (resultSize, resultSize));
		HDRIFunctorComputeMissingValues functor2 (tmp, mask);
		m_result->applyFunctor (functor2);


		{
			FeatureLensFlare_Stacker stacker;
			stacker.add (PtrFeature(new FeatureVignettingMask ("lensflaresimple_mask.jpg")));
			stacker.add (PtrFeature(new FeatureLensFlare_Star (3.14f/2, 6, m_starMult)));
			//stacker.add (PtrImperf(new ImperfGlow2 ()));
			stacker.add (PtrFeature(new FeatureLensFlare_Glow1 (m_starMult)));
			PtrHDRImage3f tmp;
			stacker.show (m_result, tmp); // <- this "nop operation" fixes a bug. Why? I don't know..
			stacker.show (m_result, m_result);
		}

		// compute size of result:
		float2 resAngle(m_envangles);

		//float aspectRatio = 800.0f/640.0f;
		float fovHorAngle = m_fowAngles.x;//63.395f/180*pi;
		float fovVertAngle = m_fowAngles.y;//fovHorAngle/aspectRatio;

		float2 offset ((resAngle.x-fovHorAngle) / 2, (resAngle.y-fovVertAngle) / 2);
		float2 size = resAngle - offset * 2;
		offset /= resAngle;
		size /= resAngle;

		// create result:
		PtrHDRImage3f image;
		m_result->getSubImage (image, m_sizeOfResult, offset, offset);
		m_result = image;
	}
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f& out)
	{
		PtrHDRImage3f hdri3f = in;
		
		computeResult (in->getSize());
		{
			HDRIFunctor3fAddHDRI functor (in->getMult(), m_result);
			in->applyFunctor (functor);
		}
		{
			PtrFeature imperf =  (PtrFeature (new FeatureCentralSymmetry (PtrFuncFtoF3(new FuncCS (m_csMult1, m_csMult2)))));
			PtrHDRImage3f tmp = hdri3f;
			imperf->featureFtoF (tmp, hdri3f);
		}
		out = hdri3f;
		
		out = in;
		//out = m_result;
	}
};