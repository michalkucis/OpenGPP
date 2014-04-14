#include "OpenGPP.h"
#include "CLWrapper.h"
#include <fstream>
#include <ImfRgbaFile.h>
#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfOutputFile.h>
#include <ImathBox.h>
#include <windows.h>

#include "Tests.h"
#include "UserTransformations.h"


class Window
{
	SDL_Window* m_window;
	int m_width, m_height;
	bool m_quitApplication;

	Ptr<ClFacade> m_facade;
protected:
	void initOpenGL()
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
	void initOpenCL()
	{
		int2 maxResolution(1036,777);
		m_facade = new ClFacade (maxResolution);
	}
	void runMainCycle()
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
					//handleKeyDown(&event.key.keysym);
					break;
				case SDL_QUIT:
					return;
				}
			}
			if (m_quitApplication)
				break;

			render();
			SDL_GL_SwapWindow(m_window);
			return;
		}
	}
	void initData()
	{		

	}
	void clearData()
	{
	}
	void render()
	{
		performTests(m_facade);
	}
public:
	Window (int2 windowSize)
	{
		m_width = windowSize.x;
		m_height = windowSize.y;
		m_quitApplication = false;

		SDL_Init(SDL_INIT_VIDEO); // Init SDL2

		m_window = SDL_CreateWindow(
			"Demo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_width, m_height, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

		SDL_GLContext glcontext = SDL_GL_CreateContext(m_window);

		initOpenGL();
		initOpenCL();
		initData();
		runMainCycle();
		clearData();
		m_quitApplication = false;
		SDL_GL_DeleteContext(glcontext);  
		SDL_DestroyWindow(m_window);
		SDL_Quit();
	}
};


int main(int, const char**)
{
	int2 resolution (640, 468);

	Window window (resolution);

	return 0;
}