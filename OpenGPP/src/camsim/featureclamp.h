/////////////////////////////////////////////////////////////////////////////////
//
//  FeatureClamp: definuje efekt, ktory 'clampuje' hodnoty pixelov vo farebnej mape
//
/////////////////////////////////////////////////////////////////////////////////


#pragma once
#include "feature.h"

class FeatureClamp: public Feature
{
	float m_minValue;
	float m_maxValue;
public:
	FeatureClamp (float maxValue):
		m_minValue (0.0f), m_maxValue (maxValue)
	{
	}
	type getType ()
	{
		return Feature::FEATURE_3F_TO_3F;
	}
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f &out)
	{
		out = in;

		out->clamp (float3(m_minValue), float3(m_maxValue));
	}
};

typedef boost::shared_ptr<FeatureClamp> PtrFeatureClamp;