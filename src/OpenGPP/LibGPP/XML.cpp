#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMAttr.hpp>
#include <iostream>
#include <sstream>

#include "XML.h"
#include "Error.h"
#include "Vector.h"
#include "Ptr.h"
#include "Map.h"


//================================================================================================


class XMLAttrString: public XMLAttr
{
	string m_value;
public:
	string getString()
	{
		return m_value;
	}
	XMLAttrString(string key, string s): XMLAttr(key)
	{
		m_value = s;
	}
};

class XMLAttrFloat: public XMLAttr
{
	float m_value;
public:
	string getString()
	{
		return float2str(m_value);
	}
	float getFloat()
	{
		return m_value;
	}
	XMLAttrFloat(string key, float f): XMLAttr(key)
	{
		m_value = f;
	}
	XMLAttrFloat(string key, string s): XMLAttr(key)
	{
		std::stringstream ss(s);
		ss >> m_value;
		if (! ss.eof())
			error0(ERR_XML, "The function parameter does not contain converted float to string");
	}
};

class XMLAttrInt: public XMLAttr
{
	int m_value;
public:
	string getString()
	{
		return int2str(m_value);
	}
	float getFloat()
	{
		return (float) m_value;
	}
	int getInt()
	{
		return m_value;
	}
	XMLAttrInt(string key, int n): XMLAttr(key)
	{
		m_value = n;
	}
	XMLAttrInt(string key, string s): XMLAttr(key)
	{
		std::stringstream ss(s);
		ss >> m_value;
		if (! ss.eof())
			error0(ERR_XML, "The function parameter does not contain converted int to string");
	}
};

class XMLAttrBool: public XMLAttr
{
	bool m_value;
public:
	string getString()
	{
		return int2str(m_value);
	}
	bool getBool()
	{
		return m_value;
	}
	XMLAttrBool(string key, bool n): XMLAttr(key)
	{
		m_value = n;
	}
	XMLAttrBool(string key, string s): XMLAttr(key)
	{
		std::stringstream ss(s);
		string str;
		ss >> str;
		if(str == "true")
			m_value = true;
		else if(str == "false")
			m_value = false;
		else 
			error0(ERR_XML, "The function parameter does not contain converted int to string");
	}
};


//=============================================================================================


float XMLAttr::getFloat()
{
	error0(ERR_XML, "The attribute does not contain float value");
}

int XMLAttr::getInt()
{
	error0(ERR_XML, "The attribute does not contain integer value");
}

string XMLAttr::getString()
{
	error0(ERR_XML, "The attribute does not contain string value");
}

bool XMLAttr::getBool()
{
	error0(ERR_XML, "The attribute does not contain bool value");
}

//=============================================================================================


string XMLNode::convertXMLString2String( const XMLCh* xmlChs )
{
	char* chs = xercesc::XMLString::transcode(xmlChs);
	string s = chs;
	xercesc::XMLString::release(&chs);
	return s;
}

void XMLNode::addAttr( string key, string value )
{
	XMLAttr* attr = NULL;

	bool boolValue = false;
	if (value == "true" || value == "false")
		boolValue = true;
	std::stringstream ssInt(value);
	int n;
	ssInt >> n;
	std::stringstream ssFloat(value);
	float f;
	ssFloat >> f;

	if(boolValue)
		attr = new XMLAttrBool(key, value);
	else if(value.empty())
		attr = new XMLAttrString(key, "");
	else if(ssInt.eof())
		attr = new XMLAttrInt(key, n);
	else if(ssFloat.eof())
		attr = new XMLAttrFloat(key, f);
	else
		attr = new XMLAttrString(key, value);

	m_mapAttrs.insert(key, Ptr<XMLAttr>(attr));
}

