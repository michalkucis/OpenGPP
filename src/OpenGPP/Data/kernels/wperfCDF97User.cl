#define CDF97_ALPHA (-1.586134342f)
#define CDF97_BETA (-0.05298011854f)
#define CDF97_GAMMA (0.8829110762f)
#define CDF97_DELTA (0.4435068522)
#define CDF97_ZETA (0.70710678118f)
#define CDF97_INVZETA (1.41421356237f)

  
typedef struct 
{
	int2 resolution;
	int2 halfResolution;
	int bufferStride;

} StructConstMemory;



__kernel void copyGlobalLocalGlobal(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	//int2 myID = {get_global_id(0), get_global_id(1)};
	//// load elements:
	//localMemory[myID.x] = in[myID.x + constMemory->bufferStride*myID.y];
	//localMemory[myID.x+get_global_size(0)] = in[myID.x+get_global_size(0) + constMemory->bufferStride*myID.y];
	//barrier(CLK_LOCAL_MEM_FENCE);
	//
	//// init variables:
	//__local float* arr = localMemory;
	//int stride = constMemory->bufferStride;
	//int half = constMemory->halfResolution.x;
	//int resX = constMemory->resolution.x;
	//int resY = constMemory->resolution.y;
	//int nd1 = myID.x * 2;
	//int nd2 = nd1 + 1 >= resX ? nd1 - 1 : nd1 + 1;

	//out[myID.x + constMemory->bufferStride*myID.y] = arr[nd1];
	//out[myID.x+half + constMemory->bufferStride*myID.y] = arr[nd2];

	int offsetLocalIn1 = get_local_id(0)*2 + get_local_size(0)*get_local_id(1);
	int offsetLocalIn2 = get_local_id(0)*2+1 + get_local_size(0)*get_local_id(1);
	int offsetLocalOut1 = get_local_id(0) + get_local_size(0)*get_local_id(1);
	int offsetLocalOut2 = get_local_id(0)+get_local_size(0) + get_local_size(0)*get_local_id(1);
	int offsetGlobal1 = get_global_id(0) + constMemory->bufferStride*get_global_id(1);
	int offsetGlobal2 = get_global_id(0) + constMemory->bufferStride*get_global_id(1);
	//int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[offsetLocalIn1] = in[offsetGlobal1];
	localMemory[offsetLocalIn2] = in[offsetGlobal2];	
	barrier(CLK_LOCAL_MEM_FENCE);
	
	//// init variables:
	__local float* arr = localMemory;
	//int stride = constMemory->bufferStride;
	//int half = constMemory->halfResolution.y;
	//int resX = constMemory->resolution.x;
	//int resY = constMemory->resolution.y;
	//int nd1 = myID.y * 2;
	//int nd2 = nd1 + 1 >= resY ? nd1 - 1 : nd1 + 1;

	out[offsetGlobal1] = arr[offsetLocalOut1];
	out[offsetGlobal2] = arr[offsetLocalOut2];
}

__kernel void copyGlobalLocalColumn(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[myID.y] = in[myID.x + constMemory->bufferStride*myID.y];
	localMemory[myID.y+get_global_size(1)] = in[myID.x + constMemory->bufferStride*(myID.y+get_global_size(1))];
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.y;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;
	int nd1 = myID.y * 2;
	int nd2 = nd1 + 1 >= resY ? nd1 - 1 : nd1 + 1;

	out[myID.x + constMemory->bufferStride*myID.y] = arr[nd1];
	out[myID.x + constMemory->bufferStride*(myID.y+half)] = arr[nd2];
}



