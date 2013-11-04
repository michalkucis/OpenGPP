#include "std.h"
#include <cmath>

float clFFT_log2(float x)
{
	float exponent = logf(x) / logf(2);
	return exponent;
}

int clFFT_round(float x)
{
	return (int)ceil(x + 0.5f);
}