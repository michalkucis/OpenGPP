#include "xml.h"
#include "hdrimage.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLString.hpp>

#include <sstream>

using namespace xercesc;



#define DEF(SZ) XMLCh* sz##SZ = XMLString::transcode (#SZ)
#define REL(SZ) XMLString::release (&(sz##SZ))


string XMLexception::get ()
{
	std::ostringstream oss;
	oss << "<camera_dataset>";
	if (!m_camera.empty() || !m_version.empty())
		oss << "/<camera name=" << m_camera << " version=" << m_version << ">";
	if (0 != m_node.compare (""))
		oss << "/<" << m_node << ">";
	if (0 != m_desc.compare (""))
		oss << ": " << m_desc;
	return oss.str ();
}

bool XMLbase::checkAttribute (xercesc::DOMNode* node, string name, string value)
{
	XMLCh* xmlname = XMLString::transcode (name.c_str());
	XMLCh* xmlvalue = XMLString::transcode (value.c_str());
	for (uint i = 0; i < node->getAttributes()->getLength(); i++)
	{
		for (uint i = 0; i < node->getAttributes()->getLength(); i++)
		{
			const XMLCh* attr = node->getAttributes()->item(i)->getNodeName();
			const XMLCh* content = node->getAttributes()->item(i)->getTextContent();
			
			if (0 == XMLString::compareIString (xmlname, attr)
				&& 0 == XMLString::compareIString (xmlvalue, content))
			{
				XMLString::release (&xmlvalue);
				XMLString::release (&xmlname);
				return true;
			}
		}
	}
	XMLString::release (&xmlvalue);
	XMLString::release (&xmlname);
	return false;
}

xercesc::DOMNode* XMLbase::getChild (std::string name, xercesc::DOMNode* parent)
{
	DOMNode* ret = NULL;
	XMLCh* xmlname = XMLString::transcode (name.c_str());
	
	xercesc::DOMElement* element = static_cast<DOMElement*> (parent);
	if (! element)
		throwInvalid ("This node is not element");

	DOMNodeList* list = element->getElementsByTagName (xmlname);
	XMLString::release (&xmlname);

	if (! list->getLength ())
		throwNotFound (name);

	return list->item(0);
/*	for (uint i = 0; i < parent->getChildNodes()->getLength(); i++)
	{
		DOMNode* child = parent->getChildNodes()->item(i);
		uint count = child->getChildNodes()->getLength();
		const XMLCh* name = child->getTextContent ();

		//const XMLCh* xmllocalname = child->getLocalName();
		if (0 == XMLString::compareIString (name, xmlname))
			ret = child;
	}
*/	

}

bool XMLbase::getAttributeBool (xercesc::DOMNode* node, string name)
{
	bool btrue = checkAttribute (node, name, "true");
	bool bfalse = checkAttribute (node, name, "false");
	if (btrue != bfalse)
		return btrue;
	
	string msg = "attribute 'bool ";
	msg += name;
	msg += "'";
	throwInvalid (msg);
	return false;
}

string XMLbase::getAttributeString (xercesc::DOMNode* node, string name)
{
	XMLCh* xmlname = XMLString::transcode (name.c_str());
	for (uint i = 0; i < node->getAttributes()->getLength(); i++)
	{
		//for (uint i = 0; i < node->getAttributes()->getLength(); i++)
		//{
		const DOMNode* attrnode = node->getAttributes()->item(i);
		const DOMAttr* attr = static_cast<const DOMAttr*> (attrnode);
		
		const XMLCh* attrname = attr->getName ();
			
		if (0 == XMLString::compareIString (xmlname, attrname))
		{
			XMLString::release (&xmlname);
			const XMLCh* xmlvalue = node->getAttributes()->item(i)->getTextContent();
			char* value = XMLString::transcode (xmlvalue);
			string strval = value;
			delete [] (value);
			return strval;
		}
		//}

	}
	XMLString::release (&xmlname);
	throwNotFound (name);
	return "";
}

int XMLbase::getAttributeInt (DOMNode* node, string name)
{
	string str = getAttributeString (node, name);
	int i;
	std::stringstream ss(str); 
  
	if( (ss >> i).fail() )
	{
		string msg = "attribute 'int ";
		msg += name;
		msg += "'";
		throwInvalid (msg);
	}     
	return i;
}

