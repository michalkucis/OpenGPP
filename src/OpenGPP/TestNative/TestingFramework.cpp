#include "TestingFramework.h"
#include "Vector.h"


VectorNoInit<UnitTest*> g_allTests;

UnitTest::UnitTest (std::string testName):  m_testName(testName)
{
	g_allTests.pushBack(this);
}

UnitTest::~UnitTest ()
{
	for (unsigned int i = 0; i < g_allTests.getSize(); i++)
	{
		UnitTest* test = g_allTests[i];
		if (test == this)
			g_allTests[i] = NULL;
	}
}

void performTests ()
{	
	for (size_t i = 0; i < g_allTests.getSize(); i++)
	{	
		UnitTest* test = g_allTests[i];
		if (! test)
			continue;
		std::cout << "Performing test '" << test->getName() << "'... ";
		test->performTest();
		std::cout << "OK\n";
	}
}