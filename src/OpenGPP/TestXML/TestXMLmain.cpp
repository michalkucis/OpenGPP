#include "xml.h"









void main(int,char**)
{
	XMLNode node;
	node.parseFile("camsim.xml");
	Vector<Ptr<XMLNode>> vecNodes = node.getVecNodes("input");
	std::cout << "Cout\n";
}