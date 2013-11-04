#pragma once

#include "Ptr.h"
#include "GLClasses.h"

class Effect
{
public:
	virtual Ptr<GLTexture2D> process (Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap) = 0;

	virtual bool requireDepth ()
	{
		return false;
	}
	virtual bool requireEnvMap ()
	{
		return false;
	}
	virtual Ptr<GLTexture2D> processDepth (Ptr<GLTexture2D> depth)
	{
		return depth;
	}
	virtual ~Effect() {}
};