/////////////////////////////////////////////////////////////////////////////////
//
//  HDRIFunctor3fMult: funktor nasobi pixely na zaklade hodnot vracajucich z funkcie
//  HDRIFunctor3fCopy: funktor kopiruje hodnoty z jednej mapy do druhej
//  HDRIFunctorMultByHDRI3f: funktor nasobi hodnoty mapy s hodnotami inej mapy
//  HDRIFunctor3fAddHDRI: funktor pricitava hodnoty mapy inej mape
//  HDRIFunctor3fCopyFrom1f: funktor nastavuje kanal jednokanalovej mapy do trojkanalovej mapy
//  HDRIFunctor3fConvolution: funktor umoznuje konvoluciu kernelom 3x3
//  HDRIFunctorMult: funktor nasobi vsetky hodnoty mapy zadanou hodnotou
//  HDRIFunctorMultByHDRI1f: funktor nasobi mapu jednokanalovou mapou
//  HDRIFunctorNoise: funktor pridava sum do obrazku
//  HDRIFunctorDepth: funktor modifikuje hlbku hlbkovej mapy
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once


#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>


#include "type.h"
#include "func.h"
#include "hdrimage.h"
#include "xml.h"

class HDRIFunctor3fMult: public HDRImageFunctor
{
	float2 m_sizeMult;
	PtrFuncF2toF m_func;
public:
	HDRIFunctor3fMult (uint2 sizeOfImage, PtrFuncF2toF func)
	{
		m_sizeMult = float2(1,1) / ((sizeOfImage - uint2 (1,1)).get<float>());
		m_func = func;
	}
	float getValue (uint x, uint y, float value)
	{
		float2 fpos (x*m_sizeMult.x, y*m_sizeMult.y);
		float mult = (*m_func) (fpos);
		return value * mult;
	}
};


class HDRIFunctor3fCopy: public HDRImageFunctor
{
	PtrHDRImage3f m_src;
public:
	HDRIFunctor3fCopy (PtrHDRImage3f src)
	{
		m_src = src;
	}
	float getRed (uint x, uint y, float red)
	{
		return *(m_src->getRedBuffer (uint2(x,y)));
	}

	float getGreen (uint x, uint y, float green)
	{
		return *(m_src->getGreenBuffer (uint2(x,y)));
	}

	float getBlue (uint x, uint y, float blue)
	{
		return *(m_src->getBlueBuffer (uint2(x,y)));
	}
};


class HDRIFunctorMultByHDRI3f: public HDRImageFunctor
{
	PtrHDRImage3f m_mult;
	float m_multCoef;
public:
	HDRIFunctorMultByHDRI3f (float targetMult, PtrHDRImage3f hdriMult)
	{
		m_mult = hdriMult;
		m_multCoef = m_mult->getMult() / targetMult;
	}
	float getRed (uint x, uint y, float red)
	{
		float mult = *(m_mult->getRedBuffer (uint2(x,y)));
		return red * mult * m_multCoef;
	}

	float getGreen (uint x, uint y, float green)
	{
		float mult = *(m_mult->getGreenBuffer (uint2(x,y)));
		return green * mult * m_multCoef;
	}

	float getBlue (uint x, uint y, float blue)
	{
		float mult = *(m_mult->getBlueBuffer (uint2(x,y)));
		return blue * mult * m_multCoef;
	}
};

class HDRIFunctor3fAddHDRI: public HDRImageFunctor
{
	PtrHDRImage3f m_add;
	float m_mult;
public:
	HDRIFunctor3fAddHDRI (float targetMult, PtrHDRImage3f hdriAdd)
	{
		m_add = hdriAdd;
		m_mult = hdriAdd->getMult() / targetMult;
	}
	float getRed (uint x, uint y, float red)
	{
		float add = *(m_add->getRedBuffer (uint2(x,y)));
		return red + add * m_mult;
	}

	float getGreen (uint x, uint y, float green)
	{
		float add = *(m_add->getGreenBuffer (uint2(x,y)));
		return green + add * m_mult;
	}

	float getBlue (uint x, uint y, float blue)
	{
		float add = *(m_add->getBlueBuffer (uint2(x,y)));
		return blue + add * m_mult;
	}
};


class HDRIFunctor3fCopyFrom1f: public HDRImageFunctor
{
	HDRImage1f* m_src;
public:
	HDRIFunctor3fCopyFrom1f (HDRImage1f* src)
	{
		m_src = src;
	}
	float getRed (uint x, uint y, float red)
	{
		return *(m_src->getBuffer (uint2(x,y)));
	}

	float getGreen (uint x, uint y, float green)
	{
		return *(m_src->getBuffer (uint2(x,y)));
	}

	float getBlue (uint x, uint y, float blue)
	{
		return *(m_src->getBuffer (uint2(x,y)));
	}
};

class HDRIFunctorApplyFunc: public HDRImageFunctor
{
	PtrFuncFtoF m_func;
public:
	HDRIFunctorApplyFunc (PtrFuncFtoF func)
	{
		m_func = func;
	}
	inline float getValue (uint x, uint y, float input)
	{
		return (*m_func)(input);
	}
};


