
#include <OpenGPP.h>

#include "GL/glut.h"
#include <stdlib.h>
#include <stdio.h>

#define checkImageWidth 64
#define checkImageHeight 64
static GLubyte checkImage[checkImageHeight][checkImageWidth][4];

static GLuint texName;

void makeCheckImage(void)
{
	int i, j, c;

	for (i = 0; i < checkImageHeight; i++) {
		for (j = 0; j < checkImageWidth; j++) {
			c = ((((i&0x8)==0)^((j&0x8))==0))*255;
			checkImage[i][j][0] = (GLubyte) c;
			checkImage[i][j][1] = (GLubyte) c;
			checkImage[i][j][2] = (GLubyte) c;
			checkImage[i][j][3] = (GLubyte) 255;
		}
	}
}

void init(void)
{    
/*	glClearColor (0.0, 0.0, 0.0, 0.0);
	glShadeModel(GL_FLAT);
	glEnable(GL_DEPTH_TEST);

	makeCheckImage();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &texName);
	glBindTexture(GL_TEXTURE_2D, texName);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
		GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, checkImageWidth, 
		checkImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 
		checkImage);
*/}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glBindTexture(GL_TEXTURE_2D, texName);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-3.0, -2.0, 0.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-3.0, 2.0, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(1.0, 2.0, 0.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(1.0, -2.0, 0.0);

	glTexCoord2f(0.0, 0.0); glVertex3f(1.0, -1.0, 0.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(1.0, 1.0, 0.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(2.41421, 1.0, -1.41421);
	glTexCoord2f(1.0, 0.0); glVertex3f(2.41421, -1.0, -1.41421);
	glEnd();
	glFlush();
	glDisable(GL_TEXTURE_2D);
}

PostProcessor* g_pp;
//Renderer* g_renderer;

void display(void)
{
	glShadeModel(GL_SMOOTH); 
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE); 
	glClearColor(0, 0, 0, 0);
	glViewport(0, 0, (GLsizei) 250, (GLsizei) 250);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	static bool bInit = false;
	if (! bInit)
	{
		g_pp = new PostProcessor;
		//g_renderer = new Renderer(1024, 768, GL_RGBA8, GL_RGBA, true, GL_DEPTH_COMPONENT24);
	}
	PostProcessor& pp = *g_pp;
	//Renderer& renderer = *g_renderer;

	if (! bInit)
	{
		//pp.m_input = renderer.createInput();
		pp.m_input = new InputLoadFromSingleFileOpenEXR("D:\\devel\\projects\\OpenGPP\\src\\OpenGPP\\Data\\exrChangeLightIntensity\\img_light1_lamp10_pos0.exr");

		SharedObjectsFactory sof(uint2(1024, 768));
		pp.m_vecEffects.pushBack(new EffectRenderToScreen(0,0,1,1));
		bInit = true;
	}

	//renderer.beginRender();
	//render();
	//renderer.endRender();
	//glViewport(0,0,250,250);
	//glShadeModel( GL_SMOOTH );

	///* Culling. */
	//glCullFace( GL_BACK );
	//glFrontFace( GL_CCW );
	//glDisable( GL_CULL_FACE );

	/* Set the clear color. */
	//glClearColor( 0, 0, 0, 0 );

	
	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();
	
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	
	/* Setup our viewport. */
	//glViewport( 0, 0, 250, 250 );

	pp.process();
	glFlush();

	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);

	glutInitWindowSize(250, 250);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(argv[0]);
	glewExperimental=GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK)
		error0 (ERR_OPENGL, (char*) glewGetErrorString (err));

	init();
	glutDisplayFunc(display);
	glutMainLoop();

	return 0; 
}