/////////////////////////////////////////////////////////////////////////////////
//
//  HDRImageFunctor: definuje rozhranie funktorov, ktore modifikuju HDRimage-s
//  HDRImageBase: zakladna trieda spolocna pre vsetky HDRImage-s
//    poskytuje zakladnu funkcionalitu
//  HDRImage1f: HDR obrazok s jednym floatovym kanalom
//  HDRImage1d: HDR obrazok s jednym doublovym kanalom
//  HDRImage3f: HDR obrazok s tromi floatovymi kanalmi
//  HDRImage3d: HDR obrazok s tromu doublovymi kanalmi
//  HDRImage3c: HDR obrazok s tromi kanalmi, pricom prvky obrazka su complexne cisla
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "type.h"
#include "func.h"


struct SDL_Surface;
class HDRImage1f;
class HDRImage3f;



class HDRImageFunctor
{
public:
	virtual float getRed (uint x, uint y, float red)
	{
		return getValue (x, y, red);
	}

	virtual float getBlue (uint x, uint y, float blue)
	{
		return getValue (x, y, blue);
	}

	virtual float getGreen (uint x, uint y, float green)
	{
		return getValue (x, y, green);
	}

	virtual float getValue (uint x, uint y, float value)
	{
		return value;
	}
};




class HDRImageBase
{
protected:
	uint2 m_availableSize;
	uint2 m_appendSize;
	uint2 m_totalSize;
protected:
	HDRImageBase (uint2 available, uint2 append = uint2 (0, 0))
	{
		m_availableSize = available;
		m_appendSize = append;
		m_totalSize = available + append;
	}
public:
	void changeAvailableSize (uint2 available)
	{
		int2 avail = available.get<int>();
		int2 append = m_totalSize.get<int>() - avail;
		assert (append.x >= 0 && append.y >= 0);
		m_availableSize = avail.get<uint> ();
		m_appendSize = append.get<uint> ();
	}
	uint2 getSize ()
	{
		return m_availableSize;
	}
	uint2 getTotalSize ()
	{
		return m_totalSize;
	}
	uint2 getAppendSize ()
	{
		return m_appendSize;
	}
	virtual void zeroesBlock (uint2 begin, uint2 end) = 0;
	void clearAppendArea ()
	{
		if (getAppendSize().x)
			zeroesBlock (uint2(getSize().x, 0), uint2 (getSize().x+m_appendSize.x-1, getSize().y-1));
			
		if (getAppendSize().y)
			zeroesBlock (uint2(0, getSize().y), uint2 (getSize().x+m_appendSize.x-1, getSize().y+m_appendSize.y-1));
	}
	bool isInside (int2 pos)
	{
		return ! (pos.x < 0 || pos.y < 0 || (uint)pos.x >= m_availableSize.x || (uint)pos.y >= m_availableSize.y);	
	}
protected:
	uint getOffset (uint2 pos);
	uint getOffset (uint x, uint y)
	{
		return getOffset (uint2 (x,y));
	}
	uint getOffsetUseTotal (uint2 pos);
	uint getOffsetUseTotal (uint x, uint y)
	{
		return getOffsetUseTotal (uint2 (x,y));
	}
	void setSize (uint2 available, uint2 append)
	{
		m_availableSize = available;
		m_appendSize = append;
		m_totalSize = available + append;
	}

};










class HDRImage1f: public HDRImageBase
{
	float* m_alpha;

	bool m_fftplanNeedUpdate;
	fftwf_plan m_fftplan;

	bool m_rfftplanNeedUpdate;
	fftwf_plan m_rfftplan;

	boost::shared_ptr<HDRImage1f> m_hdriFFT;

