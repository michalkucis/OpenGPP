/////////////////////////////////////////////////////////////////////////////////
//
//  FeatureNoise: efekt pridava sum do farebnej mapy
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "feature.h"

class FeatureNoiseSimple: public Feature
{
	PtrFuncFtoF m_func;

	float m_fmin, m_fmax;
public:
	FeatureNoiseSimple (PtrFuncFtoF func, float fmin, float fmax)
	{
		m_func = func;
		m_fmin = fmin;
		m_fmax = fmax;
/*		m_add = param.Nread + param.FPN + param.SNphAdd + param.PRNUAdd;
		m_multPRNU = param.PRNUMult;
		m_multSNph = param.SNphMult;
*/	}
	type getType ()
	{
		return Feature::FEATURE_3F_TO_3F;
	}
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f& out);
};
