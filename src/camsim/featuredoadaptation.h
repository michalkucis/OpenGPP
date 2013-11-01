/////////////////////////////////////////////////////////////////////////////////
//
//  featuredoadaptation: efekt modifikuje jas obrazu na zaklade jasu, ktory je nastaveny v 'adaptation'
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once 

#include "feature.h"
#include "adaptation.h"

class FeatureDoAdaptation: public Feature
{
	PtrAdaptation m_ptradapt;
public:
	FeatureDoAdaptation (PtrAdaptation adapt)
	{
		m_ptradapt = adapt;
	}
	type getType ()
	{
		return FEATURE_3F_TO_3F;
	}
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f &out)
	{
		out = in;
		out->setMult (out->getMult() * m_ptradapt->getMultByEV());
	}
};