__kernel void copyGlobalLocalTide(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int offsetLocal1 = get_local_id(0) + get_local_size(0)*(get_local_id(1)*2);
	int offsetLocal2 = get_local_id(0) + get_local_size(0)*(get_local_id(1)*2+1);
	int offsetGlobalIn1 = get_global_id(0) + constMemory->bufferStride*(get_global_id(1)*2);
	int offsetGlobalIn2 = get_global_id(0) + constMemory->bufferStride*(get_global_id(1)*2+1);
	int offsetGlobalOut1 = get_global_id(0) + constMemory->bufferStride*(get_global_id(1));
	int offsetGlobalOut2 = get_global_id(0) + constMemory->bufferStride*(get_global_id(1)+get_global_size(1));
	//int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[offsetLocal1] = in[offsetGlobalIn1];
	localMemory[offsetLocal2] = in[offsetGlobalIn2];	
	barrier(CLK_LOCAL_MEM_FENCE);
	
	//// init variables:
	__local float* arr = localMemory;
	//int stride = constMemory->bufferStride;
	//int half = constMemory->halfResolution.y;
	//int resX = constMemory->resolution.x;
	//int resY = constMemory->resolution.y;
	//int nd1 = myID.y * 2;
	//int nd2 = nd1 + 1 >= resY ? nd1 - 1 : nd1 + 1;

	out[offsetGlobalOut1] = arr[offsetLocal1];
	out[offsetGlobalOut2] = arr[offsetLocal2];
}


__kernel void copyGlobalLocalGlobalAsync(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	event_t eventLoad =	async_work_group_copy(
		localMemory,
		in+constMemory->bufferStride*myID.y,
		constMemory->resolution.x,
		0);
	wait_group_events(1, &eventLoad);
	
	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;
	int nd1 = myID.x * 2;
	int nd2 = nd1 + 1 >= resX ? nd1 - 1 : nd1 + 1;

	out[myID.x + constMemory->bufferStride*myID.y] = arr[nd1];
	out[myID.x+half + constMemory->bufferStride*myID.y] = arr[nd2];
}




//----------------------------------------------------------------



__kernel void horizonUsingPrivateMemory(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	float f1 = in[get_global_id(0)*2 + constMemory->bufferStride*get_global_id(1)];
	float f2 = in[get_global_id(0)*2+1 + constMemory->bufferStride*get_global_id(1)];
	

	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	float c;


	c = CDF97_ALPHA;
	localMemory[myID+0] = f1;
	barrier(CLK_LOCAL_MEM_FENCE);
	float f3 = localMemory[myID+1];
	f2 += c * (f1 + f3);
	

	c = CDF97_BETA;
	localMemory[myID+1] = f2;
	barrier(CLK_LOCAL_MEM_FENCE);
	float f0 = localMemory[myID];
	f1 += c * (f0 + f2);
	

	c = CDF97_GAMMA;
	localMemory[myID+0] = f1;
	barrier(CLK_LOCAL_MEM_FENCE);
	f3 = localMemory[myID+1];
	f2 += c * (f1 + f3);
	

	c = CDF97_DELTA;
	localMemory[myID+1]= f2;
	barrier(CLK_LOCAL_MEM_FENCE);
	f0 = localMemory[myID];
	f1 += c * (f0 + f2);

	
	f1 *= CDF97_INVZETA;
	f2 *= CDF97_ZETA;
	
	
	if (get_global_id(0) >= 2 && get_global_id(0) <= constMemory->halfResolution.x+2)
	{
		out[get_global_id(0) - 2 + constMemory->bufferStride*get_global_id(1)] = f1;
		out[get_global_id(0) - 2 + constMemory->halfResolution.x + constMemory->bufferStride*get_global_id(1)] = f2;
	}
}

__kernel void horizonUsingLocalMemory(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int nd0 = abs(get_global_id(0)*2-1);
	int nd1 = get_global_id(0)*2;
	int nd2 = get_global_id(0)*2+1;
	int nd3 = get_global_id(0)*2+2;
	localMemory[nd1] = in[get_global_id(0)*2 + constMemory->bufferStride*get_global_id(1)];
	localMemory[nd2] = in[get_global_id(0)*2+1 + constMemory->bufferStride*get_global_id(1)];
	//barrier(CLK_LOCAL_MEM_FENCE);
	__local float* arr = localMemory;


	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	float c;


	c = CDF97_ALPHA;
	barrier(CLK_LOCAL_MEM_FENCE);
	arr[nd2] += c * (arr[nd1] + arr[nd3]);
	

	c = CDF97_BETA;
	barrier(CLK_LOCAL_MEM_FENCE);
	arr[nd1] += c * (arr[nd0] + arr[nd2]);
	

	c = CDF97_GAMMA;
	barrier(CLK_LOCAL_MEM_FENCE);
	arr[nd2] += c * (arr[nd1] + arr[nd3]);
	

	c = CDF97_DELTA;
	barrier(CLK_LOCAL_MEM_FENCE);
	arr[nd1] += c * (arr[nd0] + arr[nd2]);

	if (get_global_id(0) >= 2 && get_global_id(0) <= constMemory->halfResolution.x+2)
	{
		out[get_global_id(0) - 2 + constMemory->bufferStride*get_global_id(1)] = arr[nd1] * CDF97_INVZETA;
		out[get_global_id(0) - 2 + constMemory->halfResolution.x + constMemory->bufferStride*get_global_id(1)] = arr[nd2] * CDF97_ZETA;
	}
}











