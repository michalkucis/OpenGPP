#include "EffectCLImpl.h"

void ClFFT::init( Ptr<cl::Context> context, Ptr<cl::CommandQueue> queue, clFFT_Dimension& dim, clFFT_Dim3& dim3 )
{
	m_context = context;
	m_queue = queue;
	m_size = uint3(dim3.x, dim3.y, dim3.z);
	cl_int err = CL_SUCCESS;
	*m_fftPlan = clFFT_CreatePlan((*m_context)(), clFFT_Dim3(m_size.x, m_size.y, 1), clFFT_2D, clFFT_SplitComplexFormat, &err);
	if(!*m_fftPlan || err)
		throw cl::Error(ERR_OPENCL, "clFFT_CreatePlan");
}

ClFFT::ClFFT( Ptr<cl::Context> context, Ptr<cl::CommandQueue> queue, uint dim )
{
	m_fftPlan = new clFFT_Plan;
	clFFT_Dimension dimension = clFFT_1D;
	init(context, queue, dimension, clFFT_Dim3(dim, 1, 1));
}

ClFFT::ClFFT( Ptr<cl::Context> context, Ptr<cl::CommandQueue> queue, uint2 dim )
{
	m_fftPlan = new clFFT_Plan;
	clFFT_Dimension dimension = clFFT_2D;
	init(context, queue, dimension, clFFT_Dim3(dim.x, dim.y, 1));
}

ClFFT::ClFFT( Ptr<cl::Context> context, Ptr<cl::CommandQueue> queue, uint3 dim )
{
	m_fftPlan = new clFFT_Plan;
	clFFT_Dimension dimension = clFFT_3D;
	init(context, queue, dimension, clFFT_Dim3(dim.x, dim.y, dim.z));
}

ClFFT::~ClFFT()
{
	clFFT_DestroyPlan(*m_fftPlan);
	delete m_fftPlan;
}

void ClFFT::perform( cl::Buffer real, cl::Buffer im, clFFT_Direction& dir )
{
	size_t requiredSize = m_size.getArea()*sizeof(float);
	size_t realBytes = real.getInfo<CL_MEM_SIZE>();
	size_t imBytes = real.getInfo<CL_MEM_SIZE>();
	assert(realBytes >= requiredSize);
	assert(imBytes >= requiredSize);

	try {
		cl_int err = clFFT_ExecutePlannar((*m_queue)(), *m_fftPlan, 1, dir, real(), im(), real(), im(),  0, NULL, NULL);
		if(!*m_fftPlan || err)
			throw cl::Error(ERR_OPENCL, "clFFT_ExecutePlannar");
	} catchCLError;
}