	float m_mult;
public:
	HDRImage1f ():
		HDRImageBase (uint2 (0,0), uint2 (0,0))
	{
		m_alpha = NULL;
		m_fftplanNeedUpdate = true;
		m_rfftplanNeedUpdate = true;
		m_mult = 1.0f;
	}
	HDRImage1f (uint2 size, uint2 append = uint2 (0,0)):
		HDRImageBase (uint2 (0,0), uint2 (0,0))
	{
		m_alpha = NULL;
		setSize (size,append);
		m_mult = 1.0f;
	}
	~HDRImage1f ()
	{
		clear ();
	}
	float getMult ()
	{
		return m_mult;
	}
	void setMult (float mult)
	{
		m_mult = mult;
	}
	float computeSum ()
	{
		float sum = 0;
		uint2 size = getSize();
		uint i = 0;
		uint offset = m_appendSize.x;
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				sum += m_alpha[i];
				i++;
			}
			i += offset;
		}
		return sum;
	}
	float computeAvg ()
	{
		return computeSum()/getSize().getArea();
	}
	float getPixel (uint2 pos)
	{
		return m_alpha[getOffsetUseTotal(pos.x, pos.y)];
	}
	float getPixel (uint posX, uint posY)
	{
		return getPixel (uint2 (posX, posY));
	}
	void setPixel (uint2 pos, float val)
	{
		m_alpha[getOffsetUseTotal(pos.x, pos.y)] = val;
	}
	void zeroesBlock (uint2 begin, uint2 end)
	{
		uint2 mn = uint2::getMin (begin, end);
		uint2 mx = uint2::getMax (begin, end);
		uint2 size = mx - mn + 1;
		for (uint y = mn.y; y <= mx.y; y++)
		{
			uint off = getOffsetUseTotal (mn.x, y);
			memset (m_alpha+off, 0, sizeof (float)*size.x);
		}
	}
private:
	void fftplanUpdate ()
	{
		m_fftplan = fftwf_plan_dft_r2c_2d (getTotalSize().y, getTotalSize().x, 
				m_alpha, (fftwf_complex*)m_hdriFFT->getBuffer(), FFTW_MEASURE);
	}
public:
	// return shared HDRImage1f
	boost::shared_ptr<HDRImage1f> computeFFT ()
	{
		if (! m_hdriFFT.get())
			m_hdriFFT = boost::shared_ptr<HDRImage1f> (new HDRImage1f);
		uint2 size = (getSize()+uint2(0,2));
		uint2 append = getAppendSize();
		if (size != m_hdriFFT->getSize() || append != m_hdriFFT->getAppendSize())
			m_hdriFFT->setSize (size, append);
		if (m_fftplanNeedUpdate)
		{
			fftplanUpdate ();			
			m_fftplanNeedUpdate = false;
		}
		m_hdriFFT->setMult (m_mult);
		fftwf_execute (m_fftplan);
		return m_hdriFFT;
	}

	void setSize (uint2 size, uint2 append = uint2 (0,0))
	{
		if (size == getSize() && append == getAppendSize())
			return;
		HDRImageBase::setSize (size, append);
		if (m_alpha)
			fftwf_free (m_alpha);

		m_fftplanNeedUpdate = true;
		m_rfftplanNeedUpdate = true;

		m_alpha = fftwf_alloc_real (getTotalSize().getArea());
	}
	void clear ()
	{
		HDRImageBase::setSize (uint2 (0,0), uint2 (0,0));
		if (m_alpha)
		{
			fftwf_free (m_alpha);
			m_alpha = NULL;
		}
	}
	float* getBuffer ()
	{
		return m_alpha;
	}
	float* getBuffer (uint2 pos)
	{
		return m_alpha + getOffset (pos);
	}
	boost::shared_ptr<HDRImage3f> getHDRImage3f ();

	template <typename HDRIFunctor_t>
	void applyFunctor (HDRIFunctor_t& functor)
	{
		uint2 size = getSize ();
		
		for (uint y = 0; y < size.y; y++)
		{
			float* p = getBuffer (uint2(0, y));
			for (uint x = 0; x < size.x; x++)
			{
				*p = functor.getValue(x, y, *p);
				p++;
			}
		}
	}
	SDL_Surface* getSDLSurface (float multCoef, SDL_Surface* surface);
};





