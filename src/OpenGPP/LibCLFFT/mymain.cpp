#include <stdlib.h>
#include <cstdlib>
#include "procs.h"
#include "clFFT.h"
#include "std.h"
#include "clKhronos.h"


//typedef long int uint64_t;
typedef enum {
	clFFT_OUT_OF_PLACE,
	clFFT_IN_PLACE,
}clFFT_TestType;


int runTest(cl_context context, cl_command_queue queue,
	clFFT_Dim3 n, /*int batchSize, */clFFT_Direction dir, clFFT_Dimension dim, 
	int numIter, clFFT_TestType testType)
{	
	cl_int err = CL_SUCCESS;
	int iter;
	//double t;
	int batchSize = 1;

	//uint64_t t0, t1;
	int mx = round(log2((float)n.x));
	int my = round(log2((float)n.y));
	int mz = round(log2((float)n.z));

	int length = n.x * n.y * n.z * batchSize;

	double gflops = 5e-9 * ((double)mx + (double)my + (double)mz) * (double)n.x * (double)n.y * (double)n.z * (double)batchSize * (double)numIter;

	clFFT_SplitComplex data_i_split (NULL, NULL);
	clFFT_SplitComplex data_cl_split (NULL, NULL);
	//clFFT_Complex *data_i = NULL;
	//clFFT_Complex *data_cl = NULL;
	clFFT_SplitComplexDouble data_iref (NULL, NULL); 
	clFFT_SplitComplexDouble data_oref (NULL, NULL);

	clFFT_Plan plan = NULL;
	cl_mem data_in = NULL;
	cl_mem data_out = NULL;
	cl_mem data_in_real = NULL;
	cl_mem data_in_imag = NULL;
	cl_mem data_out_real = NULL;
	cl_mem data_out_imag = NULL;

	data_i_split.real     = (float *) malloc(sizeof(float) * length);
	data_i_split.imag     = (float *) malloc(sizeof(float) * length);
	data_cl_split.real    = (float *) malloc(sizeof(float) * length);
	data_cl_split.imag    = (float *) malloc(sizeof(float) * length);
	if(!data_i_split.real || !data_i_split.imag || !data_cl_split.real || !data_cl_split.imag)
	{
		err = -1;
		log_error("Out-of-Resources\n");
		goto cleanup;
	}

	data_iref.real   = (double *) malloc(sizeof(double) * length);
	data_iref.imag   = (double *) malloc(sizeof(double) * length);
	data_oref.real   = (double *) malloc(sizeof(double) * length);
	data_oref.imag   = (double *) malloc(sizeof(double) * length);	
	if(!data_iref.real || !data_iref.imag || !data_oref.real || !data_oref.imag)
	{
		err = -3;
		log_error("Out-of-Resources\n");
		goto cleanup;
	}

	for(int i = 0; i < length; i++)
	{
		data_i_split.real[i] = 2.0f * (float) rand() / (float) RAND_MAX - 1.0f;
		data_i_split.imag[i] = 2.0f * (float) rand() / (float) RAND_MAX - 1.0f;
		data_cl_split.real[i] = 0.0f;
		data_cl_split.imag[i] = 0.0f;			
		data_iref.real[i] = data_i_split.real[i];
		data_iref.imag[i] = data_i_split.imag[i];
		data_oref.real[i] = data_iref.real[i];
		data_oref.imag[i] = data_iref.imag[i];	
	}

	plan = clFFT_CreatePlan( context, n, dim, clFFT_SplitComplexFormat, &err );
	if(!plan || err) 
	{
		log_error("clFFT_CreatePlan failed\n");
		goto cleanup;
	}

	data_in_real = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, length*sizeof(float), data_i_split.real, &err);
	if(!data_in_real || err) 
	{
		log_error("clCreateBuffer failed\n");
		goto cleanup;
	}

	data_in_imag = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, length*sizeof(float), data_i_split.imag, &err);
	if(!data_in_imag || err) 
	{
		log_error("clCreateBuffer failed\n");
		goto cleanup;
	}

	data_out_real = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, length*sizeof(float), data_cl_split.real, &err);
	if(!data_out_real || err) 
	{
		log_error("clCreateBuffer failed\n");
		goto cleanup;
	}

	data_out_imag = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, length*sizeof(float), data_cl_split.imag, &err);
	if(!data_out_imag || err) 
	{
		log_error("clCreateBuffer failed\n");
		goto cleanup;
	}			

	err = CL_SUCCESS;

	//t0 = mach_absolute_time();
		for(iter = 0; iter < numIter; iter++)
			err |= clFFT_ExecutePlannar(queue, plan, batchSize, dir, data_in_real, data_in_imag, data_out_real, data_out_imag, 0, NULL, NULL);

	err |= clFinish(queue);

	if(err) 
	{
		log_error("clFFT_Execute\n");
		goto cleanup;	
	}

	//t1 = mach_absolute_time(); 
	//t = subtractTimes(t1, t0);
	//char temp[100];
	//sprintf(temp, "GFlops achieved for n = (%d, %d, %d), batchsize = %d", n.x, n.y, n.z, batchSize);
	//log_perf(gflops / (float) t, 1, "GFlops/s", "%s", temp);
	
	err |= clEnqueueReadBuffer(queue, data_out_real, CL_TRUE, 0, length*sizeof(float), data_cl_split.real, 0, NULL, NULL);
	err |= clEnqueueReadBuffer(queue, data_out_imag, CL_TRUE, 0, length*sizeof(float), data_cl_split.imag, 0, NULL, NULL);

	if(err) 
	{
		log_error("clEnqueueReadBuffer failed\n");
		goto cleanup;
	}	

	//computeReferenceD(&data_oref, n, batchSize, dim, dir);

	//double diff_avg, diff_max, diff_min;
	//diff_avg = computeL2Error(&data_cl_split, &data_oref, n.x*n.y*n.z, batchSize, &diff_max, &diff_min);
	//if(diff_avg > eps_avg)
	//	log_error("Test failed (n=(%d, %d, %d), batchsize=%d): %s Test: rel. L2-error = %f eps (max=%f eps, min=%f eps)\n", n.x, n.y, n.z, batchSize, (testType == clFFT_OUT_OF_PLACE) ? "out-of-place" : "in-place", diff_avg, diff_max, diff_min);
	//else
	//	log_info("Test passed (n=(%d, %d, %d), batchsize=%d): %s Test: rel. L2-error = %f eps (max=%f eps, min=%f eps)\n", n.x, n.y, n.z, batchSize, (testType == clFFT_OUT_OF_PLACE) ? "out-of-place" : "in-place", diff_avg, diff_max, diff_min);			

