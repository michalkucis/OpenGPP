#include "Functions.h"
#include <boost\math\special_functions\erf.hpp>

float getQuantilNormalDistro( float x, float median, float deviation )
{
	// http://www.boost.org/doc/libs/1_35_0/libs/math/doc/sf_and_dist/html/math_toolkit/dist/dist_ref/dists/normal_dist.html
	float m = median;
	float s = deviation;
	return m - s * sqrtf(2) * boost::math::erfc_inv(2*x);
}

float getCDFNormalDistro( float x, float median, float deviation )
{
	float m = median;
	float s = deviation;
	return 0.5f * boost::math::erfc(-(x-m)/s*sqrtf(2));
}