class HDRImage3c: public HDRImageBase
{
	fftwf_complex* m_red;
	fftwf_complex* m_green;
	fftwf_complex* m_blue;

	boost::shared_ptr<HDRImage3f> m_hdriRFFT;
	fftwf_plan m_rfftplanR, 
		m_rfftplanG, 
		m_rfftplanB;

	bool m_rfftplanNeedUpdate;

	float m_mult;

public:
	HDRImage3c ():
		HDRImageBase (uint2 (0, 0))
	{
		m_red = NULL;
		m_green = NULL;
		m_blue = NULL;
		m_rfftplanNeedUpdate = true;

		m_mult = 1.0f;
	}
	HDRImage3c (uint2 size, uint2 append = uint2 (0,0)):
		HDRImageBase (size, append)
	{
		uint arrSize = m_totalSize.getArea();
		m_red = ::fftwf_alloc_complex (arrSize/2+1);
		m_green = ::fftwf_alloc_complex (arrSize/2+1);
		m_blue = ::fftwf_alloc_complex (arrSize/2+1);
		m_rfftplanNeedUpdate = true;

		m_mult = 1.0f;
	}
	float getMult ()
	{
		return m_mult;
	}
	void setMult (float mult)
	{
		m_mult = mult;
	}
	void clear ()
	{
		if (m_red)
		{
			::fftwf_free (m_red);
			m_red = NULL;
		}
		if (m_green)
		{
			::fftwf_free (m_green);
			m_green = NULL;
		}
		if (m_blue)
		{
			::fftwf_free (m_blue);
			m_blue = NULL;
		}
	}
	~HDRImage3c ()
	{
		clear ();
	}

	void setSize (uint2 size, uint2 append = uint2 (0,0))
	{
		if (size == getSize() && append == getAppendSize())
			return;
		HDRImageBase::setSize (size, append);
		clear ();
		uint arrSize = m_totalSize.getArea();
		m_red = ::fftwf_alloc_complex (arrSize);
		m_green = ::fftwf_alloc_complex (arrSize);
		m_blue = ::fftwf_alloc_complex (arrSize);
		m_rfftplanNeedUpdate = true;
	}

	void getPixel (uint2 pos, fftwf_complex* c3)
	{
		uint off = getOffsetUseTotal(pos.x, pos.y);
		c3[0][0] = m_red[off][0];
		c3[1][0] = m_green[off][0];
		c3[2][0] = m_blue[off][0];
		c3[0][1] = m_red[off][1];
		c3[1][1] = m_green[off][1];
		c3[2][1] = m_blue[off][1];
	}
	void setPixel (uint2 pos, fftwf_complex* c3)
	{
		uint off = getOffsetUseTotal(pos.x, pos.y);
		m_red[off][0] = c3[0][0];
		m_green[off][0] = c3[1][0];
		m_blue[off][0] = c3[2][0];
		m_red[off][1] = c3[0][1];
		m_green[off][1] = c3[1][1];
		m_blue[off][1] = c3[2][1];
	}
	void zeroesBlock (uint2 begin, uint2 end)
	{
		uint2 mn = uint2::getMin (begin, end);
		uint2 mx = uint2::getMax (begin, end);
		uint2 size = mx - mn + 1;
		for (uint y = mn.y; y <= mx.y; y++)
		{
			uint off = getOffsetUseTotal (mn.x, y);
			memset (m_red+off, 0, sizeof (fftwf_complex)*size.x);
			memset (m_green+off, 0, sizeof (fftwf_complex)*size.x);
			memset (m_blue+off, 0, sizeof (fftwf_complex)*size.x);
		}
	}
	fftwf_complex* getRedBuffer ()
	{
		return m_red;
	}
	fftwf_complex* getBlueBuffer ()
	{
		return m_blue;
	}
	fftwf_complex* getGreenBuffer ()
	{
		return m_green;
	}

private:
	void rfftplanUpdate ();
public:
	// return shared HDRImage3f
	boost::shared_ptr<HDRImage3f> computeRFFT ();
};

