#pragma once
#include <vector>

#include "xml.h"
#include "hdrimage.h"

class FeatureProcessor
{
	std::vector<Feature*> m_vecFeatures;

	Feature* getFeature(XMLparser* parser, XMLfeatures::feature_t& f);
	void init (PtrXMLfeatures features, XMLparser* parser);
public:
	~FeatureProcessor ()
	{
		for (uint i = 0; i < m_vecFeatures.size(); i++)
			delete (m_vecFeatures[i]);
	}
	FeatureProcessor ()
	{
	}
	FeatureProcessor (PtrXMLfeatures features, XMLparser* parser)
	{
		init (features, parser);
	}
	FeatureProcessor (string filename, string nameFeature)
	{
		XMLparser parser (filename);
		PtrXMLfeatures features (parser.parseFeatures(nameFeature));
		init (features, &parser);
	}
	PtrHDRImage3f Process (PtrHDRImage3f color, PtrHDRImage1f depth, PtrHDRImage3f envmap);
};
typedef boost::shared_ptr<FeatureProcessor> PtrFeatureProcessor;


PtrFeatureProcessor getDefaultFeatureProcessor ();