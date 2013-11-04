/////////////////////////////////////////////////////////////////////////////////
//
//  subor obaluje funkcionalitu sdl-ka
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <SDL_keysym.h>
#include <SDL_stdinc.h>

struct SDL_Surface;

namespace sdl
{
	// handlers:
	void onInit ();
	void onDestroy ();
	void onWindowResized (int width, int height);
	void onKeyDown (SDLKey key, Uint16 mod);
	void onKeyUp (SDLKey key, Uint16 mod);
	void onMouseMove (unsigned x, unsigned y, int xrel, int yrel, Uint8 buttons);
	void onMouseDown (Uint8 button, unsigned x, unsigned y);
	void onMouseUp (Uint8 button, unsigned x, unsigned y);
	void onRedraw (SDL_Surface* screen);

	void sendQuit();
	void sendRedraw();

	void init (int width, int height);
	void mainLoop ();
	void destroy ();
}