class HDRImage3d: public HDRImageBase
{
	double* m_red, *m_green, *m_blue;
public:
	HDRImage3d ():
		HDRImageBase (uint2 (0,0))
	{
		m_red = NULL;
		m_green = NULL;
		m_blue = NULL;
	}
	HDRImage3d (uint2 size, uint2 append = uint2(0,0)):
		HDRImageBase (uint2(0,0), uint2(0,0))
	{
		m_red = NULL;
		m_green = NULL;
		m_blue = NULL;
		setSize (size, append);
	}
	~HDRImage3d ()
	{
		clear ();
	}
	void clear ()
	{
		HDRImageBase::setSize (uint2(0,0), uint2(0,0));
		if (m_red)
		{
			free (m_red);
			m_red = NULL;
		}
		if (m_green)
		{
			free (m_green);
			m_green = NULL;
		}
		if (m_blue)
		{
			free (m_blue);
			m_blue;
		}
	}
	double3 getPixel (uint2 pos)
	{
		double3 d3;
		uint off = getOffsetUseTotal(pos.x, pos.y);
		d3.x = m_red[off];
		d3.y = m_green[off];
		d3.z = m_blue[off];
		return d3;
	}
	void setPixel (uint2 pos, double3 d3)
	{
		uint off = getOffsetUseTotal(pos.x, pos.y);
		m_red[off] = d3.x;
		m_green[off] = d3.y;
		m_blue[off] = d3.z;
	}
	void zeroesBlock (uint2 begin, uint2 end)
	{
		uint2 mn = uint2::getMin (begin, end);
		uint2 mx = uint2::getMax (begin, end);
		uint2 size = mx - mn + 1;
		for (uint y = mn.y; y <= mx.y; y++)
		{
			uint off = getOffsetUseTotal (mn.x, y);
			memset (m_red+off, 0, sizeof (double)*size.x);
			memset (m_green+off, 0, sizeof (double)*size.x);
			memset (m_blue+off, 0, sizeof (double)*size.x);
		}
	}
	void setSize (uint2 available, uint2 append = uint2 (0,0))
	{
		if (getSize() == available && getAppendSize() == append)
			return;

		clear ();

		HDRImageBase::setSize (available, append);
		uint arrSize = m_totalSize.getArea();
		m_red = (double*) malloc (sizeof(double) * arrSize);
		m_green = (double*) malloc (sizeof(double) * arrSize);
		m_blue = (double*) malloc (sizeof(double) * arrSize);
	}
};

class HDRImage1d: public HDRImageBase
{
	double* m_alpha;
public:
	HDRImage1d ():
		HDRImageBase (uint2 (0,0))
	{
		m_alpha = NULL;
	}
	HDRImage1d (uint2 size, uint2 append = uint2(0,0)):
		HDRImageBase (uint2(0,0), uint2(0,0))
	{
		m_alpha = NULL;
		setSize (size, append);
	}
	~HDRImage1d ()
	{
		clear ();
	}
	void clear ()
	{
		HDRImageBase::setSize (uint2(0,0), uint2(0,0));
		if (m_alpha)
		{
			free (m_alpha);
			m_alpha = NULL;
		}
	}
	double getPixel (uint2 pos)
	{
		double d;
		uint off = getOffsetUseTotal(pos.x, pos.y);
		d = m_alpha[off];
		return d;
	}
	double* getBuffer ()
	{
		return m_alpha;
	}
	void setPixel (uint2 pos, double d)
	{
		uint off = getOffsetUseTotal(pos.x, pos.y);
		m_alpha[off] = d;
	}
	void zeroesBlock (uint2 begin, uint2 end)
	{
		uint2 mn = uint2::getMin (begin, end);
		uint2 mx = uint2::getMax (begin, end);
		uint2 size = mx - mn + 1;
		for (uint y = mn.y; y <= mx.y; y++)
		{
			uint off = getOffsetUseTotal (mn.x, y);
			memset (m_alpha+off, 0, sizeof (double)*size.x);
		}
	}
	void setSize (uint2 available, uint2 append = uint2 (0,0))
	{
		if (getSize() == available && getAppendSize() == append)
			return;

		clear ();

		HDRImageBase::setSize (available, append);
		uint arrSize = m_totalSize.getArea();
		m_alpha = (double*) malloc (sizeof(double) * arrSize);
	}
};




