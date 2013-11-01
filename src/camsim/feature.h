/////////////////////////////////////////////////////////////////////////////////////////
//
//  Feature: poskytuje rozhranie pre efekty, ktore umoznuju simulovat vlastnosti kamery
//
/////////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <boost/shared_ptr.hpp>
#include "hdrimage.h"


class Feature
{
public:
	enum type {FEATURE_3F_TO_3F, FEATURE_3F_TO_3C, FEATURE_3C_TO_3F, FEATURE_3C_TO_3C}; 
	
	virtual type getType () = 0;
	virtual void setDepthMap (PtrHDRImage1f depth) {}
	virtual void setEnvMap (PtrHDRImage3f envmap) {}
	virtual void featureFtoF (boost::shared_ptr<HDRImage3f> in, boost::shared_ptr<HDRImage3f> &out)
	{
		throw std::exception ("Not implement");
	}
	virtual void featureFtoC (boost::shared_ptr<HDRImage3f> in, boost::shared_ptr<HDRImage3c> &out)
	{
		throw std::exception ("Not implement");
	}
	virtual void featureCtoF (boost::shared_ptr<HDRImage3c> in, boost::shared_ptr<HDRImage3f> &out)
	{
		throw std::exception ("Not implement");
	}
	virtual void featureCtoC (boost::shared_ptr<HDRImage3c> in, boost::shared_ptr<HDRImage3c> &out)
	{
		throw std::exception ("Not implement");
	}


};

typedef boost::shared_ptr<Feature> PtrFeature;