__kernel void horizonMultipleAccess(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[myID.x] = in[myID.x + constMemory->bufferStride*myID.y];
	localMemory[myID.x+get_global_size(0)] = in[myID.x+get_global_size(0) + constMemory->bufferStride*myID.y];
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


	out[myID.x + constMemory->bufferStride*myID.y] = arr[nd1];
	out[myID.x+half + constMemory->bufferStride*myID.y] = arr[nd2];
}

__kernel void horizonExtraBorders(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[myID.x+1] = in[myID.x + constMemory->bufferStride*myID.y];
	localMemory[myID.x+get_global_size(0)+1] = in[myID.x+get_global_size(0) + constMemory->bufferStride*myID.y];
	if (myID.x == 0)
		localMemory[1] = localMemory[0];
	if (myID.x == get_global_size(0)-1)
		localMemory[get_global_size(0)] = localMemory[myID.x];
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
	arr[nd1] += CDF97_ALPHA * (arr[nd0] + arr[nd2]);
	barrier(CLK_LOCAL_MEM_FENCE);
	arr[nd2] += CDF97_BETA * (arr[nd1] + arr[nd3]);
	barrier(CLK_LOCAL_MEM_FENCE);
	arr[nd1] += CDF97_GAMMA * (arr[nd0] + arr[nd2]);
	barrier(CLK_LOCAL_MEM_FENCE);
	arr[nd2] += CDF97_DELTA * (arr[nd1] + arr[nd3]);
	barrier(CLK_LOCAL_MEM_FENCE);


	arr[nd2] *= CDF97_ZETA;
	arr[nd1] *= CDF97_INVZETA;


	out[myID.x + constMemory->bufferStride*myID.y] = arr[nd1];
	out[myID.x+half + constMemory->bufferStride*myID.y] = arr[nd2];
}

__kernel void horizonLiftingSync(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	event_t eventLoad =	async_work_group_copy(
		localMemory,
		in+constMemory->bufferStride*myID.y,
		constMemory->resolution.x,
		0);
	wait_group_events(1, &eventLoad);
	
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
	/*event_t eventSave = async_work_group_copy(
		out+constMemory->bufferStride*myID.y,
		localMemory,
		constMemory->resolution.x,
		0);
		*/

	out[myID.x + constMemory->bufferStride*myID.y] = arr[nd1];
	out[myID.x+half + constMemory->bufferStride*myID.y] = arr[nd2];
}