class HDRImage3f: public HDRImageBase
{
	float* m_red;
	float* m_green;
	float* m_blue;

	bool m_fftplanNeedUpdate;
	fftwf_plan m_fftplanR, 
		m_fftplanG, 
		m_fftplanB;

	float m_mult;

public:
	boost::shared_ptr<HDRImage3c> m_hdriFFT;

public:
	HDRImage3f ():
		HDRImageBase (uint2 (0,0))
	{
		m_red = NULL;
		m_green = NULL;
		m_blue = NULL;
		m_fftplanNeedUpdate = true;
		m_mult = 1.0f;
	}
	HDRImage3f (uint2 size, uint2 append = uint2(0,0)):
		HDRImageBase (uint2(0,0), uint2(0,0))
	{
		m_red = NULL;
		m_green = NULL;
		m_blue = NULL;
		m_fftplanNeedUpdate = true;
		setSize (size, append);

		m_mult = 1.0f;
	}
	~HDRImage3f ()
	{
		clear ();
	}

	void setMult (float mult)
	{
		m_mult = mult;
	}

	float getMult ()
	{
		return m_mult;
	}

	bool loadFromFile (cchar* pathname, uint2 appendMult = uint2 (1,1),
				uint2 appendAdd = uint2 (0,0));

	bool loadFromHDRFile (cchar* pathname, uint2 appendMult = uint2 (1,1), 
				uint2 appendAdd = uint2 (0,0));
	
	void clear ()
	{
		HDRImageBase::setSize (uint2(0,0), uint2(0,0));
		if (m_red)
		{
			fftwf_free (m_red);
			m_red = NULL;
		}
		if (m_green)
		{
			fftwf_free (m_green);
			m_green = NULL;
		}
		if (m_blue)
		{
			fftwf_free (m_blue);
			m_blue;
		}
	}

