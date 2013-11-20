#pragma once

#include "Input.h"
#include "Effect.h"
#include "Vector.h"

class PostProcessor
{
public:
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