#define CDF97_ALPHA (-1.586134342f)
#define CDF97_BETA (-0.05298011854f)
#define CDF97_GAMMA (0.8829110762f)
#define CDF97_DELTA (0.4435068522)
#define CDF97_ZETA (1.0f/1.1496043988602418f)/*(0.70710678118f)*/
#define CDF97_INVZETA (1.1496043988602418f) /*(1.41421356237f)*/

  
typedef struct 
{
	int2 resolution;
	int2 halfResolution;
	int bufferStride;

} StructConstMemory;


__kernel void simpleCopy(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	int offset = myID.x + myID.y*get_global_size(0);
	out[offset] = in[offset];
}


__kernel void copyGlobalLocalGlobal(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int offsetLocalIn1 = get_local_id(0)*2 + get_local_size(0)*get_local_id(1);
	int offsetLocalIn2 = get_local_id(0)*2+1 + get_local_size(0)*get_local_id(1);
	int offsetLocalOut1 = get_local_id(0)*2 + get_local_size(0)*get_local_id(1);
	int offsetLocalOut2 = get_local_id(0)*2+1 + get_local_size(0)*get_local_id(1);
	int offsetGlobal1 = get_global_id(0) + constMemory->bufferStride*get_global_id(1);
	int offsetGlobal2 = get_global_id(0) + constMemory->halfResolution.x + constMemory->bufferStride*get_global_id(1);
	//int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[offsetLocalIn1] = in[offsetGlobal1];
	localMemory[offsetLocalIn2] = in[offsetGlobal2];	
	barrier(CLK_LOCAL_MEM_FENCE);

	//// init variables:
	__local float* arr = localMemory;
	
	out[offsetGlobal1] = arr[offsetLocalOut1];
	out[offsetGlobal2] = arr[offsetLocalOut2];
}

__kernel void horizonLiftingSync(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[myID.x] = in[myID.x + constMemory->bufferStride*get_global_id(1)];
	localMemory[myID.x+get_global_size(0)] = in[myID.x+get_global_size(0) + constMemory->bufferStride*get_global_id(1)];
	barrier(CLK_LOCAL_MEM_FENCE);

	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;
	int nd0 = abs (myID.x * 2 - 1);
	int nd1 = myID.x * 2;
	int nd2 = nd1 + 1 >= resX ? nd1 - 1 : nd1 + 1;
	int nd3 = nd1 + 2 >= resX ? 2*resX - nd1 - 4 : nd1 + 2;

	
	// compute:
	arr[nd2] += CDF97_ALPHA * (arr[nd1] + arr[nd3]);
	barrier(CLK_LOCAL_MEM_FENCE);
	arr[nd1] += CDF97_BETA * (arr[nd0] + arr[nd2]);
	barrier(CLK_LOCAL_MEM_FENCE);
	arr[nd2] += CDF97_GAMMA * (arr[nd1] + arr[nd3]);
	barrier(CLK_LOCAL_MEM_FENCE);
	arr[nd1] += CDF97_DELTA * (arr[nd0] + arr[nd2]);
	barrier(CLK_LOCAL_MEM_FENCE);

	arr[nd1] *= CDF97_INVZETA;
	arr[nd2] *= CDF97_ZETA;
	
	// save elements:

	out[myID.x + constMemory->bufferStride*myID.y] = arr[nd1];
	out[myID.x+half + constMemory->bufferStride*myID.y] = arr[nd2];
}


__kernel void horizonLiftingSyncWide(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory,
	int numParts)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	int offset = get_global_size(0) * 2;

	for (int i = 0; i < numParts*2; i++)
		localMemory[myID.x + i*get_global_size(0)] = in[myID.x + constMemory->bufferStride*myID.y + i*get_global_size(0)];
	barrier(CLK_LOCAL_MEM_FENCE);

	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	
	#define MACRO_ND0 abs(myID.x*2-1+offset*i)
	#define MACRO_ND1 (myID.x * 2 + offset*i)
	#define MACRO_ND2 (myID.x * 2 + 1 + offset*i)
	#define MACRO_ND3 (MACRO_ND1 + 2 >= resX ? 2*resX - MACRO_ND1 - 4 : MACRO_ND1 + 2)
	for (int i = 0; i < numParts; i++)
		arr[MACRO_ND2] += CDF97_ALPHA * (arr[MACRO_ND1] + arr[MACRO_ND3]);
	barrier(CLK_LOCAL_MEM_FENCE);
	for (int i = 0; i < numParts; i++)
		arr[MACRO_ND1] += CDF97_BETA * (arr[MACRO_ND0] + arr[MACRO_ND2]);
	barrier(CLK_LOCAL_MEM_FENCE);
	for (int i = 0; i < numParts; i++)
		arr[MACRO_ND2] += CDF97_GAMMA * (arr[MACRO_ND1] + arr[MACRO_ND3]);
	barrier(CLK_LOCAL_MEM_FENCE);
	for (int i = 0; i < numParts; i++)
		arr[MACRO_ND1] += CDF97_DELTA * (arr[MACRO_ND0] + arr[MACRO_ND2]);
	barrier(CLK_LOCAL_MEM_FENCE);

	for (int i = 0; i < numParts; i++)
	{
		out[myID.x + constMemory->bufferStride*myID.y + i*get_global_size(0)] = localMemory[myID.x * 2 + get_global_size(0)*2*i] * CDF97_INVZETA;
		out[myID.x+half + constMemory->bufferStride*myID.y + i*get_global_size(0)] = localMemory[myID.x * 2 + 1 + get_global_size(0)*2*i] * CDF97_ZETA;
	}
}