uint XMLbase::getAttributeUInt (xercesc::DOMNode* node, string name)
{
	string str = getAttributeString (node, name);
	uint i;
	std::stringstream ss(str); 
  
	if( (ss >> i).fail() )
	{
		string msg = "attribute 'uint ";
		msg += name;
		msg += "'";
		throwInvalid (msg);
	}     
	return i;
}

float XMLbase::getAttributeFloat (xercesc::DOMNode* node, string name)
{
	string str = getAttributeString (node, name);
	float i;
	std::stringstream ss(str); 
  
	if( (ss >> i).fail() )
	{
		string msg = "attribute 'float ";
		msg += name;
		msg += "'";
		throwInvalid (msg);
	}     
	return i;
}

XMLfuncInter::XMLfuncInter(string camera, string version, xercesc::DOMNode* node):
		XMLfunc (camera, version)
{
	xercesc::DOMElement* elem = dynamic_cast <xercesc::DOMElement*> (node);
	DEF(point2);
	xercesc::DOMNodeList* list = elem->getElementsByTagName (szpoint2);
	m_points = new float2 [list->getLength()+2];
	for (uint i = 0; i < list->getLength(); i++)
	{
		xercesc::DOMNode* node = list->item(i);
		float2 v = getFloat2 (node);
		m_points[i+1] = v;
	}
	m_points[0] = float2 (-FLT_MAX, 0);
	m_points[list->getLength()+1] = float2 (FLT_MAX, 0);
	REL(point2);
}

template <uint Degree>
XMLfuncPolynome<Degree>::XMLfuncPolynome(string camera, string version, xercesc::DOMNode* node):
		XMLfunc (camera, version)
{
	xercesc::DOMElement* elem = dynamic_cast <xercesc::DOMElement*> (node);
	DEF(value);
	xercesc::DOMNodeList* list = elem->getElementsByTagName (szvalue);
	if (list->getLength() != Degree+1)
		throwInvalid ("'degree' != count of the 'value's");
	for (uint i = 0; i < list->getLength(); i++)
	{
		xercesc::DOMNode* node = list->item(i);
		const XMLCh* text =  node->getTextContent ();
		char* sz = XMLString::transcode (text);
		std::stringstream ss (sz);
		float f;
		if( (ss >> f).fail() )
		{
			throwInvalid ("a 'value' in the func");
		} 
		delete [] (sz);
		int index = Degree - i;
		m_coef[index] = f;
	}
	REL(value);
}

void XMLbase::getGrid (xercesc::DOMNode* node, HDRImage1f& gridX, HDRImage1f& gridY)
{
	uint2 size; //= getPoint2D (node);
	size.x = getAttributeInt (node, "countX");
	size.y = getAttributeInt (node, "countY");
	xercesc::DOMElement* elem = dynamic_cast <xercesc::DOMElement*> (node);
	DEF(point2);
	xercesc::DOMNodeList* list = elem->getElementsByTagName (szpoint2);
	if (list->getLength() != size.getArea())
		throwInvalid ("'size of grid' != count of the 'point2's");
	gridX.setSize (size);
	gridY.setSize (size);
	for (uint i = 0; i < list->getLength(); i++)
	{
		DOMNode* node = list->item (i);
		float2 f2 = getFloat2 (node);
		gridX.setPixel (uint2 (i%size.x, i/size.x), f2.x);
		gridY.setPixel (uint2 (i%size.x, i/size.x), f2.y);
	}
	REL(point2);
}


float XMLbase::getFloat (xercesc::DOMNode* node)
{
	float x;
	x = getAttributeFloat (node, "x");
	return x;
}

float2 XMLbase::getFloat2  (xercesc::DOMNode* node)
{
	float x, y;
	x = getAttributeFloat (node, "x");
	y = getAttributeFloat (node, "y");
	return float2 (x, y); 
}

float3 XMLbase::getFloat3  (xercesc::DOMNode* node)
{
	float x, y, z;
	x = getAttributeFloat (node, "x");
	y = getAttributeFloat (node, "y");
	z = getAttributeFloat (node, "z");
	return float3 (x, y, z); 
}

