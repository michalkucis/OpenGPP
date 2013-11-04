#include "PostProcessor.h"
#include <gl\glew.h>
#include <gl\GL.h>

void PostProcessor::preprocessGL()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1 , 1 , 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
