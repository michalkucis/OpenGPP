/////////////////////////////////////////////////////////////////////////////////
//
//  featuredof: efekt hlbky ostrosti
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "feature.h"
#include "dof.h"


class FeatureDOF: public Feature
{
public:
	enum etype_t {DIFFUSION, GATHER};
private:
	etype_t m_type;
	float m_focusDist, m_apertureValue, m_focalLen, m_sensorSize;

	PtrHDRImage1f m_depth;
	float m_prevSizeX;
	PtrDOF m_dof;
public:
	FeatureDOF (etype_t type, float focusDist, float apertureValue, float focalLen, float sensorSize)
	{
		m_type = type;
		m_focusDist = focusDist;
		m_apertureValue = apertureValue;
		m_focalLen = focalLen;
		m_sensorSize = sensorSize;
		m_prevSizeX = -1;
	}
	type getType ()
	{
		return Feature::FEATURE_3F_TO_3F;
	}
	void setDepthMap (PtrHDRImage1f depth)
	{
		m_depth = depth;
	}
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f& out)
	{
		if (! m_depth.get())
			throw XMLexception ("", "", "Depth of field effect requires depth map. The depth map is not set.");

		if ((!m_dof.get()) || (m_prevSizeX != m_depth->getSize().x))
		{
			m_prevSizeX = (float) m_depth->getSize().x;

			PtrFuncRadiusOfCoC func = PtrFuncRadiusOfCoC (new FuncFocusedToDist 
				(m_focusDist, m_apertureValue, m_focalLen, m_sensorSize, (float)m_depth->getSize().x));

			switch (m_type)
			{
			case XMLdepthoffield::TYPE_DIFFUSION:
				m_dof = createDiffusionDOF (func);
				break;
			case XMLdepthoffield::TYPE_GATHER:
				m_dof = createLinGatherDOF (func);
				break;
			}
		}
		m_dof->doDOF (in, m_depth, out);
	}
};
