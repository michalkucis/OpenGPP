#define CDF97_ALPHA (-1.586134342f)
#define CDF97_BETA (-0.05298011854f)
#define CDF97_GAMMA (0.8829110762f)
#define CDF97_DELTA (0.4435068522)
#define CDF97_ZETA (0.70710678118f)
#define CDF97_INVZETA (1.41421356237f)

typedef float element_t;


float colorToElement(float4 color)
{
	return color.x;
}
float4 elementToColor(float channel)
{
	float4 color = {channel, channel, channel, channel};
	return color;
}

float absf (float x)
{
	return x < 0 ? -x : x;
}


typedef struct ConstMemory_
{
	int2 textureResolution;
	int2 megaTextureResolution; // #define MEGA_TEXTURE_RESOLUTION (texSize*2-2)
	int2 halfTextureResolution;
} ConstMemory_t;

typedef struct OftenUsedVars_
{
	int2 textureResolution;
} OftenUsedVars_t;

void initOftenUsedVars(
	OftenUsedVars_t* out, 
	__constant ConstMemory_t* in)
{
	out->textureResolution = in->textureResolution;
}



void saveElement(
	__local element_t* localMemory, 
	int myID,
	int maxID,
	int offset,
	element_t f)
{
	int id = myID + offset + maxID;
	id %= maxID;
	localMemory[id] = f;
}


element_t loadElement(
	__local element_t* localMemory,
	int myID, 
	int maxID,
	int offset)
{
	int id = myID + offset + maxID;
	id %= maxID;
	return localMemory[id];
}





void writeTexel(
	__global write_only image2d_t out,
	OftenUsedVars_t* vars,
	int2 coord,
	element_t element)
{
	int2 texRes = vars->textureResolution;
	if ((coord.x < texRes.x) && (coord.y < texRes.y))
		write_imagef(out, coord, elementToColor(element));
}



element_t readTexel(
	__global read_only image2d_t in, 
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int2 coord)
{
	int2 texSize = vars->textureResolution;
	coord = convert_int2(abs(coord)) % constMemory->megaTextureResolution;
	coord -= (coord/texSize) * (coord-texSize+1) * 2;

	sampler_t s = CLK_ADDRESS_NONE;
	float4 color = read_imagef(in, s, coord);

	return colorToElement(color);
}



//====================================================================



element_t cdf97loadElementForLine(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0) + offset - 4;
	int2 coord = {nColumn, nLine};
	return readTexel(in, constMemory, vars, coord);	
}



void cdf97saveElementForLine(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t e)
{
	int nLine = get_global_id(1);
	int nOffset = get_global_id(0) - 2;
	int nColumn = nOffset + offset * constMemory->halfTextureResolution.x;
	int2 coord = {nColumn, nLine};

	if (get_local_id(0) >= 2 && get_local_id(0)+2 < get_local_size(0))
		writeTexel(out, vars, coord, e);	
}



__kernel void cdf97linesTransform(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local element_t* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	
	localMemory[myID] = cdf97loadElementForLine(in, constMemory, &vars, 0);
	localMemory[myID+maxID] = cdf97loadElementForLine(in, constMemory, &vars, maxID);
	barrier(CLK_LOCAL_MEM_FENCE);
	float f1 = localMemory[myID*2];
	float f2 = localMemory[myID*2+1];

	myID = get_local_id(0);
	maxID = get_local_size(0);
	float c;


	c = CDF97_ALPHA;
	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);
	f2 += c * (f1 + f3);
	

	c = CDF97_BETA;
	saveElement(localMemory, myID,  maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f0 = loadElement(localMemory, myID, maxID, 0);
	f1 += c * (f0 + f2);
	

	c = CDF97_GAMMA;
	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	f3 = loadElement(localMemory, myID, maxID, 1);
	f2 += c * (f1 + f3);
	

	c = CDF97_DELTA;
	saveElement(localMemory, myID,  maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	f0 = loadElement(localMemory, myID, maxID, 0);
	f1 += c * (f0 + f2);

	
	f1 *= CDF97_INVZETA;
	f2 *= CDF97_ZETA;
	
	
	cdf97saveElementForLine(out, constMemory, &vars, 0, f1);
	cdf97saveElementForLine(out, constMemory, &vars, 1, f2);
}



//====================================================================




element_t cdf97loadElementForColumn(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1)*2 + offset - 4;
	int nColumn = get_global_id(0);
	int2 coord = {nColumn, nLine};
	return readTexel(in, constMemory, vars, coord);	
}



void cdf97saveElementForColumn(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t e)
{
	int nColumn = get_global_id(0);
	int nOffset = get_global_id(1) - 2;
	int nLine = nOffset + offset * constMemory->halfTextureResolution.y;
		
	int2 coord = {nColumn, nLine};
	if (get_local_id(1) >= 2 && get_local_id(1)+2 < get_local_size(1) && nOffset < constMemory->halfTextureResolution.y)
		writeTexel(out, vars, coord, e);	
}


__kernel void cdf97columnsTransform(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local element_t* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	

	element_t f1 = cdf97loadElementForColumn(in, constMemory, &vars, 0);
	element_t f2 = cdf97loadElementForColumn(in, constMemory, &vars, 1);
	

	int myID = get_local_id(1);
	int maxID = get_local_size(1);
	float c;


	c = CDF97_ALPHA;
	saveElement(localMemory, myID, maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);
	f2 += c * (f1 + f3);
	

	c = CDF97_BETA;
	saveElement(localMemory, myID, maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f0 = loadElement(localMemory, myID, maxID, 0);
	f1 += c * (f0 + f2);
	
	c = CDF97_GAMMA;
	saveElement(localMemory, myID, maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	f3 = loadElement(localMemory, myID, maxID, 1);
	f2 += c * (f1 + f3);
	

	c = CDF97_DELTA;
	saveElement(localMemory, myID, maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	f0 = loadElement(localMemory, myID, maxID, 0);
	f1 += c * (f0 + f2);


	f1 *= CDF97_INVZETA;
	f2 *= CDF97_ZETA;
	
	
	cdf97saveElementForColumn(out, constMemory, &vars, 0, f1);
	cdf97saveElementForColumn(out, constMemory, &vars, 1, f2);
}
