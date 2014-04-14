#pragma once
#include "userTransformations.h"
#include <sstream>




class Test
{
protected:
	Vector<Ptr<UserTransformation>> m_vecUT;

public:
	virtual void init(Ptr<ClFacade> facade, int2 resolution)=0;
	virtual int getPerformsCount(int2 resolution)=0;
	virtual string getName()=0;
	virtual void perform ()
	{
		for (uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->process();
	}
	virtual void clear()
	{
		m_vecUT.clear();
	}
};





class TestIO: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 1000;
	}
	string getName()
	{
		return "Copying between host and global memory";
	}
};



//class TestIOLocalMemory: public Test
//{
//public:
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//		m_vecUT.pushBack(new UTcopyUsingLocalMemory(facade));
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return 1000;
//	}
//	string getName()
//	{
//		return "Copying between host and on chip memory";
//	}
//};


class TestHorizonSyncLifting: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonLiftingSync(facade));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 1000;
	}
	string getName()
	{
		return "Horizontal synchronized lifting";
	}
};


class TestHorizonAsyncLifting: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonLiftingAsync(facade));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 1000;
	}
	string getName()
	{
		return "Horizontal asynchronous lifting";
	}
};

class TestHorizonAsyncConvolution: public Test
{
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonConvolutionAsync(facade));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 1000;
	}
	string getName()
	{
		return "Horizontal asynchronous convolution";
	}
};

class TestHorizonAsyncConvolution2: public Test
{
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonConvolutionAsync2(facade));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 1000;
	}
	string getName()
	{
		return "Horizontal asynchronous convolution - 2 rows";
	}
};

class TestHorizonAsyncConvolution4: public Test
{
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UThorizonConvolutionAsync4(facade));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 1000;
	}
	string getName()
	{
		return "Horizontal asynchronous convolution - 4 rows";
	}
};

class TestVerticalAsyncConvolution: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTverticalConvolutionAsync(facade));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 100;
	}
	string getName()
	{
		return "Vertical - naive solution";
	}
};

class TestVerticalWithTranspose: public Test
{
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTtranspose2x2(facade));
		m_vecUT.pushBack(new UThorizonConvolutionAsyncTransposed(facade));
		m_vecUT.pushBack(new UTtranspose2x2(facade));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 100;
	}
	string getName()
	{
		return "Vertical - with transposition";
	}
};

class TestVerticalOverlappedTiles: public Test
{
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTverticalOverlappedTiles(facade));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 100;
	}
	string getName()
	{
		return "Vertical - overlapped tiles";
	}
};


class TestVerticalSlidingWindow: public Test
{
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		//m_vecUT.pushBack(new UTverticalSlidingWindow(facade));
		m_vecUT.pushBack(new UTverticalOverlappedTiles(facade));
		//m_vecUT.pushBack(new UTverticalConvolutionAsync(facade));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 100;
	}
	string getName()
	{
		return "Vertical - sliding window";
	}
};


void initResolutions(Vector<int2>& vecResolutions)
{
	vecResolutions.clear();

	//vecResolutions.pushBack(int2(64, 64));
	//vecResolutions.pushBack(int2(128,128));
	//vecResolutions.pushBack(int2(256,256));
	vecResolutions.pushBack(int2(512,512));
	vecResolutions.pushBack(int2(1024,1024));
	vecResolutions.pushBack(int2(2048,2048));
	vecResolutions.pushBack(int2(3072,3072));
	//vecResolutions.pushBack(int2(4096,4096));	
}




void initTests(Vector<Ptr<Test>>& tests)
{
	tests.clear();
	//tests.pushBack(new TestIO);
	//tests.pushBack(new TestIOLocalMemory);
	//tests.pushBack(new TestHorizonSyncLifting);
	//tests.pushBack(new TestHorizonAsyncLifting);
	//tests.pushBack(new TestHorizonAsyncConvolution);
	//tests.pushBack(new TestHorizonAsyncConvolution2);
	//tests.pushBack(new TestHorizonAsyncConvolution4);

	tests.pushBack(new TestVerticalAsyncConvolution);
	tests.pushBack(new TestVerticalWithTranspose);
	tests.pushBack(new TestVerticalOverlappedTiles);
	tests.pushBack(new TestVerticalSlidingWindow);

	//tests.pushBack(new TestIO);
	//tests.pushBack(new TestIO2);
	//tests.pushBack(new TestIO3);
	//tests.pushBack(new TestIO4);
	//tests.pushBack(new TestIO5);
	//tests.pushBack(new TestIO6);

	//tests.pushBack(new TestMemoryUsingLocal);
	//tests.pushBack(new TestMemoryUsingPrivate);

	//tests.pushBack(new TestHorizonSyncLiftingThreadCopy);
	//tests.pushBack(new TestHorizonAsyncLiftingThreadCopy);

	

	//tests.pushBack(new TestTransposeInsideTiles);
	//tests.pushBack(new TestTransposeInsideTiles2);
	//tests.pushBack(new TestTransposeNaive);
	//tests.pushBack(new TestTranspose);

	//
	//tests.pushBack(new TestHorizonSyncLiftingAsyncCopy);
	//tests.pushBack(new TestHorizonSyncLiftingThreadCopy);
	//tests.pushBack(new TestHorizonSyncLiftingThreadCopyMoreThreads);
	//tests.pushBack(new TestHorizonAsyncLiftingThreadCopy);
	//tests.pushBack(new TestHorizonAsyncConvolutionThreadCopy);
	//tests.pushBack(new TestHorizonAsyncConvolutionThreadCopyOpt);
	//tests.pushBack(new TestHorizonAsyncConvolutionThreadCopy2Lines);
	//tests.pushBack(new TestHorizonAsyncConvolutionThreadCopy4Lines);
	//
	//tests.pushBack(new TestVerticalPerformTransHorizonTrans);
	//tests.pushBack(new TestVerticalNaive);
	//tests.pushBack(new TestVerticalOverlapping);
}

class TestDebug: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution=int2(0,0))
	{
		resolution = int2(1024,1024);
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
		m_vecUT.pushBack(new UTverticalSlidingWindow(facade));
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.mat"));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return 1000;
	}
	string getName()
	{
		return "Debugging";
	}
};


void performTests(Ptr<ClFacade> facade)
{
	//{
	//	TestDebug debug;
	//	debug.init(facade);
	//	
	//	debug.perform();
	//}

	Vector<int2> vecResolutions;
	initResolutions(vecResolutions);

	std::cout << "Method\tResolution\tCount\tms\n";
	for (uint nRes = 0; nRes < vecResolutions.getSize(); nRes++)
	{
		Vector<Ptr<Test>> tests;
		initTests(tests);

		int2 res = vecResolutions[nRes];
		for (uint nTest = 0; nTest < tests.getSize(); nTest++)
		{
			try {
				Ptr<Test> test = tests[nTest];

				std::cout << test->getName() << "\t" 
					<< res.x << "x" << res.y << "\t"
					<< test->getPerformsCount(res) << "\t";
				test->init(facade, res);

				Ptr<cl::CommandQueue> queue = facade->getQueue();
				queue->finish();

				int count = test->getPerformsCount(res);
				int start, end;

				start = GetTickCount();
				for (int i = 0; i < test->getPerformsCount(res); i++)
				{
					test->perform();
				}
				try {
					queue->finish();
				} catchCLError;
				end = GetTickCount();
				std::cout << ((float)end-start)/count << std::endl;
			}
			catch (Error& err)
			{
				std::cout << "N/A" << std::endl;
			}
		}
	}

	std::cout << "All tests have finished...";
	std::cin.ignore();
	std::cin.get();
}