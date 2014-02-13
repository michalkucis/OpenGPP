
#define ALPHA (-0.5f)
#define BETA (0.25f)

#define ZETA (0.70710678118f)
#define INV_ZETA (1.41421356237f)

float4 readImage(__global read_only image2d_t im, int2 coord)
{
	uint2 ucoord = abs(coord);
	coord.x = (int) ucoord.x;
	coord.y = (int) ucoord.y;
	int2 imageRes = {get_image_width(im), get_image_height(im)};
	int2 maxCoord = imageRes - 1;
	int2 invCoord = maxCoord - coord;
	coord = maxCoord - abs(invCoord);
	sampler_t s = CLK_ADDRESS_NONE;
	return read_imagef(im, s, coord);
}

float4 loadElementForLine(
	__global read_only image2d_t in,
	int offset)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 4 - get_group_id(0)*8;
	int2 coord = {nColumn, nLine};
	return readImage(in, coord);	
}

void saveElementForLine(
	__global write_only image2d_t out,
	int offset,
	float4 f4)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 4 -get_group_id(0)*8;
	int2 coord = {nColumn, nLine};
	if (nColumn < get_image_width(out) && get_local_id(0) >= 2 && get_local_id(0)+2 < get_local_size(0))
		write_imagef(out, coord, f4);	
}

float4 loadElementForColumn(
	__global read_only image2d_t in,
	int offset)
{
	int nLine = get_global_id(1)*2 + offset - 4 - get_group_id(1)*8;
	int nColumn = get_global_id(0);
	int2 coord = {nColumn, nLine};
	return readImage(in, coord);	
}

void saveElementForColumn(
	__global write_only image2d_t out,
	int offset,
	float4 f4)
{
	int nLine = get_global_id(1)*2 + offset - 4 -get_group_id(1)*8;
	int nColumn = get_global_id(0);
	int2 coord = {nColumn, nLine};
	if (nColumn < get_image_height(out) && get_local_id(1) >= 2 && get_local_id(1)+2 < get_local_size(1))
		write_imagef(out, coord, f4);	
}

void saveElement(
	__local float4* localMemory, 
	int myID,
	float4 f)
{
	localMemory[myID] = f;
}

float4 loadElement(
	__local float4* localMemory,
	int myID, 
	int maxID,
	int offset)
{
	int id = myID + offset + maxID;
	id %= maxID;
	return localMemory[id];
}

float getCoeficient(int index)
{
	switch(index)
	{
	case 0: return ALPHA;
	case 1: return BETA; 
	}
	return 0.0f;
}

__kernel void transformLines(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory)
{
	float4 f1 = loadElementForLine(in, 0);
	float4 f2 = loadElementForLine(in, 1);

	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	

	float c = ALPHA;
	saveElement(localMemory, myID, f1);
	float4 f3 = loadElement(localMemory, myID, maxID, 1);
		

	f2 += c * (f1 + f3);

	c = BETA;
	saveElement(localMemory, myID, f2);
	float4 f0 = loadElement(localMemory, myID, maxID, -1);

	f1 += c * (f0 + f2);

	
	f1 *= INV_ZETA;
	f2 *= ZETA;

	
	saveElementForLine(out, 0, f1);
	saveElementForLine(out, 1, f2);
}


__kernel void reconstructLines(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory)
{
	float4 f1 = loadElementForLine(in, 0);
	float4 f2 = loadElementForLine(in, 1);

	f1 *= ZETA;
	f2 *= INV_ZETA;

	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	
	float c = BETA;
	saveElement(localMemory, myID, f2);
	float4 f0 = loadElement(localMemory, myID, maxID, -1);

	f1 -= c * (f0 + f2);

	c = ALPHA;
	saveElement(localMemory, myID, f1);
	float4 f3 = loadElement(localMemory, myID, maxID, 1);
		
	f2 -= c * (f1 + f3);
	
	saveElementForLine(out, 0, f1);
	saveElementForLine(out, 1, f2);
}


__kernel void transformColumns(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory)
{
	float4 f1 = loadElementForColumn(in, 0);
	float4 f2 = loadElementForColumn(in, 1);

	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	

	float c = ALPHA;
	saveElement(localMemory, myID, f1);
	float4 f3 = loadElement(localMemory, myID, maxID, 1);
		

	f2 += c * (f1 + f3);

	c = BETA;
	saveElement(localMemory, myID, f2);
	float4 f0 = loadElement(localMemory, myID, maxID, -1);

	f1 += c * (f0 + f2);

	
	f1 *= INV_ZETA;
	f2 *= ZETA;

	
	saveElementForColumn(out, 0, f1);
	saveElementForColumn(out, 1, f2);
}


__kernel void reconstructColumns(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory)
{
	float4 f1 = loadElementForColumn(in, 0);
	float4 f2 = loadElementForColumn(in, 1);

	f1 *= ZETA;
	f2 *= INV_ZETA;

	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	
	float c = BETA;
	saveElement(localMemory, myID, f2);
	float4 f0 = loadElement(localMemory, myID, maxID, -1);

	f1 -= c * (f0 + f2);

	c = ALPHA;
	saveElement(localMemory, myID, f1);
	float4 f3 = loadElement(localMemory, myID, maxID, 1);
		
	f2 -= c * (f1 + f3);
	
	saveElementForColumn(out, 0, f1);
	saveElementForColumn(out, 1, f2);
}

