#pragma once

#include "Base.h"

class GeneratorOfInputSequenceFilenames
{
	string m_strFormat;
	int m_actualNum, m_actualRepeat;
	int m_begin, m_end, m_step, m_repeatStep;
public:
	GeneratorOfInputSequenceFilenames(string strFormat, int begin, int end, int step, int repeatStep);
	string getFilename ();
	void next ();
};

class GeneratorOfOutputSequenceFilenames
{
	string m_strFormat;
	int m_actualNum;
	int m_begin, m_step;
public:
	GeneratorOfOutputSequenceFilenames(string strFormat, int begin, int step);
	string getFilename ();
	void next ();
};