/////////////////////////////////////////////////////////////////////////////////
//
//  featuredistortionwarp: efekt, ktory simuluje geometricke skreslenie
//    geometricke skreslenie je definovane deformacnou mriezkou
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "feature.h"
#include "hdrimage.h"
#include "distortionwarpfunctor.h"

class FeatureDistortionWarp: public Feature
{
	PtrHDRImage1f m_gridX, m_gridY;
	float3 m_mult[2];
public:
	FeatureDistortionWarp (PtrHDRImage1f gridX, PtrHDRImage1f gridY)
	{
		m_gridX = gridX;
		m_gridY = gridY;
	}
	type getType ()
	{
		return FEATURE_3F_TO_3F;
	}
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f& out)
	{
		out = in;

		DistortionWarpFunctor functor (in, m_gridX, m_gridY);
		
		for (uint nChannel = 0; nChannel < 3; nChannel++)
			functor.apply (nChannel);
	}
};