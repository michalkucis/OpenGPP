/////////////////////////////////////////////////////////////////////////////////
//
//  XMLexception: vynimka vyvolavana pri chybach pri parsrovani xml suboru
//  XMLexceptionNotFound: vynimka - polozka nenajdena
//  XMLexceptionInvalid: vynimka - nastala chyba, polozka je neplatna
//  XMLbase: bazova trieda pre XML* triedy.. poskytuje uzitocnu funkcionalitu, ktora sa opakuje
//    pri spracovavani roznych XML-elementov
//  XMLfunc: bazova trieda obalujuca funkcionalitu xml-funkcii
//  XMLfuncInter: trieda obalujuca linearnu interpolaciu definovanu v xml subore
//  XMLfuncPolynome: trieda obalujuca polynomialnu funkciu
//  XML* - nazov napovie
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "type.h"
#include "func.h"
#include "adaptation.h"
#include "featurecolorfilterarray.h"
#include <boost\shared_ptr.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <string>
#include <vector>
using std::string;


class HDRImage1f;
class XMLfunc;
typedef boost::shared_ptr<XMLfunc> PtrXMLfunc;
typedef boost::shared_ptr<HDRImage1f> PtrHDRImage1f;

class XMLexception
{
	string m_camera;
	string m_version;
	string m_node;
	string m_desc;
public:
	XMLexception (string camera, string version, string node = "", string desc = "")
	{
		m_camera = camera;
		m_version = version;
		m_node = node;
		m_desc = desc;
	}

	string getCamera ()
	{
		return m_camera;
	}
	string getVersion ()
	{
		return m_version;
	}
	string getNode ()
	{
		return m_node;
	}
	string getDesc ()
	{
		return m_desc;
	}

	string get ();
};

class XMLexceptionNotFound: public XMLexception
{
public:
	XMLexceptionNotFound (string camera, string version, string node = "", string desc = ""):
		XMLexception (camera, version, node, desc + " not found")
	{
	}
};

class XMLexceptionInvalid: public XMLexception
{
public:
	XMLexceptionInvalid (string camera, string version, string node = "", string desc = ""):
		XMLexception (camera, version, node, desc + " is invalid")
	{
	}
};
class XMLbase
{
	string m_strCamera;
	string m_strVersion;
	string m_strNode;

public:
	XMLbase (string camera, string version, string node = "")
	{
		m_strCamera = camera;
		m_strVersion = version;
		m_strNode = node;
	}
	bool checkAttribute (xercesc::DOMNode* node, string name, string value);
	xercesc::DOMNode* getChild (std::string name, xercesc::DOMNode* parent);
	bool getAttributeBool (xercesc::DOMNode* node, string name);
	string getAttributeString (xercesc::DOMNode* node, string name);
	float getAttributeFloat (xercesc::DOMNode* node, string name);
	int getAttributeInt (xercesc::DOMNode* node, string name);
	uint getAttributeUInt (xercesc::DOMNode* node, string name);
	PtrXMLfunc getChildFunc (xercesc::DOMNode* node);
	void getGrid (xercesc::DOMNode* node, HDRImage1f& gridX, HDRImage1f& gridY);

	float getFloat  (xercesc::DOMNode* node);
	float2 getFloat2 (xercesc::DOMNode* node);
	float3 getFloat3  (xercesc::DOMNode* node);

	void throwNotFound (std::string object)
	{
		throw XMLexceptionNotFound (m_strCamera, m_strVersion, m_strNode, object);
	}
	void throwInvalid (std::string object)
	{
		throw XMLexceptionInvalid (m_strCamera, m_strVersion, m_strNode, object);
	}
	void throwException (std::string desc)
	{
		throw XMLexception (m_strCamera, m_strVersion, m_strNode, desc);
	}
};



class XMLfunc: public FuncFtoF, protected XMLbase
{
public:
	XMLfunc (string camera, string version):
		XMLbase (camera, version)
	{
	}
};

typedef boost::shared_ptr<XMLfunc> PtrXMLfunc;