__kernel void horizonLiftingAsync(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	int size = constMemory->resolution.x;
	// load elements:
	float l1 = in[myID.x + constMemory->bufferStride*get_global_id(1)];
	localMemory[myID.x + 4] = l1;
	float l2 = in[myID.x + get_global_size(0) + constMemory->bufferStride*get_global_id(1)];
	localMemory[myID.x + 4 + get_global_size(0)] = l2;

	if (myID.x <= 4 && myID.x >= 1)
		localMemory[4 - myID.x] = l1;
	if (myID.x >= get_global_size(0)-5)
		localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	int base = myID.x*2;
	float f[9];
	for(int i = 0; i < 9; i++)
		f[i] = arr[base+i];
	for(int i = 1; i < 8; i+=2)
		f[i] += (f[i-1] + f[i+1])*CDF97_ALPHA;
	for(int i = 2; i < 7; i+=2)
		f[i] += (f[i-1] + f[i+1])*CDF97_BETA;
	for(int i = 3; i < 6; i+=2)
		f[i] += (f[i-1] + f[i+1])*CDF97_GAMMA;
	f[4] += (f[3] + f[5])*CDF97_DELTA;

	f[4] *= CDF97_INVZETA;
	f[5] *= CDF97_ZETA;
	
	// save elements:

	out[myID.x + constMemory->bufferStride*myID.y] = f[4];
	out[myID.x+half + constMemory->bufferStride*myID.y] = f[5];
}

__kernel void horizonLiftingAsyncWide(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory,
	int nParts)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	int size = constMemory->resolution.x;
	// load elements:
	for (int i = 0; i < nParts; i++)
	{
		float l1 = in[myID.x + constMemory->bufferStride*get_global_id(1)];
		localMemory[myID.x + 4] = l1;
		float l2 = in[myID.x+get_global_size(0) + constMemory->bufferStride*get_global_id(1)];
		localMemory[myID.x + 4 + get_global_size(0)] = l2;

		if (myID.x <= 4 && myID.x >= 1 && i == 0)
			localMemory[4 - myID.x] = l1;
		if (myID.x >= get_global_size(0)-4 && i == nParts-1)
			localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	}
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	for (int i = 0; i < nParts; i++)
	{
		int base = myID.x*2 + i*get_global_size(0)*2;
		float f[9];
		for(int i = 0; i < 9; i++)
			f[i] = arr[base+i];
		for(int i = 1; i < 8; i+=2)
			f[i] += (f[i-1] + f[i+1])*CDF97_ALPHA;
		for(int i = 2; i < 7; i+=2)
			f[i] += (f[i-1] + f[i+1])*CDF97_BETA;
		for(int i = 3; i < 6; i+=2)
			f[i] += (f[i-1] + f[i+1])*CDF97_GAMMA;
		f[4] += (f[3] + f[5])*CDF97_DELTA;

		f[4] *= CDF97_INVZETA;
		f[5] *= CDF97_ZETA;
	
		// save elements:

		out[myID.x + i*get_global_id(0) + constMemory->bufferStride*myID.y] = f[4];
		out[myID.x+half + i*get_global_id(0) + constMemory->bufferStride*myID.y] = f[5];
	}
}




__kernel void horizonConvolutionAsync(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	float l1 = in[myID.x + constMemory->bufferStride*get_global_id(1)];
	localMemory[myID.x + 4] = l1;
	float l2 = in[myID.x+get_global_size(0) + constMemory->bufferStride*get_global_id(1)];
	localMemory[myID.x + 4 + get_global_size(0)] = l2;

	if (myID.x <= 4 && myID.x >= 1)
		localMemory[4 - myID.x] = l1;
	if (myID.x >= get_global_size(0)-5)
		localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	int base = myID.x*2;
	float f1 = arr[base] * 0.026749f;
	f1 += arr[base+1] * -0.016864f;
	f1 += arr[base+2] * -0.078223f;
	f1 += arr[base+3] * 0.266864f;
	f1 += arr[base+4] * 0.602949f;
	f1 += arr[base+5] * 0.266864f;
	f1 += arr[base+6] * -0.078223f;
	f1 += arr[base+7] * -0.016864f;
	f1 += arr[base+8] * 0.026749f;
	
	int base2 = myID.x*2+2;
	float f2 = arr[base2] * 0.091272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+2] * -0.591272f;
	f2 += arr[base2+3] * 1.115087f;
	f2 += arr[base2+4] * -0.591272f;
	f2 += arr[base2+5] * -0.057544f;
	f2 += arr[base2+6] * 0.091272f;

	// save elements:

	out[myID.x + constMemory->bufferStride*myID.y] = f1;
	out[myID.x+half + constMemory->bufferStride*myID.y] = f2;
	
	//out[myID.x*2 + constMemory->bufferStride*myID.y] = arr[myID.x*2 + 8];
	//out[myID.x*2+1 + constMemory->bufferStride*myID.y] = arr[myID.x*2+1 + 8];
}



__kernel void horizonConvolutionAsyncWide(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory,
	int nParts)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	for (int i = 0; i < nParts; i++)
	{
		float l1 = in[myID.x + constMemory->bufferStride*get_global_id(1)];
		localMemory[myID.x + 4] = l1;
		float l2 = in[myID.x+get_global_size(0) + constMemory->bufferStride*get_global_id(1)];
		localMemory[myID.x + 4 + get_global_size(0)] = l2;

		if (myID.x <= 4 && myID.x >= 1 && i == 0)
			localMemory[4 - myID.x] = l1;
		if (myID.x >= get_global_size(0)-5 && i == nParts-1)
			localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	}
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	for (int i = 0; i < nParts; i++)
	{
		int base = myID.x*2 + i*get_global_size(0)*2;
		float f1 = arr[base] * 0.026749f;
		f1 += arr[base+1] * 0.016864f;
		f1 += arr[base+2] * -0.078223f;
		f1 += arr[base+3] * -0.266864f;
		f1 += arr[base+4] * 0.602949f;
		f1 += arr[base+5] * -0.266864f;
		f1 += arr[base+6] * -0.078223f;
		f1 += arr[base+7] * 0.016864f;
		f1 += arr[base+8] * 0.026749f;
	
		int base2 = myID.x*2+2 + i*get_global_size(0)*2;
		float f2 = arr[base2] * -0.091272f;
		f2 += arr[base2+1] * -0.057544f;
		f2 += arr[base2+1] * 0.591272f;
		f2 += arr[base2+1] * 1.115087f;
		f2 += arr[base2+1] * 0.591272f;
		f2 += arr[base2+1] * -0.057544f;
		f2 += arr[base2+1] * -0.091272f;

		// save elements:

		out[myID.x + i*get_global_id(0) + constMemory->bufferStride*myID.y] = f1;
		out[myID.x+half + i*get_global_id(0) + constMemory->bufferStride*myID.y] = f2;
	}
}








