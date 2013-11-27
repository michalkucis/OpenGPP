#pragma once

#include "Functions.h"
#include "Vector.h"
#include "XML.h"
#include "Ptr.h"
#include "Base.h"

class FuncFtoFPolynomial: public FuncFtoF
{
	Vector<float> m_constants;
public:
	FuncFtoFPolynomial(float a)
	{
		m_constants.pushBack(a);
	}
	FuncFtoFPolynomial(float a, float b)
	{
		m_constants.pushBack(b);

		m_constants.pushBack(a);
	}
	FuncFtoFPolynomial(float a, float b, float c)
	{
		m_constants.pushBack(c);

		m_constants.pushBack(b);

		m_constants.pushBack(a);
	}
	FuncFtoFPolynomial(float a, float b, float c, float d)
	{
		m_constants.pushBack(d);

		m_constants.pushBack(c);

		m_constants.pushBack(b);
	
		m_constants.pushBack(a);
	}	
	FuncFtoFPolynomial(Ptr<XMLNode> nodeFunc)
	{
		string type = nodeFunc->getAttrString("type");
		if(type != "polynomial")
			error0(ERR_XML, "Invalid type of the xml <func>");
		Vector<Ptr<XMLNode>> nodeValues = nodeFunc->getVecNodes("value");
		for (int i = (int) nodeValues.getSize()-1; i >= 0; i--)
		{
			Ptr<XMLNode> node = nodeValues[i];
			float value = node->getAttrFloat("x");
			m_constants.pushBack(value);
		}
	}
	float operator()(float x)
	{
		float y = 0;
		float value = 1;
		for (uint i = 0; i < m_constants.getSize(); i++)
		{
			y += m_constants[i] * value;
			value *= x;
		}
		return y;
	}
	string getString(string strResult = "y", string strInput = "x");
};
