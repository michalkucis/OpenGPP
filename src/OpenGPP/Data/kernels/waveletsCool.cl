
// CONSTANTS:
#define ALPHA (-0.5f)
#define BETA (0.25f)

#define ZETA (0.70710678118f)
#define INV_ZETA (1.41421356237f)




typedef struct ConstMemory_
{
	int2 textureResolution;
	int2 megaTextureResolution; // #define MEGA_TEXTURE_RESOLUTION (texSize*2-2)
	int2 halfTextureResolution;
	int2 secondTextureExtension;
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



float4 readTexel(
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
	return color;
}



void writeTexel(
	__global write_only image2d_t out,
	OftenUsedVars_t* vars,
	int2 coord,
	float4 value)
{
	int2 texRes = vars->textureResolution;
	if ((coord.x < texRes.x) && (coord.y < texRes.y))
		write_imagef(out, coord, value);
}




float4 loadElementForLine(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 2 - get_group_id(0)*4;
	int2 coord = {nColumn, nLine};
	return readTexel(in, constMemory, vars, coord);	
}



void saveElementForLine(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	float4 f4)
{
	int nLine = get_global_id(1);
	int nOffset = get_global_id(0) - 1 - get_group_id(0)*2;;
	int nColumn = nOffset + offset * constMemory->halfTextureResolution.x;
	int2 coord = {nColumn, nLine};
	//f4.x = offset;
	if (get_local_id(0) >= 1 && get_local_id(0)+1 < get_local_size(0) && nOffset < constMemory->halfTextureResolution.x)
		writeTexel(out, vars, coord, f4);	
}



float4 loadElementForColumn(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1)*2 + offset - 2 - get_group_id(1)*4;
	int nColumn = get_global_id(0);
	int2 coord = {nColumn, nLine};
	return readTexel(in, constMemory, vars, coord);	
}



void saveElementForColumn(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	float4 f4)
{
	int nColumn = get_global_id(0);
	int nOffset = get_global_id(1) - 1 - get_group_id(1)*2;;
	int nLine = nOffset + offset * constMemory->halfTextureResolution.y;
		
	int2 coord = {nColumn, nLine};
	if (get_local_id(1) >= 1 && get_local_id(1)+1 < get_local_size(1) && nOffset < constMemory->halfTextureResolution.y)
		writeTexel(out, vars, coord, f4);	
}


float4 loadElementForLineReconstruct(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1);
	int nOffset = get_global_id(0) - 1 - get_group_id(0)*2;
	int texSize = constMemory->halfTextureResolution.x + offset*constMemory->secondTextureExtension.x;
	nOffset -= (nOffset/texSize) * (nOffset-texSize+1) * 2;
	int nColumn = nOffset + offset * constMemory->halfTextureResolution.x;
	int2 coord = {nColumn, nLine};

	return readTexel(in, constMemory, vars, coord);	
}

void saveElementForLineReconstruct(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	float4 f4)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 2 - get_group_id(0)*4;
		
	int2 coord = {nColumn, nLine};
	if (get_local_id(0) >= 1 && get_local_id(0)+1 < get_local_size(0))
		writeTexel(out, vars, coord, f4);	
}


float4 loadElementForColumnReconstruct(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nColumn = get_global_id(0);
	int nOffset = get_global_id(1) - 1 - get_group_id(1)*2;
	int texSize = constMemory->halfTextureResolution.y + offset*constMemory->secondTextureExtension.y;
	nOffset -= (nOffset/texSize) * (nOffset-texSize+1) * 2;
	int nLine = nOffset + offset * constMemory->halfTextureResolution.y;
	int2 coord = {nColumn, nLine};

	return readTexel(in, constMemory, vars, coord);	
}

void saveElementForColumnReconstruct(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	float4 f4)
{
	int nLine = get_global_id(1)*2 + offset - 2 - get_group_id(1)*4;
	int nColumn = get_global_id(0);
		
	int2 coord = {nColumn, nLine};
	if (get_local_id(1) >= 1 && get_local_id(1)+1 < get_local_size(1))
		writeTexel(out, vars, coord, f4);	
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


__kernel void linesTransform(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	

	float4 f1 = loadElementForLine(in, constMemory, &vars, 0);
	float4 f2 = loadElementForLine(in, constMemory, &vars, 1);
	

	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	float c;


	c = ALPHA;
	saveElement(localMemory, myID, f1);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
	float4 f3 = loadElement(localMemory, myID, maxID, 1);
	f2 += c * (f1 + f3);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);

	c = BETA;
	saveElement(localMemory, myID, f2);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
	float4 f0 = loadElement(localMemory, myID, maxID, -1);
	f1 += c * (f0 + f2);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);


	
	f1 *= INV_ZETA;
	f2 *= ZETA;
	
	
	saveElementForLine(out, constMemory, &vars, 0, f1);
	saveElementForLine(out, constMemory, &vars, 1, f2);
}







