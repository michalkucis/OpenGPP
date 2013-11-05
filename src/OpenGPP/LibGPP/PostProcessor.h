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
	void process ()
	{
		preprocessGL();
	
		Vector<bool> vecDepthRequirements;
		getDepthRequirements (vecDepthRequirements);
		bool requiredEnvMap = isRequiredEnvMap(); 

		Ptr<GLTexture2D> tex = m_input->getProcessed();
		Ptr<GLTexture2D> depth;
		Ptr<GLTextureEnvMap> envMap = requiredEnvMap ? m_input->getProcessedEnvMap() : NULL;
		if (vecDepthRequirements.getSize() && vecDepthRequirements[0])
			depth = m_input->getProcessedDepth();
		for (uint i = 0; i < m_vecEffects.getSize(); i++)
		{
			tex = m_vecEffects[i]->process(tex, depth, envMap);
			if (vecDepthRequirements[i])
				depth = m_vecEffects[i]->processDepth(depth);
			else
				depth = Ptr<GLTexture2D>();
		}
		m_input->next();
	}
};