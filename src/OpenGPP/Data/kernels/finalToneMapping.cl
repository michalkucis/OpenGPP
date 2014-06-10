#define WCDF53_EPSILON (0.00001f)
#define WCDF53_ALPHA (0.8f)

#define CDF53_ALPHA (-0.5f)
#define CDF53_BETA (0.25f)

sampler_t c_sampler = CLK_ADDRESS_REPEAT;
#define MIN_INPUT_VALUE 0.000001f


typedef struct ConstMemory_
{
	int2 resolution;
	int2 halfResolution;
	int bufferStride;
} ConstMemory_t;


float4 rgb2yuv(float4 rgba)
{
	float r = rgba.x;
	float g = rgba.y;
	float b = rgba.z;

	float y = 0.2126f*r + 0.7152f*g + 0.0722f*b;
	float u =-0.09991*r - 0.33609f*g + 0.436f*b;
	float v = 0.615f*r - 0.55861f*g - 0.05639f*b;

	float4 yuv = {y, u, v, 1};
	return yuv;
}


float4 yuv2rgb(float y, float2 uv)
{
	float u = uv.x;
	float v = uv.y;

	float r = 1.0f*y + 0.0f*u   + 1.28033f*v;
	float g = 1.0f*y - 0.21482f*u - 0.38059f*v;
	float b = 1.0f*y + 2.12798f*u + 0.0f*v;

	float4 rgb = {r, g, b, 1};
	return rgb;
}


float4 rgb2logYUV (float4 rgba)
{
	float4 color = log(max(rgba, MIN_INPUT_VALUE));
	float4 yuv = rgb2yuv(color);
	
	return yuv;
}


float4 logYUV2rgb (float4 yuv)
{
	float2 uv = {yuv.y, yuv.z};
	float4 rgba = yuv2rgb(yuv.x, uv);

	rgba = exp(rgba);
	return rgba;
}


//------------------------------------------------------------


__kernel void prepare(
	__global read_only image2d_t inImageRGB,
	__global write_only float* outBufferY,
	__global write_only float2* outBufferUV,
	__constant ConstMemory_t* memory)
{
	int2 coord = {get_global_id(0), get_global_id(1)};
	float4 color = read_imagef(inImageRGB, c_sampler, coord);

	//float4 yuv = rgb2logYUV(color);
	float4 yuv = color;

	int offset = coord.x + coord.y*memory->bufferStride;
	
	outBufferY[offset] = yuv.x;
	outBufferUV[offset] = yuv.yz;
}


__kernel void post(
	__global read_only float* inBufferY,
	__global read_only float2* inBufferUV,
	__global write_only image2d_t outImageRGB,
	__constant ConstMemory_t* memory)
{
	int2 coord = {get_global_id(0), get_global_id(1)};
	int offset = coord.x + coord.y*memory->bufferStride;
	float y = inBufferY[offset];
	//float2 uv = inBufferUV[offset];
	//
	//float4 yuv = {y, uv.x, uv.y, 1};
	//float4 rgba = logYUV2rgb(yuv);

	float4 rgba = {y,y,y,1};

	write_imagef(outImageRGB, coord, rgba);
}



//---------------------------------------------------------------------


//void cdf53saveWeightForLine(
//	__global write_only image2d_t out,
//	__constant ConstMemory_t* constMemory,
//	OftenUsedVars_t* vars,
//	int offset,
//	element_t e)
//{
//	int nLine = get_global_id(1);
//	int nColumn = get_global_id(0)*2 + offset - 2 - get_group_id(0)*4;
//	int2 coord = {nColumn, nLine};
//
//	if (get_local_id(0) >= 1 && get_local_id(0)+1 < get_local_size(0))
//		writeTexel(out, vars, coord, e);	
//}