PtrXMLfunc XMLbase::getChildFunc (xercesc::DOMNode* node)
{
	DOMNode* nodefunc = getChild ("func", node);
	if ((getAttributeString (nodefunc, "type")) == "lininter")
		return PtrXMLfunc (new XMLfuncInter (m_strCamera, m_strVersion, nodefunc));
	else if (getAttributeString (nodefunc, "type") == "polynomial")
	{
		switch (getAttributeInt (nodefunc, "degree"))
		{
#define CASE_N(n) case n: return PtrXMLfunc (new XMLfuncPolynome<n> (m_strCamera, m_strVersion, nodefunc)); break;
		CASE_N(0)
		CASE_N(1)
		CASE_N(2)
		CASE_N(3)
		CASE_N(4)
		CASE_N(5)
		CASE_N(6)
		CASE_N(7)
		CASE_N(8)
		CASE_N(9)
		CASE_N(10)
		CASE_N(11)
		CASE_N(12)
		CASE_N(13)
		CASE_N(14)
		CASE_N(15)
		CASE_N(16)
#undef CASE_N
		default:
			throwInvalid ("degree of the function is not implemented");
		}
	}
	else 
		throwInvalid ("type of the function");
	PtrXMLfunc null;
	return null;
}

XMLfeatures::XMLfeatures (xercesc::DOMNode* node):
	XMLbase("","","features")
{
	xercesc::DOMElement* elem = dynamic_cast <xercesc::DOMElement*> (node);

	xercesc::DOMElement* child = elem->getFirstElementChild();
	if (! child)
	{
		throwNotFound("features");
	}
	do 
	{
		char* szNodeName = XMLString::transcode(child->getNodeName());
		string nodeName = szNodeName;
		free(szNodeName);
		string name = getAttributeString (child, "name");

#define CASE(NAME,TYPE) (nodeName==string(NAME)) {f.type=TYPE; f.strType=string(NAME); f.name=name;}
		feature_t f;
		if CASE("vignettingRadial", VIGNETTING_RADIAL)
		else if CASE("vignettingMask", VIGNETTING_MASK)
		else if CASE("distortionRadial", DISTORTION_RADIAL)
		else if CASE("distortionGrid", DISTORTION_GRID)
		else if CASE("chromaberRadial", CHROMABER_RADIAL)
		else if CASE("blur", BLUR)
		else if CASE("depthoffield", DEPTHOFFIELD)
		else if CASE("lensflareSimple", LENSFLARE_SIMPLE)
		else if CASE("motionblurParam", MOTIONBLUR_PARAM)

		else if CASE("apertureAdapt", APERTUREADAPT)
		else if CASE("clampValues", CLAMPVALUES)
		else if CASE("colorfilterarray", COLORFILTERARRAY)
		else if CASE("noiseSimple", NOISE_SIMPLE)
		else if CASE("transformValues", TRANSFORMVALUES)
		else
			throwInvalid("feature");
		vecFeatures.push_back(f);
	} while (child = child->getNextElementSibling ());

}

XMLparser::XMLparser (string filename):
	XMLbase ("", "")
{
	try
	{
		XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
	}
	catch (XMLException& e)
	{
		char* message = XMLString::transcode (e.getMessage());
		string str = message; 
		XMLString::release (&message);
		throwException (str);
	}
	m_parser = sharedNew<XercesDOMParser> ();
	m_parser->setValidationScheme (XercesDOMParser::Val_Never);
	m_parser->setDoNamespaces (false);
	m_parser->setDoSchema (false);
	m_parser->setLoadExternalDTD (false);

	m_parser->parse (filename.c_str());
	DOMDocument* doc = m_parser->getDocument();
	if (! doc)
		throwException ("xml file cannot be loaded");
}


PtrXMLinput XMLparser::parseInput (string inputName)
{
	DOMDocument* doc = m_parser->getDocument();
	DEF(input);

	DOMElement* root = doc->getDocumentElement ();
	DOMNodeList* inputs = root->getElementsByTagName (szinput);

	bool valid = false;
	for (uint i = 0; i < inputs->getLength(); i++)
	{
		DOMNode* node = inputs->item(i);
		if (checkAttribute (node, "name", inputName))
		{
			REL(input);
			return sharedNew<XMLinput> (node, this);	
		}
	}
	REL(input);

	if (! valid)
		throwNotFound ("input");

	return sharedNull<XMLinput>();
}


