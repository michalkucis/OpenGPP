#pragma once

#include "featureconstmult.h"
#include "func.h"



class FeatureVignettingMask: public Feature
{
	std::string m_pathMask;
	PtrHDRImage3f m_mask;
public:
	void initMask (uint2 size)
	{
		if (m_mask.get()!=NULL && m_mask->getSize() == size)
			return;
		HDRImage3f loader;
		if (! loader.loadFromFile (m_pathMask.c_str()))
			throw XMLexception("","","Vignetting mask not found");
		loader.getSubImage (m_mask, size, float2(0,0), float2(0,0));
	}
	FeatureVignettingMask (std::string path)
	{
		m_pathMask = path;
	}
	type getType ()
	{
		return FEATURE_3F_TO_3F;
	}
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f &out)
	{
/*		initMask (in->getSize());
		out = m_mask;
*/		out = in;
		initMask (in->getSize());
		HDRIFunctorMultByHDRI3f functor (in->getMult(), m_mask);
		in->applyFunctor (functor);
	}
};