/////////////////////////////////////////////////////////////////////////////////
//
//  Func1Arg: rozhranie sablony funktora s jednym vstupnym parametrom
//  Func2Arg: rozhranie sablony funktora s dvoma vstupnymi parametrom
//  FuncFtoF: rozhranie sablony funktora s jednym realnym parametrom a jednym vystupnou realnou hodnotou
//  FuncFtoFconst: konstantna funkcia
//  FuncFtoFinv: funkcia vracajuca 1/x
//  FuncF3toFsumChannel: funkcia vracajuca sumu hodnot float3
//  FuncF2toFconvFromFtoF: funkcia prevadza F2toF funkciu na FtoF funkciu (pomocou predpisu sqrt(x*x+y*y) )
//  FuncStar: funkcia definuje hviezdu v obraze
//  FuncGlow1: funkcia definuje "linearnu" ziaru 
//  FuncGlow2: funkcia definuje "exponencionalnu" ziaru
//  FuncFuzinessConverter: funkcia konvertuje pocet riadkov viditelnych pri rozostreni na mieru rozostrenia
//  FuncFocusedToDist: definuje rozostrenie
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <boost/shared_ptr.hpp>
#include "type.h"


template <typename Tin, typename Tout>
class Func1Arg
{
public:
	typedef Tin in_t;
	typedef Tout out_t;
	virtual Tout operator () (Tin in) = 0;
};


template <typename Tin1, typename Tin2, typename Tout>
class Func2Arg
{
public:
	typedef Tin1 in1_t;
	typedef Tin2 in2_t;
	typedef Tout out_t;
	virtual Tout operator () (Tin1 in1, Tin2 in2) = 0;
};









#define DEF_FUNC1(arg,ret,suffix)\
	typedef Func1Arg<arg,ret> Func##suffix;\
	typedef boost::shared_ptr<Func##suffix> PtrFunc##suffix


#define DEF_FUNC2(arg1,arg2,ret,suffix)\
	typedef Func2Arg<arg1,arg2,ret> Func##suffix;\
	typedef boost::shared_ptr<Func##suffix> PtrFunc##suffix







DEF_FUNC1(float,float,FtoF);
DEF_FUNC1(float,float3,FtoF3);
DEF_FUNC1(float3,float,F3toF);
DEF_FUNC1(float2,float,F2toF);
DEF_FUNC1(float3,float,F3toF);
DEF_FUNC2(float3,float,float3,F3FtoF3);
DEF_FUNC2(int2,int2,float,I2I2toF);





typedef FuncFtoF FuncRadiusOfCoC;
typedef PtrFuncFtoF PtrFuncRadiusOfCoC;



class FuncFtoFconst: public FuncFtoF
{
	float m_const;
public:
	FuncFtoFconst (float value): m_const(value)
	{
	}
	out_t operator () (in_t in)
	{
		return m_const;
	}
};


class FuncFtoFinv: public FuncFtoF
{
	out_t operator () (in_t in)
	{
		if (in)
			return 1/in;
		else
			return 0;
	}
};


class FuncF3toFsumChannel: public FuncF3toF
{
public:
	float operator () (float3 channels)
	{
		return channels.x + channels.y + channels.z;
	}
};


class FuncF2toFconvFromFtoF: public FuncF2toF
{
	PtrFuncFtoF m_ptrfunc;
	FuncFtoF* m_func;
	float2 m_aspectratio;
	float2 m_aspectratioHalf;
	float m_inverseAspectratioLen;
public:
	FuncF2toFconvFromFtoF (PtrFuncFtoF func, float2 aspectratio)
	{
		m_ptrfunc = func;
		m_func = m_ptrfunc.get();
		m_aspectratio = aspectratio;
		m_aspectratioHalf = m_aspectratio/2.0f;
		m_inverseAspectratioLen = 1.0f/m_aspectratio.getLength();
	}
	float operator () (float2 pos)
	{
		pos *= m_aspectratio;
		float arg = (pos - m_aspectratioHalf).getLength() * 2.0f * m_inverseAspectratioLen;
		return (*m_func) (arg);
	}
};



class FuncStar: public FuncI2I2toF
{
	uint m_nFlares;
	float* m_cos;
	float* m_sin;
	float m_mult;
public:
	FuncStar (float _angle, uint nFlares, float mult)
	{
		m_nFlares = nFlares;
		m_cos = new float[m_nFlares];
		m_sin = new float[m_nFlares];
		m_mult = mult;
		for (uint nFlare = 0; nFlare < m_nFlares; nFlare++)
		{
			float angle = (float) (_angle + (nFlare/(float)nFlares) * 3.14159 * 2);
			m_cos[nFlare] = cosf (angle);
			m_sin[nFlare] = sinf (angle);
		}
	}
	~FuncStar ()
	{
		delete [] (m_cos);
		delete [] (m_sin);
	}
private:
	float getIntGauss (float arg);

