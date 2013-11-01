/////////////////////////////////////////////////////////////////////////////////
//
//  featurefuziness: efekt, ktory umoznu rozostrenia
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "dof.h"
#include "feature.h"

class FeatureBlur: public Feature
{
	PtrFuncFtoF m_func;
	PtrHDRImage3f m_output;
	PtrHDRImage1f m_depth;
	PtrDOF m_dof;
	void initDepth (HDRImage1f* depth, uint2 size)
	{
		if (depth->getSize() == size)
			return;
		depth->setSize (size);
		float2 fsize = size.get<float> ();
		float2 fcenter = fsize / 2;
		float2 finit = float2(1.0f,1.0f)/fsize;
		float2 fcoef = fsize / fsize.getLength();
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
			{
				float2 val = fcoef*(finit + (float2((float)x,(float)y) - fcenter)/fcenter);
				m_depth->setPixel (uint2(x,y), val.getLength ());
			}
	}
public:
	FeatureBlur (PtrFuncFtoF func);
	type getType ()
	{
		return Feature::FEATURE_3F_TO_3F;
	}
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f &out);
};