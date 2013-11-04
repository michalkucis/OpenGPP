#include "adaptation.h"

float Adaptation::computeNewFnum (float actualFnum, float targetFnum)
{
	float err = targetFnum - actualFnum;

	m_sumErrFnum += err;
	float maxChangePerComputation = m_adapt.maxSpeed;

	float KI = m_adapt.Ki;
	float Kp = m_adapt.Kp;

	float change = Kp * err + KI * m_sumErrFnum;

	float clampedChange = std::max<float> (-maxChangePerComputation, std::min<float> (maxChangePerComputation, change));

	float fnum = actualFnum+clampedChange; 
	float clampedFnum = std::max<float> (m_adapt.minFnum, std::min<float> (m_adapt.maxFnum, fnum));
	return clampedFnum;
}
float Adaptation::computeNewTime (float actualTime, float targetTime)
{
	float err = targetTime - actualTime;
	float log2err = log2(err);

	float maxChangePerComputation = log2 (2);
	m_sumLogErrTime += maxChangePerComputation;

	float KI = 0.1f;
	float Kp = 0.5f;

	float change = Kp * log2err + KI * m_sumLogErrTime;

	float clampedChange = std::max<float> (-maxChangePerComputation, std::min<float> (maxChangePerComputation, change));

	float time = powf(2,log2(actualTime)+clampedChange);
	float clampedTime = std::max<float> (m_minTime, std::min<float> (m_maxTime, time));

	return clampedTime;
}
// if (isIntensity)
//	rgb = rgbIntensity;
// else
//	rgb = rgbBrightness;
void Adaptation::updateFnum (float3 rgb, bool isIntensity)
{
	float grayIntensity = getRGBtoGray (rgb);

	float middle = getMiddleGrayValue ();

	float grayBrightness = grayIntensity;
	float target = grayBrightness / middle;

	float targetFnum = sqrtf(target); 
	//printf ("actual=%f  target=%f\n", m_actualFnum, targetFnum);
		
	m_actualFnum = computeNewFnum (m_actualFnum, targetFnum);
}
/*void Adaptation::updateFnum (float3 rgb, bool isIntensity)
{
	float grayIntensity = getRGBtoGray (rgb);
		
	float t = getTime();
	float mult = grayIntensity;
	if (isIntensity)
		mult *= getMultOneEV ();
	float middle = getMiddleGrayValue ();

	float b = m_actualFnum;
	float b2 = b*b;

	float grayBrightness;
	if (isIntensity)
		grayBrightness = mult * t / b2;
	else
		grayBrightness = mult;
	float target = grayBrightness / middle;

	float targetFnum = m_actualFnum*sqrtf(target); 
		
	m_actualFnum = computeNewFnum (m_actualFnum, targetFnum);
}*/
void Adaptation::updateTime (float3 rgb, bool isIntensity)
{
	float grayIntensity = getRGBtoGray (rgb);
		
	float t = getTime();
	float mult = grayIntensity;
	if (isIntensity)
		mult *= getMultOneEV ();
	float middle = getMiddleGrayValue ();

	float b = m_actualFnum;
	float b2 = b*b;

	float grayBrightness;
	if (isIntensity)
		grayBrightness = mult * t / b2;
	else
		grayBrightness = mult;
	float target = grayBrightness / middle;

	float targetTime = m_actualTime/target; 
		
	m_actualTime = computeNewTime (m_actualTime, targetTime);
}