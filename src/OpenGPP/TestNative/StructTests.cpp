#include "TestingFramework.h"
#include "Ptr.h"
#include "Vector.h"
#include "Factory.h"
#include <assert.h>

class PtrTest: public UnitTest
{
public:
	PtrTest (): UnitTest("PtrTest") {}
	void performTest ()
	{
		Ptr<int> p;
		Ptr<int> p2(new int);
		Ptr<int> p3(p2);
		p = p2;
	}
} g_ptrTest;

class VectorTest: public UnitTest
{
public:
	VectorTest (): UnitTest("VectorTeste") {}
	void performTest ()
	{
		//std::unique_ptr<Set<int>> pClass(new Set<int>);
		Vector<int> s;
		s.setSize(10);
		s[2] = 2;
		s.clear();
		s.setSize(2);
		s.setSize(16);
		s.pushBack(2);
	}
} g_vectorTest;

//class FactoryTest: public UnitTest
//{
//public:
//	FactoryTest (): UnitTest("FactoryTest") {}
//	void performTest ()
//	{
//		PtrProduct<int> p1 = new int(0);
//		{
//			Factory<int> factory;
//			PtrProduct<int> p2,p3;
//			p1 = factory.createProduct();
//			*p1 = 10;
//			p2 = factory.createProduct();
//			p3 = factory.createProduct();
//			p1 = NULL;
//		}
//		int test = *p1;
//		assert(*p1 == 10);
//	}
//} g_factoryTest;
