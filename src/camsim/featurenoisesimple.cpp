#include "featurenoisesimple.h"
#include "hdrimagefunctor.h"

void FeatureNoiseSimple::featureFtoF (PtrHDRImage3f in, PtrHDRImage3f& out)
{
	out = in;
	HDRIFunctorNoise hdriNoise (m_func, m_fmin, m_fmax);
	out->applyFunctor (hdriNoise);
}