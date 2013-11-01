/////////////////////////////////////////////////////////////////////////////////
//
//  FeatureStaticFFT: efekt modifikuje vstupnu mapu pomocou konvolucie
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "feature.h"

class FeatureStaticFFT: public Feature
{
	PtrFuncI2I2toF m_func;
	boost::shared_ptr<HDRImage1f> m_hdriKernel;
	boost::shared_ptr<HDRImage1f> m_hdriKernelFreq;

public:
	FeatureStaticFFT (PtrFuncI2I2toF func)
	{
		m_func = func;
		m_hdriKernel = boost::shared_ptr<HDRImage1f> (new HDRImage1f);
	}
	type getType ()
	{
		return Feature::FEATURE_3C_TO_3C;
	}
	boost::shared_ptr<HDRImage1f> getKernel ()
	{
		return m_hdriKernel;
	}
protected:
	void updateHDRIKernel (uint2 size, uint2 append)
	{
		if (m_hdriKernel->getTotalSize() == (size+append))
			return;

		uint2 total = size+append;
		m_hdriKernel->setSize (size, append);

		//m_hdriKernelFreq = m_hdriKernel.computeFFT();
		int2 isize ((int)size.x, (int)size.y);
		FuncI2I2toF* func = m_func.get();
		for (uint iy = 0; iy < total.y; iy++)
			for (uint ix = 0; ix < total.x; ix++)
			{
				uint y = iy < size.y ? iy : iy-size.y-append.y;
				uint x = ix < size.x ? ix : ix-size.x-append.x;

				float val = (*func) (int2(x,y), isize);
				m_hdriKernel->setPixel (uint2 (ix,iy), val);
			}

		m_hdriKernelFreq = m_hdriKernel->computeFFT();
	}

public:
	void featureCtoC (PtrHDRImage3c in, PtrHDRImage3c &out)
	{
		out = in;
		HDRImage3c* inout_hdriFreq = in.get();

		uint2 size = inout_hdriFreq->getSize()*uint2(1,2)-uint2(0,2);
		uint2 append = inout_hdriFreq->getAppendSize()*uint2(1,2);

		updateHDRIKernel (size, append);
		int countX = inout_hdriFreq->getTotalSize().x;
		int countY = inout_hdriFreq->getTotalSize().y;

		fftwf_complex* v1base = (fftwf_complex*)m_hdriKernelFreq->getBuffer();
		fftwf_complex* v2base = (fftwf_complex*)inout_hdriFreq->getRedBuffer();
		for (int y = 0; y < countY; y++)
			for (int x = 0; x < countX; x++)
			{
				mulAndStore ((*v2base), (*v1base));
				v1base++;
				v2base++;
			}
		v1base = (fftwf_complex*)m_hdriKernelFreq->getBuffer();
		v2base = (fftwf_complex*)inout_hdriFreq->getGreenBuffer();
		for (int y = 0; y < countY; y++)
			for (int x = 0; x < countX; x++)
			{
				mulAndStore ((*v2base), (*v1base));
				v1base++;
				v2base++;
			}
		v1base = (fftwf_complex*)m_hdriKernelFreq->getBuffer();
		v2base = (fftwf_complex*)inout_hdriFreq->getBlueBuffer();
		for (int y = 0; y < countY; y++)
			for (int x = 0; x < countX; x++)
			{
				mulAndStore ((*v2base), (*v1base));
				v1base++;
				v2base++;
			}
	}
};