class XMLfuncInter: public XMLfunc
{
	float2* m_points;
public:
	XMLfuncInter(string camera, string version, xercesc::DOMNode* node);

	float operator() (float x)
	{
		int i;
		for (i = 1; true; i++)
		{
			if (x <= m_points[i].x)
				break;
		}
		float2 low = m_points[i-1];
		float2 high = m_points[i];
		float2 size = high - low;
		float rel = (x - low.x)/size.x; 
		return low.y + size.y*rel;
	}
};




template <uint Degree>
class XMLfuncPolynome: public XMLfunc
{
	float m_coef[Degree+1];
public:
	XMLfuncPolynome(string camera, string version, xercesc::DOMNode* node);

	float operator () (float x)
	{
		float xn= 1.0f;
		float y = 0;
		for (uint i = 0; i <= Degree; i++)
		{
			y += m_coef[i]*xn;
			xn *= x;
		}
		return y;
	}
};



class XMLinputFile: public XMLbase
{
public:
	string path, red, green, blue, depth, envred, envgreen, envblue;

	enum etype {TYPE_STANDARD, TYPE_EXR};

	etype type;

	XMLinputFile (xercesc::DOMNode* node):
		XMLbase ("", "", "input")
	{
		path = XMLbase::getAttributeString (node, "path");
		
		string strtype = XMLbase::getAttributeString (node, "type");
		if (strtype == "exr") 
			type = TYPE_EXR;
		else if (strtype == "standard")
			type = TYPE_STANDARD;
		else
			throwInvalid ("type");

		for (int i = 0; i < 7; i++)
		{
			string in;
			string* out;
			switch (i)
			{
			case 0: in = "red"; out=&red; break;
			case 1: in = "green"; out=&green; break;
			case 2: in = "blue"; out=&blue; break;
			case 3: in = "depth"; out=&depth; break;
			case 4: in = "envred"; out=&envred; break;
			case 5: in = "envgreen"; out=&envgreen; break;
			case 6: in = "envblue"; out=&envblue; break;
			}
			try {
				*out = XMLbase::getAttributeString (node, in);
			} catch (XMLexception&) { }
		}
	}
	XMLinputFile (XMLinputFile& p):
		XMLbase ("", "", "input")
	{
		path = p.path;
		red = p.red;
		green = p.green;
		blue = p.blue;
		depth = p.depth;
		envred = p.envred;
		envgreen = p.envgreen;
		envblue = p.envblue;
		type = p.type;
	}
};


class XMLvideoExt: public XMLbase
{
public:
	int start, end, delay, loop;

	XMLvideoExt (xercesc::DOMNode* node);
};

typedef boost::shared_ptr<XMLvideoExt> PtrXMLvideoExt;


class XMLparser;
class XMLinput;
typedef boost::shared_ptr<XMLinput> PtrXMLinput;

class XMLinput: public XMLbase
{	
public:
	std::vector<XMLinputFile> files;
	enum etype {SINGLE, SEQUENCE, LIST, VIDEO} type;

	struct seq_t {int start,end,step,repeat;} sequence;
	std::vector<PtrXMLinput> listRefs;

	float colorMult;
	bool depthLinearize;
	float depthMult, depthAdd, depthN, depthF;
	float icosmult, icosanglemult;

	string videoColorPath;
	string videoLib;
	float videoColorMult;

	XMLinput (xercesc::DOMNode* node, XMLparser* parser);
	XMLinput (): XMLbase ("","") {}
	PtrXMLinput clone ();
};



class XMLcommands: public XMLbase
{
public:
	//XMLcommands (string filename, string name);
	XMLcommands (xercesc::DOMNode* node);
	enum ecommand_t {INPUT, OUTPUT, FEATURES, COMMIT};
	struct Command_t
	{
		ecommand_t cmd;
		string param;
	};
	std::vector<Command_t> vecCmds;
};
typedef boost::shared_ptr<XMLcommands> PtrXMLcommands;

class XMLoutput: public XMLbase
{
public:
	XMLoutput (xercesc::DOMNode* node);
	
	bool m_isSingleImageOutput;
	string m_singleImageFilename;
	
