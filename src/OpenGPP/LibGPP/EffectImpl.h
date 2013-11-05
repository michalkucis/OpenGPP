#pragma once

#include "Effect.h"
#include "Vector2.h"
#include "Input.h"

class EffectRenderToScreen: public Effect
{
	float2 m_org;
	float2 m_size;
public:
	EffectRenderToScreen (float x, float y, float width, float height)
	{
		m_org = float2 (x,y);
		m_size = float2 (width, height);
	}
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
};

class EffectRenderDepthToScreen: public Effect
{
	float2 m_org;
	float2 m_size;
	GLProgram m_program;
public:
	EffectRenderDepthToScreen (float x, float y, float width, float height):
		m_program("", "shaders\\imageMult.fs")
	{
		m_org = float2 (x,y);
		m_size = float2 (width, height);
	}
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
	bool requireDepth()
	{
		return true;
	}
};

class EffectLoadInput: public Effect
{
	Ptr<Input> m_input;
public:
	EffectLoadInput (Ptr<Input> input)
	{
		m_input = input;
	}
	void setInput (Ptr<Input> input)
	{
		m_input = input;
	}
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
	{
		return m_input->getProcessed();
	}
};