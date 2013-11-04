/////////////////////////////////////////////////////////////////////////////////
//
//  featurefnumadaptation: efekt, ktory modifikuje clonove cislo na zaklade 
//    jasu obrazu. Zmena clonoveho cisla nastava v objekte typu Adaptation
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "adaptation.h"
#include "featurefnumadaptation.h"

template <bool isInputIntensity>
class FeatureFnumAdaptation: public Feature
{
	PtrAdaptation m_ptradapt;
public:
	FeatureFnumAdaptation (PtrAdaptation adapt)
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
		m_ptradapt->updateFnum (in->computeAvg() * in->getMult(), isInputIntensity);
	}
};