	bool m_isSequence;
	struct sequence_t {int start, end, step; string filename;} m_sequence;
	enum eimage {FIXED, EXR} m_imageType;

	bool m_isVideo;
	string m_videoLib,m_videoFilename,m_videoFourcc;
	float m_videoFPS;
	int m_videoRepeatOneFrame;
	
	bool m_wndVisibled;
	int2 m_wndSize;
};
typedef boost::shared_ptr<XMLoutput> PtrXMLoutput;


class XMLvignettingRadial: protected XMLbase
{
public:
	XMLvignettingRadial (xercesc::DOMNode* node):
		XMLbase ("","", "vignettingMask")
	{
		m_func = getChildFunc (node);
	}
	PtrXMLfunc m_func;
};
typedef boost::shared_ptr<XMLvignettingRadial> PtrXMLvignettingRadial;


class XMLvignettingMask: protected XMLbase
{
public:
	XMLvignettingMask (xercesc::DOMNode* node):
		XMLbase ("","", "vignettingMask")
	{
		xercesc::DOMNode* nodemask = getChild ("mask", node); 
		m_filepath = getAttributeString (nodemask, "filepath");
	}
	string m_filepath;
};
typedef boost::shared_ptr<XMLvignettingMask> PtrXMLvignettingMask;

class XMLdistortion: protected XMLbase
{
	PtrHDRImage1f m_gridX, m_gridY;

	void processRadialDistortion (xercesc::DOMNode* node);
public:
	XMLdistortion (xercesc::DOMNode* node);

	void getGrid (PtrHDRImage1f&  gridX, PtrHDRImage1f& gridY)
	{
		gridX = m_gridX;
		gridY = m_gridY;
	}
};
typedef boost::shared_ptr<XMLdistortion> PtrXMLdistortion;

class XMLchromAberration: protected XMLbase
{
	PtrXMLfunc m_redcyan, m_blueyellow;
	uint2 m_sizeOfGrid;

public:
	XMLchromAberration (xercesc::DOMNode* node);

	uint2 getSizeOfGrid ()
	{
		return m_sizeOfGrid;
	}

	PtrXMLfunc getFuncRedCyan ()
	{
		return m_redcyan;
	}

	PtrXMLfunc getFuncBlueYellow ()
	{
		return m_blueyellow;
	}
};
typedef boost::shared_ptr<XMLchromAberration> PtrXMLchromAbberation;


class XMLblur: protected XMLbase
{
	bool m_enabled;
	PtrFuncFtoF m_func;

public:
	XMLblur (xercesc::DOMNode* node);

	PtrFuncFtoF getFunc ()
	{
		return m_func;
	}
};
typedef boost::shared_ptr<XMLblur> PtrXMLblur;



class XMLlensflareSimple: protected XMLbase
{
public:
	uint2 m_sizeofenvmap;
	float2 m_envmapangle;
	float2 m_fovangle;

	float m_starMult, m_csMult1, m_csMult2;

public:
	XMLlensflareSimple (xercesc::DOMNode* node);

};
typedef boost::shared_ptr<XMLlensflareSimple> PtrXMLlensflareSimple;


class XMLmotionblurParam: protected XMLbase
{
public:
	float3 deltaCameraPos;
	float targetDist;
	float2 deltaTarget;
	float2 fovAngle;

	uint numIterations;
public:
	XMLmotionblurParam (xercesc::DOMNode* node);

};
typedef boost::shared_ptr<XMLmotionblurParam> PtrXMLmotionblurParam;



class XMLdepthoffield: protected XMLbase
{
public:
	enum etype {TYPE_GATHER, TYPE_DIFFUSION};
public:
	etype m_type;

public:
	bool m_apertureIsFixed;
	float m_apertureValue;

	float m_sensorSize;
	float m_focalLen;
	float m_focusDist;

public:
	XMLdepthoffield (xercesc::DOMNode* node);
};
typedef boost::shared_ptr<XMLdepthoffield> PtrXMLdepthoffield;



