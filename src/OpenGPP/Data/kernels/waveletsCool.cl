#define OFFSET 2

#define CDF53_ALPHA (-0.5f)
#define CDF53_BETA (0.25f)
#define CDF53_ZETA (0.70710678118f)
#define CDF53_INVZETA (1.41421356237f)

typedef float4 matrix_t;

typedef struct ConstMemory_
{
	int2 textureResolution;
	int2 megaTextureResolution; // #define MEGA_TEXTURE_RESOLUTION (texSize*2-2)
	int2 halfTextureResolution;
} ConstMemory_t;

typedef struct 
{
	int2 imageSize;
	int2 megaImageSize;
} oftenUsedValues_t;




void initOftenUsedValues(
	oftenUsedValues_t* out,
	__constant ConstMemory_t* constMemory)
{
	out->imageSize = constMemory->textureResolution;
	out->megaImageSize = constMemory->megaTextureResolution;
}


int2 getCoord(int index)
{
	int x = get_global_id(0)*2 + index%2 - OFFSET - get_group_id(0)*4;
	int y = get_global_id(1)*2 + index/2 - OFFSET - get_group_id(1)*4;
	int2 coord = {x, y};
	return coord;
}


void saveElementToLocalMemory (
	__local float2* localMemory, 
	int myID,
	int maxID,
	int offset,
	float2 f)
{
	int id = myID + offset + maxID;
	id %= maxID;
	localMemory[id] = f;
}

float2 loadElementFromLocalMemory(
	__local float2* localMemory,
	int myID, 
	int maxID,
	int offset)
{
	int id = myID + offset + maxID;
	id %= maxID;
	return localMemory[id];
}


float4 readTexel(
	__global read_only image2d_t in,
	oftenUsedValues_t* values,
	int index)
{
	int2 coord = getCoord(index);
	int2 imageSize = values->imageSize;
	coord = convert_int2(abs(coord)) % values->megaImageSize;
	coord -= (coord/imageSize) * (coord-imageSize+1) * 2;
	
	sampler_t sampler = CLK_ADDRESS_NONE;
	float4 rgba = read_imagef(in, sampler, coord);
	return rgba;
}


void writeTexel(
	__global write_only image2d_t out,
	oftenUsedValues_t* values,
	int index,
	float4 rgba)
{
	int2 coord = getCoord(index);
	int2 imageSize = values->imageSize;
	if (coord.x >= 0 && coord.y >= 0 && (coord.x < imageSize.x) && (coord.y < imageSize.y))
		write_imagef(out, coord, rgba);
}


float readMatrixPerItem(__global read_only image2d_t in,
	oftenUsedValues_t* values,
	int index)
{
	return readTexel(in, values, index).x;
}
void readMatrix(
	__global read_only image2d_t in,
	oftenUsedValues_t* values,
	matrix_t* m)
{
	int i = 0;
	m->x = readMatrixPerItem(in, values, i++);
	m->y = readMatrixPerItem(in, values, i++);
	m->z = readMatrixPerItem(in, values, i++);
	m->w = readMatrixPerItem(in, values, i++);
}

float2 getColumn1FromMatrix (
	matrix_t* m
	)
{
	float2 out;
	out.x = m->x;
	out.y = m->z;
	return out;
}

float2 getColumn2FromMatrix (
	matrix_t* m
	)
{
	float2 out;
	out.x = m->y;
	out.y = m->w;
	return out;
}

void setColumn1ToMatrix (
	matrix_t* m,
	float2 in
	)
{
	m->x = in.x;
	m->z = in.y;
}

void setColumn2ToMatrix (
	matrix_t* m, 
	float2 in
	)
{
	m->y = in.x;
	m->w = in.y;
}



void writeMatrixPerItem(__global write_only image2d_t in,
	oftenUsedValues_t* values,
	int index,
	float value)
{
	float4 rgba = value;
	writeTexel(in, values, index, rgba);
}

void writeMatrix(
	__global write_only image2d_t out,
	oftenUsedValues_t* values,
	matrix_t* m)
{
	int i = 0;
	writeMatrixPerItem(out, values, i++, m->x);
	writeMatrixPerItem(out, values, i++, m->y);
	writeMatrixPerItem(out, values, i++, m->z);
	writeMatrixPerItem(out, values, i++, m->w);
}

__kernel void tranformOneLevel(
	__global read_only image2d_t in,
	__global write_only image2d_t out,
	__local float2* localMemory,
	__constant ConstMemory_t* constMemory
	)
{
	oftenUsedValues_t values;
	initOftenUsedValues(&values, constMemory);

	matrix_t m;
	readMatrix(in, &values, &m);
	float2 f1 = getColumn1FromMatrix(&m);
	float2 f2 = getColumn2FromMatrix(&m);

	// process lines:
	int myID = get_local_id(0) + get_local_id(1)*get_local_size(0);
	int maxID = get_local_size(0)*get_local_size(1);
	float c;

	c = CDF53_ALPHA;
	saveElementToLocalMemory(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	float2 f3 = loadElementFromLocalMemory(localMemory, myID, maxID, 1);
	f2 += c * (f1 + f3);
	
	c = CDF53_BETA;
	saveElementToLocalMemory(localMemory, myID,  maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	float2 f0 = loadElementFromLocalMemory(localMemory, myID, maxID, 0);
	f1 += c * (f0 + f2);
	
	f1 *= CDF53_INVZETA;
	f2 *= CDF53_ZETA;

	setColumn1ToMatrix(&m, f1);
	setColumn2ToMatrix(&m, f2);

	writeMatrix(out, &values, &m);
}
