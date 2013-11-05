#pragma once

#include <string>
#include <vector>
#include <iostream>

class UnitTest
{
	std::string m_testName;
public:
	UnitTest (std::string testName);
	~UnitTest ();
	const char* getName () { return m_testName.c_str();}
	virtual void performTest () = 0;
};


void performTests ();