class HDRIFunctor3fConvolution: public HDRImageFunctor
{
	float m_kernel[3][3][3];
	PtrHDRImage3f m_input;
	uint2 m_size;
public:
	HDRIFunctor3fConvolution (HDRImage3f* kernel, PtrHDRImage3f input)
	{
		m_input = input;
		m_size = input->getSize();
		assert (kernel->getSize() <= uint2(3,3));
		memset (m_kernel, 0, sizeof (m_kernel));
		uint2 size = kernel->getSize();
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
				m_kernel[0][y][x] = *(kernel->getRedBuffer(uint2(x,y)));
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
				m_kernel[1][y][x] = *(kernel->getGreenBuffer(uint2(x,y)));
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
				m_kernel[2][y][x] = *(kernel->getBlueBuffer(uint2(x,y)));
	}
	float getAcc (uint index, uint inx, uint iny)
	{
		float acc = 0;
		float* p;
		switch (index)
		{
		case 0:
			p = m_input->getRedBuffer (uint2(inx, iny));
			break;
		case 1:
			p = m_input->getGreenBuffer (uint2(inx, iny));
			break;
		case 2:
			p = m_input->getBlueBuffer (uint2(inx, iny));
			break;
		}
		int width = m_input->getTotalSize().x;
		for (int i = -1; i < 2; i++)
		{
			for (int j = -1; j < 2; j++)
			{
				int x = inx + j;
				int y = iny + i;
				int ix (j), iy (i);
				if (j == -1)
					if (x < 0)
						ix = 0;
				if (j == 1)
					if (x >= (int) m_size.x)
						ix = m_size.x-1;
				if (i == -1)
					if (y < 0)
						iy = 0;
				if (i == 1)
					if (y >= (int) m_size.y)
						iy = m_size.y-1;
				acc += m_kernel[index][i+1][j+1] * p[ix+iy*width];
			}
		}
		return acc;
	}
	float getRed (uint x, uint y, float red)
	{
		//return m_input->getPixel(uint2(x,y)).x;
		return getAcc (0, x, y);
	}

	float getGreen (uint x, uint y, float green)
	{
		//return m_input->getPixel(uint2(x,y)).y;
		return getAcc (1, x, y);
	}

	float getBlue (uint x, uint y, float blue)
	{
		//return m_input->getPixel(uint2(x,y)).z;
		return getAcc (2, x, y);
	}
};


class HDRIFunctorMult: public HDRImageFunctor 
{
	float m_m;
public:
	HDRIFunctorMult (float m)
	{
		m_m = m;
	}

	float getValue (uint x, uint y, float v)
	{
		return v * m_m;
	}
};


class HDRIFunctorMultByHDRI1f: public HDRImageFunctor
{
	PtrHDRImage1f m_mask;
public:
	HDRIFunctorMultByHDRI1f (PtrHDRImage1f mask)
	{
		m_mask = mask;
	}
	float getValue (uint x, uint y, float value)
	{
		float mask = m_mask->getPixel (x, y);
		return value * mask;
	}
};

class HDRIFunctorNoise: public HDRImageFunctor
{
	static boost::mt19937 m_rng;

private:
	PtrFuncFtoF m_func;
	float m_fmin, m_fmax;
public:
	HDRIFunctorNoise (PtrFuncFtoF func, float fmin, float fmax)
	{
		m_func = func;
		m_fmin = fmin;
		m_fmax = fmax;
	}
	inline float getValue (uint x, uint y, float input)
	{
		float value = (*m_func)(input); 
		boost::normal_distribution <float> nd (0.0f, value);
		boost::variate_generator<boost::mt19937&,  
				boost::normal_distribution<float>> var_nor (m_rng, nd);
		float rnd;
		do {
			rnd = var_nor ();
			if (rnd >= m_fmin && rnd <= m_fmax && input + rnd >= 0)
				break;
		} while (true);
/*		static float smax = 0;
		if (smax < rnd)
			smax = rnd;
	*/	
		return input + rnd;
	}
};


class HDRIFunctorDepth: public HDRImageFunctor
{
	bool m_linearize;
	float m_linN, m_linF;
	float m_add, m_mult;
	float m_icosmult, m_icosanglemult;
	uint2 m_size;
public:
	HDRIFunctorDepth (XMLinput* input, uint2 size)
	{
		m_linearize = input->depthLinearize;
		m_linN = input->depthN;
		m_linF = input->depthF;
		m_add = input->depthAdd;
		m_mult = input->depthMult;
		m_icosmult = input->icosmult;
		m_icosanglemult = input->icosanglemult;
		m_size = size;
	}

	float linearizeDepth(float z)
	{
		float lin = (2.0f * m_linN) / (m_linF + m_linN - z * (m_linF - m_linN));

		return m_linN+lin*(m_linF-m_linN);
	}
	float getValue (uint x, uint y, float depth)
	{
		if (m_linearize)
			depth = linearizeDepth (depth);
		depth = m_add + m_mult * depth;

		float2 size = m_size.get<float>();
		float2 pos = int2(x,y).get<float>();
		float2 inscreen = pos - size/2 + 0.5f;
		float2 halfsize = size/2;
		float2 aspect = halfsize/(halfsize.getLength());
		float rel = inscreen.getLength()/halfsize.getLength();
		float angle = rel * 3.14f / 2;
		return m_icosmult*depth*(1/cosf(angle*m_icosanglemult));
	}
};