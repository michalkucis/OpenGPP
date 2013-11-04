/////////////////////////////////////////////////////////////////////////////////
//
//  Adaptation: zapuzdruje algoritmus adaptacie clony a adaptacie casu vzhladom na jas obrazka
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "feature.h"
#include "std.h"

struct AdaptationParam
{
	float middleGray, minFnum, maxFnum, maxSpeed, Kp, Ki;
};
class Adaptation
{
	/*float m_minFnum;
	float m_maxFnum;
	*/
	AdaptationParam m_adapt;
	float m_minTime;
	float m_maxTime;
	
	float m_multOneEV;

	float m_actualTime;
	float m_actualFnum;

	float m_sumErrFnum;
	float m_sumLogErrTime;
public:
	Adaptation (float finit)
	{
		m_adapt.minFnum = 1.0f;
		m_adapt.maxFnum = 18.0f;
		m_adapt.middleGray = 0.49f;
		m_adapt.maxSpeed = 1.0f;
		m_adapt.Kp = 0.2f;
		m_adapt.Ki = 0.01f;
		m_maxTime = 1.0f;
		m_minTime = 1/5000.0f;
		m_multOneEV = 1.0f; 

		m_sumErrFnum = 0.0f;
		m_sumLogErrTime = 0.0f;
		m_actualFnum = finit;//(m_minFnum + m_maxFnum)/2.0f;
		m_actualTime = 1.0f;
	}
	void setParam (AdaptationParam& param)
	{
		m_adapt = param;
	}
	virtual float getRGBtoGray (float3 rgb)
	{
		return (rgb.x+rgb.y+rgb.z)/3.0f;
	}
	virtual float getMiddleGrayValue ()
	{
		return m_adapt.middleGray;
	}
	virtual float getTime ()
	{
		return m_actualTime;
	}
	virtual float getFnum ()
	{
		return m_actualFnum;
	}
	virtual float getMultOneEV ()
	{
		return m_multOneEV;
	}
	virtual float computeNewFnum (float actualFnum, float targetFnum);
	virtual float computeNewTime (float actualTime, float targetTime);
	void updateFnum (float3 rgb, bool isIntensity = true);
	void updateTime (float3 rgb, bool isIntensity = true);

	float getMultByEV ()
	{
		return getMultOneEV()*getTime()/(getFnum()*getFnum());
	}
};
typedef boost::shared_ptr<Adaptation> PtrAdaptation;


// isInputIntensity=true = before FeatureAdaptation
// isInputIntensity=false = after FeatureAdaptation


