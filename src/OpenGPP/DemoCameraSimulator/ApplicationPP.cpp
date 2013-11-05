#include "ApplicationPP.h"
#include "ObjectsLoadedFromXML.h"
#include "XML.h"

uint ApplicationPP::getWindowWidth(string xmlFilename, string xmlCommands)
{
	Ptr<XMLNode> nodeRoot = new XMLNode;
	nodeRoot->parseFile(xmlFilename);
	Ptr<XMLNode> nodeCommand = nodeRoot->getNode("command", "name", xmlCommands);
	Ptr<XMLNode> nodeWindow = nodeCommand->getNode("window");
	return nodeWindow->getAttrInt("sizeX");
}

uint ApplicationPP::getWindowHeight(string xmlFilename, string xmlCommands)
{
	Ptr<XMLNode> nodeRoot = new XMLNode;
	nodeRoot->parseFile(xmlFilename);
	Ptr<XMLNode> nodeCommand = nodeRoot->getNode("command", "name", xmlCommands);
	Ptr<XMLNode> nodeWindow = nodeCommand->getNode("window");
	return nodeWindow->getAttrInt("sizeY");
}

ApplicationPP::ApplicationPP (string xmlFilename, string xmlCommands): 
	Application(getWindowWidth(xmlFilename, xmlCommands),getWindowHeight(xmlFilename, xmlCommands))
{
	m_xmlFilename = xmlFilename;
	m_xmlCommands = xmlCommands;
}

void ApplicationPP::initData ()
{
	ObjectsLoadedFromXML factory(m_xmlFilename, m_xmlCommands);
	m_pp = factory.getPostProcessor();
	m_numCycles = factory.getNumberCycles();
}

void ApplicationPP::render ()
{
	if (0 == m_numCycles)
	{
		m_quitApplication = true;
		return;
	}
	else
	{
		--m_numCycles;
		m_pp->process();
		Application::render();
	}
}

void ApplicationPP::clearData ()
{		
	m_pp->clear ();
	m_pp = NULL;
}