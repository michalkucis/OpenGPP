#pragma once

#include "GLclasses.h"

class Input
{
public:
	virtual Ptr<GLTexture2D> getProcessed () = 0;
	virtual bool isAvailableDepth()
	{
		return false;
	}
	virtual Ptr<GLTexture2D> getProcessedDepth () 
	{
		error0(ERR_STRUCT,"A depth map is required but none is loaded");
	}
	virtual bool isAvailableEnvMap()
	{
		return false;
	}
	virtual Ptr<GLTextureEnvMap> getProcessedEnvMap() 
	{
		error0(ERR_STRUCT,"");
	}
	virtual void next () = 0;
	virtual ~Input () {}
};