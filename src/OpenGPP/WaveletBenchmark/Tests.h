#pragma once
#include "userTransformations.h"
#include <sstream>


#define DISBALE_IO_USERTRANS
#define NUM_TESTS(res) (int)200*(1024.0f/res.x)


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
		return 1;
	}
	string getName()
	{
		return "Init   ";
	}
};
//
//class TestCompleteWithHostMemTransfer: public Test
//{
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//		m_vecUT.pushBack(new UThorizonConvolutionAsync2(facade));
//		m_vecUT.pushBack(new UTverticalSlidingWindow2Columns(facade));
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return NUM_TESTS(resolution);
//	}
//	string getName()
//	{
//		return "Complete test with host mem transfer";
//	}
//};
//
//class TestCompleteWithoutHostMemTransfer: public Test
//{
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//		m_vecUT.pushBack(new UThorizonConvolutionAsync2(facade));
//		m_vecUT.pushBack(new UTverticalSlidingWindow2Columns(facade));
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return NUM_TESTS(resolution);
//	}
//	string getName()
//	{
//		return "Complete test without host mem transfer";
//	}
//};

class TestI: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		//m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "to VRAM";
	}
};

class TestO: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
		//m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "to RAM ";
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

//
//class TestHorizonSyncLifting: public Test
//{
//public:
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//#endif
//		m_vecUT.pushBack(new UThorizonLiftingSync(facade));
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//#endif
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return NUM_TESTS(resolution);
//	}
//	string getName()
//	{
//		return "Horizontal synchronized lifting";
//	}
//};
//
//
//class TestHorizonAsyncLifting: public Test
//{
//public:
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//#endif
//		m_vecUT.pushBack(new UThorizonLiftingAsync(facade));
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//#endif
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return NUM_TESTS(resolution);
//	}
//	string getName()
//	{
//		return "Horizontal asynchronous lifting";
//	}
//};
//
//class TestHorizonAsyncConvolution: public Test
//{
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//#endif
//		m_vecUT.pushBack(new UThorizonConvolutionAsync(facade));
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//#endif
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return NUM_TESTS(resolution);
//	}
//	string getName()
//	{
//		return "Horizontal asynchronous convolution";
//	}
//};
//
//class TestHorizonAsyncConvolution2: public Test
//{
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//#endif
//		m_vecUT.pushBack(new UThorizonConvolutionAsync2(facade));
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//#endif
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return NUM_TESTS(resolution);
//	}
//	string getName()
//	{
//		return "Horizontal asynchronous convolution - 2 rows";
//	}
//};
//
//class TestHorizonAsyncConvolution4: public Test
//{
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//#endif
//		m_vecUT.pushBack(new UThorizonConvolutionAsync4(facade));
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//#endif
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return NUM_TESTS(resolution);
//	}
//	string getName()
//	{
//		return "Horizontal asynchronous convolution - 4 rows";
//	}
//};
//
//class TestVerticalAsyncConvolution: public Test
//{
//public:
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//#endif
//		m_vecUT.pushBack(new UTverticalConvolutionAsync(facade));
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//#endif
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return NUM_TESTS(resolution);
//	}
//	string getName()
//	{
//		return "Vertical - naive solution";
//	}
//};
//
//class TestVerticalWithTranspose: public Test
//{
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//#endif
//		m_vecUT.pushBack(new UTtranspose2x2(facade));
//		m_vecUT.pushBack(new UThorizonConvolutionAsyncTransposed(facade));
//		m_vecUT.pushBack(new UTtranspose2x2(facade));
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//#endif
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return NUM_TESTS(resolution);
//	}
//	string getName()
//	{
//		return "Vertical - with transposition";
//	}
//};
//
//class TestVerticalOverlappedTiles: public Test
//{
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//#endif
//		m_vecUT.pushBack(new UTverticalOverlappedTiles(facade));
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//#endif
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return NUM_TESTS(resolution);
//	}
//	string getName()
//	{
//		return "Vertical - overlapped tiles - 1 column";
//	}
//};


class TestVerticalSlidingWindow: public Test
{
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
#endif
		//m_vecUT.pushBack(new UTverticalSlidingWindow(facade));
		m_vecUT.pushBack(new UTverticalSlidingWindow(facade));
		//m_vecUT.pushBack(new UTverticalConvolutionAsync(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "Vertical - sliding window";
	}
};