__kernel void horizonLiftingSync2(
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


__kernel void horizonLiftingSync3(
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
	if (myID.x < 8)
		localMemory[myID.x] = localMemory[16 - myID.x];
	else if (myID.x < 16)
		localMemory[constMemory->resolution.x + myID.x-8] = localMemory[constMemory->resolution.x - myID.x + 7];


	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	int base = myID.x*2 + 4;
	int nd0 = base-1;
	int nd1 = base;
	int nd2 = base+1;
	int nd3 = base+2;

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

	if (myID.x >= 2 && myID.x < half+2)
	{
		out[myID.x + constMemory->bufferStride*myID.y] = arr[nd1];
		out[myID.x+half + constMemory->bufferStride*myID.y] = arr[nd2];
	}
}


__kernel void horizonLiftingAsync(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	float l1 = in[myID.x + constMemory->bufferStride*get_global_id(1)];
	localMemory[myID.x + 4] = l1;
	localMemory[myID.x + 4 + get_global_size(0)] = in[myID.x+get_global_size(0) + constMemory->bufferStride*get_global_id(1)];
	if (myID.x <= 4 && myID.x >= 1)
		localMemory[4 - myID.x] = l1;
	else if (myID.x >= get_global_size(0)-4 && myID.x < get_global_size(0)-1)
		localMemory[constMemory->resolution.x + myID.x-4] = localMemory[constMemory->resolution.x - myID.x + 3];
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


__kernel void horizonConvolutionAsync(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[myID.x + 4] = in[myID.x + constMemory->bufferStride*get_global_id(1)];
	localMemory[myID.x + 4 + get_global_size(0)] = in[myID.x+get_global_size(0) + constMemory->bufferStride*get_global_id(1)];
	barrier(CLK_LOCAL_MEM_FENCE);

	if (myID.x < 4)
		localMemory[myID.x] = localMemory[8 - myID.x];
	else if (myID.x < 8)
		localMemory[constMemory->resolution.x + myID.x-4] = localMemory[constMemory->resolution.x - myID.x + 3];
	barrier(CLK_LOCAL_MEM_FENCE);

	// init variables:
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;

	
	// compute:
	const float coef1[9] = {0.026749f, -0.016864f, -0.078223f, 0.266864f, 0.602949f, 0.266864f, -0.078223f, -0.016864f, 0.026749f};
	const float coef2[7] = {0.091272f, -0.057544f, -0.591272f, 1.115087f, -0.591272f, -0.057544f, 0.091272f};


	int base = myID.x*2;
	float f1 = 0;
	for (int i = 0; i < 9; i++)
		f1 += arr[base+i] * coef1[i];
	
	int base2 = myID.x*2+2;
	float f2 = 0;
	for (int i = 0; i < 7; i++)
		f2 += arr[base2+i] * coef2[i];

	// save elements:

	out[myID.x + constMemory->bufferStride*myID.y] = f1;
	out[myID.x+half + constMemory->bufferStride*myID.y] = f2;
}


__kernel void horizonConvolutionAsyncOpt(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[myID.x + 4] = in[myID.x + constMemory->bufferStride*get_global_id(1)];
	localMemory[myID.x + 4 + get_global_size(0)] = in[myID.x+get_global_size(0) + constMemory->bufferStride*get_global_id(1)];
	barrier(CLK_LOCAL_MEM_FENCE);

	if (myID.x < 4)
		localMemory[myID.x] = localMemory[8 - myID.x];
	else if (myID.x < 8)
		localMemory[constMemory->resolution.x + myID.x-4] = localMemory[constMemory->resolution.x - myID.x + 3];
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
	f1 += arr[base+1] * 0.016864f;
	f1 += arr[base+2] * -0.078223f;
	f1 += arr[base+3] * -0.266864f;
	f1 += arr[base+4] * 0.602949f;
	f1 += arr[base+5] * -0.266864f;
	f1 += arr[base+6] * -0.078223f;
	f1 += arr[base+7] * 0.016864f;
	f1 += arr[base+8] * 0.026749f;
	
	int base2 = myID.x*2+2;
	float f2 = arr[base2] * -0.091272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+1] * 0.591272f;
	f2 += arr[base2+1] * 1.115087f;
	f2 += arr[base2+1] * 0.591272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+1] * -0.091272f;

	// save elements:

	out[myID.x + constMemory->bufferStride*myID.y] = f1;
	out[myID.x+half + constMemory->bufferStride*myID.y] = f2;
}

