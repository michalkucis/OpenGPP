/////////////////////////////////////////////////////////////////////////////////
//
//  featureglow1: efekt, ktory pridava 'ziaru' okolo jasnych bodov v obraze
//    - ziara je linearne zavisla na hodnote jasu pixelov
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

class FeatureLensFlare_Glow1: public FeatureStaticFFT
{
public:
	FeatureLensFlare_Glow1 (float mult): FeatureStaticFFT (PtrFuncI2I2toF (new FuncGlow1 (mult)))
	{
	}
};