cleanup:
	clFFT_DestroyPlan(plan);	

	if(data_i_split.real)
		free(data_i_split.real);
	if(data_i_split.imag)
		free(data_i_split.imag);
	if(data_cl_split.real)
		free(data_cl_split.real);
	if(data_cl_split.imag)
		free(data_cl_split.imag);

	if(data_in_real)
		clReleaseMemObject(data_in_real);
	if(data_in_imag)
		clReleaseMemObject(data_in_imag);
	if(data_out_real && testType == clFFT_OUT_OF_PLACE)
		clReleaseMemObject(data_out_real);
	if(data_out_imag && clFFT_OUT_OF_PLACE)
		clReleaseMemObject(data_out_imag);

	if(data_iref.real)
		free(data_iref.real);
	if(data_iref.imag)
		free(data_iref.imag);		
	if(data_oref.real)
		free(data_oref.real);
	if(data_oref.imag)
		free(data_oref.imag);

	return err;
}

cl::Context getCLContext (int nPlatform, int deviceType = CL_CONTEXT_PLATFORM)
{
	cl_int err = CL_SUCCESS;
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.size()) 
	{
		cl_context_properties properties[] = 
		{ CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[nPlatform])(), 0};
		cl::Context context(deviceType, properties);
		return context;
	}
	return cl::Context();
	//error0(ERR_OPENCL, "No OpenCL platform cannot be found");
}

cl::CommandQueue getCLCommandQueue (cl::Context context)
{
	cl_int err = CL_SUCCESS;
	std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
	cl::CommandQueue queue(context, devices[0], 0, &err);			
	//if (err != CL_SUCCESS)
	//	throw cl::Error(err, "cl::CommandQueue");
	return queue;
}

int main ()
{
	cl::Context context = getCLContext(0);
	cl::CommandQueue queue = getCLCommandQueue(context);
	runTest(context(), queue(), clFFT_Dim3(2048,2048,1), clFFT_Forward, clFFT_2D, 1, clFFT_OUT_OF_PLACE);
}