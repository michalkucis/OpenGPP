const sampler_t g_sampler = CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;



__kernel void zeroesMemory(
	__global write_only float* memory)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int width = get_global_size(0);
	memory[x + y*width] = 0;
}

__kernel void drawSolidRect(
	__global write_only float* memory,
	read_only int2 begin,
	read_only int2 end,
	read_only float value)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	bool inside = x >= begin.x && x <= end.x;
	inside &= y >= begin.y && y <= end.y;
	
	int width = get_global_size(0);
	memory[x + y*width] = inside ? value : 0;
}

__kernel void multiplyArrayComplex(
	__global float* inout_re1,
	__global float* inout_im1,
	__global float* in_re2,
	__global float* in_im2)
{
	int x = get_global_id(0);
	int y = get_global_id(1);	
	int width = get_global_size(0);
	int offset = x + y*width;
	float re1 = inout_re1[offset];
	float im1 = inout_im1[offset];
	float re2 = in_re2[offset];
	float im2 = in_im2[offset];

	float re = re1*re2 - im1*im2;
	float im = re1*im2 + re2*im1;

	inout_re1[offset] = re;
	inout_im1[offset] = im;
}

__kernel void copyImages(
	__global write_only image2d_t out,
	__global read_only image2d_t in)
{
	int2 texCoord = {get_global_id(0), get_global_id(1)};
	float4 rgba = read_imagef(in, g_sampler, texCoord);
	write_imagef(out, texCoord, rgba);
}

__kernel void copyFromImage2DGL(
	__global write_only float* memory,
	read_only int2 memorySize, 
	__global read_only image2d_t image,
	float mult)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int2 coord = {x, y};

	int width = memorySize.x;

	memory[x + y*width] = read_imagef(image,g_sampler,coord).x * mult;
}

__kernel void copyToImage2DGL(
	__global read_only float* memory,
	read_only int2 memorySize, 
	__global write_only image2d_t image,
	float mult)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int2 coord = {x, y};

	int width = memorySize.x;
	float value = memory[x + y*width];
	float4 rgba = {value, value, value, 0}; 
		
	write_imagef(image,coord,rgba * mult);
}

//---------------------------------------------------------

int2 getIntPos()
{
	int2 size = {get_global_size(0), get_global_size(1)};
	int2 upos = {get_global_id(0), get_global_id(1)};
	int2 halfSize = size/2;

	int2 ipos;
	ipos.x = upos.x > halfSize.x ? upos.x - size.x : upos.x;
	ipos.y = upos.y > halfSize.y ? upos.y - size.y : upos.y;
	return ipos;
}

float2 getRelPos()
{
	int2 intPos = getIntPos();

	int2 size = {get_global_size(0), get_global_size(1)};
	int2 one = {1, 1};
	int2 allowedSize = size/2 - one;

	float2 rel = {intPos.x / (float)allowedSize.x,
		intPos.y / (float)allowedSize.y};
	return rel;
}
float getGauss (float x)
{
	float arg = (x*x)/-2.0f;

	return exp(arg);
}

#define NUM_FLARES 6
#define ORG_ANGLE 0

float getFlareVal(float2 inpos, float angle)
{
#define INC 0.1f

	float c = cos(angle);
	float s = sin(angle);
	float2 pos = {inpos.x*c + inpos.y*s, inpos.x*s - inpos.y*c};

	pos.x = fmax(pos.x, 0);
	float x = pos.y/(pos.x*INC);
	float value = pos.x ? getGauss(x) : 0;
	
	return (inpos.x || inpos.y) ? value : (1.0f/NUM_FLARES);
}

float getFlareDecrease(float2 inpos, float angle)
{
	float c = cos(angle);
	float s = sin(angle);
	float2 pos = {inpos.x*c + inpos.y*s, inpos.x*s - inpos.y*c};

	float x = length(pos);

	float yMult = 0.15;
	float base = 8;

	return yMult * pow(base, -x);
}


float filterStar(float2 rel)
{
	float sum = 0.0f;
	for (uint nFlare = 0; nFlare < NUM_FLARES; nFlare++)
	{
		float angle = (float) (ORG_ANGLE + (nFlare/(float)NUM_FLARES) * M_PI_F * 2);

		float y = getFlareVal(rel, angle);
		sum += y * getFlareDecrease(rel, angle);
	}
	return sum;
}

float filter(float2 rel)
{
	float dist = length(rel);

	const float mult = 0.3f;
	const float multArg = 10.0f;
	float glowMult = dist ? 1.0f / pow(1+dist * multArg, 1.2f) : 0;
	float glow = glowMult * mult;

	return filterStar(rel)/** (1-glowMult) + glow*/;
}

