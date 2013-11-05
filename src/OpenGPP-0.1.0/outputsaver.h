#pragma once

#include <boost\shared_ptr.hpp>

#include "xml.h"
#include "hdrimage.h"

class OutputSaverAbstract;

class OutputSaver
{	
	OutputSaverAbstract* m_delegator;

public:
	OutputSaver (std::string filename, std::string outputName);
	~OutputSaver ();
	void save (PtrHDRImage3f image);
	bool isWndVisibled ();
	int2 getWndSize ();
	//bool next ();
	bool isEnd ();
	PtrXMLoutput getXMLoutput();
};
typedef boost::shared_ptr<OutputSaver> PtrOutputSaver;