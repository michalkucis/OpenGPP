#include "FuncImpl.h"
#include <sstream>

string FuncFtoFPolynomial::getString(string strResult, string strInput)
{
	std::stringstream ss;
	ss << strResult << " = ";
	for (uint i = 0; i < m_constants.getSize(); i++)
	{
		if (i!=0)
			ss << " + ";
		float c = m_constants[i];
		string strC = float2str(c);
		ss << strC;
		for (uint j = i+1; j < m_constants.getSize(); j++)
			ss << "*" << strInput;
	}
	return ss.str();
}