class XMLapertureAdapt: protected XMLbase
{
public:
	enum etype {TYPE_FIXED, TYPE_ADAPT};
private:
	etype m_type;
	float m_fnum;
	bool m_brightness;
	AdaptationParam m_param;
public:
	XMLapertureAdapt (xercesc::DOMNode* node);

	etype getType ()
	{
		return m_type;
	}

	float getFnum ()
	{
		return m_fnum;
	}

	bool isEnabledBrightness ()
	{
		return m_brightness;
	}

	void getAdaptationParam (AdaptationParam& param)
	{
		param = m_param;
	}
};
typedef boost::shared_ptr<XMLapertureAdapt> PtrXMLapertureAdapt;


class XMLcolorfilterarray: protected XMLbase
{
private:
	FeatureColorFilterArrayParam m_red, m_green, m_blue;
public:
	XMLcolorfilterarray (xercesc::DOMNode* node);

	void getRed (FeatureColorFilterArrayParam& param)
	{
		param = m_red;
	}

	void getGreen (FeatureColorFilterArrayParam& param)
	{
		param = m_green;
	}

	void getBlue (FeatureColorFilterArrayParam& param)
	{
		param = m_blue;
	}
};
typedef boost::shared_ptr<XMLcolorfilterarray> PtrXMLcolorfilterarray;


class XMLclampValues: protected XMLbase
{
public:
	float m_maxValue;

	XMLclampValues (xercesc::DOMNode* node);
};
typedef boost::shared_ptr<XMLclampValues> PtrXMLclampValues;


class XMLnoiseSimple: protected XMLbase
{
public:
	PtrXMLfunc m_funcSignalToNoise;

	float m_min, m_max;

	XMLnoiseSimple (xercesc::DOMNode* node);
};
typedef boost::shared_ptr<XMLnoiseSimple> PtrXMLnoiseSimple;


class XMLtransformValues: protected XMLbase
{
public:
	PtrXMLfunc m_func;
	XMLtransformValues (xercesc::DOMNode* node);
};
typedef boost::shared_ptr<XMLtransformValues> PtrXMLtransformValues;



class XMLfeatures: protected XMLbase
{
public:
	enum etype_t{VIGNETTING_RADIAL,VIGNETTING_MASK,DISTORTION_RADIAL,
		DISTORTION_GRID,CHROMABER_RADIAL,BLUR,DEPTHOFFIELD,
		LENSFLARE_SIMPLE,MOTIONBLUR_PARAM,APERTUREADAPT,CLAMPVALUES,COLORFILTERARRAY,NOISE_SIMPLE,TRANSFORMVALUES};
	struct feature_t {etype_t type; string strType; string name;};
	std::vector<feature_t> vecFeatures;

	XMLfeatures (xercesc::DOMNode* node);
};
typedef boost::shared_ptr<XMLfeatures> PtrXMLfeatures;


class XMLparser: protected XMLbase
{
	boost::shared_ptr<xercesc::XercesDOMParser> m_parser;
public:
	XMLparser (string filename);
	PtrXMLinput parseInput (string inputName);
	PtrXMLoutput parseOutput (string outputName);
	PtrXMLcommands parseCommands (string name);
	PtrXMLfeatures parseFeatures (string name);
	PtrXMLvignettingMask parseVignettingMask (string name);
	PtrXMLvignettingRadial parseVignettingRadial (string name);
	PtrXMLdistortion parseDistortionRadial (string name);
	PtrXMLdistortion parseDistortionGrid (string name);
	PtrXMLchromAbberation parseChromaber (string name);
	PtrXMLblur parseBlur (string name);
	PtrXMLlensflareSimple parseLensflareSimple (string name);
	PtrXMLmotionblurParam parseMotionblurParam (string name);
	PtrXMLdepthoffield parseDepthoffield (string name);
	PtrXMLapertureAdapt parseAdaptation (string name);
	PtrXMLclampValues parseClampValues (string name);
	PtrXMLnoiseSimple parseNoiseSimple (string name);
	PtrXMLcolorfilterarray parseCFA (string name);
	PtrXMLtransformValues parseTransformValues (string name);
};


