/////////////////////////////////////////////////////////////////////////////////
//
//  FeatureTimeAdaptation: upravuje cas na zaklade nastavenia objektu 'Adaptation'
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "feature.h"
#include "adaptation.h"

template <bool isInputIntensity>
class FeatureTimeAdaptation: public Feature
{
	PtrAdaptation m_ptradapt;
public:
	FeatureTimeAdaptation (PtrAdaptation adapt)
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
		m_ptradapt->updateTime (in->computeAvg() * in->getMult(), isInputIntensity);
	}
};