__kernel void horizonConvolutionAsync2(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float2* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	float2 l1;
	l1.x = in[myID.x + constMemory->bufferStride*(get_global_id(1)*2)];
	l1.y = in[myID.x + constMemory->bufferStride*(get_global_id(1)*2+1)];
	localMemory[myID.x + 4] = l1;
	float2 l2;
	l2.x = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*2)];
	l2.y = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*2+1)];
	localMemory[myID.x + 4 + get_global_size(0)] = l2;

	if (myID.x <= 4 && myID.x >= 1)
		localMemory[4 - myID.x] = l1;
	if (myID.x >= get_global_size(0)-5)
		localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float2* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	int base = myID.x*2;
	float2 f1 = arr[base] * 0.026749f;
	f1 += arr[base+1] * -0.016864f;
	f1 += arr[base+2] * -0.078223f;
	f1 += arr[base+3] * 0.266864f;
	f1 += arr[base+4] * 0.602949f;
	f1 += arr[base+5] * 0.266864f;
	f1 += arr[base+6] * -0.078223f;
	f1 += arr[base+7] * -0.016864f;
	f1 += arr[base+8] * 0.026749f;
	
	int base2 = myID.x*2+2;
	float2 f2 = arr[base2] * 0.091272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+2] * -0.591272f;
	f2 += arr[base2+3] * 1.115087f;
	f2 += arr[base2+4] * -0.591272f;
	f2 += arr[base2+5] * -0.057544f;
	f2 += arr[base2+6] * 0.091272f;

	// save elements:

	out[myID.x + constMemory->bufferStride*(myID.y*2)] = f1.x;
	out[myID.x + constMemory->bufferStride*(myID.y*2+1)] = f1.y;

	out[myID.x+half + constMemory->bufferStride*(myID.y*2)] = f2.x;
	out[myID.x+half + constMemory->bufferStride*(myID.y*2+1)] = f2.y;

	//out[myID.x*2 + constMemory->bufferStride*myID.y] = arr[myID.x*2 + 8];
	//out[myID.x*2+1 + constMemory->bufferStride*myID.y] = arr[myID.x*2+1 + 8];
}



__kernel void horizonConvolutionAsyncWide2(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float2* localMemory,
	int nParts)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	for (int i = 0; i < nParts; i++)
	{
		float2 l1;
		l1.x = in[myID.x + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*2)];
		l1.y = in[myID.x + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*2+1)];
		localMemory[myID.x + 4 + i*get_global_size(0)*2] = l1;
		float2 l2;
		l2.x = in[myID.x+get_global_size(0) + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*2)];
		l2.y = in[myID.x+get_global_size(0) + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*2+1)];
		localMemory[myID.x + 4 + get_global_size(0) + i*get_global_size(0)*2] = l2;

		if (myID.x <= 4 && myID.x >= 1 && i == 0)
			localMemory[4 - myID.x] = l1;
		if (myID.x >= get_global_size(0)-5 && i == nParts-1)
			localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	}
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float2* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	for (int i = 0; i < 1; i++)
	{
		int base = myID.x*2 + i*get_global_size(0)*2;
		float2 f1 = arr[base] * 0.026749f;
		f1 += arr[base+1] * 0.016864f;
		f1 += arr[base+2] * -0.078223f;
		f1 += arr[base+3] * -0.266864f;
		f1 += arr[base+4] * 0.602949f;
		f1 += arr[base+5] * -0.266864f;
		f1 += arr[base+6] * -0.078223f;
		f1 += arr[base+7] * 0.016864f;
		f1 += arr[base+8] * 0.026749f;
	
		int base2 = myID.x*2+2 + i*get_global_size(0)*2;
		float2 f2 = arr[base2] * -0.091272f;
		f2 += arr[base2+1] * -0.057544f;
		f2 += arr[base2+1] * 0.591272f;
		f2 += arr[base2+1] * 1.115087f;
		f2 += arr[base2+1] * 0.591272f;
		f2 += arr[base2+1] * -0.057544f;
		f2 += arr[base2+1] * -0.091272f;

		// save elements:
		out[myID.x + i*get_global_size(0) + constMemory->bufferStride*(myID.y*2)] = f1.x;
		out[myID.x + i*get_global_size(0) + constMemory->bufferStride*(myID.y*2+1)] = f1.y;
		out[myID.x+half + i*get_global_size(0) + constMemory->bufferStride*(myID.y*2)] = f2.x;
		out[myID.x+half + i*get_global_size(0) + constMemory->bufferStride*(myID.y*2+1)] = f2.y;
	}
}


