#include "inputloader.h"
#include "outputsaver.h"

#include "sdl.h"
#include <SDL_video.h>






void ProcessWithoutFeaturesNoWnd (string filename, string input, string output)
{	
	PtrInputLoader il = sharedNew<InputLoader> (filename, input);
	PtrOutputSaver os = sharedNew<OutputSaver> (filename, output);
	while ( !il->isEnd() && !os->isEnd() )
	{
		os->save (il->getColor());
		il->next();
	}
}




namespace sdl
{
	PtrInputLoader g_il;
	PtrOutputSaver g_os;
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
			PtrHDRImage3f hdriWnd;
			//if ()
			PtrHDRImage3f color = g_il->getColor();
			color->getSubImage (hdriWnd, uint2(g_wndSize.x, g_wndSize.y), float2(0,0), float2(0,0));
			//else

			static SDL_Surface* surface = NULL;
			surface = hdriWnd->getSDLSurface (color->getMult(), surface, uint2(g_wndSize.x, g_wndSize.y));
			SDL_BlitSurface (surface, NULL, screen, NULL);

			g_os->save (g_il->getColor());
			g_il->next();
			sdl::sendRedraw ();
		}
		else
		{
			sdl::sendQuit ();
		}
	}
}

void ProcessWithoutFeaturesShowWnd (string filename, string input, string output)
{	
	sdl::g_il = sharedNew<InputLoader> (filename, input);
	sdl::g_os = sharedNew<OutputSaver> (filename, output);
	sdl::g_wndSize = sdl::g_os->getWndSize();
	sdl::init (sdl::g_os->getWndSize().x, sdl::g_os->getWndSize().y);
	sdl::mainLoop ();
	sdl::destroy ();
	sdl::g_il = sharedNull<InputLoader> ();
	sdl::g_os = sharedNull<OutputSaver> ();
}

void ProcessWithoutFeatures (string filename, string strinput, string stroutput)
{
	XMLparser parser (filename);
	PtrXMLoutput output = parser.parseOutput(stroutput);
	if (output->m_wndVisibled)
		ProcessWithoutFeaturesShowWnd (filename, strinput, stroutput);
	else
		ProcessWithoutFeaturesNoWnd (filename, strinput, stroutput);
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
		else if (argc == 4)
		{
			ProcessWithoutFeatures (argv[1], argv[2], argv[3]);
		}
	} catch (XMLexception& ex)
	{
		std::cerr << "ERROR in xml: " << ex.get() << std::endl;
	}

	return 0;
}