__kernel void horizonConvolutionAsync2Lines(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float2* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[myID.x + 4].x = in[myID.x + constMemory->bufferStride*(get_global_id(1)*2+0)];
	localMemory[myID.x + 4 + get_global_size(0)].x = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*2+0)];
	localMemory[myID.x + 4].y = in[myID.x + constMemory->bufferStride*(get_global_id(1)*2+1)];
	localMemory[myID.x + 4 + get_global_size(0)].y = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*2+1)];
	barrier(CLK_LOCAL_MEM_FENCE);

	if (myID.x < 4)
		localMemory[myID.x] = localMemory[8 - myID.x];
	else if (myID.x < 8)
		localMemory[constMemory->resolution.x + myID.x-4] = localMemory[constMemory->resolution.x - myID.x + 3];
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
	f1 += arr[base+1] * 0.016864f;
	f1 += arr[base+2] * -0.078223f;
	f1 += arr[base+3] * -0.266864f;
	f1 += arr[base+4] * 0.602949f;
	f1 += arr[base+5] * -0.266864f;
	f1 += arr[base+6] * -0.078223f;
	f1 += arr[base+7] * 0.016864f;
	f1 += arr[base+8] * 0.026749f;
	
	int base2 = myID.x*2+2;
	float2 f2 = arr[base2] * -0.091272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+1] * 0.591272f;
	f2 += arr[base2+1] * 1.115087f;
	f2 += arr[base2+1] * 0.591272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+1] * -0.091272f;

	// save elements:

	out[myID.x + constMemory->bufferStride*(get_global_id(1)*2+0)] = f1.x;
	out[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*2+0)] = f2.x;
	out[myID.x + constMemory->bufferStride*(get_global_id(1)*2+1)] = f1.y;	
	out[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*2+1)] = f2.y;
}

__kernel void horizonConvolutionAsync4Lines(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float4* localMemory)
{
	int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[myID.x + 4].x = in[myID.x + constMemory->bufferStride*(get_global_id(1)*4+0)];
	localMemory[myID.x + 4 + get_global_size(0)].x = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+0)];
	localMemory[myID.x + 4].y = in[myID.x + constMemory->bufferStride*(get_global_id(1)*4+1)];
	localMemory[myID.x + 4 + get_global_size(0)].y = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+1)];
	localMemory[myID.x + 4].z = in[myID.x + constMemory->bufferStride*(get_global_id(1)*4+2)];
	localMemory[myID.x + 4 + get_global_size(0)].z = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+2)];
	localMemory[myID.x + 4].w = in[myID.x + constMemory->bufferStride*(get_global_id(1)*4+3)];
	localMemory[myID.x + 4 + get_global_size(0)].w = in[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+3)];
	barrier(CLK_LOCAL_MEM_FENCE);

	if (myID.x < 4)
		localMemory[myID.x] = localMemory[8 - myID.x];
	else if (myID.x < 8)
		localMemory[constMemory->resolution.x + myID.x-4] = localMemory[constMemory->resolution.x - myID.x + 3];
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
	f1 += arr[base+1] * 0.016864f;
	f1 += arr[base+2] * -0.078223f;
	f1 += arr[base+3] * -0.266864f;
	f1 += arr[base+4] * 0.602949f;
	f1 += arr[base+5] * -0.266864f;
	f1 += arr[base+6] * -0.078223f;
	f1 += arr[base+7] * 0.016864f;
	f1 += arr[base+8] * 0.026749f;
	
	int base2 = myID.x*2+2;
	float4 f2 = arr[base2] * -0.091272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+2] * 0.591272f;
	f2 += arr[base2+3] * 1.115087f;
	f2 += arr[base2+4] * 0.591272f;
	f2 += arr[base2+5] * -0.057544f;
	f2 += arr[base2+6] * -0.091272f;

	// save elements:

	out[myID.x + constMemory->bufferStride*(get_global_id(1)*4+0)] = f1.x;
	out[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+0)] = f2.x;
	out[myID.x + constMemory->bufferStride*(get_global_id(1)*4+1)] = f1.y;	
	out[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+1)] = f2.y;
	out[myID.x + constMemory->bufferStride*(get_global_id(1)*4+2)] = f1.z;
	out[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+2)] = f2.z;	
	out[myID.x + constMemory->bufferStride*(get_global_id(1)*4+3)] = f1.w;
	out[myID.x+get_global_size(0) + constMemory->bufferStride*(get_global_id(1)*4+3)] = f2.w;
}


//-------------------------------------------------------------


__kernel void transposeInTiles(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int offsetLocal1 = get_local_id(0) + get_local_size(0)*(get_local_id(1));
	int offsetLocal2 = get_local_id(1) + get_local_size(1)*(get_local_id(0));
	int offsetGlobal = get_global_id(0) + constMemory->bufferStride*(get_global_id(1));
	//int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[offsetLocal1] = in[offsetGlobal];	
	barrier(CLK_LOCAL_MEM_FENCE);
	
	//// init variables:
	__local float* arr = localMemory;

	out[offsetGlobal] = arr[offsetLocal2];
}

