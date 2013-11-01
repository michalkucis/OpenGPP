#include "std.h"
#include <math.h>

float log2 (float value)
{
	return log (value) / log (2.0f);
}

float clamp (float v, float minimum, float maximum)
{
	if (v < minimum)
		v = minimum;
	if (v > maximum)
		v = maximum;
	return v;
}