class TestVerticalSlidingWindow2Columns: public Test
{
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
#endif
		//m_vecUT.pushBack(new UTverticalSlidingWindow(facade));
		m_vecUT.pushBack(new UTverticalSlidingWindow2Columns(facade));
		//m_vecUT.pushBack(new UTverticalConvolutionAsync(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "Vertical - sliding window - 2 columns";
	}
};


class TestVerticalSlidingWindow4Columns: public Test
{
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
#endif
		//m_vecUT.pushBack(new UTverticalSlidingWindow(facade));
		m_vecUT.pushBack(new UTverticalSlidingWindow4Columns(facade));
		//m_vecUT.pushBack(new UTverticalConvolutionAsync(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "Vertical - sliding window - 4 columns";
	}
};

//
//
//class TestVerticalOverlappedTiles2Rows: public Test
//{
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//#endif
//		m_vecUT.pushBack(new UTverticalOverlappedTiles2Rows(facade));
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//#endif
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return 100;
//	}
//	string getName()
//	{
//		return "Vertical - overlapped tiles - 2 rows";
//	}
//};
//
//
//class TestVerticalOverlappedTiles4Rows: public Test
//{
//	void init(Ptr<ClFacade> facade, int2 resolution)
//	{
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromEXRFile(facade, "exrChangeLightIntensity\\img_light1_lamp250_pos0.exr"));
//#endif
//		m_vecUT.pushBack(new UTverticalOverlappedTiles4Rows(facade));
//#ifndef DISBALE_IO_USERTRANS
//		m_vecUT.pushBack(new UTsaveClBufferToArray(facade));
//#endif
//
//		facade->setResolution(resolution);
//
//		for(uint i = 0; i < m_vecUT.getSize(); i++)
//			m_vecUT[i]->prepare(resolution);
//	}
//	int getPerformsCount(int2 resolution)
//	{
//		return 100;
//	}
//	string getName()
//	{
//		return "Vertical - overlapped tiles - 4 rows";
//	}
//};

class TestVertStripSync: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
#endif
		m_vecUT.pushBack(new UTvertStripsSync(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.dlm"));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "2 sync ";
	}
};



class TestVertStrip: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
#endif
		m_vecUT.pushBack(new UTvertStrips(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.dlm"));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "2 coef ";
	}
};


class TestVertStrip4Coef: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
#endif
		m_vecUT.pushBack(new UTvertStrips4Coef(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.dlm"));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "4 coef ";
	}
};


class TestVertStrip8Coef: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
#endif
		m_vecUT.pushBack(new UTvertStrips8Coef(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.dlm"));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "8 coef ";
	}
};


class TestVertStrip16Coef: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
#endif
		m_vecUT.pushBack(new UTvertStrips16Coef(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.dlm"));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "16 coef";
	}
};

class TestVertTranspose: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
#endif
		m_vecUT.pushBack(new UTtransposeBest(facade));
		m_vecUT.pushBack(new UThorLiftingAsync8Coef(facade));
		m_vecUT.pushBack(new UTtransposeBest(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.dlm"));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "trans ";
	}
};


class TestHorLiftingAsync2Coef: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
#endif
		m_vecUT.pushBack(new UThorLiftingAsync2Coef(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.dlm"));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "hor 2 async";
	}
};


class TestHorLiftingAsync8Coef: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
#endif
		m_vecUT.pushBack(new UThorLiftingAsync8Coef(facade));
		#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.dlm"));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "hor 8 async";
	}
};


class TestHorConv2Coef: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
#endif
		m_vecUT.pushBack(new UThorConv2Coef(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.dlm"));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "conv 2     ";
	}
};




class TestHorLiftSync2Coef: public Test
{
public:
	void init(Ptr<ClFacade> facade, int2 resolution)
	{
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTloadDynamic1ChannelBufferFromDLMFile(facade, "input1kx1k.mat", resolution));
#endif
		//m_vecUT.pushBack(new UTtransposeBest(facade));
		m_vecUT.pushBack(new UThorLiftSync2Coef(facade));
		//m_vecUT.pushBack(new UTtransposeBest(facade));
#ifndef DISBALE_IO_USERTRANS
		m_vecUT.pushBack(new UTsaveBufferToDLM(facade, "out.dlm"));
#endif

		facade->setResolution(resolution);

		for(uint i = 0; i < m_vecUT.getSize(); i++)
			m_vecUT[i]->prepare(resolution);
	}
	int getPerformsCount(int2 resolution)
	{
		return NUM_TESTS(resolution);
	}
	string getName()
	{
		return "lift 2 sync";
	}
};




