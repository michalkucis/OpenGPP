
// CONSTANTS:
#define CDF53_ALPHA (-0.5f)
#define CDF53_BETA (0.25f)
#define CDF53_ZETA (0.70710678118f)
#define CDF53_INVZETA (1.41421356237f)


#define CDF97_ALPHA (-1.586134342f)
#define CDF97_BETA (-0.05298011854f)
#define CDF97_GAMMA (0.8829110762f)
#define CDF97_DELTA (0.4435068522)
#define CDF97_ZETA (0.70710678118f)
#define CDF97_INVZETA (1.41421356237f)




#define WCDF53_EPSILON (0.00001f)
#define WCDF53_ALPHA (0.8f)


#define USE_SINGLE_CHANNEL



#ifndef USE_SINGLE_CHANNEL
typedef float4 element_t;
#else
typedef float element_t;
#endif
element_t colorToElement(float4 color)
{
#ifdef USE_SINGLE_CHANNEL
	return color.x;
#else
	return color;
#endif
}
float4 elementToColor(element_t channel)
{
#ifdef USE_SINGLE_CHANNEL
	float4 color = {channel, channel, channel, channel};
	return color;
#else
	return channel;
#endif
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


element_t readHorizontalWeightTexel(
	__global read_only image2d_t in, 
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int2 coord)
{
	int2 texModif = {-1, 0};
	int2 texSize = vars->textureResolution + texModif;
	coord = convert_int2(abs(coord)) % (constMemory->megaTextureResolution);
	coord -= (coord/texSize) * ((coord-texSize)*2-texModif);

	sampler_t s = CLK_ADDRESS_NONE;
	float4 color = read_imagef(in, s, coord);

	return colorToElement(color);
}

element_t readVerticalWeightTexel(
	__global read_only image2d_t in, 
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int2 coord)
{
	int2 texModif = {0, -1};
	int2 texSize = vars->textureResolution + texModif;
	coord = convert_int2(abs(coord)) % (constMemory->megaTextureResolution);
	coord -= (coord/texSize) * ((coord-texSize)*2-texModif);

	sampler_t s = CLK_ADDRESS_NONE;
	float4 color = read_imagef(in, s, coord);

	return colorToElement(color);
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



int cdf53getLine(__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int mega = constMemory->megaTextureResolution.y;
	int nLine = get_global_id(1) - get_group_id(1)*2 - 1;
	
	nLine = nLine*2 + offset;
	nLine = abs(nLine);
	nLine %= mega;
	nLine = nLine >= vars->textureResolution.y ? mega - nLine : nLine;
	return nLine;
}



int cdf53getColumn(__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int mega = constMemory->megaTextureResolution.x;
	int nColumn = get_global_id(0) - get_group_id(0)*2 - 1;
	
	nColumn = nColumn*2 + offset;
	nColumn = abs(nColumn);
	nColumn %= mega;
	nColumn = nColumn >= vars->textureResolution.x ? mega - nColumn : nColumn;
	return nColumn;
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


//====================================================================



element_t cdf53loadElementForLine(
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



void cdf53saveElementForLine(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t e)
{
	int nLine = get_global_id(1);
	int nOffset = get_global_id(0) - 1 - get_group_id(0)*2;
	int nColumn = nOffset + offset * constMemory->halfTextureResolution.x;
	int2 coord = {nColumn, nLine};

	if (get_local_id(0) >= 1 && get_local_id(0)+1 < get_local_size(0) && nOffset < constMemory->halfTextureResolution.x)
		writeTexel(out, vars, coord, e);	
}








__kernel void cdf53linesTransform(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local element_t* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	

	element_t f1 = cdf53loadElementForLine(in, constMemory, &vars, 0);
	element_t f2 = cdf53loadElementForLine(in, constMemory, &vars, 1);
	

	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	float c;


	c = CDF53_ALPHA;
	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);
	f2 += c * (f1 + f3);
	
	c = CDF53_BETA;
	saveElement(localMemory, myID,  maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f0 = loadElement(localMemory, myID, maxID, 0);
	f1 += c * (f0 + f2);
	
	
	f1 *= CDF53_INVZETA;
	f2 *= CDF53_ZETA;
	
	
	cdf53saveElementForLine(out, constMemory, &vars, 0, f1);
	cdf53saveElementForLine(out, constMemory, &vars, 1, f2);
}



//====================================================================




element_t cdf53loadElementForLineReconstruct(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1);
	int nColumn = cdf53getColumn(constMemory, vars, offset);
	int nAddr = nColumn / 2 + offset*constMemory->halfTextureResolution.x;
	int2 coord = {nAddr, nLine};

	return readTexel(in, constMemory, vars, coord);	
}


void cdf53saveElementForLineReconstruct(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t e)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 2 - get_group_id(0)*4;
		
	int2 coord = {nColumn, nLine};
	if (get_local_id(0) >= 1 && get_local_id(0)+1 < get_local_size(0))
		writeTexel(out, vars, coord, e);	
}


__kernel void cdf53linesReconstruct(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local element_t* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	
	element_t f1 = cdf53loadElementForLineReconstruct(in, constMemory, &vars, 0);
	element_t f2 = cdf53loadElementForLineReconstruct(in, constMemory, &vars, 1);
	
	
	f1 *= CDF53_ZETA;
	f2 *= CDF53_INVZETA;

	
	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	
	float c;
	

	c = CDF53_BETA;
	saveElement(localMemory, myID,  maxID, 0, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f0 = loadElement(localMemory, myID, maxID, -1);
	f1 -= c * (f0 + f2);
	

	c = CDF53_ALPHA;
	saveElement(localMemory, myID,  maxID, -1, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 0);
	f2 -= c * (f1 + f3);
	

	cdf53saveElementForLineReconstruct(out, constMemory, &vars, 0, f1);
	cdf53saveElementForLineReconstruct(out, constMemory, &vars, 1, f2);
}



//====================================================================




element_t cdf53loadElementForColumn(
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



void cdf53saveElementForColumn(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t e)
{
	int nColumn = get_global_id(0);
	int nOffset = get_global_id(1) - 1 - get_group_id(1)*2;;
	int nLine = nOffset + offset * constMemory->halfTextureResolution.y;
		
	int2 coord = {nColumn, nLine};
	if (get_local_id(1) >= 1 && get_local_id(1)+1 < get_local_size(1) && nOffset < constMemory->halfTextureResolution.y)
		writeTexel(out, vars, coord, e);	
}


__kernel void cdf53columnsTransform(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local element_t* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	

	element_t f1 = cdf53loadElementForColumn(in, constMemory, &vars, 0);
	element_t f2 = cdf53loadElementForColumn(in, constMemory, &vars, 1);
	

	int myID = get_local_id(1);
	int maxID = get_local_size(1);
	float c;


	c = CDF53_ALPHA;
	saveElement(localMemory, myID, maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);
	f2 += c * (f1 + f3);
	

	c = CDF53_BETA;
	saveElement(localMemory, myID, maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f0 = loadElement(localMemory, myID, maxID, 0);
	f1 += c * (f0 + f2);
	
	
	f1 *= CDF53_INVZETA;
	f2 *= CDF53_ZETA;
	
	
	cdf53saveElementForColumn(out, constMemory, &vars, 0, f1);
	cdf53saveElementForColumn(out, constMemory, &vars, 1, f2);
}



//=======================================================================



element_t cdf53loadElementForColumnReconstruct(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nColumn = get_global_id(0);
	int nLine = cdf53getLine(constMemory, vars, offset);
	int nAddr = nLine / 2 + offset*constMemory->halfTextureResolution.y;
	int2 coord = {nColumn, nAddr};
	
	return readTexel(in, constMemory, vars, coord);	
}


void cdf53saveElementForColumnReconstruct(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t f4)
{
	int nLine = get_global_id(1)*2 + offset - 2 - get_group_id(1)*4;
	int nColumn = get_global_id(0);
		
	int2 coord = {nColumn, nLine};
	if (get_local_id(1) >= 1 && get_local_id(1)+1 < get_local_size(1))
		writeTexel(out, vars, coord, f4);	
}



__kernel void cdf53columnsReconstruct(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local element_t* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	
	element_t f1 = cdf53loadElementForColumnReconstruct(in, constMemory, &vars, 0);
	element_t f2 = cdf53loadElementForColumnReconstruct(in, constMemory, &vars, 1);
	

	f1 *= CDF53_ZETA;
	f2 *= CDF53_INVZETA;


	int myID = get_local_id(1);
	int maxID = get_local_size(1);
	

	float c = CDF53_BETA;
	saveElement(localMemory, myID, maxID, 0, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f0 = loadElement(localMemory, myID, maxID, -1);
	f1 -= c * (f0 + f2);
	

	c = CDF53_ALPHA;
	saveElement(localMemory, myID, maxID, -1, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 0);
	f2 -= c * (f1 + f3);
	
	
	cdf53saveElementForColumnReconstruct(out, constMemory, &vars, 0, f1);
	cdf53saveElementForColumnReconstruct(out, constMemory, &vars, 1, f2);
}



//====================================================================



element_t cdf97loadElementForLine(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 4;
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
	

	element_t f1 = cdf97loadElementForLine(in, constMemory, &vars, 0);
	element_t f2 = cdf97loadElementForLine(in, constMemory, &vars, 1);
	

	int myID = get_local_id(0);
	int maxID = get_local_size(0);
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


int cdf97getColumn(__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int mega = constMemory->megaTextureResolution.x;
	int nColumn = get_global_id(0) - 2;
	
	nColumn = nColumn*2 + offset;
	nColumn = abs(nColumn);
	nColumn %= mega;
	nColumn = nColumn >= vars->textureResolution.x ? mega - nColumn : nColumn;
	return nColumn;
}


element_t cdf97loadElementForLineReconstruct(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1);
	int nColumn = cdf97getColumn(constMemory, vars, offset);
	int nAddr = nColumn / 2 + offset*constMemory->halfTextureResolution.x;
	int2 coord = {nAddr, nLine};

	return readTexel(in, constMemory, vars, coord);	
}


void cdf97saveElementForLineReconstruct(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t e)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 4;
		
	int2 coord = {nColumn, nLine};
	if (get_local_id(0) >= 2 && get_local_id(0)+2 < get_local_size(0))
		writeTexel(out, vars, coord, e);	
}


__kernel void cdf97linesReconstruct(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local element_t* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	
	element_t f1 = cdf97loadElementForLineReconstruct(in, constMemory, &vars, 0);
	element_t f2 = cdf97loadElementForLineReconstruct(in, constMemory, &vars, 1);
	
	
	f1 *= CDF97_ZETA;
	f2 *= CDF97_INVZETA;

	
	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	
	float c;
	c = CDF97_DELTA;
	saveElement(localMemory, myID,  maxID, 0, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f0 = loadElement(localMemory, myID, maxID, -1);
	f1 -= c * (f0 + f2);
	

	c = CDF97_GAMMA;
	saveElement(localMemory, myID,  maxID, -1, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 0);
	f2 -= c * (f1 + f3);


	c = CDF97_BETA;
	saveElement(localMemory, myID,  maxID, 0, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	f0 = loadElement(localMemory, myID, maxID, -1);
	f1 -= c * (f0 + f2);
	

	c = CDF97_ALPHA;
	saveElement(localMemory, myID,  maxID, -1, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	f3 = loadElement(localMemory, myID, maxID, 0);
	f2 -= c * (f1 + f3);
	

	cdf97saveElementForLineReconstruct(out, constMemory, &vars, 0, f1);
	cdf97saveElementForLineReconstruct(out, constMemory, &vars, 1, f2);
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



//=======================================================================


int cdf97getLine(__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int mega = constMemory->megaTextureResolution.y;
	int nLine = get_global_id(1) - 2;
	
	nLine = nLine*2 + offset;
	nLine = abs(nLine);
	nLine %= mega;
	nLine = nLine >= vars->textureResolution.y ? mega - nLine : nLine;
	return nLine;
}


element_t cdf97loadElementForColumnReconstruct(
	__global read_only image2d_t in,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nColumn = get_global_id(0);
	int nLine = cdf97getLine(constMemory, vars, offset);
	int nAddr = nLine / 2 + offset*constMemory->halfTextureResolution.y;
	int2 coord = {nColumn, nAddr};
	
	return readTexel(in, constMemory, vars, coord);	
}


void cdf97saveElementForColumnReconstruct(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t f4)
{
	int nLine = get_global_id(1)*2 + offset - 4;
	int nColumn = get_global_id(0);
		
	int2 coord = {nColumn, nLine};
	if (get_local_id(1) >= 2 && get_local_id(1)+2 < get_local_size(1))
		writeTexel(out, vars, coord, f4);	
}



__kernel void cdf97columnsReconstruct(
	__global read_only image2d_t in,
	__global write_only image2d_t out,

	__local element_t* localMemory,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	
	element_t f1 = cdf97loadElementForColumnReconstruct(in, constMemory, &vars, 0);
	element_t f2 = cdf97loadElementForColumnReconstruct(in, constMemory, &vars, 1);
	

	f1 *= CDF97_ZETA;
	f2 *= CDF97_INVZETA;


	int myID = get_local_id(1);
	int maxID = get_local_size(1);
	

	float c = CDF97_DELTA;
	saveElement(localMemory, myID, maxID, 0, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f0 = loadElement(localMemory, myID, maxID, -1);
	f1 -= c * (f0 + f2);
	

	c = CDF97_GAMMA;
	saveElement(localMemory, myID, maxID, -1, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 0);
	f2 -= c * (f1 + f3);


	c = CDF97_BETA;
	saveElement(localMemory, myID, maxID, 0, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	f0 = loadElement(localMemory, myID, maxID, -1);
	f1 -= c * (f0 + f2);
	

	c = CDF97_ALPHA;
	saveElement(localMemory, myID, maxID, -1, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	f3 = loadElement(localMemory, myID, maxID, 0);
	f2 -= c * (f1 + f3);
	
	
	cdf97saveElementForColumnReconstruct(out, constMemory, &vars, 0, f1);
	cdf97saveElementForColumnReconstruct(out, constMemory, &vars, 1, f2);
}



//====================================================================


void cdf53saveWeightForLine(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t e)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 2 - get_group_id(0)*4;
	int2 coord = {nColumn, nLine};

	if (get_local_id(0) >= 1 && get_local_id(0)+1 < get_local_size(0))
		writeTexel(out, vars, coord, e);	
}


__kernel void wcdf53linesTransform(
	__global read_only image2d_t in,
	__global write_only image2d_t out,
	__global write_only image2d_t weight,

	__local element_t* localMemory,	
	__local element_t* localMemory2,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);

	int myID = get_local_id(0);
	int maxID = get_local_size(0);	

	element_t f1 = cdf53loadElementForLine(in, constMemory, &vars, 0);
	element_t f2 = cdf53loadElementForLine(in, constMemory, &vars, 1);


	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);


	float w1 = 1.0f / (pow(absf(f1 - f2), WCDF53_ALPHA) + WCDF53_EPSILON);
	float w2 = 1.0f / (pow(absf(f2 - f3), WCDF53_ALPHA) + WCDF53_EPSILON);


	float d = f2 + (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF53_ALPHA);


	saveElement(localMemory, myID, maxID, 1, w2);
	saveElement(localMemory2,myID, maxID, 1, d);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t w0 = loadElement(localMemory, myID, maxID, 0);
	element_t d0 = loadElement(localMemory2, myID, maxID, 0);

	float a = f1 + (w0*d0 + w1*d) / (w0+w1) * (2 * CDF53_BETA);

	
	cdf53saveElementForLine(out, constMemory, &vars, 0, a);
	cdf53saveElementForLine(out, constMemory, &vars, 1, d);
	cdf53saveWeightForLine(weight, constMemory, &vars, 0, w1);
	cdf53saveWeightForLine(weight, constMemory, &vars, 1, w2);
}



//====================================================================


element_t cdf53loadWeightForLine(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 2 - get_group_id(0)*4;
	int2 coord = {nColumn, nLine};

	return readHorizontalWeightTexel(out, constMemory, vars, coord);	
}


__kernel void wcdf53linesReconstruct(
	__global read_only image2d_t in,
	__global write_only image2d_t out,
	__global read_only image2d_t weight,

	__local element_t* localMemory,
	__local element_t* localMemory2,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	
	element_t a = cdf53loadElementForLineReconstruct(in, constMemory, &vars, 0);
	element_t d = cdf53loadElementForLineReconstruct(in, constMemory, &vars, 1);
	element_t w1 = cdf53loadWeightForLine(weight, constMemory, &vars, 0);
	element_t w2 = cdf53loadWeightForLine(weight, constMemory, &vars, 1);

	
	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	

	saveElement(localMemory, myID, maxID, 1, w2);
	saveElement(localMemory2,myID, maxID, 1, d);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t w0 = loadElement(localMemory, myID, maxID, 0);
	element_t d0 = loadElement(localMemory2, myID, maxID, 0);

	float f1 = a - (w0*d0 + w1*d)/(w0+w1) * (2 * CDF53_BETA);
	
	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);


	float f2 =  d - (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF53_ALPHA);


	cdf53saveElementForLineReconstruct(out, constMemory, &vars, 0, f1);
	cdf53saveElementForLineReconstruct(out, constMemory, &vars, 1, f2);
}



//====================================================================


void cdf53saveWeightForColumn(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t e)
{
	int nLine = get_global_id(1)*2 + offset - 2 - get_group_id(1)*4;
	int nColumn = get_global_id(0);
	int2 coord = {nColumn, nLine};

	if (get_local_id(1) >= 1 && get_local_id(1)+1 < get_local_size(1))
		writeTexel(out, vars, coord, e);	
}


__kernel void wcdf53columnsTransform(
	__global read_only image2d_t in,
	__global write_only image2d_t out,
	__global write_only image2d_t weight,

	__local element_t* localMemory,	
	__local element_t* localMemory2,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	int myID = get_local_id(1);
	int maxID = get_local_size(1);

	element_t f1 = cdf53loadElementForColumn(in, constMemory, &vars, 0);
	element_t f2 = cdf53loadElementForColumn(in, constMemory, &vars, 1);
	

	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);
	

	float w1 = 1.0f / (pow(absf(f1 - f2), WCDF53_ALPHA) + WCDF53_EPSILON);
	float w2 = 1.0f / (pow(absf(f2 - f3), WCDF53_ALPHA) + WCDF53_EPSILON);

	float d = f2 + (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF53_ALPHA);


	saveElement(localMemory, myID, maxID, 1, w2);
	saveElement(localMemory2,myID, maxID, 1, d);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t w0 = loadElement(localMemory, myID, maxID, 0);
	element_t d0 = loadElement(localMemory2, myID, maxID, 0);

	float a = f1 + (w0*d0 + w1*d)/(w0+w1) * (2 * CDF53_BETA);


	cdf53saveElementForColumn(out, constMemory, &vars, 0, a);
	cdf53saveElementForColumn(out, constMemory, &vars, 1, d);
	cdf53saveWeightForColumn(weight, constMemory, &vars, 0, w1);
	cdf53saveWeightForColumn(weight, constMemory, &vars, 1, w2);
}


//=======================================================================


element_t cdf53loadWeightForColumn(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1)*2 + offset - 2 - get_group_id(1)*4;
	int nColumn = get_global_id(0);
	int2 coord = {nColumn, nLine};

	return readVerticalWeightTexel(out, constMemory, vars, coord);	
}


__kernel void wcdf53columnsReconstruct(
	__global read_only image2d_t in,
	__global write_only image2d_t out,
	__global read_only image2d_t weight,

	__local element_t* localMemory,
	__local element_t* localMemory2,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	
	element_t a = cdf53loadElementForColumnReconstruct(in, constMemory, &vars, 0);
	element_t d = cdf53loadElementForColumnReconstruct(in, constMemory, &vars, 1);
	element_t w1 = cdf53loadWeightForColumn(weight, constMemory, &vars, 0);
	element_t w2 = cdf53loadWeightForColumn(weight, constMemory, &vars, 1);
	

	int myID = get_local_id(1);
	int maxID = get_local_size(1);
	

	saveElement(localMemory, myID, maxID, 1, w2);
	saveElement(localMemory2,myID, maxID, 1, d);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t w0 = loadElement(localMemory, myID, maxID, 0);
	element_t d0 = loadElement(localMemory2, myID, maxID, 0);

	float f1 = a - (w0*d0 + w1*d) / (w0+w1) * (2 * CDF53_BETA);
	
	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);


	float f2 =  d - (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF53_ALPHA);
	
	cdf53saveElementForColumnReconstruct(out, constMemory, &vars, 0, f1);
	cdf53saveElementForColumnReconstruct(out, constMemory, &vars, 1, f2);
}


//====================================================================


void cdf97saveWeightForLine(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t e)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 4;
	int2 coord = {nColumn, nLine};

	if (get_local_id(0) >= 2 && get_local_id(0)+2 < get_local_size(0))
		writeTexel(out, vars, coord, e);	
}


__kernel void wcdf97linesTransform(
	__global read_only image2d_t in,
	__global write_only image2d_t out,
	__global write_only image2d_t weight,

	__local element_t* localMemory,	
	__local element_t* localMemory2,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);

	int myID = get_local_id(0);
	int maxID = get_local_size(0);	

	element_t f1 = cdf97loadElementForLine(in, constMemory, &vars, 0);
	element_t f2 = cdf97loadElementForLine(in, constMemory, &vars, 1);


	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);


	float w1 = 1.0f / (pow(absf(f1 - f2), WCDF53_ALPHA) + WCDF53_EPSILON);
	float w2 = 1.0f / (pow(absf(f2 - f3), WCDF53_ALPHA) + WCDF53_EPSILON);


	f2 += (f1*w1 + f3*w2) / (w1+w2) * (CDF97_ALPHA * 2);


	saveElement(localMemory, myID, maxID, 1, w2);
	saveElement(localMemory2,myID, maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t w0 = loadElement(localMemory, myID, maxID, 0);
	element_t f0 = loadElement(localMemory2, myID, maxID, 0);

	f1 += (f0*w0 + w1*f2) / (w0+w1) * (CDF97_BETA * 2);

	
	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	f3 = loadElement(localMemory, myID, maxID, 1);
	f2 += (f1*w1 + f3*w2) / (w1+w2) * (CDF97_GAMMA * 2);


	saveElement(localMemory, myID,  maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	f0 = loadElement(localMemory, myID, maxID, 0);
	f1 += (f0*w0 + f2*w1) / (w0+w1) * (CDF97_DELTA * 2);

	
	//f1 *= CDF97_INVZETA;
	//f2 *= CDF97_ZETA;


	cdf97saveElementForLine(out, constMemory, &vars, 0, f1);
	cdf97saveElementForLine(out, constMemory, &vars, 1, f2);
	cdf97saveWeightForLine(weight, constMemory, &vars, 0, w1);
	cdf97saveWeightForLine(weight, constMemory, &vars, 1, w2);
}



//====================================================================


element_t cdf97loadWeightForLine(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1);
	int nColumn = get_global_id(0)*2 + offset - 4;
	int2 coord = {nColumn, nLine};

	return readHorizontalWeightTexel(out, constMemory, vars, coord);	
}


__kernel void wcdf97linesReconstruct(
	__global read_only image2d_t in,
	__global write_only image2d_t out,
	__global read_only image2d_t weight,

	__local element_t* localMemory,
	__local element_t* localMemory2,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	
	element_t f1 = cdf97loadElementForLineReconstruct(in, constMemory, &vars, 0);
	element_t f2 = cdf97loadElementForLineReconstruct(in, constMemory, &vars, 1);
	element_t w1 = cdf97loadWeightForLine(weight, constMemory, &vars, 0);
	element_t w2 = cdf97loadWeightForLine(weight, constMemory, &vars, 1);


	//f1 *= CDF97_ZETA;
	//f2 *= CDF97_INVZETA;

	
	int myID = get_local_id(0);
	int maxID = get_local_size(0);
	

	saveElement(localMemory, myID, maxID, 1, w2);
	saveElement(localMemory2,myID, maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t w0 = loadElement(localMemory, myID, maxID, 0);
	element_t f0 = loadElement(localMemory2, myID, maxID, 0);


	f1 -= (f0*w0 + f2*w1) / (w0+w1) * (2 * CDF97_DELTA);
	
	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);


	f2 -= (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF97_GAMMA);

	saveElement(localMemory2,myID, maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	f0 = loadElement(localMemory2, myID, maxID, 0);


	f1 -= (f0*w0 + f2*w1) / (w0+w1) * (2 * CDF97_BETA);
	
	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	f3 = loadElement(localMemory, myID, maxID, 1);


	f2 -= (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF97_ALPHA);

	cdf97saveElementForLineReconstruct(out, constMemory, &vars, 0, f1);
	cdf97saveElementForLineReconstruct(out, constMemory, &vars, 1, f2);
}



//====================================================================


void cdf97saveWeightForColumn(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset,
	element_t e)
{
	int nLine = get_global_id(1)*2 + offset - 4;
	int nColumn = get_global_id(0);
	int2 coord = {nColumn, nLine};

	if (get_local_id(1) >= 2 && get_local_id(1)+2 < get_local_size(1))
		writeTexel(out, vars, coord, e);	
}


__kernel void wcdf97columnsTransform(
	__global read_only image2d_t in,
	__global write_only image2d_t out,
	__global write_only image2d_t weight,

	__local element_t* localMemory,	
	__local element_t* localMemory2,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	int myID = get_local_id(1);
	int maxID = get_local_size(1);

	element_t f1 = cdf97loadElementForColumn(in, constMemory, &vars, 0);
	element_t f2 = cdf97loadElementForColumn(in, constMemory, &vars, 1);
	

	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);
	

	float w1 = 1.0f / (pow(absf(f1 - f2), WCDF53_ALPHA) + WCDF53_EPSILON);
	float w2 = 1.0f / (pow(absf(f2 - f3), WCDF53_ALPHA) + WCDF53_EPSILON);

	f2 += (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF97_ALPHA);


	saveElement(localMemory, myID, maxID, 1, w2);
	saveElement(localMemory2,myID, maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t w0 = loadElement(localMemory, myID, maxID, 0);
	element_t f0 = loadElement(localMemory2, myID, maxID, 0);

	f1 += (f0*w0 + f2*w1) / (w0+w1) * (2 * CDF97_BETA);


	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	f3 = loadElement(localMemory, myID, maxID, 1);
	f2 += (f1*w1 + f3*w2) / (w1+w2) * (CDF97_GAMMA * 2);


	saveElement(localMemory, myID,  maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	f0 = loadElement(localMemory, myID, maxID, 0);
	f1 += (f0*w0 + f2*w1) / (w0+w1) * (CDF97_DELTA * 2);


	//f1 *= CDF97_INVZETA;
	//f2 *= CDF97_ZETA;


	cdf97saveElementForColumn(out, constMemory, &vars, 0, f1);
	cdf97saveElementForColumn(out, constMemory, &vars, 1, f2);
	cdf97saveWeightForColumn(weight, constMemory, &vars, 0, w1);
	cdf97saveWeightForColumn(weight, constMemory, &vars, 1, w2);
}


//=======================================================================


element_t cdf97loadWeightForColumn(
	__global write_only image2d_t out,
	__constant ConstMemory_t* constMemory,
	OftenUsedVars_t* vars,
	int offset)
{
	int nLine = get_global_id(1)*2 + offset - 4;
	int nColumn = get_global_id(0);
	int2 coord = {nColumn, nLine};

	return readVerticalWeightTexel(out, constMemory, vars, coord);	
}


__kernel void wcdf97columnsReconstruct(
	__global read_only image2d_t in,
	__global write_only image2d_t out,
	__global read_only image2d_t weight,

	__local element_t* localMemory,
	__local element_t* localMemory2,
	__constant ConstMemory_t* constMemory)
{
	OftenUsedVars_t vars;
	initOftenUsedVars(&vars, constMemory);
	
	
	element_t f1 = cdf97loadElementForColumnReconstruct(in, constMemory, &vars, 0);
	element_t f2 = cdf97loadElementForColumnReconstruct(in, constMemory, &vars, 1);
	element_t w1 = cdf97loadWeightForColumn(weight, constMemory, &vars, 0);
	element_t w2 = cdf97loadWeightForColumn(weight, constMemory, &vars, 1);
	

	//f1 *= CDF97_ZETA;
	//f2 *= CDF97_INVZETA;


	int myID = get_local_id(1);
	int maxID = get_local_size(1);
	

	saveElement(localMemory, myID, maxID, 1, w2);
	saveElement(localMemory2,myID, maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t w0 = loadElement(localMemory, myID, maxID, 0);
	element_t f0 = loadElement(localMemory2, myID, maxID, 0);


	f1 -= (f0*w0 + f2*w1) / (w0+w1) * (2 * CDF97_DELTA);
	
	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	element_t f3 = loadElement(localMemory, myID, maxID, 1);


	f2 -= (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF97_GAMMA);

	saveElement(localMemory2,myID, maxID, 1, f2);
	barrier(CLK_LOCAL_MEM_FENCE);
	f0 = loadElement(localMemory2, myID, maxID, 0);


	f1 -= (f0*w0 + f2*w1) / (w0+w1) * (2 * CDF97_BETA);
	
	saveElement(localMemory, myID,  maxID, 0, f1);
	barrier(CLK_LOCAL_MEM_FENCE);
	f3 = loadElement(localMemory, myID, maxID, 1);


	f2 -= (f1*w1 + f3*w2) / (w1+w2) * (2 * CDF97_ALPHA);


	cdf97saveElementForColumnReconstruct(out, constMemory, &vars, 0, f1);
	cdf97saveElementForColumnReconstruct(out, constMemory, &vars, 1, f2);
}
