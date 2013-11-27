#include "PostProcessor.h"
#include "EffectGLImpl.h"
#include <gl\glew.h>
#include <gl\GL.h>

void PostProcessor::preprocessGL()
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1 , 1 , 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

void PostProcessor::process()
{
	glEnable(GL_TEXTURE_2D);

	bool prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_TEST);

	preprocessGL();

	Vector<bool> vecDepthRequirements;
	getDepthRequirements (vecDepthRequirements);
	bool requiredEnvMap = isRequiredEnvMap(); 

	Ptr<GLTexture2D> tex = m_input->getProcessed();
	Ptr<GLTexture2D> depth;
	Ptr<GLTextureEnvMap> envMap = requiredEnvMap ? m_input->getProcessedEnvMap() : NULL;
	if (vecDepthRequirements.getSize() && vecDepthRequirements[0])
		depth = m_input->getProcessedDepth();
	if (m_vecEffects.getSize())
	{
		EffectResizeImages effect (m_factoryTexRGB, m_factoryTexRed);
		tex = effect.process(tex, depth, envMap);
		if (vecDepthRequirements[0])
			depth = effect.processDepth(depth);
	}
	for (uint i = 0; i < m_vecEffects.getSize(); i++)
	{
		tex = m_vecEffects[i]->process(tex, depth, envMap);
		if (vecDepthRequirements[i])
			depth = m_vecEffects[i]->processDepth(depth);
		else
			depth = Ptr<GLTexture2D>();
	}
	m_input->next();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	if(prevDepthTest)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}