double round(double d, double base = 1.0)
{
	return base*floor(d/base + 0.5);
}

void initResolutions(Vector<int2>& vecResolutions)
{
	vecResolutions.clear();

	int minRes = 128;
	int maxRes = 1024*8;
	int steps = 32;

	int prevRes = 0;
	for (int i = 0; i < steps; i++)
	{
		double range = (maxRes/minRes);
		double step = log2(range)/(steps-1);
		double val = minRes*pow(2.0, step*i);
		int res = round(val, 64);
		if (res == prevRes)
			continue;
		vecResolutions.pushBack(res);
	}
}




void initTests(Vector<Ptr<Test>>& tests)
{
	//tests.pushBack(new TestI);
	//tests.pushBack(new TestO);

	tests.pushBack(new TestIO);
	tests.pushBack(new TestHorConv2Coef);

	tests.pushBack(new TestIO);
	tests.pushBack(new TestHorLiftSync2Coef);

	tests.pushBack(new TestIO);
	tests.pushBack(new TestHorLiftingAsync2Coef);

	tests.pushBack(new TestIO);
	tests.pushBack(new TestHorLiftingAsync8Coef);

	//tests.pushBack(new TestIO);
	//tests.pushBack(new TestVertStripSync);

	//tests.pushBack(new TestIO);
	//tests.pushBack(new TestVertStrip);

	//tests.pushBack(new TestIO);
	//tests.pushBack(new TestVertStrip4Coef);

	//tests.pushBack(new TestIO);
	//tests.pushBack(new TestVertStrip8Coef);

	//tests.pushBack(new TestIO);
	//tests.pushBack(new TestVertStrip16Coef);

	//tests.pushBack(new TestIO);
	//tests.pushBack(new TestVertTranspose);

	//tests.clear();
	//tests.pushBack(new TestIO);
	//tests.pushBack(new TestI);
	//tests.pushBack(new TestO);
	//tests.pushBack(new TestHorizonSyncLifting);
	//tests.pushBack(new TestHorizonAsyncLifting);
	//tests.pushBack(new TestHorizonAsyncConvolution);
	//tests.pushBack(new TestHorizonAsyncConvolution2);
	//tests.pushBack(new TestHorizonAsyncConvolution4);

	//tests.pushBack(new TestVerticalAsyncConvolution);
	//tests.pushBack(new TestVerticalWithTranspose);
	//tests.pushBack(new TestVerticalOverlappedTiles);
	//tests.pushBack(new TestVerticalOverlappedTiles2Rows);
	//tests.pushBack(new TestVerticalOverlappedTiles4Rows);
	//tests.pushBack(new TestVerticalSlidingWindow);
	//tests.pushBack(new TestVerticalSlidingWindow2Columns);
	//tests.pushBack(new TestVerticalSlidingWindow4Columns);

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

class TestProfileMe
{
	UTprofiling m_profiling;
public:
	TestProfileMe(Ptr<ClFacade> facade):
		m_profiling(facade)
	{
	}
	void setEvent()
	{
		m_profiling.process();
	}
	ULONG waitAndGetTime()
	{
		return m_profiling.getTimerValue();
	}
};


void performTests(Ptr<ClFacade> facade)
{
	//{
	//	TestHorLiftingAsync8Coef debug;
	//	debug.init(facade, int2(2816,2816));

	//	debug.perform();
	//}
	//return;

	const int repeats = 10;
	Vector<int2> vecResolutions;
	initResolutions(vecResolutions);

	std::ofstream fs("stats.txt");
	fs << "Method\tPixels\tms\n";

	std::cout << "Method\tResolution\tns\n";
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
				
				fs << test->getName() << "\t" << res.getArea() << "\t"; 
				test->init(facade, res);
				
				int count = test->getPerformsCount(res);
				TestProfileMe start(facade), end(facade);


				for (int i = 0; i < repeats; i++)
				{
					start.setEvent();
					for (int i = 0; i < test->getPerformsCount(res); i++)
					{
						test->perform();
					}
					end.setEvent();

					ULONG diff = end.waitAndGetTime() - start.waitAndGetTime();
					std::cout << ((double)diff)/count << "\t";
					fs << ((double)diff)/count << "\t";				
				}
				std::cout << "\n";
				fs << "\n";
				fs.flush();
			}
			catch (Error& err)
			{
				std::cout << "N/A" << std::endl;
				fs << "N/A" << std::endl;
			}
		}
	}

	std::cout << "All tests have finished...";
	std::cin.ignore();
	std::cin.get();
}