__kernel void wcdf53rowsAnal(
	__global read_only float* in,
	__global write_only float* out,
	__global write_only float* weight,

	__local float* localMemory,	// sizeof(2*get_local_size(0)*sizeof(float))
	__local float* localMemory2, // sizeof(2*get_local_size(0)*sizeof(float)
	__constant ConstMemory_t* constMemory)
{
	int id = get_local_id(0);
	int countIDs = get_local_size(0);
	int nGroupID = get_group_id(0);
	int nRow = get_global_id(1);

	int resX = constMemory->resolution.x;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;

	int xLocal = id;
	int xGlobal = (countIDs*2 - 4) * nGroupID;


	// copying global to local mem:
	{
		int offGlobal = id + xGlobal + stride*nRow;
		localMemory[xLocal] = in[offGlobal + countIDs];

		int offGlobal2 = id + xGlobal + stride*nRow + countIDs;
		localMemory[xLocal + countIDs] = in[offGlobal2];
	}
	barrier(CLK_MEM_FENCE);

	
	// init pointers:
	int off0 = abs(id*2-1);
	int off1 = id*2;
	int off2 = xGlobal + off1 + 1 >= resX ? id*2+1 : id*2-1;
	int off3 = xGlobal + off1 + 2 >= resX ? id*2+2 : id*2;
	

	// analyze:
	float f1 = localMemory[off1];
	float f2 = localMemory[off2];
	float f3 = localMemory[off3];

	float w1 = 1.0f / (pow(fabs(f1 - f2), WCDF53_ALPHA) + WCDF53_EPSILON);
	float w2 = 1.0f / (pow(fabs(f2 - f3), WCDF53_ALPHA) + WCDF53_EPSILON);
	
	float d = f2 + (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF53_ALPHA);
	localMemory[off2] = d;
	barrier(CLK_MEM_FENCE);

	float d0 = localMemory[off0];
	float w0 = localMemory[abs(id-1)];
	float a = f1 + (w0*d0 + w1*d) / (w0+w1) * (2 * CDF53_BETA);
	

	localMemory[off1] = a;fadsfadsfasfd

	
	barrier(CLK_MEM_FENCE);
	// copying local to global mem:
	if (id != 0 || get_global_id(0) == 0)
	{
		if (id != countIDs-1)
		{
			if (xGlobal >= resX)
			{
				int offGlobal = xGlobal + stride*nRow;
				out[offGlobal] = localMemory[xLocal];
			}
			if (xGlobal + countIDs >= resX)
			{
				int offGlobal = xGlobal + stride*nRow + countIDs;
				out[offGlobal] = localMemory[xLocal + countIDs];
			}
		}
	}


	//int texID1 = 2*off;
	//if (texID1 >= resX)
	//	texID1 = 2*resX - texID1 - 1;
	//int texID2 = 2*id + 1;
	//if (texID2 >= resX)
	//	texID2 = 2*resX - texID2 - 1;

	//if (get_global_id(0) == 0)
	//{
	//	texID1 = 2;
	//	texID2 = 1;
	//}

	//int inOffset1 = texID1 + stride*row;
	//int inOffset2 = texID2 + stride*row;

	//float f1 = in[inOffset1];
	//float f2 = in[inOffset2];


	//localMemory[id] = f1;
	//barrier(CLK_LOCAL_MEM_FENCE);
	//float f3 = localMemory[id+1];

	//float w1 = 1.0f / (pow(fabs(f1 - f2), WCDF53_ALPHA) + WCDF53_EPSILON);
	//float w2 = 1.0f / (pow(fabs(f2 - f3), WCDF53_ALPHA) + WCDF53_EPSILON);

	//float d = f2 + (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF53_ALPHA);


	//localMemory[id+1] = w2;
	//localMemory2[id+1] = d;
	//barrier(CLK_LOCAL_MEM_FENCE);

	//float w0, d0;
	//if (id == 0)
	//{
	//	w0 = w2;
	//	d0 = d;
	//}
	//else
	//{
	//	w0 = localMemory[id];
	//	d0 = localMemory2[id];
	//}
	//float a = f1 + (w0*d0 + w1*d) / (w0+w1) * (2 * CDF53_BETA);

	////float a = f1;
	//int outputOffset = id + stride*row;
	//if(2*id < size)
	//{
	//	out[outputOffset] = a;
	//	weight[inOffset1] = w1;
	//	if (2*id+1 < size)
	//	{
	//		out[outputOffset + half] = d;
	//		weight[inOffset1+1] = w2;
	//	}
	//}
}






