#pragma once

class PIDController
{
public:
	float m_fP, m_fI, m_fD;

	float m_sumErr;
	float m_targetValue;
	float m_actualMult;
	float m_prevError;
	bool m_initPrevError;
	PIDController& operator= (PIDController& pid)
	{
		memcpy(this, &pid, sizeof(PIDController));
		return *this;
	}
	PIDController (float targetValue, float fP = 0.35, float fI = 0.0, float fD = 0.1)
	{
		m_fP = fP;
		m_fI = fI;
		m_fD = fD;
		m_targetValue = targetValue;
		m_actualMult = 1;
		m_sumErr = 0;
		m_initPrevError = false;
	}
	float recomputeLog2 (float inValue)
	{
		float inputValue = m_actualMult * inValue;
		float value = logf(inputValue)/logf(2);
		float target = logf(m_targetValue)/logf(2);
		float err = target - value;
		m_sumErr = m_sumErr*0.99f + err;
		if (! m_initPrevError)
		{
			m_initPrevError = true;
			m_prevError = err;
		}
		float newValue = value + err * m_fP + m_sumErr * m_fI + (err - m_prevError) * m_fD;
		m_prevError = err;
		m_actualMult = powf (2, newValue) / inValue;
		return m_actualMult;
	}
};