#include "func.h"
#include <boost/math/special_functions/erf.hpp>

float FuncStar::getIntGauss (float arg)
{
	float value = arg*2;
	return (1+boost::math::erf (value))/2;
}