PtrXMLcommands XMLparser::parseCommands (string name)
{
	DOMDocument* doc = m_parser->getDocument();
	DEF(commands);

	DOMElement* root = doc->getDocumentElement ();
	DOMNodeList* inputs = root->getElementsByTagName (szcommands);

	bool valid = false;
	for (uint i = 0; i < inputs->getLength(); i++)
	{
		DOMNode* node = inputs->item(i);
		if (checkAttribute (node, "name", name))
		{
			REL(commands);
			return sharedNew<XMLcommands> (node);	
		}
	}
	REL(commands);

	if (! valid)
		throwNotFound ("input");

	return sharedNull<XMLcommands>();
}



PtrXMLoutput XMLparser::parseOutput (string outputName)
{
	DOMDocument* doc = m_parser->getDocument();
	DEF(output);

	DOMElement* root = doc->getDocumentElement ();
	DOMNodeList* inputs = root->getElementsByTagName (szoutput);

	bool valid = false;
	for (uint i = 0; i < inputs->getLength(); i++)
	{
		DOMNode* node = inputs->item(i);
		if (checkAttribute (node, "name", outputName))
		{
			REL(output);
			return sharedNew<XMLoutput> (node);
		}
	}
	REL(output);

	if (! valid)
		throwNotFound ("input");

	return sharedNull<XMLoutput>();
}


XMLcommands::XMLcommands (xercesc::DOMNode* node):
	XMLbase ("", "", "input")
{
	xercesc::DOMElement* elem = dynamic_cast <xercesc::DOMElement*> (node);

	xercesc::DOMElement* child = elem->getFirstElementChild();
	if (child == NULL)
		return;
	do 
	{
		const char* name = XMLString::transcode(child->getNodeName());
		Command_t cmd;
		if (0==strcmp("commit",name))
			cmd.cmd = COMMIT;
		else if (0==strcmp("input",name))
		{
			cmd.cmd = INPUT;
			string param = getAttributeString(child, "name");
			cmd.param = param;
		}
		else if (0==strcmp("output",name))
		{
			cmd.cmd = OUTPUT;
			string param = getAttributeString(child, "name");
			cmd.param = param;
		}
		else if (0==strcmp("features",name))
		{
			cmd.cmd = FEATURES;
			string param = getAttributeString(child, "name");
			cmd.param = param;
		}
		else
			throwException("Unknown command");
		vecCmds.push_back(cmd);
	} while (child = child->getNextElementSibling ());
}

XMLinput::XMLinput (xercesc::DOMNode* node, XMLparser* parser):
		XMLbase ("", "", "input")
{
	xercesc::DOMElement* elem = dynamic_cast <xercesc::DOMElement*> (node);
	
	string strtype = getAttributeString (node, "type");
	if (strtype == "single")
		type = SINGLE;
	else if (strtype == "sequence")
	{
		type = SEQUENCE;
		xercesc::DOMNode* nodeseq = getChild ("sequence", node);
		sequence.start = getAttributeInt (nodeseq, "start");
		sequence.end = getAttributeInt (nodeseq, "end");
		sequence.step = getAttributeInt (nodeseq, "step");
		sequence.repeat = getAttributeInt (nodeseq, "repeat");
	}
	else if (strtype == "list")
	{
		type = LIST;
		
		DEF(ref);
		xercesc::DOMNodeList* list = elem->getElementsByTagName (szref);
		for (uint i = 0; i < list->getLength(); i++)
		{
			xercesc::DOMNode* node = list->item(i);
			string nodename = getAttributeString(node,"name");
			PtrXMLinput xmlinput = parser->parseInput (nodename);

			listRefs.push_back (xmlinput);
		}
		REL(ref);		
		return;
	}
	else if (strtype == "video")
	{
		type = VIDEO;
		videoColorPath = getAttributeString(getChild ("color", node),"path");
		videoColorMult = getAttributeFloat(getChild ("color", node),"mult");
		videoLib = getAttributeString(getChild ("library", node),"name");
		return;	
	}
	else
		throwInvalid ("type");
	DEF(file);
	xercesc::DOMNodeList* list = elem->getElementsByTagName (szfile);
	for (uint i = 0; i < list->getLength(); i++)
	{
		xercesc::DOMNode* node = list->item(i);
		files.push_back (XMLinputFile (node));
	}
	REL(file);

	colorMult = XMLbase::getAttributeFloat(getChild ("color", node), "mult");
	depthLinearize = XMLbase::getAttributeBool(getChild ("depth", node), "linearize");
	depthMult = XMLbase::getAttributeFloat(getChild ("depth", node), "mult");
	depthAdd = XMLbase::getAttributeFloat(getChild ("depth", node), "add");
	depthN = XMLbase::getAttributeFloat(getChild ("depth", node), "linearizeNear");
	depthF = XMLbase::getAttributeFloat(getChild ("depth", node), "linearizeFar");

	icosmult = XMLbase::getAttributeFloat(getChild ("depth", node), "icosmult");
	icosanglemult = XMLbase::getAttributeFloat(getChild ("depth", node), "anglemult");
}
		
