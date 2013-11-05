#include "Application.h"
#include "Error.h"
#include <windows.h>
#include <SDL.h>
#undef main
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>



//void ApplicationSDL13::setupOpenGL()
//{
//	glewExperimental=GL_TRUE;
//	static bool bGlewInit = false;
//	if (! bGlewInit)
//	{
//		bGlewInit = true;
//		GLenum err = glewInit();
//		if (err != GLEW_OK)
//			error0 (ERR_OPENGL, (char*) glewGetErrorString (err));
//	}	
//	float ratio = (float) m_width / (float) m_height;
//
//	/* Our shading model--Gouraud (smooth). */
//	glShadeModel( GL_SMOOTH );
//
//	/* Culling. */
//	glCullFace( GL_BACK );
//	glFrontFace( GL_CCW );
//	glEnable( GL_CULL_FACE );
//
//	/* Set the clear color. */
//	glClearColor( 0, 0, 0, 0 );
//
//	/* Setup our viewport. */
//	glViewport( 0, 0, m_width, m_height );
//
//	/*
//	* Change to the projection matrix and set
//	* our viewing volume.
//	*/
//	glMatrixMode( GL_PROJECTION );
//	glLoadIdentity( );
//	/*
//	* EXERCISE:
//	* Replace this with a call to glFrustum.
//	*/
//	//gluPerspective( 60.0, ratio, 1.0, 1024.0 );
//}
//
//void ApplicationSDL13::init()
//{
//	///* First, initialize SDL's video subsystem. */
//	//if( SDL_Init(0) < 0 ) {
//	//	/* Failed, exit. */
//	//	fprintf( stderr, "Video initialization failed: %s\n",
//	//		SDL_GetError( ) );
//	//	sendQuit();
//	//	return;
//	//}
//	//if( SDL_VideoInit(NULL) != 0)
//	//	error(ERR_SDL);
//
//	//m_window = SDL_CreateWindow("OpenGPP", 20, 20, 1280, 480, SDL_WINDOW_SHOWN);
//
//	//std::cout << "Detected render drivers:\n";
//	//for (int i = 0; i < SDL_GetNumRenderDrivers(); i++)
//	//{
//	//	SDL_RendererInfo info;
//	//	if(SDL_GetRenderDriverInfo(i,&info) != -1)
//	//		std::cout << "Driver: " << info.name << std::endl;
//	//}
//	//int selectedDriver = 1;
//	//SDL_RendererInfo info;
//	//SDL_GetRenderDriverInfo(selectedDriver,&info);
//	//std::cout << "Selected driver: " << info.name << std::endl;
//	//SDL_Renderer* renderer = SDL_CreateRenderer(m_window, selectedDriver, SDL_RENDERER_ACCELERATED);
//	//if (renderer == NULL)
//	//	error(ERR_SDL);
//
//	//setupOpenGL( );
//
//	const SDL_VideoInfo* info = NULL;
//	/* Dimensions of our window. */
//	int width = 0;
//	int height = 0;
//	/* Color depth in bits of our window. */
//	int bpp = 0;
//	/* Flags we will pass into SDL_SetVideoMode. */
//	int flags = 0;
//
//	/* First, initialize SDL's video subsystem. */
//	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
//		/* Failed, exit. */
//		fprintf( stderr, "Video initialization failed: %s\n",
//			SDL_GetError( ) );
//		sendQuit();
//		return;
//	}
//
//	/* Let's get some video information. */
//	info = SDL_GetVideoInfo( );
//
//	if( !info ) {
//		/* This should probably never happen. */
//		fprintf( stderr, "Video query failed: %s\n",
//			SDL_GetError( ) );
//		sendQuit();
//		return;
//	}
//
//	/*
//	* Set our width/height to 640/480 (you would
//	* of course let the user decide this in a normal
//	* app). We get the bpp we will request from
//	* the display. On X11, VidMode can't change
//	* resolution, so this is probably being overly
//	* safe. Under Win32, ChangeDisplaySettings
//	* can change the bpp.
//	*/
//	width = m_width;
//	height = m_height;
//	bpp = info->vfmt->BitsPerPixel;
//
//	/*
//	* Now, we want to setup our requested
//	* window attributes for our OpenGL window.
//	* We want *at least* 5 bits of red, green
//	* and blue. We also want at least a 16-bit
//	* depth buffer.
//	*
//	* The last thing we do is request a double
//	* buffered window. '1' turns on double
//	* buffering, '0' turns it off.
//	*
//	* Note that we do not use SDL_DOUBLEBUF in
//	* the flags to SDL_SetVideoMode. That does
//	* not affect the GL attribute state, only
//	* the standard 2D blitting setup.
//	*/
//	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
//	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
//	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
//	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
//	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
//
//	/*
//	* We want to request that SDL provide us
//	* with an OpenGL window, in a fullscreen
//	* video mode.
//	*
//	* EXERCISE:
//	* Make starting windowed an option, and
//	* handle the resize events properly with
//	* glViewport.
//	*/
//	flags = SDL_OPENGL;
//
//	/*
//	* Set the video mode
//	*/
//	if( SDL_SetVideoMode( width, height, bpp, flags ) == 0 ) {
//		/* 
//		* This could happen for a variety of reasons,
//		* including DISPLAY not being set, the specified
//		* resolution not being available, etc.
//		*/
//		fprintf( stderr, "Video mode set failed: %s\n",
//			SDL_GetError( ) );
//		sendQuit();
//		return;
//	}
//
//	/*
//	* At this point, we should have a properly setup
//	* double-buffered window for use with OpenGL.
//	*/
//	setupOpenGL( );
//}
//
//void ApplicationSDL13::exec()
//{
//	bool initialized = false;
//	while (1)
//	{
//		/* Our SDL event placeholder. */
//		SDL_Event event;
//		
//		/* Grab all the events off the queue. */
//		while( SDL_PollEvent( &event ) ) {
//
//			switch( event.type ) {
//			case SDL_KEYDOWN:
//				/* Handle key presses. */
//				handleKeyDown( &event.key.keysym );
//				break;
//			case SDL_QUIT:
//				/* Handle quit requests (like Ctrl-c).*/
//				clearData();
//				return;
//			}
//		}
//		if (SDL_QuitRequested())
//		{
//			//clearData();
//			return;
//		}
//		if (! initialized)
//		{
//			initialized = true;
//			initData();
//		}
//		render();
//	}
//}
//
//void ApplicationSDL13::clear()
//{
//}
//
//void ApplicationSDL13::drawScreen( void )
//{
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//
//	static GLfloat v0[] = { -1.0f, -1.0f,  0.0f };
//	static GLfloat v1[] = {  1.0f, -1.0f,  0.0f };
//	static GLfloat v2[] = {  1.0f,  1.0f,  0.0f };
//	static GLfloat v3[] = { -1.0f,  1.0f,  0.0f };
//	static GLubyte red[]    = { 255,   0,   0, 255 };
//	static GLubyte green[]  = {   0, 255,   0, 255 };
//	static GLubyte blue[]   = {   0,   0, 255, 255 };
//	static GLubyte white[]  = { 255, 255, 255, 255 };
//	static GLubyte yellow[] = {   0, 255, 255, 255 };
//	static GLubyte black[]  = {   0,   0,   0, 255 };
//	static GLubyte orange[] = { 255, 255,   0, 255 };
//	static GLubyte purple[] = { 255,   0, 255,   0 };
//
//	glClearColor(0.0, 0.0, 1.0, 1.0);
//	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
//
//	/* Send our triangle data to the pipeline. */
//
//	glBegin( GL_TRIANGLES );
//	glColor4ubv( red );
//	glVertex3fv( v0 );
//	glColor4ubv( green );
//	glVertex3fv( v1 );
//	glColor4ubv( blue );
//	glVertex3fv( v2 );
//
//	glColor4ubv( red );
//	glVertex3fv( v0 );
//	glColor4ubv( blue );
//	glVertex3fv( v2 );
//	glColor4ubv( white );
//	glVertex3fv( v3 );
//
//	glEnd( );
//
//	//g_rt->endRender ();
//	//g_pp->process ();
//}
//
//void ApplicationSDL13::handleKeyDown( SDL_keysym* keysym )
//{
//	switch( keysym->sym ) {
//	case SDLK_ESCAPE:
//		sendQuit();
//		break;
//	case SDLK_SPACE:
//		break;
//	default:
//		break;
//	}
//}
//
//void ApplicationSDL13::render()
//{
//	//drawScreen();
//	SDL_GL_SwapBuffers( );
//}
//
//void ApplicationSDL13::sendQuit()
//{
//	SDL_Quit();
//}

