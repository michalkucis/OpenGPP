#pragma once

#include "PostProcessor.h"
#include "Ptr.h"
#include "Application.h"

class ApplicationPP: public Application
{
	string m_xmlFilename;
	string m_xmlCommands;

	Ptr<PostProcessor> m_pp;
	int m_numCycles;

protected:
	uint getWindowWidth(string xmlFilename, string xmlCommands);
	uint getWindowHeight(string xmlFilename, string xmlCommands);
public:
	ApplicationPP (string xmlFilename, string xmlCommands);
	void initData ();
	void render ();
	void clearData ();
};