__kernel void transposeInTiles2(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int offsetLocal1 = mad24(get_local_size(0),get_local_id(1),get_local_id(0));
	int offsetLocal2 = mad24(get_local_size(1),get_local_id(0),get_local_id(1));
	int offsetGlobal = mad24(convert_uint(constMemory->bufferStride),get_global_id(1),get_global_id(0));
	//int2 myID = {get_global_id(0), get_global_id(1)};
	// load elements:
	localMemory[offsetLocal1] = in[offsetGlobal];	
	barrier(CLK_LOCAL_MEM_FENCE);
	
	//// init variables:
	__local float* arr = localMemory;

	out[offsetGlobal] = arr[offsetLocal2];
}


__kernel void transpose(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int offGlobalIn = get_global_id(0) + get_global_id(1)*constMemory->bufferStride;
	int offGlobalOut = (get_group_id(1)*get_local_size(1)+get_local_id(0))*constMemory->bufferStride
		+ get_group_id(0)*get_local_size(0) + get_local_id(1);
	int offLocalIn = get_local_id(0) + get_local_id(1)*get_local_size(0);
	int offLocalOut = get_local_id(1) + get_local_id(0)*get_local_size(1);

	float value = in[offGlobalIn];
	localMemory[offLocalIn] = value;

	barrier(CLK_LOCAL_MEM_FENCE);

	value = localMemory[offLocalOut];
	out[offGlobalOut] = value;
}

__kernel void transposeNaive(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	int offGlobalIn = get_global_id(0) + get_global_id(1)*constMemory->bufferStride;
	int offGlobalOut = get_global_id(1) + get_global_id(0)*get_global_size(1);

	float value = in[offGlobalIn];
	out[offGlobalOut] = value;
}

//-----------------------------------------------------------


__kernel void verticalNaive(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* localMemory)
{
	// init variables:
	int2 myID = {get_global_id(0), get_global_id(1)};
	__local float* arr = localMemory;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.y;
	int resX = constMemory->resolution.x;
	int resY = constMemory->resolution.y;
	int nd0 = abs (myID.y * 2 - 1);
	int nd1 = myID.y * 2;
	int nd2 = nd1 + 1 >= resY ? nd1 - 1 : nd1 + 1;
	int nd3 = nd1 + 2 >= resY ? 2*resY - nd1 - 4 : nd1 + 2;

	// load elements:
	localMemory[myID.y] = in[myID.x + myID.y*constMemory->bufferStride];
	localMemory[myID.y+half] = in[myID.x + (myID.y+half)*constMemory->bufferStride];
	barrier(CLK_LOCAL_MEM_FENCE);

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
	out[myID.x + constMemory->bufferStride*(myID.y+half)] = arr[nd2];
}


