#include "inputloader.h"
#include "outputsaver.h"
#include "featureprocessor.h"
#include "sdl.h"

#include <SDL_video.h>
#include <vector>
#include <iostream>




PtrInputLoader g_il;
PtrOutputSaver g_os;
PtrFeatureProcessor g_fp = getDefaultFeatureProcessor ();



void ProcessNoWnd (string filename)
{	
	while ( !g_il->isEnd() && !g_os->isEnd() )
	{
		PtrHDRImage3f color = g_il->getColor();
		PtrHDRImage1f depth = g_il->getDepth();
		PtrHDRImage3f envmap = g_il->getEnvmap();
		PtrHDRImage3f image = g_fp->Process(color, depth, envmap);
		g_os->save (image);
		g_il->next();
		if (g_il->isSingleImageInput() && g_os->getXMLoutput()->m_isVideo)
			break;
		if (g_il->isSingleImageInput() && g_os->getXMLoutput()->m_isSequence)
			break;
	}
}


namespace sdl
{
	int2 g_wndSize;
	void onInit () { }
	void onDestroy () { }
	void onWindowResized (int width, int height) { }
	void onKeyDown (SDLKey key, Uint16 mod) { }
	void onKeyUp (SDLKey key, Uint16 mod) { }
	void onMouseMove (unsigned x, unsigned y, int xrel, int yrel, Uint8 buttons) { }
	void onMouseDown (Uint8 button, unsigned x, unsigned y) { }
	void onMouseUp (Uint8 button, unsigned x, unsigned y) { }
	void onRedraw (SDL_Surface* screen)
	{
		if ( !g_il->isEnd() && !g_os->isEnd() )
		{
			PtrHDRImage3f color = g_il->getColor();
			PtrHDRImage1f depth = g_il->getDepth();
			PtrHDRImage3f envmap = g_il->getEnvmap();
			PtrHDRImage3f image = g_fp->Process(color, depth, envmap);

			PtrHDRImage3f hdriWnd;

			image->getSubImage (hdriWnd, uint2(g_wndSize.x, g_wndSize.y), float2(0,0), float2(0,0));
			
			static SDL_Surface* surface = NULL;
			surface = hdriWnd->getSDLSurface (image->getMult(), surface, uint2(g_wndSize.x, g_wndSize.y));
			SDL_BlitSurface (surface, NULL, screen, NULL);

			g_os->save (image);
			g_il->next();
			sdl::sendRedraw ();
			if (g_il->isSingleImageInput() && g_os->getXMLoutput()->m_isVideo)
				sdl::sendQuit();
			if (g_il->isSingleImageInput() && g_os->getXMLoutput()->m_isSequence)
				sdl::sendQuit();
		}
		else
		{
			sdl::sendQuit();
		}
	}
}




void ProcessShowWnd (string filename, PtrXMLoutput xmloutput)
{	
	sdl::g_wndSize = g_os->getWndSize();
	sdl::init (g_os->getWndSize().x, g_os->getWndSize().y);
	sdl::mainLoop ();
	sdl::destroy ();
}

void ProcessWithFeatures (string filename, PtrXMLoutput xmlout)
{
	XMLparser parser (filename);
	if (g_os->isWndVisibled())
		ProcessShowWnd (filename, xmlout);
	else
		ProcessNoWnd (filename);
}

string g_strInput, g_strOutput, g_strFeatures;
void Process (string filename, string xmlcmds)
{
	XMLparser parser (filename);
	PtrXMLcommands cmds = parser.parseCommands(xmlcmds);
	for (uint i = 0; i < cmds->vecCmds.size(); i++)
	{
		XMLcommands::Command_t cmd = cmds->vecCmds.at(i);
		switch (cmd.cmd)
		{
		case XMLcommands::INPUT:
			g_strInput = cmd.param;
			g_il = sharedNew<InputLoader> (filename, cmd.param);
			break;
		case XMLcommands::OUTPUT:
			g_strOutput = cmd.param;
			g_os = sharedNew<OutputSaver> (filename, cmd.param);
			break;
		case XMLcommands::FEATURES:
			g_strFeatures = cmd.param;
			g_fp = sharedNew<FeatureProcessor> (filename, cmd.param);
			break;
		case XMLcommands::COMMIT:
			std::cout << g_strInput << " >> " << g_strFeatures <<
				" >> " << g_strOutput << std::endl;
			if (! g_il.get())
				throw XMLexception ("","","Cannot commit without input");
			if (! g_os.get())
				throw XMLexception ("","","Cannot commit without output");
			ProcessWithFeatures(filename, g_os->getXMLoutput());
			break;
		}
	}
	g_il = sharedNull<InputLoader> ();
	g_os = sharedNull<OutputSaver> ();
}

using std::endl;
using std::cout;
int main (int argc, const char** argv)
{
	try
	{	
		if (argc == 1)
		{
			cout << "io.exe XML_FILE $INPUT_ELEM_IN_XML $OUTPUT_ELEM_IN_XML" << endl << endl;
			cout << "example:" << endl;
			cout << "  io.exe \"io Debug.xml\" grayscale grayscale" << endl;
			cout << "  io.exe \"io Debug.xml\" color color" << endl;
			cout << "  io.exe \"io Debug.xml\" reposition2 sequence1" << endl;
			cout << "  io.exe \"io Debug.xml\" changeLightIntensity sequence2" << endl;
			cout << "  io.exe \"io Debug.xml\" dataset1_0000 exr" << endl;
			cout << "  io.exe \"io Debug.xml\" avi reposition.avi" << endl;
		}
		else if (argc == 3)
		{
			Process (argv[1], argv[2]);
		}
	} catch (XMLexception& ex)
	{
		std::cerr << "ERROR in xml: " << ex.get() << std::endl;
	}

	return 0;
}