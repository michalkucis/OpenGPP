#include <iostream>
#include <OpenGPP.h>


class ApplicationTutorial03: public Application
{
	Ptr<PostProcessor> m_pp;

public:
	ApplicationTutorial03 (): Application(250, 250)
	{
	}
	void initData()
	{
		m_pp = new PostProcessor(new SharedObjectsFactory(uint2(250,250)));
		m_pp->m_input = new InputLoadFromSingleFileOpenEXR("exrChangeLightIntensity\\img_light1_lamp0_pos0.exr");
		m_pp->m_vecEffects.pushBack(new EffectRenderToScreen(0,0,1,1));
	}
	void render()
	{
		m_pp->process();
	}
	void clearData ()
	{		
		m_pp->clear ();
	}
};


int main( int argc, char* argv[] )
{
	try
	{	
		ApplicationTutorial03 a; 
		a.run();
	} catch (Error& ex) {
		std::string desc = ex.getDesc();
		printf ("%s\n", desc.c_str());
		std::cout << "Wait until any button to be pressed...";
		std::cin.ignore(1);	
	}

	return 0;
}
