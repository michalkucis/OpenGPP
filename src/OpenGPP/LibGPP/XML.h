#pragma once

#include <xercesc\util\Xerces_autoconf_config.hpp>
#include <xercesc\dom\DOMElement.hpp>
#include "Base.h"
#include "Vector.h"
#include "Ptr.h"
#include "Map.h"

#pragma comment (lib, "xerces-c_3D.lib")


class XMLAttr
{
	string m_key;
public:
	XMLAttr(string key)
	{
		m_key = key;
	}
	string getKey()
	{
		return m_key;
	}
	virtual float getFloat();
	virtual int getInt();
	virtual string getString();
	virtual bool getBool();
};



class XMLNode
{
protected:
	string m_tag;
	Map<string, Ptr<XMLAttr>> m_mapAttrs;
	Map<string, Vector<Ptr<XMLNode>>> m_mapVecNodes;
	Vector<Ptr<XMLNode>> m_vecAllNodes;

	string convertXMLString2String(const XMLCh* xmlChs);
	void addAttr(string key, string value);
	void parseXercescXMLNode(xercesc::DOMElement* element);
public:
	XMLNode();
	XMLNode(xercesc::DOMElement* e);
	~XMLNode();
	string getTag();	
	Vector<Ptr<XMLNode>> getVecNodes();
	Vector<Ptr<XMLNode>> getVecNodes(string tag);
	bool existNode(string tag);
	Ptr<XMLNode> getNode(string tag);
	Ptr<XMLNode> getNode(string tag, string attrName, string attrValue);
	Ptr<XMLAttr> getAttr(string attrName);
	bool existAttr(string attrName);
	string getAttrString(string attrName);
	float getAttrFloat(string attrName);
	int getAttrInt(string attrName);
	bool getAttrBool(string attrName);
	void parseFile(string filename);
};