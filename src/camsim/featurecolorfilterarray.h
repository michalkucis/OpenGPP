/////////////////////////////////////////////////////////////////////////////////
//
//  FeatureColorFilterArray: definuje efekt, ktory simuluje color filter array
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "feature.h"

struct FeatureColorFilterArrayParam
{
	PtrHDRImage1f kernel;
	PtrHDRImage1f mask;
	int2 kernelOff;

	FeatureColorFilterArrayParam ()
	{
		kernel = PtrHDRImage1f (new HDRImage1f);		
		mask = PtrHDRImage1f (new HDRImage1f);
	}
};

class FeatureColorFilterArray: public Feature
{
	FeatureColorFilterArrayParam m_param[3];
	HDRImage3f m_mask;
	PtrHDRImage3f m_hdr;
	uint2 m_hdrBordersMin, m_hdrBordersMax;
public:
	FeatureColorFilterArray (FeatureColorFilterArrayParam& red, 
		FeatureColorFilterArrayParam& green, FeatureColorFilterArrayParam& blue)
	{
		m_param[0] = red;
		m_param[1] = green;
		m_param[2] = blue;
		m_hdr = PtrHDRImage3f (new HDRImage3f);
		//m_hdrAppend = m_param[0].kernel->getSize();
		uint2 bordersMin = uint2 (0,0);
		uint2 bordersMax = uint2 (0,0);
		for (uint i = 0; i < 3; i++)
		{
			uint2 size = m_param[i].kernel->getSize();
			int2 bmin = m_param[i].kernelOff;
			int2 bmax = size.get<int>() - bmin - 1;
			bordersMin = uint2 (std::max<uint> (bmin.x, bordersMin.x), std::max<uint> (bmin.y, bordersMin.y));
			bordersMax = uint2 (std::max<uint> (bmax.x, bordersMax.x), std::max<uint> (bmax.y, bordersMax.y));
			
		}
		//m_hdrAppend = uint2 (m_hdrAppend.x-1, m_hdrAppend.y-1);
	}

	type getType ()
	{
		return FEATURE_3F_TO_3F;
	}
private:
	void initMask (PtrHDRImage3f in)
	{
		if (m_mask.getSize() == in->getSize())
			return;
		m_mask.setSize (in->getSize());
		for (int i = 0; i < 3; i++)
		{
			float* p;
			FeatureColorFilterArrayParam* param = &(m_param[i]);
			switch (i)
			{
			case 0: p = m_mask.getRedBuffer(); break;
			case 1: p = m_mask.getGreenBuffer(); break;
			case 2: p = m_mask.getBlueBuffer(); break;
			}

			for (uint y = 0; y < m_mask.getSize().y; y++)
				for (uint x = 0; x < m_mask.getSize().x; x++)
				{
					uint2 paramSize = param->mask->getSize(); 
					p[m_mask.getTotalSize().x*y + x] = param->mask->getPixel (x%paramSize.x, y%paramSize.y);
				}
		}
	}
	void copyAndAddBorders (PtrHDRImage3f in, PtrHDRImage3f out, uint2 borderMin, uint2 borderMax)
	{
		out->setSize (in->getSize() + borderMin + borderMax);
	}
	void copyAndRemoveBorders (PtrHDRImage3f in, PtrHDRImage3f out, uint2 borderMin, uint2 borderMax)
	{
		out->setSize (in->getSize() - borderMin - borderMax);
	}
public:
	void featureFtoF (PtrHDRImage3f in,PtrHDRImage3f& out)
	{
		m_hdr->setSize(in->getSize());
		out = m_hdr;
		//copyAndAddBorders (in, m_hdr, m_hdrBordersMin, m_hdrBordersMax);

		// mosaicing:
		initMask (in);
		for (uint nChannel = 0; nChannel < 3; nChannel++)
		{
			float* f1, *f2;
			FeatureColorFilterArrayParam& param = m_param[nChannel];
			switch (nChannel)
			{
			case 0: f1 = in->getRedBuffer(); f2 = m_mask.getRedBuffer(); break;
			case 1: f1 = in->getGreenBuffer(); f2 = m_mask.getGreenBuffer(); break;
			case 2: f1 = in->getBlueBuffer(); f2 = m_mask.getBlueBuffer(); break;
			}

			for (uint y = 0; y < in->getSize().y; y++)
			{
				for (uint x = 0; x < in->getSize().x; x++)
				{
					f1[x] *= f2[x];
				}
				f1 += in->getTotalSize().x;
				f2 += m_mask.getTotalSize().x;
			}
		}

		// demosaicing:
		for (uint nChannel = 0; nChannel < 3; nChannel++)
		{
			float* f1, *f2, *out;
			FeatureColorFilterArrayParam& param = m_param[nChannel];
			switch (nChannel)
			{
			case 0: f1 = in->getRedBuffer(); f2 = param.kernel->getBuffer(); out = m_hdr->getRedBuffer(); break;
			case 1: f1 = in->getGreenBuffer(); f2 = param.kernel->getBuffer(); out = m_hdr->getGreenBuffer(); break;
			case 2: f1 = in->getBlueBuffer(); f2 = param.kernel->getBuffer(); out = m_hdr->getBlueBuffer(); break;
			}
			int2 kmin = -param.kernelOff;
			int2 kmax = param.kernel->getSize().get<int>() + kmin;
			int2 minI = int2 (0,0);
			int2 maxI = in->getSize().get<int>() - int2(1,1); 
			int2 size = in->getSize().get<int>();
			int2 total = in->getTotalSize().get<int>();
			int2 sizeKernel = param.kernel->getSize().get<int>();
			//for (int y = 0; y < size.y; y++)
			//{
			//	for (int x = 0; x < size.x; x++)

			for (int y = -kmin.y; y < size.y-kmax.y; y++)
			{
				for (int x = -kmin.x; x < size.x-kmax.x; x++)
				{
					float sum = 0.0f;
					for (int ix = kmin.x; ix < kmax.x; ix++)
					{
						for (int iy = kmin.y; iy < kmax.y; iy++)
						{
							int2 i (ix + x, iy + y);
							//i = int2::getMax (minI, int2::getMin(maxI, i));
							int2 ikernel (ix-kmin.x, (iy-kmin.y));
							float f = f2 [ikernel.x + ikernel.y*sizeKernel.x]; 
							f *= f1 [i.x + total.x*i.y];
							sum += f;
						}
					}
					out [x + size.x*y] = sum;
				}
				//f1 += m_hdr->getTotalSize().x;
				//f2 += m_mask.getTotalSize().x;
			}
		}
		//copyAndRemoveBorders (m_hdr, in, m_hdrBordersMin, m_hdrBordersMax)
	}
};