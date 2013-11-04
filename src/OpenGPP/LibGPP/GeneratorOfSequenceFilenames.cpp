#include "GeneratorOfSequenceFilenames.h"
#include "Base.h"
#include <sstream>
#include <assert.h>

#pragma warning (disable : 4996)


GeneratorOfInputSequenceFilenames::GeneratorOfInputSequenceFilenames(string strFormat, int begin, int end, int step, int repeatStep) :
	m_strFormat(strFormat), 
	m_begin(begin),
	m_actualNum(begin),
	m_end(end), 
	m_step(step),
	m_repeatStep(repeatStep)
{
	m_actualRepeat = 0;
}

string GeneratorOfInputSequenceFilenames::getFilename()
{
	char buffer [1024];
	sprintf (buffer, m_strFormat.c_str(), m_actualNum);
	return (string) buffer;
}

void GeneratorOfInputSequenceFilenames::next()
{
	m_repeatStep++;
	if (m_repeatStep < m_repeatStep)
		return;

	m_actualNum += m_step;
	if (m_step > 0)
	{
		if (m_actualNum > m_end)
			m_actualNum = m_begin;
	}
	else
	{
		if (m_actualNum < m_end)
			m_actualNum = m_begin;
	}
}

GeneratorOfOutputSequenceFilenames::GeneratorOfOutputSequenceFilenames
	(string strFormat, int begin, int step) :
	m_strFormat(strFormat), 
	m_begin(begin),
	m_actualNum(begin),
	m_step(step)
{
}

string GeneratorOfOutputSequenceFilenames::getFilename()
{
	char buffer [1024];
	sprintf (buffer, m_strFormat.c_str(), m_actualNum);
	return (string) buffer;
}

void GeneratorOfOutputSequenceFilenames::next()
{
	m_actualNum += m_step;
}