	void clamp (float3 minValue, float3 maxValue)
	{
		float* pred = getRedBuffer ();
		float* pgreen = getGreenBuffer ();
		float* pblue = getBlueBuffer ();
		uint2 size = getSize();
		{
			float minf = minValue.x / getMult ();
			float maxf = maxValue.x / getMult ();
			for (uint y = 0; y < size.y; y++)
			{
				for (uint x = 0; x < size.x; x++)
				{
					*pred = std::min (maxf, std::max (minf, *pred));
					pred++;
				}
				pred += m_appendSize.x;
			}
		}
		{
			float minf = minValue.y / getMult ();
			float maxf = maxValue.y / getMult ();
			for (uint y = 0; y < size.y; y++)
			{
				for (uint x = 0; x < size.x; x++)
				{
					*pgreen = std::min (maxf, std::max (minf, *pgreen));
					pgreen++;
				}
				pgreen += m_appendSize.x;
			}
		}
		{
			float minf = minValue.z / getMult ();
			float maxf = maxValue.z / getMult ();
			for (uint y = 0; y < size.y; y++)
			{
				for (uint x = 0; x < size.x; x++)
				{
					*pblue = std::min (maxf, std::max (minf, *pblue));
					pblue++;
				}
				pblue += m_appendSize.x;
			}
		}
	}
	template <typename HDRIFunctor_t>
	void applyFunctor (HDRIFunctor_t& functor)
	{
		uint2 size = getSize ();
		
		for (uint y = 0; y < size.y; y++)
		{
			float* pred = getRedBuffer (uint2(0, y));
			for (uint x = 0; x < size.x; x++)
			{
				*pred = functor.getRed (x, y, *pred);
				pred++;
			}
		}
		for (uint y = 0; y < size.y; y++)
		{
			float* pgreen = getGreenBuffer (uint2(0, y));
			for (uint x = 0; x < size.x; x++)
			{
				*pgreen = functor.getGreen (x, y, *pgreen);
				pgreen++;
			}
		}
		for (uint y = 0; y < size.y; y++)
		{
			float* pblue = getBlueBuffer (uint2(0, y));
			for (uint x = 0; x < size.x; x++)
			{
				*pblue = functor.getBlue (x, y, *pblue);
				pblue++;
			}
		}
	}
	float3 computeSum ()
	{
		float red = 0;
		float green = 0;
		float blue = 0;
		float* pred = getRedBuffer ();
		float* pgreen = getGreenBuffer ();
		float* pblue = getBlueBuffer ();
		uint2 size = getSize();
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				red += *pred;
				pred++;
			}
			pred += m_appendSize.x;
		}
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				green += *pgreen;
				pgreen++;
			}
			pgreen += m_appendSize.x;
		}
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				blue += *pblue;
				pblue++;
			}
			pblue += m_appendSize.x;
		}
		return float3 (red, green, blue);
	}
	void getMinMax (float3& fmin, float3& fmax)
	{
		fmin = float3 (FLT_MAX);
		fmax = float3 (FLT_MIN);
		float& fminR = fmin.x;
		float& fminG = fmin.y;
		float& fminB = fmin.z;
		float& fmaxR = fmax.x;
		float& fmaxG = fmax.y;
		float& fmaxB = fmax.z;

		float* pred = getRedBuffer ();
		float* pgreen = getGreenBuffer ();
		float* pblue = getBlueBuffer ();
		uint2 size = getSize();
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				fminR = std::min (fminR, *pred);
				fmaxR = std::max (fmaxR, *pred);
				pred++;
			}
			pred += m_appendSize.x;
		}
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				fminG = std::min (fminG, *pgreen);
				fmaxG = std::max (fmaxG, *pgreen);
				pgreen++;
			}
			pgreen += m_appendSize.x;
		}
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				fminB = std::min (fminB, *pblue);
				fmaxB = std::max (fmaxB, *pblue);
				pblue++;
			}
			pblue += m_appendSize.x;
		}
		fmin *= m_mult;
		fmax *= m_mult;
	}
	float3 computeAvg ()
	{
		return computeSum()/(float)getSize().getArea();
	}

public:
	SDL_Surface* getSDLSurface (float multCoef, SDL_Surface* surface, uint2 maxSurfaceSize);

	void getHDRImage1f (HDRImage1f* out, FuncF3toF* func, uint2 append = uint2 (0,0));

	void getSubImage (boost::shared_ptr<HDRImage3f>& out, uint2 size, float2 reloffsetMin, float2 reloffsetMax);

	float3 getPixel (uint2 pos)
	{
		float3 d3;
		uint off = getOffsetUseTotal(pos.x, pos.y);
		d3.x = m_red[off];
		d3.y = m_green[off];
		d3.z = m_blue[off];
		return d3;
	}
	void setPixel (uint2 pos, float3 d3)
	{
		uint off = getOffsetUseTotal(pos.x, pos.y);
		m_red[off] = d3.x;
		m_green[off] = d3.y;
		m_blue[off] = d3.z;
	}
	void zeroesBlock (uint2 begin, uint2 end)
	{
		uint2 mn = uint2::getMin (begin, end);
		uint2 mx = uint2::getMax (begin, end);
		uint2 size = mx - mn + 1;
		for (uint y = mn.y; y <= mx.y; y++)
		{
			uint off = getOffsetUseTotal (mn.x, y);
			memset (m_red+off, 0, sizeof (float)*size.x);
			memset (m_green+off, 0, sizeof (float)*size.x);
			memset (m_blue+off, 0, sizeof (float)*size.x);
		}
	}
	void setSize (uint2 available, uint2 append = uint2 (0,0))
	{
		if (getSize() == available && getAppendSize() == append)
			return;

		clear ();

		HDRImageBase::setSize (available, append);
		uint arrSize = m_totalSize.getArea();
		m_red = ::fftwf_alloc_real (arrSize);
		m_green = ::fftwf_alloc_real (arrSize);
		m_blue = ::fftwf_alloc_real (arrSize);
		m_fftplanNeedUpdate = true;
	}
