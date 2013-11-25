#include "AppTest.h"
#include "openGPP.h"

class AppInputTest: public AppTest, public UnitTest
{
	Ptr<SharedObjectsFactory> m_sof;
public:
	AppInputTest (string testName, int countRenders = 1): UnitTest (testName), m_sof(new SharedObjectsFactory(1024,768))
	{
		//m_countRenders = countRenders;
	}
	virtual Ptr<Input> createInput () = 0;
	void initData ()
	{
		AppTest::initData();
		//m_pp.m_input = createInput ();
		//m_pp.m_vecEffects.pushBack(Ptr<Effect>(new EffectRenderToScreen(0,0,400,400)));
	}
	void performTest ()
	{
		run ();
	}
};

class AppInputSingleOpenCV: public AppInputTest
{
public:
	AppInputSingleOpenCV (): AppInputTest ("InputLoadFromSingleFileOpenCV")
	{ }
	Ptr<Input> createInput ()
	{
		return new InputLoadFromSingleFileOpenCV("OpenGL_logo.png");
	}
} m_appInputLoadFromSingleFileOpenCV;

class AppInputLoadFromResizedSingleFileOpenCV: public AppInputTest
{
public:
	AppInputLoadFromResizedSingleFileOpenCV (): AppInputTest ("InputLoadFromResizedSingleFileOpenCV")
	{ }
	Ptr<Input> createInput ()
	{
		return new InputLoadFromSingleFileOpenCV("OpenGL_logo.png", int2(800,800), InputLoadFromSingleFileOpenCV::LINEAR);
	}
} m_appInputLoadFromResizedSingleFileOpenCV;

class AppInputLoadFromSingleFileOpenEXR: public AppInputTest
{
public:
	AppInputLoadFromSingleFileOpenEXR (): AppInputTest ("InputLoadFromSingleFileOpenEXR")
	{ }
	Ptr<Input> createInput ()
	{
		return new InputLoadFromSingleFileOpenEXR("img_movingscene_0.exr");
	}
} m_appInputLoadFromSingleFileOpenEXR;

class AppInputLoadFromResizedSingleFileOpenEXR: public AppInputTest
{
public:
	AppInputLoadFromResizedSingleFileOpenEXR (): AppInputTest ("InputLoadFromResizedSingleFileOpenEXR")
	{ }
	Ptr<Input> createInput ()
	{
		return new InputLoadFromSingleFileOpenEXR("img_movingscene_0.exr", int2(800,800), InputLoadFromSingleFileOpenEXR::LINEAR);
	}
} m_appInputLoadFromResizedSingleFileOpenEXR;

class AppInputLoadFromSequenceOpenEXR: public AppInputTest
{
public:
	AppInputLoadFromSequenceOpenEXR (): AppInputTest ("InputLoadFromSequenceOpenEXR") {}
	Ptr<Input> createInput ()
	{
		return new InputLoadFromSequence<InputLoadFromSingleFileOpenEXR>(GeneratorOfInputSequenceFilenames (m_sof, "img_movingscene_",".exr",0,3,1));
	}
} m_appInputLoadFromSequenceOpenEXR;


class AppInputLoadFromResizedSequenceOpenEXR: public AppInputTest
{
public:
	AppInputLoadFromResizedSequenceOpenEXR (): AppInputTest ("InputLoadFromResizedSequenceOpenEXR") {}
	Ptr<Input> createInput ()
	{
		return new InputLoadFromSequence<InputLoadFromSingleFileOpenEXR>(GeneratorOfInputSequenceFilenames (m_sof, "img_movingscene_",".exr",0,3,1), int2(800,800), InputLoadFromSingleFileOpenEXR::LINEAR);
	}
} m_appInputLoadFromResizedSequenceOpenEXR;

class AppInputLoadFromVideoOpenCV: public AppInputTest
{
public:
	AppInputLoadFromVideoOpenCV (): AppInputTest ("AppInputLoadFromVideoOpenCV",13) {}
	Ptr<Input> createInput ()
	{
		return new InputLoadFromVideoFileOpenCV ("clock.avi");
	}
} m_appInputLoadFromVideoOpenCV;


class AppInputLoadFromResizedVideoOpenCV: public AppInputTest
{
public:
	AppInputLoadFromResizedVideoOpenCV (): AppInputTest ("AppInputLoadFromResizedVideoOpenCV",13) {}
	Ptr<Input> createInput ()
	{
		return new InputLoadFromVideoFileOpenCV ("clock.avi", int2(800,800), InputLoadFromSingleFileOpenEXR::LINEAR);
	}
} AppInputLoadFromResizedVideoOpenCV;
