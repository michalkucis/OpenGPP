#pragma once

#include "Ptr.h"
#include "GLClasses.h"
#include "GLClassesFactories.h"

class SharedObjectsFactory
{
protected:
	Ptr<GLTexturesRGBFactory> m_rgbtexFactory;
	Ptr<GLTexturesRedFactory> m_redtexFactory;
	uint2 m_processingResolution;

public:
	SharedObjectsFactory(uint2 resolution)
	{
		m_processingResolution = resolution;
	}

	Ptr<GLTexturesRGBFactory> getFactoryRGBTexture()
	{
		if (! m_rgbtexFactory)
			m_rgbtexFactory = new GLTexturesRGBFactory(m_processingResolution);
		return m_rgbtexFactory;
	}

	Ptr<GLTexturesRedFactory> getFactoryRedTexture()
	{
		if (! m_redtexFactory)
			m_redtexFactory = new GLTexturesRedFactory(m_processingResolution);
		return m_redtexFactory;
	}
};