__kernel void verticalOverlapping(
	__global read_only float* in,
	__global write_only float* out,
	__constant StructConstMemory* constMemory,
	__local float* arr)
{
	int offsetBuffer1 = get_global_id(0) + (get_group_id(1)*2*get_local_size(1)+get_local_id(1)) *constMemory->bufferStride;
	int offsetBuffer2 = offsetBuffer1 + get_local_size(1)*constMemory->bufferStride;
	int localOffset1 = get_local_id(0) + (get_local_id(1)+4)*get_local_size(0);
	int localOffset2 = localOffset1 + get_local_size(1)*get_local_size(0);

	arr[localOffset1] = in[offsetBuffer1];
	arr[localOffset2] = in[offsetBuffer2];
	if(get_local_id(1) <= 2 && get_local_id(0) > 0)
		arr[localOffset1 - get_local_id(1)*get_local_size(0)*2] = arr[localOffset1];
	barrier(CLK_LOCAL_MEM_FENCE);
	if(get_local_id(1) <= 31 && get_local_id(1) > 27)
		arr[localOffset2 + (get_local_size(1)-get_local_id(1))*get_local_size(0)*2] = arr[localOffset2];
	barrier(CLK_LOCAL_MEM_FENCE);

	int base = get_local_id(0) + (get_local_id(1)*2+4)*get_local_size(0);
	float f1 = arr[base] * 0.026749f;
	f1 += arr[base+1] * 0.016864f;
	f1 += arr[base+2] * -0.078223f;
	f1 += arr[base+3] * -0.266864f;
	f1 += arr[base+4] * 0.602949f;
	f1 += arr[base+5] * -0.266864f;
	f1 += arr[base+6] * -0.078223f;
	f1 += arr[base+7] * 0.016864f;
	f1 += arr[base+8] * 0.026749f;
	
	int base2 = base + get_local_size(0)*2;
	float f2 = arr[base2] * -0.091272f;
	f2 += arr[base2+1] * -0.057544f;
	f2 += arr[base2+2] * 0.591272f;
	f2 += arr[base2+3] * 1.115087f;
	f2 += arr[base2+4] * 0.591272f;
	f2 += arr[base2+5] * -0.057544f;
	f2 += arr[base2+6] * -0.091272f;

	out[get_global_id(0) + get_global_id(1) * constMemory->bufferStride] = f1;
	out[get_global_id(0) + (get_global_id(1) + constMemory->halfResolution.y) * constMemory->bufferStride] = f2;


	//int2 myID = {get_global_id(0), get_global_id(1)};
	//__local float* arr = localMemory;
	//int stride = constMemory->bufferStride;
	//int half = constMemory->halfResolution.y;
	//int resX = constMemory->resolution.x;
	//int resY = constMemory->resolution.y;
	//int nd1 = get_local_id(0) + (get_local_id(1)*2)*get_local_size(0);
	//int nd2 = nd1 + get_local_size(0);
	//int nd0 = ((nd1 - get_local_size(0))<0) ? nd1 : nd1 - get_local_size(0);
	//int nd3 = (nd2 + get_local_size(0))%(get_local_size(0)*get_local_size(1)*2);

	//int inAddr1; 
	//if (get_global_id(1) <= 1) 
	//	inAddr1 = get_local_id(0) + (4-get_local_id(1)*2)*constMemory->bufferStride;
	//else
	//	inAddr1 = get_global_id(0) + (get_global_id(1)*2 + get_group_id(1)*(-8) + (-4))*constMemory->bufferStride;
	//int inAddr2 = inAddr1 + constMemory->bufferStride;

	//if (inAddr2 >= constMemory->resolution.y*constMemory->bufferStride)
	//	inAddr2 = get_global_id(0) + (2*constMemory->resolution.y-3-get_global_id(1)*2 - get_group_id(1)*(-8) - (-4))*constMemory->bufferStride;
	//if (inAddr1 >= constMemory->resolution.y*constMemory->bufferStride)
	//	inAddr1 = get_global_id(0) + (2*constMemory->resolution.y-2-get_global_id(1)*2 - get_group_id(1)*(-8) - (-4))*constMemory->bufferStride;


	//// load elements:
	//localMemory[nd1] = in[inAddr1];
	//localMemory[nd2] = in[inAddr2];
	//barrier(CLK_LOCAL_MEM_FENCE);

	//// compute:
	//arr[nd2] += CDF97_ALPHA * (arr[nd1] + arr[nd3]);
	//barrier(CLK_LOCAL_MEM_FENCE);
	////arr[nd1] += CDF97_BETA * (arr[nd0] + arr[nd2]);
	////barrier(CLK_LOCAL_MEM_FENCE);
	//arr[nd2] += CDF97_GAMMA * (arr[nd1] + arr[nd3]);
	//barrier(CLK_LOCAL_MEM_FENCE);
	////arr[nd1] += CDF97_DELTA * (arr[nd0] + arr[nd2]);
	////barrier(CLK_LOCAL_MEM_FENCE);

	//arr[nd1] *= CDF97_INVZETA;
	//arr[nd2] *= CDF97_ZETA;
	//
	//// save elements:
	//if ((get_local_id(1) >= 2) && get_local_id(1) < get_local_size(1)-2)
	//{
	//	out[myID.x + constMemory->bufferStride*myID.y] = arr[nd1];
	//	out[myID.x + constMemory->bufferStride*(myID.y+half)] = arr[nd2];
	//}
}