__kernel void horizonConvolutionAsync4(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float4* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	float4 l1;
	l1.x = in[myID.x + constMemory->bufferStride*(get_global_id(1)*4)];
	l1.y = in[myID.x + constMemory->bufferStride*(get_global_id(1)*4+1)];
	l1.z = in[myID.x + constMemory->bufferStride*(get_global_id(1)*4+2)];
	l1.w = in[myID.x + constMemory->bufferStride*(get_global_id(1)*4+3)];
	localMemory[myID.x + 4] = l1;
	float4 l2;
	l2.x = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4)];
	l2.y = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+1)];
	l2.z = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+2)];
	l2.w = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+3)];
	localMemory[myID.x + 4 + get_global_size(0)] = l2;

	if (myID.x <= 4 && myID.x >= 1)
		localMemory[4 - myID.x] = l1;
	if (myID.x >= get_global_size(0)-5)
		localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float4* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	int base = myID.x*2;
	float4 f1 = arr[base] * 0.026749f;
	f1 += arr[base+1] * -0.016864f;
	f1 += arr[base+2] * -0.078223f;
	f1 += arr[base+3] * 0.266864f;
	f1 += arr[base+4] * 0.602949f;
	f1 += arr[base+5] * 0.266864f;
	f1 += arr[base+6] * -0.078223f;
	f1 += arr[base+7] * -0.016864f;
	f1 += arr[base+8] * 0.026749f;
	
	int base2 = myID.x*2+2;
	float4 f2 = arr[base2] * 0.091272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+2] * -0.591272f;
	f2 += arr[base2+3] * 1.115087f;
	f2 += arr[base2+4] * -0.591272f;
	f2 += arr[base2+5] * -0.057544f;
	f2 += arr[base2+6] * 0.091272f;

	// save elements:
	out[myID.x + constMemory->bufferStride*(myID.y*4)] = f1.x;
	out[myID.x + constMemory->bufferStride*(myID.y*4+1)] = f1.y;
	out[myID.x + constMemory->bufferStride*(myID.y*4+2)] = f1.z;
	out[myID.x + constMemory->bufferStride*(myID.y*4+3)] = f1.w;
	
	out[myID.x+half + constMemory->bufferStride*(myID.y*4)] = f2.x;
	out[myID.x+half + constMemory->bufferStride*(myID.y*4+1)] = f2.y;	
	out[myID.x+half + constMemory->bufferStride*(myID.y*4+2)] = f2.z;
	out[myID.x+half + constMemory->bufferStride*(myID.y*4+3)] = f2.w;

	//out[myID.x*2 + constMemory->bufferStride*myID.y] = arr[myID.x*2 + 8];
	//out[myID.x*2+1 + constMemory->bufferStride*myID.y] = arr[myID.x*2+1 + 8];
}



__kernel void horizonConvolutionAsyncWide4(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float4* localMemory,
	int nParts)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	for (int i = 0; i < nParts; i++)
	{
		float4 l1;
		l1.x = in[myID.x + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*4)];
		l1.y = in[myID.x + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*4+1)];
		l1.z = in[myID.x + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*4+2)];
		l1.w = in[myID.x + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*4+3)];
		localMemory[myID.x + 4 + i*get_global_size(0)*2] = l1;
		float4 l2;
		l2.x = in[myID.x+get_global_size(0) + i*get_global_size(0)*2 + constMemory->bufferStride*get_global_id(1)*4];
		l2.y = in[myID.x+get_global_size(0) + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*4+1)];
		l2.z = in[myID.x+get_global_size(0) + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*4+2)];
		l2.w = in[myID.x+get_global_size(0) + i*get_global_size(0)*2 + constMemory->bufferStride*(get_global_id(1)*4+3)];
		localMemory[myID.x + 4 + get_global_size(0) + i*get_global_size(0)*2] = l2;

		if (myID.x <= 4 && myID.x >= 1 && i == 0)
			localMemory[4 - myID.x] = l1;
		if (myID.x >= get_global_size(0)-5 && i == nParts-1)
			localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	}
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float4* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	for (int i = 0; i < nParts; i++)
	{
		int base = myID.x*2 + i*get_global_size(0)*2;
		float4 f1 = arr[base] * 0.026749f;
		f1 += arr[base+1] * 0.016864f;
		f1 += arr[base+2] * -0.078223f;
		f1 += arr[base+3] * -0.266864f;
		f1 += arr[base+4] * 0.602949f;
		f1 += arr[base+5] * -0.266864f;
		f1 += arr[base+6] * -0.078223f;
		f1 += arr[base+7] * 0.016864f;
		f1 += arr[base+8] * 0.026749f;
	
		int base2 = myID.x*2+2 + i*get_global_size(0)*2;
		float4 f2 = arr[base2] * -0.091272f;
		f2 += arr[base2+1] * -0.057544f;
		f2 += arr[base2+2] * 0.591272f;
		f2 += arr[base2+3] * 1.115087f;
		f2 += arr[base2+4] * 0.591272f;
		f2 += arr[base2+5] * -0.057544f;
		f2 += arr[base2+6] * -0.091272f;

		// save elements:

		out[myID.x + i*get_global_size(0) + constMemory->bufferStride*(myID.y*4)] = f1.x;
		out[myID.x + i*get_global_size(0) + constMemory->bufferStride*(myID.y*4+1)] = f1.y;
		out[myID.x + i*get_global_size(0) + constMemory->bufferStride*(myID.y*4+2)] = f1.z;
		out[myID.x + i*get_global_size(0) + constMemory->bufferStride*(myID.y*4+3)] = f1.w;
	
		out[myID.x+half + i*get_global_size(0) + constMemory->bufferStride*(myID.y*4)] = f2.x;
		out[myID.x+half + i*get_global_size(0) + constMemory->bufferStride*(myID.y*4+1)] = f2.y;	
		out[myID.x+half + i*get_global_size(0) + constMemory->bufferStride*(myID.y*4+2)] = f2.z;
		out[myID.x+half + i*get_global_size(0) + constMemory->bufferStride*(myID.y*4+3)] = f2.w;
	}
}



