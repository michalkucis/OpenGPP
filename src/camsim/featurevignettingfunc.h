/////////////////////////////////////////////////////////////////////////////////
//
//  FeatureVignettingFunc: pridava do snimkov centralne symetricku vinetaciu na zaklade funkcie
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "featureconstmult.h"
#include "func.h"

class FeatureVignettingFunc: public Feature
{
	PtrFeatureConstMult1 m_mult;
	float2 m_aspectratio;
	PtrFuncFtoF m_func;
public:
	FeatureVignettingFunc (PtrFuncFtoF func, float2 aspectratio = float2(1,1))
	{
		m_aspectratio = aspectratio;
		m_aspectratio.y = m_aspectratio.y / m_aspectratio.x;
		m_aspectratio.x = 1.0f;
		m_func = func;
		m_mult = sharedNew<FeatureConstMult1> (PtrFuncF2toF (new FuncF2toFconvFromFtoF (func, m_aspectratio)));
	}
	type getType ()
	{
		return FEATURE_3F_TO_3F;
	}

public:
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f &out)
	{
		float2 size = in->getSize().get<float>();
		float2 aspectratio;
		aspectratio.x = 1.0f;
		aspectratio.y = size.y / size.x;
		if (aspectratio != m_aspectratio)
		{
			m_aspectratio = aspectratio;
			m_mult = sharedNew<FeatureConstMult1> (PtrFuncF2toF (new FuncF2toFconvFromFtoF (m_func, m_aspectratio)));
		}

		out = in;

		if (m_mult->getSize() != in->getSize())
			m_mult->initHDRImult (in->getSize());

		HDRIFunctor3fMult functor (in->getSize(), PtrFuncF2toF (new FuncF2toFconvFromFtoF (m_func, m_aspectratio)));
		in->applyFunctor (functor);
	}
};