PtrXMLinput XMLinput::clone ()
{
	PtrXMLinput ptr = sharedNew<XMLinput> ();
#define copy(A) {ptr->A = A;}
	copy(colorMult);
	copy(depthAdd);
	copy(depthF);
	copy(depthLinearize);
	copy(depthMult);
	copy(depthN);
	copy(files);
	copy(icosanglemult);
	copy(icosmult);
	copy(sequence);
	copy(type);
	copy(listRefs);
	copy(videoLib);
	copy(videoColorMult);
	copy(videoColorPath);
#undef copy
	return ptr;
}

XMLoutput::XMLoutput (xercesc::DOMNode* node):	
	XMLbase ("", "", "output")
{
	m_isSingleImageOutput = m_isSequence = m_isVideo = false;
	string strtype = getAttributeString (node, "type");
	if (strtype == "nosave")
	{
	}
	else if (strtype == "image")
	{
		m_isSingleImageOutput = true;
		m_singleImageFilename = XMLbase::getAttributeString(getChild ("file", node), "name");
	}
	else if (strtype == "sequence")
	{
		m_isSequence = true;
		xercesc::DOMNode* seqnode = getChild ("sequence", node);
		m_sequence.start = XMLbase::getAttributeInt(seqnode, "start");
		m_sequence.end = XMLbase::getAttributeInt(seqnode, "end");
		m_sequence.step = XMLbase::getAttributeInt(seqnode, "step");
		m_sequence.filename = XMLbase::getAttributeString (getChild("file", node), "name");
	}
	else if (strtype == "video")
	{
		m_isVideo = true;
		xercesc::DOMNode* videonode = getChild ("video", node);
		
		m_videoFPS = XMLbase::getAttributeFloat (videonode, "fps");
		m_videoFilename = XMLbase::getAttributeString (videonode, "filename");
		m_videoFourcc = XMLbase::getAttributeString (videonode, "fourcc");
		m_videoRepeatOneFrame = XMLbase::getAttributeInt (videonode, "repeatFrame");
		if (4 != m_videoFourcc.size())
			throwInvalid ("invalid fourcc");
		m_videoLib = XMLbase::getAttributeString (getChild("library", node), "name");
	}
	else
		throwInvalid ("type");


	if (strtype == "image" || strtype == "sequence")
	{
		string type = XMLbase::getAttributeString(getChild ("file", node), "type");
		if (type == "fixed")
			m_imageType = FIXED;
		else if (type == "exr")
			m_imageType = EXR;
		else
			throwInvalid ("file type");
	}

	m_wndVisibled = getAttributeBool (getChild ("window", node), "enabled");
	if (m_wndVisibled)
	{
		m_wndSize.x = getAttributeUInt (getChild ("window", node), "x");
		m_wndSize.y = getAttributeUInt (getChild ("window", node), "y");
	}
}


