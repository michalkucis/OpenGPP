/////////////////////////////////////////////////////////////////////////////////
//
//  featureconstmult: efekt nasobi vstupny obraz maskou, ktora je definovane 2D funkciou
//	  tato funkcia sa nastavuje ako parameter konstruktera
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost\shared_ptr.hpp>
#include "feature.h"
#include "func.h"
#include "hdrimage.h"
#include "hdrimagefunctor.h"


class FeatureConstMult1: public Feature
{
	PtrFuncF2toF m_func;
	HDRImage1f m_mult;
public:
	FeatureConstMult1 (PtrFuncF2toF func)
	{
		m_func = func;
	}

	type getType ()
	{
		return FEATURE_3F_TO_3F;
	}

	uint2 getSize ()
	{
		return m_mult.getSize();
	}

public:
	void initHDRImult (uint2 size)
	{
		if (m_mult.getSize() == size)
			return;

		m_mult.setSize (size);
		
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
			{
				uint2 pos (x,y);
				float fx = ((float)x) / (size.x-1); 
				float fy = ((float)y) / (size.y-1);
				float2 fpos (fx, fy);
				float value = (*m_func) (fpos);
				m_mult.setPixel (pos, value); 
			}
	}

public:
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f &out)
	{
		out = in;

		if (m_mult.getSize() != in->getSize())
			initHDRImult (in->getSize());

		HDRIFunctor3fMult functor (in->getSize(), m_func);
		in->applyFunctor (functor);
	}
};
typedef boost::shared_ptr<FeatureConstMult1> PtrFeatureConstMult1;