float toGrayscale(float3 rgb)
{
	return (rgb.x + rgb.y + rgb.z) / 3;
}

const sampler_t g_samplerLinear = CLK_ADDRESS_CLAMP | CLK_FILTER_LINEAR;



__kernel void fillBufferFromImage(
	__global write_only float* buffer,
	int2 bufferSize,
	__global read_only image2d_t im)
{
	int nx = get_global_id(0);
	int ny = get_global_id(1);
	float2 texCoord = {
		nx / (float)get_global_size(0) * get_image_width(im), 
		ny / (float)get_global_size(1) * get_image_height(im)};
	float3 rgb = read_imagef(im, g_samplerLinear, texCoord).xyz;
	buffer[nx + bufferSize.x*ny] = toGrayscale(rgb);
}


float getBufferValueNearest(
	float2 bufferCoord, 
	__global read_only float* buffer,
	int2 bufferSize)
{
	int2 nCoord = {(int) bufferCoord.x, (int) bufferCoord.y};
	return buffer[nCoord.x + nCoord.y*bufferSize.x];
}

float getBufferValueLinear(
	float2 bufferCoord, 
	__global read_only float* buffer,
	int2 bufferSize)
{
	int2 nCoordSmaller = {(int) bufferCoord.x, (int) bufferCoord.y};
	int2 nCoordBigger = {nCoordSmaller.x+1, nCoordSmaller.y+1};
	float4 values;
	values.x = buffer[nCoordSmaller.x + nCoordSmaller.y*bufferSize.x];
	values.y = buffer[nCoordBigger.x + nCoordSmaller.y*bufferSize.x];
	values.z = buffer[nCoordSmaller.x + nCoordBigger.y*bufferSize.x];
	values.w = buffer[nCoordBigger.x + nCoordBigger.y*bufferSize.x];
	float2 w = {bufferCoord.x - nCoordSmaller.x, bufferCoord.y - nCoordSmaller.y};
	float v1 = values.x*(1-w.x) + values.y*w.x;
	float v2 = values.z*(1-w.x) + values.w*w.x;
	return v1*(1-w.y)+v2*w.y;
	//return w.x;
}

__kernel void fillImageFromBufferAndImage(
	__global write_only image2d_t imRes,
	__global read_only image2d_t imSrc,
	__global read_only float* buffer,
	int2 bufferSize,
	int2 usefulBufferSize,
	float outMult)
{
	int nx = get_global_id(0);
	int ny = get_global_id(1);

	float2 bufferCoord = {nx / (float) get_global_size(0) * usefulBufferSize.x,
		ny / (float) get_global_size(1) * usefulBufferSize.y};
	float3 rgb = getBufferValueLinear(bufferCoord, buffer, bufferSize);

	float4 lensFlare = {rgb.x, rgb.y, rgb.z, 1};
	float2 inTexCoord = {nx * get_image_width(imSrc) / (float)get_image_width(imRes), ny * get_image_height(imSrc) / (float)get_image_height(imRes)};
	int2 outTexCoord = {nx , ny};
	float4 color = read_imagef(imSrc, g_sampler, inTexCoord);
	float4 resColor = color + lensFlare*outMult;
	write_imagef(imRes, outTexCoord, resColor);
}

__kernel void fillImageFromCutBufferAndImage(
	__global write_only image2d_t imRes,
	__global read_only image2d_t imSrc,
	__global read_only float* buffer,
	int2 bufferSize,
	int2 usefulBufferSize,
	float2 cutBufferOrg,
	float2 cutBufferSize,
	float outMult)
{
	int nx = get_global_id(0);
	int ny = get_global_id(1);

	float2 bufferCoord = {nx / (float) get_global_size(0) * usefulBufferSize.x,
		ny / (float) get_global_size(1) * usefulBufferSize.y};
	bufferCoord = (bufferCoord * cutBufferSize) + cutBufferOrg * bufferSize;
	float3 rgb = getBufferValueLinear(bufferCoord, buffer, bufferSize);

	float4 lensFlare = {rgb.x, rgb.y, rgb.z, 1};
	float2 inTexCoord = {nx * get_image_width(imSrc) / (float)get_image_width(imRes), 
		ny * get_image_height(imSrc) / (float)get_image_height(imRes)};
	int2 outTexCoord = {nx , ny};
	float4 color = read_imagef(imSrc, g_sampler, inTexCoord);
	float4 resColor = color + lensFlare*outMult;
	write_imagef(imRes, outTexCoord, resColor);
}

__kernel void star(
	__global write_only float* memory,
	int2 memorySize,
	float outMult)
{
	int width = memorySize.x;
	int2 coord = {get_global_id(0), get_global_id(1)};
	int offset = coord.x + width*coord.y;
	
	memory[offset] = outMult * filter(getRelPos());
}