#define PARSE_(TYPE, FUNC_NAME, SZ_NODE_NAME) \
	boost::shared_ptr<TYPE> XMLparser::FUNC_NAME (string name) \
	{ \
	using namespace xercesc; \
	DOMDocument* doc = m_parser->getDocument();\
	XMLCh* xmlchName = XMLString::transcode (SZ_NODE_NAME);\
	\
	DOMElement* root = doc->getDocumentElement ();\
	DOMNodeList* inputs = root->getElementsByTagName (xmlchName);\
	\
	bool valid = false;\
	for (uint i = 0; i < inputs->getLength(); i++)\
	{\
	DOMNode* node = inputs->item(i);\
	if (checkAttribute (node, "name", name))\
	{\
	XMLString::release (&xmlchName);\
	return sharedNew<TYPE> (node);\
}\
}\
	XMLString::release (&xmlchName);\
	\
	if (! valid)\
	throwNotFound (SZ_NODE_NAME);\
	\
	return sharedNull<TYPE>();\
}

PARSE_(XMLchromAberration, parseChromaber, "chromaberRadial");
PARSE_(XMLvignettingMask, parseVignettingMask, "vignettingMask")
PARSE_(XMLvignettingRadial, parseVignettingRadial, "vignettingRadial")
PARSE_(XMLfeatures, parseFeatures, "features")
PARSE_(XMLdistortion, parseDistortionRadial, "distortionRadial")
PARSE_(XMLdistortion, parseDistortionGrid, "distortionGrid")
PARSE_(XMLblur, parseBlur, "blur")
PARSE_(XMLlensflareSimple, parseLensflareSimple, "lensflareSimple")
PARSE_(XMLmotionblurParam, parseMotionblurParam, "motionblurParam")
PARSE_(XMLdepthoffield, parseDepthoffield, "depthoffield")

PARSE_(XMLapertureAdapt, parseAdaptation, "apertureAdapt")
PARSE_(XMLclampValues, parseClampValues, "clampValues")
PARSE_(XMLnoiseSimple, parseNoiseSimple, "noiseSimple")
PARSE_(XMLcolorfilterarray, parseCFA, "colorfilterarray")
PARSE_(XMLtransformValues, parseTransformValues, "transformValues")


void XMLdistortion::processRadialDistortion (xercesc::DOMNode* node)
{
	uint2 gridSize;
	{
		gridSize.x = getAttributeUInt (getChild ("sizeOfGrid", node), "x"); 
		gridSize.y = getAttributeUInt (getChild ("sizeOfGrid", node), "y"); 
	}
	PtrXMLfunc func = getChildFunc (node);

	float3 mult[2];
	{
		xercesc::DOMNode* param;
		param = getChild ("param", node);
		mult[0].x = getAttributeFloat (param, "alpha");
		mult[0].y = getAttributeFloat (param, "gamma");
		mult[0].z = getAttributeFloat (param, "u0");
		mult[1].x = 0;
		mult[1].y = getAttributeFloat (param, "beta");
		mult[1].z = getAttributeFloat (param, "v0");
	}

	//
	m_gridX = PtrHDRImage1f (new HDRImage1f);
	m_gridY = PtrHDRImage1f (new HDRImage1f);

	m_gridX->setSize (gridSize);
	m_gridY->setSize (gridSize);
	for (uint y = 0; y < gridSize.y; y++)
		for (uint x = 0; x < gridSize.x; x++)
		{
			float rx = x/((float)gridSize.x-1)*2-1;
			float ry = y/((float)gridSize.y-1)*2-1;

			float prevr;
			float r = prevr = sqrtf(rx*rx+ry*ry) / sqrtf(2);
			r = (*func) (r);

			rx *= r/prevr;
			ry *= r/prevr;

			// rx = <-1;1> 			
			//use coef:

			float _rx = rx*mult[0].x + ry*mult[0].y + mult[0].z;
			float _ry = rx*mult[1].x + ry*mult[1].y + mult[1].z;

			rx = (_rx+1)/2;
			ry = (_ry+1)/2;

			//set to grid:
			m_gridX->setPixel(uint2(x,y), rx);
			m_gridY->setPixel(uint2(x,y), ry);
		}
}


