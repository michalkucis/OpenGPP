#include "featuretransformvalues.h"
#include "hdrimagefunctor.h"

void FeatureTransformValues::featureFtoF (PtrHDRImage3f in, PtrHDRImage3f& out)
{
	out = in;
	HDRIFunctorApplyFunc hdriFunctor (m_func);
	out->applyFunctor (hdriFunctor);
}