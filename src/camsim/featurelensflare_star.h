/////////////////////////////////////////////////////////////////////////////////
//
//  FeatureStar: pridava ziaru v tvare hviezdy okolo jasnych pixelov v obraze
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "featurestaticfft.h"

class FeatureLensFlare_Star: public FeatureStaticFFT
{
public:
	FeatureLensFlare_Star (float angle, uint nFlares, float multStar): 
		FeatureStaticFFT (PtrFuncI2I2toF(new FuncStar(angle,nFlares,multStar)))
	{
	}
};