void Application::initOpenGL()
{
	glewExperimental=GL_TRUE;
	static bool bGlewInit = false;
	if (! bGlewInit)
	{
		bGlewInit = true;
		GLenum err = glewInit();
		if (err != GLEW_OK)
			error0 (ERR_OPENGL, (char*) glewGetErrorString (err));
	}	
	float ratio = (float) m_width / (float) m_height;

	/* Our shading model--Gouraud (smooth). */
	glShadeModel( GL_SMOOTH );

	/* Culling. */
	glCullFace( GL_BACK );
	glFrontFace( GL_CCW );
	glEnable( GL_CULL_FACE );

	/* Set the clear color. */
	glClearColor( 0, 0, 0, 0 );

	/* Setup our viewport. */
	glViewport( 0, 0, m_width, m_height );

	/*
	* Change to the projection matrix and set
	* our viewing volume.
	*/
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );
}

void Application::runMainCycle( SDL_Window* window )
{
	while (1)
	{
		/* Our SDL event placeholder. */
		SDL_Event event;

		/* Grab all the events off the queue. */
		while( SDL_PollEvent( &event ) )
		{
			switch(event.type)
			{
			case SDL_KEYDOWN:
				handleKeyDown(&event.key.keysym);
				break;
			case SDL_QUIT:
				return;
			}
		}
		if (m_quitApplication)
			break;

		render();
		SDL_GL_SwapWindow(window);
	}
}

void Application::run()
{
	SDL_Init(SDL_INIT_VIDEO); // Init SDL2

	SDL_Window* window = SDL_CreateWindow(
		"Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_width, m_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

	SDL_GLContext glcontext = SDL_GL_CreateContext(window);

	initOpenGL();
	initData();
	runMainCycle(window);
	clearData();
	m_quitApplication = false;
	SDL_GL_DeleteContext(glcontext);  
	SDL_DestroyWindow(window);
	SDL_Quit();
}
