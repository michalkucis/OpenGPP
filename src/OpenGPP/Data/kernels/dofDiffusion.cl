const sampler_t g_sampler = CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

typedef struct _computeCoC_t
{
	float inM;
	float b1m;
	float s;
} computeCoC_t;

float computeCoC(float depth, __constant computeCoC_t* coc)
{
	float inM = coc->inM;
	float b1m = coc->b1m;
	float s = coc->s;
	float x = inM * depth;
	float y = b1m / x * fabs(s-x);
	return depth != 0 ? y : 0;
}

__kernel void kernelComputeCoC(
			__global read_only image2d_t imColor,
			__global read_only image2d_t imDepth,
			__global write_only image2d_t imRes,
			__constant read_only computeCoC_t* compCoC,
			float multOut)
{
	int2 coordTexIn = {get_global_id(0), get_global_id(1)};
	int2 coordTexOut = {get_global_id(0), get_global_id(1)};
	float depth = read_imagef(imDepth, g_sampler, coordTexIn).x;
	float coc = computeCoC(depth, compCoC);
	float4 outColor = {coc,0,0,0};
	outColor *= multOut;
	write_imagef(imRes, coordTexOut, outColor);	
}

//========================================================================

#define MAX_TEXTURE_SIZE (2048)
#define MAX_OFFSET (1024*8)
#define COUNT_GLOBAL_CHANNELS (6)

typedef struct
{
	int offsets[16];
	int numActiveThreads[16];
	int numIter;
} computeDoFDiffusion_t;

typedef struct
{
	__global float* arrA;
	__global float* arrB;
	__global float* arrC;
	__global float* arrRed;
	__global float* arrGreen;
	__global float* arrBlue;
} ABCRGB_t;

void initABCRGB(
	write_only ABCRGB_t* abcrgb,
	__global float* memoryABCRGB)
{
	__global float* base = memoryABCRGB + get_global_id(1)*MAX_OFFSET*COUNT_GLOBAL_CHANNELS;
	int offset = MAX_OFFSET;

	abcrgb->arrA = base;
	abcrgb->arrB = base + offset;
	abcrgb->arrC = base + offset*2;
	abcrgb->arrRed = base + offset*3;
	abcrgb->arrGreen = base + offset*4;
	abcrgb->arrBlue = base + offset*5;
}

bool isActiveThread(
	int index, 
	int nIter, 
	__constant read_only computeDoFDiffusion_t* diffOffsets)
{
	int numActiveThreads = diffOffsets->numActiveThreads[nIter];
	return index < numActiveThreads;
}

int getOffset(
	int index, 
	int nIter, 
	__constant read_only computeDoFDiffusion_t* diffOffsets)
{
	int offset = diffOffsets->offsets[nIter];
	return offset+index;
}

float getA(
	int index, 
	int nIter,
	ABCRGB_t* abcrgb, 
	__constant read_only computeDoFDiffusion_t* diffOffsets)
{
	int offset = getOffset(index, nIter, diffOffsets);
	return abcrgb->arrA[offset];
}

float getB(
	int index, 
	int nIter,
	ABCRGB_t* abcrgb, 
	__constant read_only computeDoFDiffusion_t* diffOffsets)
{
	int offset = getOffset(index, nIter, diffOffsets);
	return abcrgb->arrB[offset];
}

float getC(
	int index, 
	int nIter,
	ABCRGB_t* abcrgb, 
	__constant read_only computeDoFDiffusion_t* diffOffsets)
{
	int offset = getOffset(index, nIter, diffOffsets);
	return abcrgb->arrC[offset];
}

float3 getRGB(
	int index, 
	int nIter,
	ABCRGB_t* abcrgb, 
	__constant read_only computeDoFDiffusion_t* diffOffsets)
{
	int offset = getOffset(index, nIter, diffOffsets);
	float3 rgb = {abcrgb->arrRed[offset], abcrgb->arrGreen[offset], abcrgb->arrBlue[offset]};
		
	return rgb;
}

void setA(
	float value,
	int index,
	int nIter,
	ABCRGB_t* abcrgb, 
	__constant read_only computeDoFDiffusion_t* diffOffsets)
{	
	int offset = getOffset(index, nIter, diffOffsets);
	abcrgb->arrA[offset] = value;
}

void setB(
	float value,
	int index,
	int nIter,
	ABCRGB_t* abcrgb, 
	__constant read_only computeDoFDiffusion_t* diffOffsets)
{	
	int offset = getOffset(index, nIter, diffOffsets);
	abcrgb->arrB[offset] = value;
}

