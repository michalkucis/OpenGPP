#pragma once

#include "Vector2.h"
#include "Vector3.h"
#include "Error.h"

float getQuantilNormalDistro (float x, float median, float deviation);

float getCDFNormalDistro (float x, float median, float deviation);


template <typename Tin, typename Tout>
class Func1Arg
{
public:
	typedef Tin in_t;
	typedef Tout out_t;
	virtual Tout operator () (Tin in) = 0;
	virtual string getString(string strResult = "y", string strInput = "x")
	{
		error(ERR_NOT_IMPLEMENTED);
	}
	virtual ~Func1Arg () {}
};

template <typename Tin1, typename Tin2, typename Tout>
class Func2Arg
{
public:
	typedef Tin1 in1_t;
	typedef Tin2 in2_t;
	typedef Tout out_t;
	virtual Tout operator () (Tin1 in1, Tin2 in2) = 0;
	virtual ~Func2Arg () {}
};

#define DEF_FUNC1(arg,ret,suffix)\
	typedef Func1Arg<arg,ret> Func##suffix;

#define DEF_FUNC2(arg1,arg2,ret,suffix)\
	typedef Func2Arg<arg1,arg2,ret> Func##suffix;

DEF_FUNC1(float,float,FtoF);
DEF_FUNC1(float,float3,FtoF3);
DEF_FUNC1(float3,float,F3toF);
DEF_FUNC1(float2,float,F2toF);
DEF_FUNC1(float3,float,F3toF);
DEF_FUNC2(float3,float,float3,F3FtoF3);
DEF_FUNC2(int2,int2,float,I2I2toF);

class FuncFtoFLinear: public FuncFtoF
{
	float m_mult;
public:
	FuncFtoFLinear (float mult): m_mult(mult)
	{}
	float operator () (float x)
	{
		return x * m_mult;
	}
};

class FuncFtoFCompCoC: public FuncFtoF
{
	float m_focusDistance;
	float m_aperture;
	float m_focalLength;
	float m_outMult;
	float m_inputDistanceMult;
public:
	// focusDist = "1.36"
	// aperture = "3"
	// focalLen ="48" mm
	// sensorSize = "42" mm (width)
	// resolution = width of the texture
	// inputDepthMult = texture depth is multiplied by this value to achieve depth in meters
	FuncFtoFCompCoC (float focusDist, float aperture, float focalLen, float sensorSize, /*float resolution, */float inputDistanceMult, float outMult = 1.0f)
	{
		m_focusDistance = focusDist;
		m_aperture = aperture;
		m_focalLength = focalLen;
		m_outMult = outMult/sensorSize;
		m_inputDistanceMult = inputDistanceMult;
	}
	FuncFtoFCompCoC& operator = (FuncFtoFCompCoC& func)
	{
		memcpy(this, &func, sizeof(*this));
		return *this;
	}
protected:
	float f() // <- ohniskova vzdialenost
	{ return m_focalLength; } 
	float s() // <- zaostrovacia vzdialenost v mm
	{ return m_focusDistance*1000;}
	float m()
	{ return m_outMult; }
	float N() // <-clonove cislo
	{ return m_aperture; }

public:
	float getB1 ()
	{
		return f()*f()/(s() - f()) / N();
	}
	float getS()
	{
		return s();
	}
	float getM()
	{
		return m();
	}
	float operator() (float x)
	{
		x *= 1000 * m_inputDistanceMult; // <- metre do milimetrov
		float ms = f() / (s() - f());// <- priecne zvacsenie
		float xd = abs (s()-x); 
		float pmxd = s() > x ? -xd : xd;
		float b1 = f() * ms / N();
		float b2 = xd / (s() + pmxd);
		float b = b1*b2;
		return b * m();
	}

	float getInMult() 
	{
		return 1000 * m_inputDistanceMult;
	}
};
