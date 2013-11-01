#pragma once

#include <string>
#include <boost/shared_ptr.hpp>
#include "hdrimage.h"
#include "xml.h"

class InputLoaderAbstract;

class InputLoader
{
	InputLoaderAbstract* m_il;
public:
	InputLoader (boost::shared_ptr<XMLinput> input);
	InputLoader (std::string filename, std::string inputName);
	~InputLoader ();
	bool isSingleImageInput ();
	int getNumIteration ();
	PtrHDRImage3f getColor ();
	PtrHDRImage1f getDepth ();
	PtrHDRImage3f getEnvmap ();
	bool isDepth ();
	bool isEnvmap ();
	bool isEnd ();
	void next ();
};
typedef boost::shared_ptr<InputLoader> PtrInputLoader;