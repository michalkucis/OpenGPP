/////////////////////////////////////////////////////////////////////////////////
//
//  featureirrtosignal: efekt umoznuje modifikovat vsetky pixely vo farebnej mape 
//    na zaklade definovanej funkcie
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "feature.h"


class FeatureTransformValues: public Feature
{
	PtrFuncFtoF m_func;
public:
	FeatureTransformValues (PtrFuncFtoF func)
	{
		m_func = func;
	}
	type getType ()
	{
		return Feature::FEATURE_3F_TO_3F;
	}
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f& out);
};