void XMLNode::parseXercescXMLNode( xercesc::DOMElement* element )
{
	xercesc::DOMElement* e = element->getFirstElementChild();
	for(; e ; e = e->getNextElementSibling())
	{
		Ptr<XMLNode> node = new XMLNode(e);
		m_vecAllNodes.pushBack(node);
		string tag = node->getTag();
		if(m_mapVecNodes.exist(tag))
		{
			Vector<Ptr<XMLNode>>& vecNodes = m_mapVecNodes.find(tag);
			vecNodes.pushBack(node);
		}
		else
		{
			m_mapVecNodes.insert(node->getTag(), (Vector<Ptr<XMLNode>>) node);
		}
	}
	xercesc::DOMNamedNodeMap* map = element->getAttributes();
	XMLSize_t size = map->getLength();
	for(XMLSize_t i = 0; i < size; i++)
	{
		const xercesc::DOMNode* node = map->item(i);
		const xercesc::DOMAttr* attr = static_cast<const xercesc::DOMAttr*> (node);
		const XMLCh* xmlchAttrName = attr->getName();
		string attrName = convertXMLString2String(xmlchAttrName);

		const XMLCh* xmlchAttrValue = attr->getTextContent();
		string attrValue = convertXMLString2String(xmlchAttrValue);

		addAttr(attrName, attrValue);
	}
}

XMLNode::XMLNode()
{
}

XMLNode::XMLNode(xercesc::DOMElement* e)
{
	string tag = convertXMLString2String(e->getTagName());
	m_tag = tag;
	parseXercescXMLNode(e);
}

XMLNode::~XMLNode()
{
}

string XMLNode::getTag()
{
	return m_tag;
}

Vector<Ptr<XMLNode>> XMLNode::getVecNodes()
{
	return m_vecAllNodes;
}

Vector<Ptr<XMLNode>> XMLNode::getVecNodes(string tag)
{
	return m_mapVecNodes.find(tag);
}

bool XMLNode::existNode(string tag)
{
	return m_mapVecNodes.exist(tag);
}

Ptr<XMLNode> XMLNode::getNode(string tag)
{
	if (! m_mapVecNodes.exist(tag))
		error1(ERR_XML, "The node '%s' cannot be found", tag.c_str());
	return (m_mapVecNodes.find(tag))[0];
}

Ptr<XMLNode> XMLNode::getNode(string tag, string attrName, string attrValue)
{
	if(! existNode(tag))
		error3(ERR_XML, "The XML file does not contain valid <%s %s='%s'>", 
			tag.c_str(), attrName.c_str(), attrValue.c_str());
	Vector<Ptr<XMLNode>> vecNodes = getVecNodes(tag);
	for(uint i = 0; i < vecNodes.getSize(); i++)
	{
		Ptr<XMLNode> node = vecNodes[i];
		string attr = node->getAttrString(attrName);
		if (attr == attrValue)
			return node;
	}
	error3(ERR_XML, "The XML file does not contain valid <%s %s='%s'>", 
		tag.c_str(), attrName.c_str(), attrValue.c_str());
}

Ptr<XMLAttr> XMLNode::getAttr(string attrName)
{
	if (! m_mapAttrs.exist(attrName))
		error1(ERR_XML, "The attribute '%s' does not exist", attrName.c_str());
	return m_mapAttrs.find(attrName);
}

bool XMLNode::existAttr(string attrName)
{
	return m_mapAttrs.exist(attrName);
}

string XMLNode::getAttrString(string attrName)
{
	Ptr<XMLAttr> attr = getAttr(attrName);
	return attr->getString();
}

float XMLNode::getAttrFloat(string attrName)
{
	Ptr<XMLAttr> attr = getAttr(attrName);
	return attr->getFloat();
}

int XMLNode::getAttrInt(string attrName)
{
	Ptr<XMLAttr> attr = getAttr(attrName);
	return attr->getInt();
}

bool XMLNode::getAttrBool(string attrName)
{
	Ptr<XMLAttr> attr = getAttr(attrName);
	return attr->getBool();
}

void XMLNode::parseFile(string filename)
{
	xercesc::XMLPlatformUtils::Initialize();
	xercesc::XercesDOMParser parser;
	parser.setValidationScheme (xercesc::XercesDOMParser::Val_Never);
	parser.setDoNamespaces (false);
	parser.setDoSchema (false);
	parser.setLoadExternalDTD (false);

	parser.parse (filename.c_str());
	xercesc::DOMDocument* doc = parser.getDocument();
	if (! doc)
		error0 (ERR_XML, "The XML file cannot be opened");

	xercesc::DOMElement* root = doc->getDocumentElement ();
	if (! root)
		error0(ERR_XML, "The XML file is invalid");
	parseXercescXMLNode(root);
}
