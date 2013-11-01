#pragma once
#include "feature.h"
#include "featuredoadaptation.h"
#include "featurefnumadaptation.h"

class FeatureApertureAdapt: public Feature
{
private:
	FeatureFnumAdaptation<true>* m_compAdapt;
	FeatureDoAdaptation* m_adapt;
public:
	FeatureApertureAdapt (float fnum, AdaptationParam& ap)
	{			
		PtrAdaptation adaptation = PtrAdaptation (new Adaptation (fnum));
		adaptation->setParam (ap);
		m_compAdapt = new FeatureFnumAdaptation<true> (adaptation);
		m_adapt = new FeatureDoAdaptation (adaptation);
	}
	~FeatureApertureAdapt ()
	{
		delete (m_compAdapt);
		delete (m_adapt);
	}

	void featureFtoF (boost::shared_ptr<HDRImage3f> in, boost::shared_ptr<HDRImage3f> &out)
	{
		PtrHDRImage3f tmp;
		m_compAdapt->featureFtoF(in, tmp);
		m_adapt->featureFtoF(tmp, out);
	}

	type getType ()
	{
		return Feature::FEATURE_3F_TO_3F;
	}
};