private:
	void fftplanUpdate ()
	{
		m_fftplanR = fftwf_plan_dft_r2c_2d (getTotalSize().y, getTotalSize().x, 
			 m_red, m_hdriFFT->getRedBuffer(), FFTW_MEASURE);
		m_fftplanG = fftwf_plan_dft_r2c_2d (getTotalSize().y, getTotalSize().x, 
			 m_green, m_hdriFFT->getGreenBuffer(), FFTW_MEASURE);
		m_fftplanB = fftwf_plan_dft_r2c_2d (getTotalSize().y, getTotalSize().x, 
			 m_blue, m_hdriFFT->getBlueBuffer(), FFTW_MEASURE);
	}
public:
	// return shared HDRImage3f
	boost::shared_ptr<HDRImage3c> computeFFT ()
	{
		if (! m_hdriFFT.get())
			m_hdriFFT = boost::shared_ptr<HDRImage3c> (new HDRImage3c);
		uint2 size = (getSize()/uint2(1,2)+uint2(0,1));
		uint2 append = getTotalSize()/uint2(1,2)+uint2(0,1)-size;
		if ((size != m_hdriFFT->getSize()) || (append != (m_hdriFFT->getAppendSize())))
			m_hdriFFT->setSize (size, append);
		if (m_fftplanNeedUpdate)
		{
			fftplanUpdate ();			
			m_fftplanNeedUpdate = false;
		}
		m_hdriFFT->setMult (m_mult);
		fftwf_execute (m_fftplanR);
		fftwf_execute (m_fftplanG);
		fftwf_execute (m_fftplanB);
		return m_hdriFFT;
	}
	float* getRedBuffer ()
	{
		return m_red;
	}
	float* getBlueBuffer ()
	{
		return m_blue;
	}
	float* getGreenBuffer ()
	{
		return m_green;
	}
	float* getRedBuffer (uint2 pos)
	{
		return m_red + getOffset (pos);
	}
	float* getBlueBuffer (uint2 pos)
	{
		return m_blue + getOffset (pos);
	}
	float* getGreenBuffer (uint2 pos)
	{
		return m_green + getOffset (pos);
	}
	float* getBuffer (uint channel)
	{
		return getBuffer (uint2(0,0), channel);
	}
	float* getBuffer (uint2 pos, uint channel)
	{
		switch (channel)
		{
		case 0:
			return getRedBuffer (pos);
		case 1:
			return getGreenBuffer (pos);
		case 2:
			return getBlueBuffer (pos);
		default:
			assert (0);
		}
		return NULL;
	}
};


typedef boost::shared_ptr<HDRImage1f> PtrHDRImage1f;
typedef boost::shared_ptr<HDRImage3f> PtrHDRImage3f;
typedef boost::shared_ptr<HDRImage3c> PtrHDRImage3c;









// nie je kopirovana hodnota getMult
void copyHDRImage (HDRImage1f* hdriDst, HDRImage1f* hdriSrc,
		uint2 dstOrg, uint2 srcOrg, uint2 size);

void copyHDRImage (HDRImage3f* hdriDst, HDRImage3f* hdriSrc,
		uint2 dstOrg, uint2 srcOrg, uint2 size);

void copyHDRImage (HDRImage3c* hdriDst, HDRImage3c* hdriSrc);

void copyHDRImage (HDRImage3c* hdriDst, HDRImage1f* hdriSrc);

void addHDRImage (HDRImage3f* hdriDst, HDRImage1f* hdriSrc);


void createSummedAreaTable (HDRImage3f* in, HDRImage3d* out);
void createSummedAreaTable (HDRImage1f* in, HDRImage1d* out);
void createSummedAreaTable (float* in, double* out, uint2 size, uint2 appendIn, uint2 appendOut);