	float getFlareVal (float2 pos, float size, int nFlare)
	{
		const float inc = 0.15f;
		const float offset = 0.6f/size;
		const float thresholdX = 2;
		const float decA = m_mult * (200.0f/size) / 4; // <- crazy...
		const float decC = 4;
		const float pi = 3.14159f;

		float _cosf = m_cos[nFlare];
		float _sinf = m_sin[nFlare];
		pos = float2 (_cosf*pos.x+_sinf*pos.y, _cosf*pos.y+_sinf*pos.x);
		if (pos.x <= 0)
			return 0;

		float center = pos.y/(pos.x*inc);
		float xplus = (pos.y+offset)/(pos.x*inc);
		float xminus = (pos.y-offset)/(pos.x*inc);
	
		float val = getIntGauss(xplus) - getIntGauss(xminus);
	
		float decY = pos.x*decC+1;
		return val*decA/(decY*decY*decY*decY);
	}

public:
	float operator () (int2 npos, int2 size)
	{
		float sum = 0.0f;
		float sizex = (float) size.x;
		for (uint nFlare = 0; nFlare < m_nFlares; nFlare++)
		{
			float2 pos = npos.get<float>() / sizex;
			sum += getFlareVal (pos, sizex, nFlare);
		}

		//sum += (npos==int2(0,0)) ? 1.0f : 0;
		return sum;
	}
};



class FuncGlow1: public FuncI2I2toF
{
	float m_mult;
public:
	FuncGlow1 (float mult)
	{
		m_mult = mult;
	}
public:
	float operator () (int2 npos, int2 size)
	{
		const float begin = m_mult*(1/0.15f)*1.0f/1024.0f*(float2(800.0f,(float)size.y)/size.get<float>()).getArea();

		if (npos == int2(0,0))
			return 1.0f;
		float2 pos = npos.get<float>();
		float dist = pos.getLength();
		float coef = begin/powf(dist,1.2f);
		return coef;
	}
};



class FuncGlow2: public FuncI2I2toF
{
public:
	FuncGlow2 ()
	{
	}
public:
	float operator () (int2 npos, int2 size)
	{
		const float begin = 0.005f;

		if (npos == int2(0,0))
			return 0.0f;
		float2 pos = npos.get<float>();
		float dist = pos.getLength();
		float coef = begin/powf(dist,2.0f);
		return coef;
	}
};


class FuncFuzinessConverter: public FuncFtoF
{
	PtrFuncFtoF m_func;
	float m_numLines;
public:
	FuncFuzinessConverter (PtrFuncFtoF func)
	{
		m_func = func;
		m_numLines = 1.0f;
	}
	void setSize (uint2 size)
	{
		m_numLines = (float) (size.get<float>()).getLength();
	}
	float operator () (float value)
	{
		float lines = (*m_func) (value);
		return m_numLines/lines*0.5f;
	}
};


class FuncFocusedToDist: public FuncRadiusOfCoC
{
	float m_dist;
	float m_aperture;
	float m_focalLength;
	float m_outMult;
public: 
	FuncFocusedToDist (float focusDist, float apert, float focalLen, float sensorSize, float resolution)
	{
		m_dist = focusDist;
		m_aperture = apert;
		m_focalLength = focalLen;
		m_outMult = resolution/sensorSize;
	}
	float operator () (float val)
	{
/*		float z = 8.5; //linearizeDepth (val);
		float v1 = sinf (3.14f/3);
		float v2 = sinf (3.14f/256);
		float height = 640.0f;
		float dist = m_dist;
		float coc = abs(z-dist)/z*v2/v1 * height;
		return coc;
*/
/*		float b = val;
		float a = 115.5f;//m_dist;
		float apert = 1/m_aperture;
		float f = 48;
		float res = 10;
		float d = 32;
		float bI = a*f/b;
		float fabs = abs(f - bI);
		return b * apert * abs(f - a*f/b) * res / (a*d);
*/
		val *= 1000; // <- metre do milimetrov
		float f = m_focalLength; // <- ohniskova vzdialenost
		float s = m_dist*1000;// <- zaostrovacia vzdialenost
		float N = m_aperture; // <-clonove cislo
		float ms = f / (s - f);// <- priecne zvacsenie
		float xd = abs (s-val); 
		float pmxd = s > val ? -xd : xd;
		float b1 = f * ms / N;
		float b2 = xd / (s + pmxd);
		float b = b1*b2;
		return b * m_outMult;
	}
};

/*
class FuncFocusedToDist: public FuncRadiusOfCoC
{
	float m_dist;
	float m_apert;
	float m_area;
	float m_resolution;
	float m_focal;

public:
	FuncFocusedToDist (float dist, float apert, float focal, float area, float resolution)
	{
		m_dist = dist*1000.0f;
		m_apert = apert;
		m_area = area;
		m_resolution = resolution;
		m_focal = focal;
	}
	float operator () (float b)
	{
		b *= 1000;
		//return s(val) / fov(val);

		float a = m_dist;
		float apert = m_apert;
		float f = m_focal;
		float area = m_area;
		float resolution = m_resolution;

		float bL = a * f / b;
		float cocL = std::abs(f - bL) * f * apert / bL;
		return cocL * area * resolution;
	}
};*/