void setC(
	float value,
	int index,
	int nIter,
	ABCRGB_t* abcrgb, 
	__constant read_only computeDoFDiffusion_t* diffOffsets)
{	
	int offset = getOffset(index, nIter, diffOffsets);
	abcrgb->arrC[offset] = value;
}

void setRGB(
	float3 value,
	int index,
	int nIter,
	ABCRGB_t* abcrgb, 
	__constant read_only computeDoFDiffusion_t* diffOffsets)
{	
	int offset = getOffset(index, nIter, diffOffsets);
	abcrgb->arrRed[offset] = value.x;
	abcrgb->arrGreen[offset] = value.y;
	abcrgb->arrBlue[offset] = value.z;
}


void readCoCRow(
	__global read_only image2d_t imDepth, 
	__constant read_only computeCoC_t* compCoC,
	read_only int2 coord,
	write_only float3* out_coc)
{
	float depth;
	float cocMult = get_image_width(imDepth);
	depth = read_imagef(imDepth, g_sampler, coord).x;
	out_coc->y = cocMult*computeCoC(depth, compCoC);
	int2 coordDown = {coord.x-1, coord.y};
	depth = read_imagef(imDepth, g_sampler, coordDown).x;
	out_coc->x = cocMult*computeCoC(depth, compCoC);
	int2 coordUp = {coord.x+1, coord.y};
	depth = read_imagef(imDepth, g_sampler, coordUp).x;
	out_coc->z = cocMult*computeCoC(depth, compCoC);
}

void readCoCColumn(
	__global read_only image2d_t imDepth, 
	__constant read_only computeCoC_t* compCoC,
	read_only int2 coord,
	write_only float3* out_coc)
{
	float depth;
	float cocMult = get_image_width(imDepth);
	depth = read_imagef(imDepth, g_sampler, coord).x;
	out_coc->y = cocMult*computeCoC(depth, compCoC);
	int2 coordDown = {coord.x, coord.y-1};
	depth = read_imagef(imDepth, g_sampler, coordDown).x;
	out_coc->x = cocMult*computeCoC(depth, compCoC);
	int2 coordUp = {coord.x, coord.y+1};
	depth = read_imagef(imDepth, g_sampler, coordUp).x;
	out_coc->z = cocMult*computeCoC(depth, compCoC);
}

void computeABC(
	read_only float3* coc, 
	write_only float* pa, 
	write_only float* pb, 
	write_only float* pc)
{
	float coef = 2;
	float a = min(coc->x,coc->y);
	float c = min(coc->y,coc->z);
	a = - a/coef;
	c = - c/coef;
	float b = 1 - a - c;
	*pa = a;
	*pb = b;
	*pc = c;
}