__kernel void verticalConvolutionAsync(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	float l1 = in[myID.y + constMemory->bufferStride*get_global_id(0)];
	localMemory[myID.x + 4] = l1;
	float l2 = in[myID.y+get_global_size(1) + constMemory->bufferStride*get_global_id(0)];
	localMemory[myID.x + 4 + get_global_size(0)] = l2;

	if (myID.x <= 4 && myID.x >= 1)
		localMemory[4 - myID.x] = l1;
	if (myID.x >= get_global_size(0)-5)
		localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.y;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	int base = myID.x*2;
	float f1 = arr[base] * 0.026749f;
	f1 += arr[base+1] * -0.016864f;
	f1 += arr[base+2] * -0.078223f;
	f1 += arr[base+3] * 0.266864f;
	f1 += arr[base+4] * 0.602949f;
	f1 += arr[base+5] * 0.266864f;
	f1 += arr[base+6] * -0.078223f;
	f1 += arr[base+7] * -0.016864f;
	f1 += arr[base+8] * 0.026749f;
	
	int base2 = myID.x*2+2;
	float f2 = arr[base2] * 0.091272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+2] * -0.591272f;
	f2 += arr[base2+3] * 1.115087f;
	f2 += arr[base2+4] * -0.591272f;
	f2 += arr[base2+5] * -0.057544f;
	f2 += arr[base2+6] * 0.091272f;

	// save elements:

	out[myID.y + constMemory->bufferStride*myID.x] = f1;
	out[myID.y+half + constMemory->bufferStride*myID.x] = f2;
	
	//out[myID.x*2 + constMemory->bufferStride*myID.y] = arr[myID.x*2 + 8];
	//out[myID.x*2+1 + constMemory->bufferStride*myID.y] = arr[myID.x*2+1 + 8];
}



__kernel void verticalConvolutionAsyncWide(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory,
	int nParts)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	for (int i = 0; i < nParts; i++)
	{
		float l1 = in[myID.y + constMemory->bufferStride*get_global_id(0)];
		localMemory[myID.x + 4] = l1;
		float l2 = in[myID.y+get_global_size(1) + constMemory->bufferStride*get_global_id(0)];
		localMemory[myID.x + 4 + get_global_size(0)] = l2;

		if (myID.x <= 4 && myID.x >= 1 && i == 0)
			localMemory[4 - myID.x] = l1;
		if (myID.x >= get_global_size(0)-5 && i == nParts-1)
			localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	}
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.y;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	for (int i = 0; i < nParts; i++)
	{
		int base = myID.x*2 + i*get_global_size(0)*2;
		float f1 = arr[base] * 0.026749f;
		f1 += arr[base+1] * 0.016864f;
		f1 += arr[base+2] * -0.078223f;
		f1 += arr[base+3] * -0.266864f;
		f1 += arr[base+4] * 0.602949f;
		f1 += arr[base+5] * -0.266864f;
		f1 += arr[base+6] * -0.078223f;
		f1 += arr[base+7] * 0.016864f;
		f1 += arr[base+8] * 0.026749f;
	
		int base2 = myID.x*2+2 + i*get_global_size(0)*2;
		float f2 = arr[base2] * -0.091272f;
		f2 += arr[base2+1] * -0.057544f;
		f2 += arr[base2+2] * 0.591272f;
		f2 += arr[base2+3] * 1.115087f;
		f2 += arr[base2+4] * 0.591272f;
		f2 += arr[base2+5] * -0.057544f;
		f2 += arr[base2+6] * -0.091272f;

		// save elements:

		out[myID.y + i*get_global_id(1) + constMemory->bufferStride*myID.x] = f1;
		out[myID.y+half + i*get_global_id(1) + constMemory->bufferStride*myID.x] = f2;
	}
}


__kernel void verticalOverlappedTiles(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 gid = {get_global_id(0), get_global_id(1)};
	int2 lid = {get_local_id(0), get_local_id(1)};
	int2 memBlockSize = {32, 64+8};
	int half = constMemory->halfResolution.y;
	// load elements:
	float l1 = in[gid.x + constMemory->bufferStride*(lid.y + get_local_size(1)*get_group_id(1)*2)];
	localMemory[lid.y + 4 + lid.x*memBlockSize.y] = l1;
	float l2 = in[gid.x + constMemory->bufferStride*(lid.y + get_local_size(1)*get_group_id(1)*2 + get_local_size(1))];
	localMemory[lid.y + 4 + get_local_size(1) + lid.x*memBlockSize.y] = l2;
	if (get_group_id(1) == 0)
	{
		if (lid.y <= 4 && lid.y >= 1)
			localMemory[4 - lid.y + lid.x*memBlockSize.y] = l1;
	}
	else
	{
		if (lid.y <= 4 && lid.y >= 1)
		{
			float l1 = in[gid.x + constMemory->bufferStride*(lid.y + get_group_id(1)*get_local_size(1)*2 - 4)];
			localMemory[lid.y + lid.x*memBlockSize.y] = l1;
		}
	}

	if (get_group_id(1) == get_num_groups(1)-1)
	{
		if (gid.y >= get_global_size(1)-5 && gid.y <= get_global_size(1)-2)
			localMemory[4 + get_local_size(0)*3 - lid.y - 2 + lid.x*memBlockSize.y] = l2;
	}
	else
	{
		if (lid.y <= 3)
		{
			float l1 = in[gid.x + constMemory->bufferStride*(gid.y + get_local_size(1)*2)];
			localMemory[lid.y + 4 + get_local_size(0)*2 + lid.x*memBlockSize.y] = l1;
		}
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	__local float* arr = localMemory;

	int base = lid.y*2 + lid.x*memBlockSize.y;
	float f1 = arr[base] * 0.026749f;
	f1 += arr[base+1] * -0.016864f;
	f1 += arr[base+2] * -0.078223f;
	f1 += arr[base+3] * 0.266864f;
	f1 += arr[base+4] * 0.602949f;
	f1 += arr[base+5] * 0.266864f;
	f1 += arr[base+6] * -0.078223f;
	f1 += arr[base+7] * -0.016864f;
	f1 += arr[base+8] * 0.026749f;

	int base2 = lid.y*2+2 + lid.x*memBlockSize.y;
	float f2 = arr[base2] * 0.091272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+2] * -0.591272f;
	f2 += arr[base2+3] * 1.115087f;
	f2 += arr[base2+4] * -0.591272f;
	f2 += arr[base2+5] * -0.057544f;
	f2 += arr[base2+6] * 0.091272f;

	out[gid.x + constMemory->bufferStride*(gid.y)] = f1;
	out[gid.x + constMemory->bufferStride*(gid.y+half)] = f2;

	//float f1 = arr[lid.y*2 + 7 + lid.x*memBlockSize.y];
	//float f2 = arr[lid.y*2 + 8 + lid.x*memBlockSize.y];
	//out[gid.x + constMemory->bufferStride*(gid.y*2)] = f1;
	//out[gid.x + constMemory->bufferStride*(gid.y*2+1)] = f2;
}