__kernel void linesReconstruct(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	
	float4 f1 = loadElementForLineReconstruct(in, constMemory, &vars, 0);
	float4 f2 = loadElementForLineReconstruct(in, constMemory, &vars, 1);
	

	f1 *= ZETA;
	f2 *= INV_ZETA;


	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	
	float c;


	c = BETA;
	saveElement(localMemory, myID, f2);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
	float4 f0 = loadElement(localMemory, myID, maxID, -1);
	f1 -= c * (f0 + f2);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);


	c = ALPHA;
	saveElement(localMemory, myID, f1);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
	float4 f3 = loadElement(localMemory, myID, maxID, 1);
	f2 -= c * (f1 + f3);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
	
		
	saveElementForLineReconstruct(out, constMemory, &vars, 0, f1);
	saveElementForLineReconstruct(out, constMemory, &vars, 1, f2);
}



__kernel void columnsTransform(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	float4 f1 = loadElementForColumn(in, constMemory, &vars, 0);
	float4 f2 = loadElementForColumn(in, constMemory, &vars, 1);
	
	
	int myID = get_local_id(1);
	int maxID = get_local_size(1);
	float c;

	
	c = ALPHA;
	saveElement(localMemory, myID, f1);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
	float4 f3 = loadElement(localMemory, myID, maxID, 1);
	f2 += c * (f1 + f3);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);


	c = BETA;
	saveElement(localMemory, myID, f2);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
	float4 f0 = loadElement(localMemory, myID, maxID, -1);
	f1 += c * (f0 + f2);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);


	f1 *= INV_ZETA;
	f2 *= ZETA;
	
	
	saveElementForColumn(out, constMemory, &vars, 0, f1);
	saveElementForColumn(out, constMemory, &vars, 1, f2);
}


__kernel void columnsReconstruct(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local float4* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	
	float4 f1 = loadElementForColumnReconstruct(in, constMemory, &vars, 0);
	float4 f2 = loadElementForColumnReconstruct(in, constMemory, &vars, 1);
	

	f1 *= ZETA;
	f2 *= INV_ZETA;


	int myID = get_local_id(1);
	int maxID = get_local_size(1);
	

	float c = BETA;
	saveElement(localMemory, myID, f2);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
	float4 f0 = loadElement(localMemory, myID, maxID, -1);
	f1 -= c * (f0 + f2);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);


	c = ALPHA;
	saveElement(localMemory, myID, f1);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);
	float4 f3 = loadElement(localMemory, myID, maxID, 1);
	f2 -= c * (f1 + f3);
	barrier(CLK_LOCAL_MEM_FENCE|CLK_GLOBAL_MEM_FENCE);

	
	saveElementForColumnReconstruct(out, constMemory, &vars, 0, f1);
	saveElementForColumnReconstruct(out, constMemory, &vars, 1, f2);
}


/*
float4 loadElementForLineReconstruct(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1);
	int nOffset = get_global_id(0) - 1 - get_group_id(0)*2;
	int nColumn = nOffset + offset * constMemory->halfTextureResolution.x;
	int2 coord = {nColumn, nLine};
//	if (nOffset >= constMemory->halfTextureResolution.x)

	return readTexel(in, constMemory, vars, coord);	
}



void saveElementForLineReconstruct(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	float4 f4)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 2 - get_group_id(0)*4;
		
	int2 coord = {nColumn, nLine};
	if (get_local_id(0) >= 1 && get_local_id(0)+1 < get_local_size(0))
		writeTexel(out, vars, coord, f4);	
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
	
	for(int i = 0; i < NUM_STEPS; i+=2)
	{
		float c = getCoeficient(i+1);
		saveElement(localMemory, myID, f2);
		float4 f0 = loadElement(localMemory, myID, maxID, -1);

		f1 -= c * (f0 + f2);

		c = getCoeficient(i);
		saveElement(localMemory, myID, f1);
		float4 f3 = loadElement(localMemory, myID, maxID, 1);
		
		f2 -= c * (f1 + f3);
	}
	
	saveElementForLine(out, 0, f1);
	saveElementForLine(out, 1, f2);
}


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
	case 2: return GAMMA; 
	case 3: return DELTA; 
	}
	return 0.0f;
}
*/