void readABCRGB_row(__global read_only image2d_t imColor,
	__global read_only image2d_t imDepth, 
	__constant read_only computeCoC_t* compCoC,
	__constant read_only computeDoFDiffusion_t* compDofRow,
	write_only ABCRGB_t* abcrgb )
{
	int maxOffsetX = get_image_width(imColor);
	for(int iter = 0; iter < 4; iter++)
	{
		int offsetX = get_local_id(0) + iter*get_local_size(0);
		int offsetY = get_global_id(1);
		int2 coord = {offsetX, offsetY};
		
		float a = 0;
		float b = 1;
		float c = 0;
		float3 rgb = {1,1,1};
		if (offsetX < maxOffsetX)
		{
			float3 coc;
			readCoCRow(imDepth, compCoC, coord, &coc);
			computeABC(&coc, &a, &b, &c);
			rgb = (read_imagef(imColor, g_sampler, coord)).xyz;
		}
 		
		setA(a, offsetX, 0, abcrgb, compDofRow);
		setB(b, offsetX, 0, abcrgb, compDofRow);
		setC(c, offsetX, 0, abcrgb, compDofRow);
		setRGB(rgb, offsetX, 0, abcrgb, compDofRow);
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
}

void readABCRGB_column(__global read_only image2d_t imColor,
	__global read_only image2d_t imDepth, 
	__constant read_only computeCoC_t* compCoC,
	__constant read_only computeDoFDiffusion_t* compDofColumn,
	write_only ABCRGB_t* abcrgb )
{
	int maxOffsetY = get_image_height(imColor);
	for(int iter = 0; iter < 4; iter++)
	{
		int offsetY = get_local_id(0) + iter*get_local_size(0);
		int offsetX = get_global_id(1);
		int2 coord = {offsetX, offsetY};
		
		float a = 0;
		float b = 1;
		float c = 0;
		float3 rgb = {1,1,1};
		if (offsetY < maxOffsetY)
		{
			float3 coc;
			readCoCColumn(imDepth, compCoC, coord, &coc);
			computeABC(&coc, &a, &b, &c);
			rgb = (read_imagef(imColor, g_sampler, coord)).xyz;
		}
 		
		setA(a, offsetY, 0, abcrgb, compDofColumn);
		setB(b, offsetY, 0, abcrgb, compDofColumn);
		setC(c, offsetY, 0, abcrgb, compDofColumn);
		setRGB(rgb, offsetY, 0, abcrgb, compDofColumn);
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
}

void buildABCRGB(
	ABCRGB_t* abcrgb, 
	__constant read_only computeDoFDiffusion_t* diff)
{
	int j = get_local_id(0);
	for (int L = 1; L < diff->numIter; L++)
	{
		float aL = 0;
		float bL = 1;
		float cL = 0;
		float3 rgbL = {1,1,1};
		if (isActiveThread(j, L, diff))
		{
			float alpha = getA(2*j+1, L-1, abcrgb, diff)/getB(2*j, L-1, abcrgb, diff);
			float gamma = getC(2*j+1, L-1, abcrgb, diff)/getB(2*j+2, L-1, abcrgb, diff);
			aL = -(alpha*getA(2*j, L-1, abcrgb, diff));
			bL = getB(2*j+1, L-1, abcrgb, diff) 
				- (alpha*getC(2*j, L-1, abcrgb, diff) 
				+ gamma*getA(2*j+2, L-1, abcrgb, diff));
			cL = -(gamma*getC(2*j+2, L-1, abcrgb, diff));
			rgbL = getRGB(2*j+1, L-1, abcrgb, diff)
				- (alpha*getRGB(2*j, L-1, abcrgb, diff)
				+ gamma*getRGB(2*j+2, L-1, abcrgb, diff));
		}
		int offset = getOffset(j, L, diff);
		abcrgb->arrA[offset] = aL;
		abcrgb->arrB[offset] = bL;
		abcrgb->arrC[offset] = cL;
		abcrgb->arrRed[offset] = rgbL.x;
		abcrgb->arrGreen[offset] = rgbL.y;
		abcrgb->arrBlue[offset] = rgbL.z;

		barrier(CLK_GLOBAL_MEM_FENCE);
	}
}

void normalizeRGB_M(
	ABCRGB_t* abcrgb, 
	__constant read_only computeDoFDiffusion_t* diff)
{
	if (0 == get_local_id(0))
	{
		int M = diff->numIter-1;
		float3 rgb = getRGB(0, M, abcrgb, diff);
		float b0 = getB(0, M, abcrgb, diff);
		setRGB(rgb/b0, 0, M, abcrgb, diff);
	}

	barrier(CLK_GLOBAL_MEM_FENCE);
}

void writeRGBToImage_row(
		read_only ABCRGB_t* data, 
		write_only image2d_t im)
{
	int maxOffsetX = get_image_width(im);
	for(int iter = 0; iter < 2; iter++)
	{
		int offsetX = get_local_id(0) + iter*get_local_size(0);
		if (offsetX < maxOffsetX)
		{
			int offsetY = get_global_id(1);
			int2 coord = {offsetX, offsetY};

			float4 rgba = {data->arrRed[offsetX], 
				data->arrGreen[offsetX], 
				data->arrBlue[offsetX], 
				0};
				write_imagef(im, coord, rgba);
		}
	}
}

void writeRGBToImage_column(
		read_only ABCRGB_t* data, 
		write_only image2d_t im)
{
	int maxOffsetY = get_image_height(im);
	for(int iter = 0; iter < 2; iter++)
	{
		int offsetY = get_local_id(0) + iter*get_local_size(0);
		if (offsetY < maxOffsetY)
		{
			int offsetX = get_global_id(1);
			int2 coord = {offsetX, offsetY};

			float4 rgba = {data->arrRed[offsetY], 
				data->arrGreen[offsetY], 
				data->arrBlue[offsetY], 
				0};
				write_imagef(im, coord, rgba);
		}
	}
}

void recompRGB(
	ABCRGB_t* abcrgb,
	__constant read_only computeDoFDiffusion_t* diff)
{
	int N = diff->numActiveThreads[0];

	for (int L=diff->numIter-2; L >= 1; L--)
	{
		int j = get_local_id(0);
	
		float3 rgbL = 0;
		if (isActiveThread(j, L, diff))
		{
			int jp = j/2;
			if (j&1) // odd number:
			{
				float3 rgb = getRGB(jp, L+1, abcrgb, diff);
				setRGB(rgb, j, L, abcrgb, diff);
			}
			else
			{
				float3 rgb = getRGB(j, L, abcrgb, diff)
					- getC(j, L, abcrgb, diff) * getRGB(jp, L+1, abcrgb, diff)
					- getA(j, L, abcrgb, diff) * getRGB(jp-1, L+1, abcrgb, diff);
				rgb = rgb / getB(j, L, abcrgb, diff);
				setRGB(rgb, j, L, abcrgb, diff);
			}
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
	}

	for (int nIter = 0; nIter < 2; nIter++)
	{
		int L = 0;
		int j = get_local_id(0) + nIter*get_local_size(0);
		if (j < N)
		{
			int jp = j/2;
			if (j&2) // odd number:
			{
				float3 rgb = getRGB(jp, 1, abcrgb, diff);
				setRGB(rgb, j, 0, abcrgb, diff);
			}
			else
			{
				float3 rgb = getRGB(j, 0, abcrgb, diff)
					- getC(j, 0, abcrgb, diff) * getRGB(jp, 1, abcrgb, diff)
					- getA(j, 0, abcrgb, diff) * getRGB(jp, 1, abcrgb, diff);
				rgb = rgb / getB(j, 0, abcrgb, diff);
				setRGB(rgb, j, 0, abcrgb, diff);
			}
		}
	}
}

__kernel void diffusionHorizontal(
	__global read_only image2d_t imColor,
	__global read_only image2d_t imDepth,
	__global write_only image2d_t imRes,
	__constant read_only computeCoC_t* compCoC,
	__constant read_only computeDoFDiffusion_t* compDofRow,
	__global float* memoryABCRGB)
{
	if (get_global_id(1) >= get_image_height(imRes))
		return;

	ABCRGB_t abcrgb;
	initABCRGB(&abcrgb, memoryABCRGB);
	readABCRGB_row(imColor, imDepth, compCoC, compDofRow, &abcrgb);
	buildABCRGB(&abcrgb, compDofRow);
	normalizeRGB_M(&abcrgb, compDofRow);
	recompRGB(&abcrgb, compDofRow);
	writeRGBToImage_row(&abcrgb, imRes);
}

__kernel void diffusionVertical(
	__global read_only image2d_t imColor,
	__global read_only image2d_t imDepth,
	__global write_only image2d_t imRes,
	__constant read_only computeCoC_t* compCoC,
	__constant read_only computeDoFDiffusion_t* compDofColumn,
	__global float* memoryABCRGB)
{
	if (get_global_id(1) >= get_image_width(imRes))
		return;

	ABCRGB_t abcrgb;
	initABCRGB(&abcrgb, memoryABCRGB);
	readABCRGB_column(imColor, imDepth, compCoC, compDofColumn, &abcrgb);
	buildABCRGB(&abcrgb, compDofColumn);
	normalizeRGB_M(&abcrgb, compDofColumn);
	recompRGB(&abcrgb, compDofColumn);
	writeRGBToImage_column(&abcrgb, imRes);
}

// the kernel cannot work, it requires global synchronization 
__kernel void diffusion(
			__global read_only image2d_t imColor,
			__global read_only image2d_t imDepth,
			__global write_only image2d_t imRes,
			__constant read_only computeCoC_t* compCoC,
			__constant read_only computeDoFDiffusion_t* compDofRow,
			__constant read_only computeDoFDiffusion_t* compDofColumn,
			__global float* memoryABCRGB)
{


	//ABCRGB_t abcrgb;
	//initABCRGB(&abcrgb, memoryABCRGB);	
	//if (get_global_id(1) < get_image_height(imRes))
	//	readABCRGB_row(imColor, imDepth, compCoC, compDofRow, &abcrgb);
	//buildABCRGB(&abcrgb, compDofRow);
	//normalizeRGB_M(&abcrgb, compDofRow);
	//recompRGB(&abcrgb, compDofRow);
	//transposeRGB(memoryABCRGB);
	
	//writeRGBToImage_column(&abcrgb, imRes);
}