__kernel void transpose(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int offGlobalIn = get_global_id(0) + get_global_id(1)*constMemory->bufferStride;
	int offGlobalOut = get_global_id(0) + get_global_id(1)*constMemory->bufferStride;
		/*(get_group_id(1)*get_local_size(1)+get_local_id(0))*constMemory->bufferStride
		+ get_group_id(0)*get_local_size(0) + get_local_id(1);*/
	int offLocalIn = get_local_id(0) + get_local_id(1)*get_local_size(0);
	int offLocalOut = get_local_id(1) + get_local_id(0)*get_local_size(1);

	float value = in[offGlobalIn];
	localMemory[offLocalIn] = value;

	barrier(CLK_LOCAL_MEM_FENCE);

	value = localMemory[offLocalOut];
	out[offGlobalOut] = value;
}


__kernel void transpose2x2(
	__global read_only float2* in,
	__global write_only float2* out,
	__constant StructConstMemory* constMemory,
	__local float2* localMemory)
{
	int half = constMemory->halfResolution.x;

	int offGlobal1 = (get_global_id(0)) + get_global_id(1)*constMemory->bufferStride;
	int offGlobal2 = (get_global_id(0)) + get_global_id(1)*constMemory->bufferStride+half;

	int offLocalIn1 = get_local_id(0) + (get_local_id(1)*2)*get_local_size(0);
	int offLocalIn2 = get_local_id(0) + (get_local_id(1)*2+1)*get_local_size(0);
	int offLocalOut1 = get_local_id(1) + (get_local_id(0)*2)*get_local_size(1);
	int offLocalOut2 = get_local_id(1) + (get_local_id(0)*2+1)*get_local_size(1);

	float2 v1 = ((__global float2*) in)[offGlobal1];
	localMemory[offLocalIn1] = v1;
	float2 v2 = ((__global float2*) in)[offGlobal2];
	localMemory[offLocalIn2] = v2;

	barrier(CLK_LOCAL_MEM_FENCE);

	v1 = localMemory[offLocalOut1];
	out[offGlobal1] = v1;
	v2 = localMemory[offLocalOut2];
	out[offGlobal2] = v2;
}


__kernel void horizonConvolutionAsyncTransposed(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int half = constMemory->halfResolution.x;
	int2 myID = {get_global_id(0), get_global_id(1)};

	// load elements:
	int2 globalID1 = {
		((myID.x)%32) + ((myID.y) & 0xffffffe0),
		((myID.y)%32) + ((myID.x) & 0xffffffe0)};
	int2 globalID2 = {
		((myID.x+half)%32) + ((myID.y) & 0xffffffe0),
		((myID.y)%32) + ((myID.x+half) & 0xffffffe0)};
	
	float l1 = in[globalID1.x + globalID1.y*constMemory->bufferStride];
	localMemory[myID.x + 4] = l1;
	float l2 = in[globalID2.x + globalID2.y*constMemory->bufferStride];
	localMemory[myID.x + 4 + get_global_size(0)] = l2;

	if (myID.x <= 4 && myID.x >= 1)
		localMemory[4 - myID.x] = l1;
	if (myID.x >= get_global_size(0)-5)
		localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	int base = myID.x*2;
	float f1 = arr[base] * 0.026749f;
	f1 += arr[base+1] * -0.016864f;
	f1 += arr[base+2] * -0.078223f;
	f1 += arr[base+3] * 0.266864f;
	f1 += arr[base+4] * 0.602949f;
	f1 += arr[base+5] * 0.266864f;
	f1 += arr[base+6] * -0.078223f;
	f1 += arr[base+7] * -0.016864f;
	f1 += arr[base+8] * 0.026749f;
	
	int base2 = myID.x*2+2;
	float f2 = arr[base2] * 0.091272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+2] * -0.591272f;
	f2 += arr[base2+3] * 1.115087f;
	f2 += arr[base2+4] * -0.591272f;
	f2 += arr[base2+5] * -0.057544f;
	f2 += arr[base2+6] * 0.091272f;

	// save elements:
	int2 globalID1Out = {
		((myID.x)%32) + (myID.y & 0xffffffe0),
		((myID.y)%32) + (myID.x & 0xffffffe0)};
	int2 globalID2Out = {
		((myID.x+half)%32) + ((myID.y) & 0xffffffe0),
		((myID.y)%32) + ((myID.x+half) & 0xffffffe0)};
	out[globalID1Out.x + globalID1Out.y*constMemory->bufferStride] = f1;
	out[globalID2Out.x + globalID2Out.y*constMemory->bufferStride] = f2;
	
	//out[myID.x*2 + constMemory->bufferStride*myID.y] = arr[myID.x*2 + 8];
	//out[myID.x*2+1 + constMemory->bufferStride*myID.y] = arr[myID.x*2+1 + 8];
}