/*
__kernel void kernelTest(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory)
{
	float4 f1 = loadElementForLine(in, 0);
	float4 f2 = loadElementForLine(in, 1);

	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	
	for(int i = 0; i < NUM_STEPS; i+=2)
	{
		float c = getCoeficient(i);
		saveElement(localMemory, myID, f1);
		float4 f3 = loadElement(localMemory, myID, maxID, 1);
		

		f2 += c * (f1 + f3);

		c = getCoeficient(i+1);
		saveElement(localMemory, myID, f2);
		float4 f0 = loadElement(localMemory, myID, maxID, -1);

		f1 += c * (f0 + f2);
	}
	
	f1 *= INV_ZETA;
	f2 *= ZETA;
	
	saveElementForLine(out, 0, f1);
	saveElementForLine(out, 1, f2);
}
void saveElement(
	__local float4* localMemory, 
	int myID,
	float4 f)
{
	localMemory[myID] = f;
}

float4 loadElement(
	__local float4* localMemory,
	int myID, 
	int maxID,
	int offset)
{
	int id = myID + offset;
	id %= maxID;
	return localMemory[id];
}

float4 loadElementForLine(
	__global read_only image2d_t in,
	int offset)
{
	int nLine = get_global_id(1);
	int nColumn = offset + get_global_id(0)*2 + (get_group_id(0)*2+1)*(-4);
	int2 coord = {nColumn, nLine};
	sampler_t s = CLK_ADDRESS_MIRRORED_REPEAT;
	return read_imagef(in, s, coord);	
}

void saveElementForLine(
	__global write_only image2d_t out,
	int offset,
	float4 f4)
{
	int nLine = get_global_id(1);
	int nColumn = offset + get_global_id(0)*2 + (get_group_id(0)*2+1)*(-4);
	int2 coord = {nColumn, nLine};
	if (nColumn >= 4 && nColumn <= get_local_size(0)-4 && nColumn <= get_image_width(out))
		write_imagef(out, coord, f4);	
}

float4 loadElementForColumn(
	__global read_only image2d_t in,
	int offset)
{
	int nLine = offset + get_global_id(0)*2 + (get_group_id(0)*2+1)*(-4);
	int nColumn = get_global_id(1);
	int2 coord = {nColumn, nLine};
	sampler_t s = CLK_ADDRESS_MIRRORED_REPEAT;
	return read_imagef(in, s, coord);
}

void saveElementForColumn(
	__global write_only image2d_t out,
	int offset,
	float4 f4)
{
	int nLine = offset + get_global_id(1)*2 + (get_group_id(1)*2+1)*(-4);
	int nColumn = get_global_id(0);
	int2 coord = {nColumn, nLine};
	if (nLine >= 4 && nLine <= get_local_size(1)-4 && nLine <= get_image_height(out))
		write_imagef(out, coord, f4);	
}

float getCoeficient(int index)
{
	switch(index)
	{
	case 0: return ALPHA;
	case 1: return BETA; 
	case 2: return GAMMA; 
	case 3: return DELTA; 
	}
	return 0.0f;
}


__kernel void kernelProcessLines(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory
	)
{
	float4 f1 = loadElementForLine(in, 0);
	float4 f2 = loadElementForLine(in, 1);

	int myID = get_local_id(0);
	int maxID = get_local_size(0);

	for(int i = 0; i < 4; i+=2)
	{
		float c = getCoeficient(i);
		saveElement(localMemory, myID, f1);
		float4 f0 = loadElement(localMemory, myID, maxID, 1);

		f1 += c * (f0 + f2);

		saveElement(localMemory, myID, f0);
		float4 f3 = loadElement(localMemory, myID, maxID, -1);

		f2 += c * (f1 + f3);
	}

	f1 *= INV_ZETA;
	f2 *= ZETA;

	saveElementForLine(out, 0, f1);
	saveElementForLine(out, 1, f2);
}


__kernel void kernelProcessColumns(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory
	)
{
	float4 f1 = loadElementForColumn(in, 0);
	float4 f2 = loadElementForColumn(in, 1);

	int myID = get_local_id(1);
	int maxID = get_local_size(1);

	for(int i = 0; i < 4; i+=2)
	{
		float c = getCoeficient(i);
		saveElement(localMemory, myID, f1);
		float4 f0 = loadElement(localMemory, myID, maxID, 1);

		f1 += c * (f0 + f2);

		saveElement(localMemory, myID, f0);
		float4 f3 = loadElement(localMemory, myID, maxID, -1);

		f2 += c * (f1 + f3);
	}

	f1 *= INV_ZETA;
	f2 *= ZETA;

	saveElementForColumn(out, 0, f1);
	saveElementForColumn(out, 1, f2);
}
*/