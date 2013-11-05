#pragma once

#include "GLClasses.h"
#include "Ptr.h"
#include "Input.h"
#include "Error.h"

class InputRenderedColor: public Input
{
protected:
	Ptr<bool> m_rendererInit;
	Ptr<GLTexture2D> m_texColor;
public:
	InputRenderedColor (Ptr<bool> rendererInit, Ptr<GLTexture2D> texColor)
	{
		m_rendererInit = rendererInit;
		m_texColor = texColor;
	}
	Ptr<GLTexture2D> getProcessed ()
	{
		if (! *m_rendererInit)
			error0(ERR_STRUCT,"Renderer is not initialized");
		return m_texColor;
	}
	void next ()
	{

	}
};
