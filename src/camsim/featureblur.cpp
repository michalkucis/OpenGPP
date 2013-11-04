#include "featureblur.h"

FeatureBlur::FeatureBlur (PtrFuncFtoF func)
{
	m_func = func;
	m_output = PtrHDRImage3f(new HDRImage3f);
	m_depth = PtrHDRImage1f(new HDRImage1f);

	m_func = PtrFuncFtoF (new FuncFuzinessConverter (func));
	m_dof = createLinGatherDOF (m_func);
}

void FeatureBlur::featureFtoF (PtrHDRImage3f in, PtrHDRImage3f &out)
{
	initDepth (m_depth.get(), in->getSize());
	m_output->setSize (in->getSize(), in->getAppendSize());

	FuncFuzinessConverter* f = dynamic_cast<FuncFuzinessConverter*> (m_func.get());
	assert (f);
	f->setSize (in->getSize());

	m_dof->doDOF (in, m_depth, m_output);
	out = m_output;
}