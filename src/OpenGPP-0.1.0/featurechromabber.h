/////////////////////////////////////////////////////////////////////////////////
//
//  FeatureChromAbber: definuje efekt chromatickej aberacie
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "feature.h"
#include "initgrid.h"
#include "distortionwarpfunctor.h"

class FeatureChromAbber: public Feature
{
	typedef std::vector<float> vecF_t;
	PtrHDRImage1f m_gridXred, m_gridYred;
	PtrHDRImage1f m_gridXblue, m_gridYblue;

	PtrFuncFtoF m_funcRedcyan, m_funcBlueyellow;

	uint2 m_sizeOfGrid;
	float2 m_aspectRatio;

public:
	FeatureChromAbber (PtrFuncFtoF funcRedcyan, PtrFuncFtoF funcBlueyellow, uint2 sizeOfGrid)
	{
		m_funcRedcyan = funcRedcyan;
		m_funcBlueyellow = funcBlueyellow;
		m_sizeOfGrid = sizeOfGrid;
		m_gridXred = PtrHDRImage1f (new HDRImage1f);
		m_gridYred = PtrHDRImage1f (new HDRImage1f);
		m_gridXblue = PtrHDRImage1f (new HDRImage1f);
		m_gridYblue = PtrHDRImage1f (new HDRImage1f);

		m_aspectRatio = float2 (0,0);
	}
	type getType ()
	{
		return FEATURE_3F_TO_3F;
	}
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f &out)
	{
		float2 actualAspectRatio = in->getSize().get<float>();
		actualAspectRatio.y = actualAspectRatio.y / actualAspectRatio.x;
		actualAspectRatio.x = 1.0f;
		if (m_aspectRatio != actualAspectRatio)
		{
			m_aspectRatio = actualAspectRatio;
			initGrid (m_gridXred, m_gridYred, m_sizeOfGrid, m_aspectRatio, m_funcRedcyan.get());
			initGrid (m_gridXblue, m_gridYblue, m_sizeOfGrid, m_aspectRatio, m_funcBlueyellow.get());
		}

		out = in;
	
		DistortionWarpFunctor functorR (in, m_gridXred, m_gridYred);
		DistortionWarpFunctor functorB (in, m_gridXblue, m_gridYblue);
		functorR.apply (0);
		functorB.apply (2);
	}
};