__kernel void horizonConvolutionAsyncWideTransposed(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory,
	int nParts)
{
	int half = constMemory->halfResolution.x;
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	for (int i = 0; i < nParts; i++)
	{
		int2 globalID1 = {
			((myID.x)&0x1f) + ((myID.y) & 0xffffffe0),
			((myID.y)&0x1f) + ((myID.x+2*i*get_global_size(0)) & 0xffffffe0)};
		int2 globalID2 = {
			((myID.x)&0x1f) + ((myID.y) & 0xffffffe0),
			((myID.y)&0x1f) + ((myID.x+get_global_size(0)+2*i*get_global_size(0)) & 0xffffffe0)};
	
		float l1 = in[globalID1.x + globalID1.y*constMemory->bufferStride];
		localMemory[myID.x + 4 + 2*i*get_global_size(0)] = l1;
		float l2 = in[globalID2.x + globalID2.y*constMemory->bufferStride];
		localMemory[myID.x + 4 + get_global_size(0) + 2*i*get_global_size(0)] = l2;

		if (myID.x <= 4 && myID.x >= 1 && i == 0)
			localMemory[4 - myID.x] = l1;
		if (myID.x >= get_global_size(0)-5 && i == nParts-1)
			localMemory[4 + constMemory->resolution.x + get_global_size(0) - myID.x - 2] = l2;
	}
	barrier(CLK_LOCAL_MEM_FENCE);


	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	for (int i = 0; i < nParts; i++)
	{
		int base = myID.x*2 + i*get_global_size(0)*2;
		float f1 = arr[base] * 0.026749f;
		f1 += arr[base+1] * -0.016864f;
		f1 += arr[base+2] * -0.078223f;
		f1 += arr[base+3] * 0.266864f;
		f1 += arr[base+4] * 0.602949f;
		f1 += arr[base+5] * 0.266864f;
		f1 += arr[base+6] * -0.078223f;
		f1 += arr[base+7] * -0.016864f;
		f1 += arr[base+8] * 0.026749f;
	
		int base2 = myID.x*2+2 + i*get_global_size(0)*2;
		float f2 = arr[base2] * 0.091272f;
		f2 += arr[base2+1] * -0.057544f;
		f2 += arr[base2+1] * -0.591272f;
		f2 += arr[base2+1] * 1.115087f;
		f2 += arr[base2+1] * -0.591272f;
		f2 += arr[base2+1] * -0.057544f;
		f2 += arr[base2+1] * 0.091272f;

		// save elements:
		int2 globalID1Out = {
			((myID.x)&0x1f) + (myID.y & 0xffffffe0),
			((myID.y)&0x1f) + ((myID.x + i*get_global_size(0)) & 0xffffffe0)};
		int2 globalID2Out = {
			((myID.x)&0x1f) + ((myID.y) & 0xffffffe0),
			((myID.y)&0x1f) + ((myID.x+half + i*get_global_size(0)) & 0xffffffe0)};
		out[globalID1Out.x + constMemory->bufferStride*globalID1Out.y] = f1;
		out[globalID2Out.x + constMemory->bufferStride*globalID2Out.y] = f2;
	}
}

#define MASK_ADDRESS(ADDR) (ADDR&(1024*4-1))

