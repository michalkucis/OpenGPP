#pragma once

#include "Input.h"
#include "Effect.h"
#include "Vector.h"
#include "GLClassesFactories.h"
#include "SharedObjectsFactory.h"

class PostProcessor
{	
protected:
	Ptr<GLTexturesRGBFactory> m_factoryTexRGB;
	Ptr<GLTexturesRedFactory> m_factoryTexRed;
public:
	PostProcessor (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed)
	{
		m_factoryTexRGB = factoryRGB;
		m_factoryTexRed = factoryRed;
	}
	PostProcessor (Ptr<SharedObjectsFactory> sof)
	{
		m_factoryTexRGB = sof->getFactoryRGBTexture();
		m_factoryTexRed = sof->getFactoryRedTexture();
	}
	PostProcessor (uint2 res)
	{
		Ptr<SharedObjectsFactory> sof = new SharedObjectsFactory(res);
		m_factoryTexRGB = sof->getFactoryRGBTexture();
		m_factoryTexRed = sof->getFactoryRedTexture();
	}
	Ptr<Input> m_input;
	Vector<Ptr<Effect>> m_vecEffects;
	void clear ()
	{
		m_input = Ptr<Input>();
		m_vecEffects.clear();
	}
protected:
	void getDepthRequirements (Vector<bool>& vec)
	{
		vec.setSize(m_vecEffects.getSize());
		bool requireBool = false;
		for (int i = vec.getSize()-1; i >= 0; i--)
		{
			requireBool |= m_vecEffects[i]->requireDepth();
			vec[i] = requireBool;
		}
	}
	bool isRequiredEnvMap ()
	{
		bool required = false;
		for (int i = m_vecEffects.getSize()-1; i >= 0; i--)
		{
			required |= m_vecEffects[i]->requireEnvMap();
		}
		return required;
	}
	void preprocessGL();
public:
	void process ();
};