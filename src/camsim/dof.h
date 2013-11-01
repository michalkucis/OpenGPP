/////////////////////////////////////////////////////////////////////////////////
//
//  DOF: abstraktna trieda, ktora definuje rozhranie pre algoritmy simulujuce efekt hlbky ostrosti
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "func.h"
#include "hdrimage.h"
#include <boost/shared_ptr.hpp>







class DOF
{
	/*class FuncFocusedToInf: public FuncRadiusOfCoC
	{
	public:
		float operator() (float z)
		{
			//static float minZ = 10000000000000.0f;
			//minZ = std::min (minZ, z);
			float radius = c_sizeOfCoCInDepthOne/z/2;
			radius = std::min<float> (c_maxoffset, radius);
			return radius;
		}
	};*/
protected:
	PtrFuncRadiusOfCoC m_func;
	float getRadiusOfCoC (float z)
	{
		return (*m_func) (z);
	}
	float getAreaOfCoC (float z)
	{
		float r = getRadiusOfCoC (z);
		return r*r*3.14f;
	}
public:
	DOF (PtrFuncRadiusOfCoC func): m_func (func)
	{
	}

	float getMaxRadiusOfCoC () const
	{
		return 50;
	}

	void getHDRICoC (HDRImage1f* in_depth, HDRImage1f* out_coc)
	{
		out_coc->setSize (in_depth->getSize());
		uint width = in_depth->getSize().x;
		uint height = in_depth->getSize().y;
		for (uint ny = 0; ny < height; ny++)
			for (uint nx = 0; nx < width; nx++)
			{
				float z = in_depth->getPixel (nx, ny);
				float radius = std::min<float> ((float) (getMaxRadiusOfCoC ()), (float) (getRadiusOfCoC(z)));

				float coc = std::max<float> (1.0f, radius);
				out_coc->setPixel (uint2(nx, ny), coc);
			}
	}

	virtual void doDOF (PtrHDRImage3f in, PtrHDRImage1f depth, PtrHDRImage3f &out) = 0;
};
typedef boost::shared_ptr<DOF> PtrDOF;

PtrDOF createLinGatherDOF (PtrFuncRadiusOfCoC func);

PtrDOF createDiffusionDOF (PtrFuncRadiusOfCoC func);