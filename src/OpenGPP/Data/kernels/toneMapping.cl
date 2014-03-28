sampler_t c_sampler = CLK_ADDRESS_REPEAT;
#define MIN_INPUT_VALUE 0.000001f
#define BETA 0.7f
#define MIDDLE_GREY 0.1999f

float absf (float x)
{
	return x < 0 ? -x : x;
}


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

void writeTexel(
	__global write_only image2d_t out,
	int2 coord,
	float4 value)
{
	if ((coord.x < get_image_width(out)) && (coord.y < get_image_height(out)))
		write_imagef(out, coord, value);
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


__kernel void prepareHDR(
	__global read_only image2d_t inImageRGB,
	__global write_only image2d_t outImageY,
	__global write_only image2d_t outImageU,
	__global write_only image2d_t outImageV)
{
	int2 coord = {get_global_id(0), get_global_id(1)};
	float4 color = read_imagef(inImageRGB, c_sampler, coord);

	float4 yuv = rgb2logYUV(color);

	writeTexel(outImageY, coord, yuv.x);
	writeTexel(outImageU, coord, yuv.y);
	writeTexel(outImageV, coord, yuv.z);
}



__kernel void performToneMapping(
	__global read_only image2d_t inImageY,
	__global write_only image2d_t outImageY)
{
	int2 coord = {get_global_id(0), get_global_id(1)};
	float y = read_imagef(inImageY, c_sampler, coord).x;

	if (coord.x != 0 || coord.y != 0)
		y = sign(y)*pow(absf(y), BETA);
	else
		y = log(MIDDLE_GREY);

	writeTexel(outImageY, coord, y);
}



__kernel void createLDR(
	__global read_only image2d_t inImageY,
	__global write_only image2d_t outImageRGB,
	__global read_only image2d_t inImageU,
	__global read_only image2d_t inImageV)
{
	int2 coord = {get_global_id(0), get_global_id(1)};
	float y = read_imagef(inImageY, c_sampler, coord).x;
	float u = read_imagef(inImageU, c_sampler, coord).x;
	float v = read_imagef(inImageV, c_sampler, coord).x;
	
	float4 yuv = {y, u, v, 1};
	float4 rgba = logYUV2rgb(yuv);
	
	writeTexel(outImageRGB, coord, rgba);
}