__kernel void verticalSlidingWindow(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 gid = {get_global_id(0), get_global_id(1)};
	int2 lid = {get_local_id(0), get_local_id(1)};
	int2 memBlockSize = {32, 64};
	int half = constMemory->halfResolution.y;
	// load elements:
	float l1 = in[gid.x + constMemory->bufferStride*(lid.y + get_local_size(1)*get_group_id(1)*2)];
	localMemory[lid.x + lid.y*memBlockSize.x] = l1;
	float l2 = in[gid.x + constMemory->bufferStride*(lid.y + get_local_size(1)*get_group_id(1)*2 + get_local_size(1))];
	localMemory[lid.x + (lid.y + get_local_size(1))*memBlockSize.x] = l2;

	if (lid.y <= 4 && lid.y >= 1)
		localMemory[MASK_ADDRESS(lid.x - lid.y*memBlockSize.x)] = l1;
	if (lid.y <= 3)
	{
		float l1 = in[gid.x + constMemory->bufferStride*(gid.y + get_local_size(1)*2)];
		localMemory[lid.x + (get_local_size(1)*2 + lid.y)*memBlockSize.x] = l1;
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	__local float* arr = localMemory;

	uint offset = memBlockSize.x;
	uint base = lid.x + lid.y*memBlockSize.x*2 + offset*(-4);
	float f1 = arr[MASK_ADDRESS(base)] * 0.026749f;
	f1 += arr[MASK_ADDRESS(base+offset)] * -0.016864f;
	f1 += arr[MASK_ADDRESS(base+offset*2)] * -0.078223f;
	f1 += arr[MASK_ADDRESS(base+offset*3)] * 0.266864f;
	f1 += arr[base+offset*4] * 0.602949f;
	f1 += arr[base+offset*5] * 0.266864f;
	f1 += arr[base+offset*6] * -0.078223f;
	f1 += arr[base+offset*7] * -0.016864f;
	f1 += arr[base+offset*8] * 0.026749f;

	int base2 = lid.x + lid.y*memBlockSize.x*2 + offset*(-2);
	float f2 = arr[MASK_ADDRESS(base2)] * 0.091272f;
	f2 += arr[MASK_ADDRESS(base2+offset)] * -0.057544f;
	f2 += arr[base2+offset*2] * -0.591272f;
	f2 += arr[base2+offset*3] * 1.115087f;
	f2 += arr[base2+offset*4] * -0.591272f;
	f2 += arr[base2+offset*5] * -0.057544f;
	f2 += arr[base2+offset*6] * 0.091272f;
	//float f2 = arr[MASK_ADDRESS(base2+offset*3)];
	//float f1 = arr[MASK_ADDRESS(base+offset*4)];
	out[gid.x + constMemory->bufferStride*(gid.y)] = f1;
	out[gid.x + constMemory->bufferStride*(gid.y+half)] = f2;


	int nWindowPos = (constMemory->resolution.y / memBlockSize.y) - 1;
	for (int i = 1; i < nWindowPos; i++)
	{
		int globalOff = (gid.x + constMemory->bufferStride*(lid.y + 4 + i*memBlockSize.y));
		int localOff = MASK_ADDRESS(lid.x + (lid.y+4)*memBlockSize.x + i*2048); 
		float l1 = in[globalOff];
		//l1 = get_local_id(1);
		localMemory[localOff] = l1;
		float l2 = in[globalOff + constMemory->bufferStride*get_local_size(1)];
		//l2 = get_local_id(1);
		localMemory[MASK_ADDRESS(localOff + memBlockSize.x*get_local_size(1))] = l2;
		barrier(CLK_LOCAL_MEM_FENCE);

		uint offset = memBlockSize.x;
		uint base = lid.x + lid.y*memBlockSize.x*2 + offset*(i*memBlockSize.y-4);
		float f1 = arr[MASK_ADDRESS(base)] * 0.026749f;
		f1 += arr[MASK_ADDRESS(base+offset)] * -0.016864f;
		f1 += arr[MASK_ADDRESS(base+offset*2)] * -0.078223f;
		f1 += arr[MASK_ADDRESS(base+offset*3)] * 0.266864f;
		f1 += arr[MASK_ADDRESS(base+offset*4)] * 0.602949f;
		f1 += arr[MASK_ADDRESS(base+offset*5)] * 0.266864f;
		f1 += arr[MASK_ADDRESS(base+offset*6)] * -0.078223f;
		f1 += arr[MASK_ADDRESS(base+offset*7)] * -0.016864f;
		f1 += arr[MASK_ADDRESS(base+offset*8)] * 0.026749f;

		int base2 = lid.x + lid.y*memBlockSize.x*2 + offset*(i*memBlockSize.y+2-4);
		float f2 = arr[MASK_ADDRESS(base2)] * 0.091272f;
		f2 += arr[MASK_ADDRESS(base2+offset)] * -0.057544f;
		f2 += arr[MASK_ADDRESS(base2+offset*2)] * -0.591272f;
		f2 += arr[MASK_ADDRESS(base2+offset*3)] * 1.115087f;
		f2 += arr[MASK_ADDRESS(base2+offset*4)] * -0.591272f;
		f2 += arr[MASK_ADDRESS(base2+offset*5)] * -0.057544f;
		f2 += arr[MASK_ADDRESS(base2+offset*6)] * 0.091272f;
		//float f2 = arr[MASK_ADDRESS(base2+offset*3)];
		//float f1 = arr[MASK_ADDRESS(base+offset*4)];

		out[gid.x + (gid.y+i*32)*constMemory->bufferStride] = f1;
		out[gid.x + (gid.y+i*32+half)*constMemory->bufferStride] = f2;
	}

	{
		int globalOff = (gid.x + constMemory->bufferStride*(constMemory->resolution.y - get_global_size(1)*2 + gid.y));
		int localOff = MASK_ADDRESS(lid.x + memBlockSize.x*(constMemory->resolution.y - get_global_size(1)*2 + gid.y));
		if (get_local_id(1) >= 4)
		{	
			float l1 = in[globalOff];
			l1 = get_local_id(1);
			localMemory[localOff] = l1;
		}
		float l2 = in[globalOff + constMemory->bufferStride*get_global_size(1)];
		//l2 = get_local_id(1);
		localMemory[localOff + get_local_size(0)*get_local_size(1)] = l2;
		if (gid.y >= get_global_size(1)-5 && gid.y <= get_global_size(1)-2)
			localMemory[MASK_ADDRESS(lid.x + memBlockSize.x*(get_local_size(1)*3 - lid.y - 2)+nWindowPos*2048)] = l2;
	
		barrier(CLK_LOCAL_MEM_FENCE);

		uint base = lid.x + lid.y*memBlockSize.x*2 + offset*(nWindowPos*memBlockSize.y-4);
		float f1 = arr[MASK_ADDRESS(base)] * 0.026749f;
		f1 += arr[MASK_ADDRESS(base+offset)] * -0.016864f;
		f1 += arr[MASK_ADDRESS(base+offset*2)] * -0.078223f;
		f1 += arr[MASK_ADDRESS(base+offset*3)] * 0.266864f;
		f1 += arr[MASK_ADDRESS(base+offset*4)] * 0.602949f;
		f1 += arr[MASK_ADDRESS(base+offset*5)] * 0.266864f;
		f1 += arr[MASK_ADDRESS(base+offset*6)] * -0.078223f;
		f1 += arr[MASK_ADDRESS(base+offset*7)] * -0.016864f;
		f1 += arr[MASK_ADDRESS(base+offset*8)] * 0.026749f;

		int base2 = lid.x + lid.y*memBlockSize.x*2 + offset*(nWindowPos*memBlockSize.y+2-4);
		float f2 = arr[MASK_ADDRESS(base2)] * 0.091272f;
		f2 += arr[MASK_ADDRESS(base2+offset)] * -0.057544f;
		f2 += arr[MASK_ADDRESS(base2+offset*2)] * -0.591272f;
		f2 += arr[MASK_ADDRESS(base2+offset*3)] * 1.115087f;
		f2 += arr[MASK_ADDRESS(base2+offset*4)] * -0.591272f;
		f2 += arr[MASK_ADDRESS(base2+offset*5)] * -0.057544f;
		f2 += arr[MASK_ADDRESS(base2+offset*6)] * 0.091272f;

		//float f2 = arr[MASK_ADDRESS(base2+offset*3)];
		//float f1 = arr[MASK_ADDRESS(base+offset*4)];
		out[gid.x + constMemory->bufferStride*(gid.y+nWindowPos*32)] = f1;
		out[gid.x + constMemory->bufferStride*(gid.y+nWindowPos*32+half)] = f2;
	}
}

