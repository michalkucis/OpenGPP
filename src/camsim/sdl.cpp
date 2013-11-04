#include "sdl.h"
#include <SDL.h>
#undef main
#include "type.h"
#include <sstream>
#include <SDL_image.h>

#pragma warning (disable : 4996)
namespace sdl
{

void updateFPSCaption ()
{
	static unsigned int fps = 0;
	static uint secs = SDL_GetTicks ();
	if(SDL_GetTicks()-secs>=1000)
	{
		++fps;
		std::stringstream out;
		out << "FPS: "<< fps;
		char buffer[1024];
		sprintf ((char*) buffer, "FPS: %4.2f",  fps / ((float)(SDL_GetTicks() - secs))*1000);
		SDL_WM_SetCaption(buffer, NULL);
		fps=0;
		secs=SDL_GetTicks();
	}
	else
		++fps;
}

void sendQuit()
{
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

void sendRedraw()
{
    SDL_Event event;
    event.type = SDL_VIDEOEXPOSE;
    SDL_PushEvent(&event);
}

SDL_Surface* g_screen;
void onWindowRedraw ()
{  
    onRedraw (g_screen);
	SDL_Flip (g_screen); 
    sendRedraw ();
}

void init (int width, int height)
{
	SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER);
	if (! IMG_Init (IMG_INIT_JPG))
		exit (-1);
    g_screen = SDL_SetVideoMode (width, height, 0, SDL_DOUBLEBUF | SDL_RESIZABLE);
    if (g_screen == NULL) 
        exit(-1);
	SDL_WM_SetCaption ("Initializing...", NULL);
	onInit ();
}

void destroy ()
{
	SDL_Quit ();
	IMG_Quit ();
}

void mainLoop ()
{
    bool active = true;

    for (;;)
    {
        SDL_Event event;
        if (SDL_WaitEvent (&event) == 0) 
			return;

        bool redraw = false;
        do
        {
            switch (event.type)
            {
                case SDL_ACTIVEEVENT:
                    if (event.active.state == SDL_APPACTIVE)
                        active = (event.active.gain ? true : false);
                    break;
                case SDL_KEYDOWN :
                    onKeyDown (event.key.keysym.sym, event.key.keysym.mod);
                    break;
                case SDL_KEYUP :
                    onKeyUp (event.key.keysym.sym, event.key.keysym.mod);
                    break;
                case SDL_MOUSEMOTION :
                    onMouseMove (event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel, event.motion.state);
                    break;
                case SDL_MOUSEBUTTONDOWN :
                    onMouseDown (event.button.button, event.button.x, event.button.y);
                    break;
                case SDL_MOUSEBUTTONUP :
                    onMouseUp (event.button.button, event.button.x, event.button.y);
                    break;
                case SDL_QUIT :
                    return;
                case SDL_VIDEORESIZE :
                    onWindowResized (event.resize.w, event.resize.h);
                    break;
                case SDL_VIDEOEXPOSE :
                    redraw = true;
                    break;
                default :
                    break;
            }
        } while (SDL_PollEvent(&event) == 1);

        if (active && redraw)
		{
            onWindowRedraw ();
			updateFPSCaption ();
		}
    }
}

}