XMLdistortion::XMLdistortion (xercesc::DOMNode* node):
	XMLbase ("", "", "distortion")
{
	xercesc::DOMElement* elem = dynamic_cast <xercesc::DOMElement*> (node);

	XMLCh* xmlchDistortionRadial = XMLString::transcode ("distortionRadial");
	const XMLCh* xmlchNodeName = elem->getNodeName();

	if (XMLString::compareIString(xmlchDistortionRadial, xmlchNodeName))
	{
		m_gridX = PtrHDRImage1f (new HDRImage1f);
		m_gridY = PtrHDRImage1f (new HDRImage1f);

		HDRImage1f& gridX = *(m_gridX.get());
		HDRImage1f& gridY = *(m_gridY.get());
		XMLbase::getGrid (getChild ("grid", node), gridX, gridY);
	}
	else
	{
		processRadialDistortion (node);
	}
	XMLString::release(&xmlchDistortionRadial);
}

XMLchromAberration::XMLchromAberration (DOMNode* node):
	XMLbase ("", "", "chromatic_abberation")
{
	m_sizeOfGrid.x = getAttributeUInt (getChild ("sizeOfGrid", node), "x");
	m_sizeOfGrid.y = getAttributeUInt (getChild ("sizeOfGrid", node), "y");

	DOMNode* nodered = getChild ("red_cyan", node);
	m_redcyan = getChildFunc (nodered);

	DOMNode* nodeblue = getChild ("blue_yellow", node);
	m_blueyellow = getChildFunc (nodeblue);
}

XMLblur::XMLblur (xercesc::DOMNode* node):
	XMLbase ("","","blur")
{
	m_func = getChildFunc (node);
}

XMLlensflareSimple::XMLlensflareSimple (xercesc::DOMNode* node):
XMLbase ("", "", "lensflareSimple")
{
	m_envmapangle.x = getAttributeFloat (getChild("envmap", node), "angleX");
	m_envmapangle.y = getAttributeFloat (getChild("envmap", node), "angleY");
	m_sizeofenvmap.x = getAttributeUInt (getChild("envmap", node), "sizeX");
	m_sizeofenvmap.y = getAttributeUInt (getChild("envmap", node), "sizeY");

	m_fovangle.x = getAttributeFloat (getChild("fov", node), "angleX");
	m_fovangle.y = getAttributeFloat (getChild("fov", node), "angleY");

	m_starMult  = getAttributeFloat (getChild("params", node), "starMult");
	m_csMult1 = getAttributeFloat (getChild("params", node), "csMult1");
	m_csMult2 = getAttributeFloat (getChild("params", node), "csMult2");
}

XMLmotionblurParam::XMLmotionblurParam (DOMNode* node):
	XMLbase ("", "", "motion_blur")
{
	numIterations = getAttributeUInt (getChild("iterations", node), "number");

	deltaCameraPos = getFloat3(getChild("point3", getChild ("deltapos", node)));
	deltaTarget = getFloat2 (getChild("point2", getChild ("deltatarget", node)));
	targetDist = getFloat (getChild("float", getChild ("distancetarget", node)));
	fovAngle = getFloat2 (getChild("point2", getChild ("fovangle", node)));
}

XMLdepthoffield::XMLdepthoffield (DOMNode* node):
	XMLbase ("", "", "depth_of_field")
{
	string type = getAttributeString (getChild ("type", node), "name");
	if (type == "gather")
		m_type = TYPE_GATHER;
	else if (type == "diffusion")
		m_type = TYPE_DIFFUSION;
	else
		throwInvalid ("type name");

	type = getAttributeString (getChild ("aperture", node), "type");
	if (type == "fixed")
	{
		m_apertureIsFixed = true;
		m_apertureValue = getAttributeFloat (getChild ("aperture", node), "fnum");
	}
	else if (type == "adapt")
	{
		m_apertureIsFixed = false;
	}
	else
		throwInvalid ("aperture type");

	type = getAttributeString (getChild ("distance", node), "type");
	if (type == "fixed")
	{
		//m_focusIsFixed = true;
		m_focusDist = getAttributeFloat (getChild ("distance", node), "depth");
	}
	else
		throwInvalid ("distance");

	DOMNode* params = getChild ("parameters", node);
	m_sensorSize = getAttributeFloat (params, "sensorSize");
	m_focalLen = getAttributeFloat (params, "focalLen");
}