__kernel void wcdf53linesTransform(
	__global read_only float* in,
	__global write_only float* out,
	__global write_only float* weight,

	__local float* localMemory,	
	__local float* localMemory2,
	__constant ConstMemory_t* constMemory)
{
	int id = get_local_id(0);
	int countIDs = get_local_size(0);
	int row = get_global_id(1);
	int size = constMemory->resolution.x;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;

	int texID1 = 2*id;
	if (texID1 >= size)
		texID1 = 2*size - texID1 - 1;
	int texID2 = 2*id + 1;
	if (texID2 >= size)
		texID2 = 2*size - texID2 - 1;

	int inOffset1 = texID1 + stride*row;
	int inOffset2 = texID2 + stride*row;

	float f1 = in[inOffset1];
	float f2 = in[inOffset2];


	localMemory[id] = f1;
	barrier(CLK_LOCAL_MEM_FENCE);
	float f3 = localMemory[id+1];

	float w1 = 1.0f / (pow(fabs(f1 - f2), WCDF53_ALPHA) + WCDF53_EPSILON);
	float w2 = 1.0f / (pow(fabs(f2 - f3), WCDF53_ALPHA) + WCDF53_EPSILON);

	float d = f2 + (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF53_ALPHA);


	localMemory[id+1] = w2;
	localMemory2[id+1] = d;
	barrier(CLK_LOCAL_MEM_FENCE);

	float w0, d0;
	if (id == 0)
	{
		w0 = w2;
		d0 = d;
	}
	else
	{
		w0 = localMemory[id];
		d0 = localMemory2[id];
	}
	float a = f1 + (w0*d0 + w1*d) / (w0+w1) * (2 * CDF53_BETA);

	//float a = f1;
	int outputOffset = id + stride*row;
	if(2*id < size)
	{
		out[outputOffset] = a;
		weight[inOffset1] = w1;
		if (2*id+1 < size)
		{
			out[outputOffset + half] = d;
			weight[inOffset1+1] = w2;
		}
	}
}

__kernel void wcdf53linesReconstruct(
	__global read_only float* in,
	__global write_only float* out,
	__global read_only float* weight,

	__local float* localMemory,	
	__local float* localMemory2,
	__constant ConstMemory_t* constMemory)
{
	int id = get_local_id(0);
	int countIDs = get_local_size(0);
	int row = get_global_id(1);
	int size = constMemory->resolution.x;
	int stride = constMemory->bufferStride;
	int half = constMemory->halfResolution.x;
	
	int texID1 = id;
	if (texID1 >= half)
		texID1 = abs((2*size - 2*id - 1)/2);
	int texID2 = id;
	if (texID2 >= size - half)
		texID2 = abs((2*size - 2*id - 2)/2);
	texID2 += half;

	int inOffset1 = texID1 + stride*row;
	int inOffset2 = texID2 + stride*row;

	float a = in[inOffset1];
	float d = in[inOffset2];
	float w1, w2;
	{
		int texID1 = 2*id;
		if (texID1 >= size)
			texID1 = 2*size - texID1 - 1;
		int texID2 = 2*id + 1;
		if (texID2 >= size)
			texID2 = 2*size - texID2 - 1;
		w1 = weight[texID1 + stride*row];
		w2 = weight[texID2 + stride*row];
	}

	localMemory[id+1] = w2;
	localMemory2[id+1] = d;
	barrier(CLK_LOCAL_MEM_FENCE);

	float w0, d0;
	if (id == 0)
	{
		w0 = w2;
		d0 = d;
	}
	else
	{
		w0 = localMemory[id];
		d0 = localMemory2[id];
	}
	float f1 = a - (w0*d0 + w1*d) / (w0+w1) * (2 * CDF53_BETA);


	localMemory[id] = f1;
	barrier(CLK_LOCAL_MEM_FENCE);
	float f3 = localMemory[id+1];

	float f2 = d - (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF53_ALPHA);

	int outputOffset = 2*id + stride*row;
	//float a = f1;
	if(2*id < size)
	{
		out[outputOffset] = f1;
		if (2*id+1 < size)
		{
			out[outputOffset + 1] = f2;
		}
	}
}