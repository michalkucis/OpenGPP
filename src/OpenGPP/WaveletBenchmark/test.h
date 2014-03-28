#pragma once
#include "userTransformations.h"

class Test
{
protected:
	Vector<Ptr<UserTransformation>> m_vecUT;
public:
	virtual void init(Ptr<ClFacade> facade)=0;
	virtual int getPerformsCount()=0;
	virtual string getName()=0;
	virtual void perform ()
	{
		for (int i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->process();
	}
	virtual void clear()
	{
		m_vecUT.clear();
	}
};



class TestIO: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 100;
	}
	string getName()
	{
		return "Read & write mem between host and global space";
	}
};


class TestIO2: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTcopyGlobalLocalGlobal(facade, res));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 100;
	}
	string getName()
	{
		return "Read & write mem between host and local space";
	}
};


class TestIO3: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTcopyGlobalLocalGlobalAsync(facade, res));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 100;
	}
	string getName()
	{
		return "Read & write mem between host and local space with command async...";
	}

};


class TestIO4: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTcopyGlobalLocalLine(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 100;
	}
	string getName()
	{
		return "Read & write mem between global and local space per line...";
	}
};


class TestIO5: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTcopyGlobalLocalColumn(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 100;
	}
	string getName()
	{
		return "Read & write mem between global and local space per column...";
	}
};

class TestIO6: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTcopyGlobalLocalTide(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 100;
	}
	string getName()
	{
		return "Read & write mem between global and local space per tide...";
	}
};


class TestMemoryUsingLocal: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
	
		
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonMemUsingLocalMemory(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 4000;
	}
	string getName()
	{
		return "Testing access to multiple items in local memory";
	}
};


class TestMemoryUsingPrivate: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
	
		
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonMemUsingPrivateMemory(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 4000;
	}
	string getName()
	{
		return "Testing access to multiple items in private memory";
	}
};




class TestHorizonSyncLiftingAsyncCopy: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
	
		
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonLiftingSync(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 4000;
	}
	string getName()
	{
		return "Synchronous lifting and async copy data";
	}
};

class TestHorizonSyncLiftingThreadCopy: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonLiftingSync2(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 4000;
	}
	string getName()
	{
		return "Synchronous lifting with thread copy";
	}
};

class TestHorizonSyncLiftingThreadCopyMoreThreads: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonLiftingSync3(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 4000;
	}
	string getName()
	{
		return "Synchronous lifting with thread copy and extended borders";
	}
};

class TestHorizonAsyncLiftingThreadCopy: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonLiftingAsync(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 4000;
	}
	string getName()
	{
		return "Asynchronous lifting with thread copy";
	}
};


class TestHorizonAsyncConvolutionThreadCopy: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonConvolutionAsync(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 4000;
	}
	string getName()
	{
		return "Asynchronous convolution with thread copy";
	}
};

class TestHorizonAsyncConvolutionThreadCopyOpt: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonConvolutionAsyncOpt(facade, res));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 4000;
	}
	string getName()
	{
		return "Optimized asynchronous convolution with thread copy";
	}
};

class TestTransposeInsideTiles: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTtransposeInsideTiles(facade));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 1000;
	}
	string getName()
	{
		return "Transpose inplace tiles";
	}
};

class TestTranspose: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTtranspose(facade));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 1000;
	}
	string getName()
	{
		return "Global transpose";
	}
};


//class TestHorizonAsyncConvolutionThreadCopyOpt4Lines: public Test
//{
//	void init(Ptr<ClFacade> facade)
//	{
//		int2 res (1036,777);
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//		m_vecUT.pushBack(new UThorizonConvolutionAsyncOpt4Lines(facade, res));
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//	}
//	int getPerformsCount()
//	{
//		return 1000;
//	}
//	string getName()
//	{
//		return "Optimized asynchronous convolution with thread copy";
//	}
//};

class TestCopyGlobalLocalGlobal: public Test
{
	void init(Ptr<ClFacade> facade)
	{
		int2 res (1036,777);
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTcopyGlobalLocalGlobal(facade, res));
		//m_vecUT.pushBack(new UTcopyGlobalLocalGlobal(facade, res));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
	}
	int getPerformsCount()
	{
		return 100;
	}
	string getName()
	{
		return "Copy global to local + R/W";
	}
};


void performTests(Ptr<ClFacade> facade)
{
	Vector<Ptr<Test>> tests;
	int saveOutput = -1;
	tests.pushBack(new TestIO);
	//tests.pushBack(new TestIO2);
	//tests.pushBack(new TestIO3);
	//tests.pushBack(new TestIO4);
	//tests.pushBack(new TestIO5);
	//tests.pushBack(new TestIO6);

	//tests.pushBack(new TestMemoryUsingLocal);
	//tests.pushBack(new TestMemoryUsingPrivate);

	//tests.pushBack(new TestHorizonSyncLiftingThreadCopy);
	//tests.pushBack(new TestHorizonAsyncLiftingThreadCopy);

	//tests.pushBack(new TestHorizonSyncLiftingAsyncCopy);
	//tests.pushBack(new TestHorizonSyncLiftingThreadCopy);
	//tests.pushBack(new TestHorizonSyncLiftingThreadCopyMoreThreads);
	//tests.pushBack(new TestHorizonAsyncLiftingThreadCopy);
	//tests.pushBack(new TestHorizonAsyncConvolutionThreadCopy);
	//tests.pushBack(new TestHorizonAsyncConvolutionThreadCopyOpt);
	//tests.pushBack(new TestHorizonAsyncConvolutionThreadCopyMultiLine);


	tests.pushBack(new TestTransposeInsideTiles);
/*	tests.pushBack(new TestTranspose);
	tests.pushBack(new TestIO);
	tests.pushBack(new TestHorizonSyncLiftingThreadCopyMoreThreads);
	//tests.pushBack(new TestHorizonSyncLiftingAsyncCopy);
	tests.pushBack(new TestHorizonAsyncConvolutionThreadCopy);
	tests.pushBack(new TestHorizonAsyncConvolutionThreadCopyOpt);
*/	for (int i = 0; i < tests.getSize(); i++)
	{
		Ptr<Test> test = tests[i];
		test->init(facade);

		int count = test->getPerformsCount();
		int start, end;
		Ptr<cl::CommandQueue> queue = facade->getQueue();
		std::cout << "Test '" << test->getName() << "' started...";
		queue->finish();
		start = GetTickCount();
		for (int i = 0; i < test->getPerformsCount(); i++)
		{
			test->perform();
		}
		queue->finish();
		end = GetTickCount();
		if (saveOutput == i)
		{
			Ptr<UTsaveBufferToDLM> ut = new UTsaveBufferToDLM(facade, "out.dlm");
			std::cout << " Saving output...";
			ut->process();
		}
		std::cout << " Finished" << std::endl;
		std::cout << "Results:" << std::endl;
		//std::cout << "  Elapsed time: " << end - start << " ms\n";
		std::cout << "  Count:        " << count << "\n";
		std::cout << "  Average time: " << (end-start)/(float)count << " ms\n";
		std::cout << "\n";
		test->clear();
	}

	std::cout << "All tests have finished...";
	std::cin.ignore();
	std::cin.get();
}