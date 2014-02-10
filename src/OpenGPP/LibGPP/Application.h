#pragma once

#pragma comment(lib, "SDL2.lib")

struct SDL_keysym;

// SDL 2
#define HAVE_M_PI
#include <SDL.h>
#undef main

class Application
{
protected:
	int m_width, m_height;
	bool m_quitApplication;
public:
	Application (int width, int height)
	{
		m_width = width;
		m_height = height;
		m_quitApplication = false;
	}
protected:
	virtual void initData ()
	{ }
	virtual void render () 
	{ }
	virtual void clearData () 
	{ }
	virtual void handleKeyDown(SDL_Keysym* keysym)
	{ }
	void initOpenGL();
	void runMainCycle(SDL_Window* window);
public:
	void run ();
	void sendQuit();

};

// SDL 1.3
//class ApplicationSDL13
//{
//	int m_width, m_height;
//public:
//	ApplicationSDL13 (int width, int height)
//	{
//		m_width = width;
//		m_height = height;
//	}
//protected:
//	void setupOpenGL ();
//	void init ();
//	void exec ();
//	void clear ();
//	void drawScreen ();
//	void sendQuit ();
//	virtual void initData ()
//	{ }
//	virtual void handleKeyDown( SDL_keysym* keysym );
//	virtual void render ();
//	virtual void clearData () 
//	{ }
//public:
//	void run ()
//	{
//		init();
//		exec();
//		clear();
//	}
//
//};