XMLapertureAdapt::XMLapertureAdapt (xercesc::DOMNode* basenode):
	XMLbase ("", "", "apertureAdapt")
{
	{
		xercesc::DOMNode* node = getChild ("param", basenode);
		string type = getAttributeString (node, "type");
		if (type == "fixed")
			m_type = TYPE_FIXED;
		else if (type == "adapt")
			m_type = TYPE_ADAPT;
		else
			throwInvalid ("type");

		m_fnum = getAttributeFloat (node, "fnum");
		m_brightness = getAttributeBool (node, "brightness");
	}

	if (m_type == TYPE_ADAPT)
	{
		xercesc::DOMNode* node = getChild ("adapt", basenode);
		m_param.middleGray = getAttributeFloat (node, "middlegray");
		m_param.minFnum = getAttributeFloat (node, "minFnum");
		m_param.maxFnum = getAttributeFloat (node, "maxFnum");
		m_param.maxSpeed = getAttributeFloat (node, "maxSpeed");
		m_param.Kp = getAttributeFloat (node, "PIregKp");
		m_param.Ki = getAttributeFloat (node, "PIregKi"); 
	}
}


XMLcolorfilterarray::XMLcolorfilterarray (xercesc::DOMNode* basenode):
	XMLbase ("", "", "colorfilterarray")
{
	for (uint i = 0; i < 3; i++)
	{
		string color;
		FeatureColorFilterArrayParam* p; 
		switch (i)
		{
		case 0: color="red"; p = &m_red;break;
		case 1: color="green"; p = &m_green;break;
		case 2: color="blue"; p = &m_blue;break;
		}
		xercesc::DOMNode* node = getChild(color, basenode);
		{//mask:
			DOMNode* nodemask = getChild("mask", node);
			uint x = getAttributeUInt (nodemask, "x");
			uint y = getAttributeUInt (nodemask, "y");
			string mask = getAttributeString (nodemask, "mask");
			p->mask->setSize (uint2 (x,y));
			if (mask.length() != x*y)
				throwInvalid ("mask length");
			for (uint i = 0; i < mask.length(); i++)
			{
				float value;
				char c = mask[i];
				if (c == '0')
					value = 0;
				else if (c == '1')
					value = 1;
				else
					throwInvalid ("mask");

				p->mask->setPixel (uint2(i%x, i/x), value);	
			}
		}
		{ // kernel:
			DOMNode* nodekernel = getChild ("kernel", node);
			int ix = getAttributeInt (nodekernel, "offsetX");
			int iy = getAttributeInt (nodekernel, "offsetY");
			p->kernelOff = int2 (ix, iy);

			uint x = getAttributeUInt (nodekernel, "sizeX");
			uint y = getAttributeUInt (nodekernel, "sizeY");
			p->kernel->setSize (uint2 (x, y));

			xercesc::DOMElement* elem = dynamic_cast <xercesc::DOMElement*> (node);
			DEF(value);
			xercesc::DOMNodeList* list = elem->getElementsByTagName (szvalue);
			for (uint i = 0; i < list->getLength(); i++)
			{
				xercesc::DOMNode* node = list->item(i);
				float f = getFloat (node);
				p->kernel->setPixel(uint2(i%x, i/x), f);
			}
			REL(value);
		}
	}
}


XMLclampValues::XMLclampValues (xercesc::DOMNode* node):
	XMLbase ("", "", "clamp")
{
	m_maxValue = getAttributeFloat(getChild("max", node),"value");
}


XMLnoiseSimple::XMLnoiseSimple (xercesc::DOMNode* node):
	XMLbase ("","","noiseSimple")
{
	m_funcSignalToNoise = getChildFunc (node);
	m_min = getAttributeFloat (getChild("limit", node), "min");
	m_max = getAttributeFloat (getChild("limit", node), "max"); 
}


XMLtransformValues::XMLtransformValues (xercesc::DOMNode* node):
	XMLbase ("","","transformValues")
{
	m_func = getChildFunc (node);
}