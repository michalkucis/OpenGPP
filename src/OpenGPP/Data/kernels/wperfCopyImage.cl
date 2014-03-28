__kernel void copy1in1Image(
	__global read_only image2d_t in,
	__global write_only image2d_t out)
{
  sampler_t s = CLK_ADDRESS_CLAMP;
  int2 coord = {get_global_id(0), get_global_id(1)};
  float4 rgba = read_imagef(in, s, coord);
  barrier(CLK_GLOBAL_MEM_FENCE);
  write_imagef(out, coord, rgba);
}


//====================================================================


__kernel void copy2in1Image1(
	__global read_only image2d_t in,
	__global write_only image2d_t out)
{
  sampler_t s = CLK_ADDRESS_CLAMP;
  int2 coord1 = {get_global_id(0)*2, get_global_id(1)};
  int2 coord2 = {get_global_id(0)*2+1, get_global_id(1)};
  float4 rgba1 = read_imagef(in, s, coord1);
  float4 rgba2 = read_imagef(in, s, coord2);
  barrier(CLK_GLOBAL_MEM_FENCE);
  write_imagef(out, coord1, rgba1);
  if (get_global_id(0)*2+1 < get_image_width(in))
	 write_imagef(out, coord2, rgba2);
}


__kernel void copy2in1Image2(
	__global read_only image2d_t in,
	__global write_only image2d_t out)
{
  sampler_t s = CLK_ADDRESS_CLAMP;
  int2 coord1 = {get_global_id(0), get_global_id(1)*2};
  int2 coord2 = {get_global_id(0), get_global_id(1)*2+1};
  float4 rgba1 = read_imagef(in, s, coord1);
  float4 rgba2 = read_imagef(in, s, coord2);
  barrier(CLK_GLOBAL_MEM_FENCE);
  write_imagef(out, coord1, rgba1);
  if (get_global_id(1)*2+1 < get_image_height(in))
	 write_imagef(out, coord2, rgba2);
}


//====================================================================


__kernel void copy1in1Buffer1(
	__global read_only image2d_t in,
	__global float* out,
	int nItemsPerLine)
{
  sampler_t s = CLK_ADDRESS_CLAMP;
  int2 coord = {get_global_id(0), get_global_id(1)};
  float4 rgba = read_imagef(in, s, coord);
  barrier(CLK_GLOBAL_MEM_FENCE);
  out[coord.x + coord.y*nItemsPerLine] = rgba.x;
}


__kernel void copy1in1Buffer2(
	__global float* in,
	__global float* out,
	int nItemsPerLine)
{
  int2 coord = {get_global_id(0), get_global_id(1)};
  float4 rgba = in[coord.x + coord.y*nItemsPerLine];
  barrier(CLK_GLOBAL_MEM_FENCE);
  out[coord.x + coord.y*nItemsPerLine] = rgba.x;
}

__kernel void copy1in1Buffer3(
	__global float* in,
	__global write_only image2d_t out,
	int nItemsPerLine)
{
  int2 coord = {get_global_id(0), get_global_id(1)};
  float4 rgba = in[coord.x + coord.y*nItemsPerLine];
  barrier(CLK_GLOBAL_MEM_FENCE);
  write_imagef(out, coord, rgba);
}



//====================================================================


__kernel void copy2in1Buffer1(
	__global read_only image2d_t in,
	__global float* out,
	int nItemsPerLine)
{
  sampler_t s = CLK_ADDRESS_CLAMP;
  int2 coord1 = {get_global_id(0)*2, get_global_id(1)};
  int2 coord2 = {get_global_id(0)*2+1, get_global_id(1)};
  float r1 = read_imagef(in, s, coord1).x;
  float r2 = read_imagef(in, s, coord2).x;
  barrier(CLK_GLOBAL_MEM_FENCE);
  out[coord1.x + coord1.y*nItemsPerLine] = r1;
  if (get_global_id(0)*2+1 < nItemsPerLine)
	out[coord2.x + coord2.y*nItemsPerLine] = r2;
}


__kernel void copy2in1Buffer2(
	__global float* in,
	__global float* out,
	int nItemsPerLine)
{
  int2 coord1 = {get_global_id(0)*2, get_global_id(1)};
  int2 coord2 = {get_global_id(0)*2+1, get_global_id(1)};
  float r1 = in[coord1.x + coord1.y*nItemsPerLine];
  float r2 = in[coord2.x + coord2.y*nItemsPerLine];
  barrier(CLK_GLOBAL_MEM_FENCE);
  out[coord1.x + coord1.y*nItemsPerLine] = r1;
  if (get_global_id(0)*2+1 < nItemsPerLine)
	out[coord2.x + coord2.y*nItemsPerLine] = r2;
}

__kernel void copy2in1Buffer3(
	__global float* in,
	__global float* out,
	int nItemsPerLine)
{
  int2 coord1 = {get_global_id(0), get_global_id(1)*2};
  int2 coord2 = {get_global_id(0), get_global_id(1)*2+1};
  float r1 = in[coord1.x + coord1.y*nItemsPerLine];
  float r2 = in[coord2.x + coord2.y*nItemsPerLine];
  barrier(CLK_GLOBAL_MEM_FENCE);
  out[coord1.x + coord1.y*nItemsPerLine] = r1;
  if (get_global_id(1)*2+1 < nItemsPerLine)
	out[coord2.x + coord2.y*nItemsPerLine] = r2;
}

__kernel void copy2in1Buffer4(
	__global float* in,
	__global write_only image2d_t out,
	int nItemsPerLine)
{
  int2 coord1 = {get_global_id(0), get_global_id(1)*2};
  int2 coord2 = {get_global_id(0), get_global_id(1)*2+1};
  float r1 = in[coord1.x + coord1.y*nItemsPerLine];
  float r2 = in[coord2.x + coord2.y*nItemsPerLine];
  barrier(CLK_GLOBAL_MEM_FENCE);
  write_imagef(out, coord1, r1);
  if (get_global_id(1)*2+1 < get_image_height(out))
	 write_imagef(out, coord2, r2);
}


//====================================================================


bool isOKCoord(int2 coord, int nItemsPerDir)
{
	if (coord.x >= nItemsPerDir)
		return false;
	if (coord.y >= nItemsPerDir)
		return false;
	return true;
}

bool isOKCoordIm(
	int2 coord, 
	__global write_only image2d_t im)
{
	if (coord.x >= get_image_width(im))
		return false;
	if (coord.y >= get_image_height(im))
		return false;
	return true;
}


//====================================================================


__kernel void copy4in1Buffer1(
	__global read_only image2d_t in,
	__global float* out,
	int nItemsPerLine)
{
  sampler_t s = CLK_ADDRESS_CLAMP;
  int2 coord1 = {get_global_id(0)*2, get_global_id(1)*2};
  int2 coord2 = {get_global_id(0)*2+1, get_global_id(1)*2};
  int2 coord3 = {get_global_id(0)*2, get_global_id(1)*2+1};
  int2 coord4 = {get_global_id(0)*2+1, get_global_id(1)*2+1};
  float r1 = read_imagef(in, s, coord1).x;
  float r2 = read_imagef(in, s, coord2).x;
  float r3 = read_imagef(in, s, coord3).x;
  float r4 = read_imagef(in, s, coord4).x;
  barrier(CLK_GLOBAL_MEM_FENCE);
  if (isOKCoord(coord1, nItemsPerLine))
	  out[coord1.x + coord1.y*nItemsPerLine] = r1;
  if (isOKCoord(coord2, nItemsPerLine))
	  out[coord2.x + coord2.y*nItemsPerLine] = r2;
  if (isOKCoord(coord3, nItemsPerLine))
	  out[coord2.x + coord2.y*nItemsPerLine] = r3;
  if (isOKCoord(coord4, nItemsPerLine))
	  out[coord2.x + coord2.y*nItemsPerLine] = r4;
}


__kernel void copy4in1Buffer2(
	__global float* in,
	__global float* out,
	int nItemsPerLine)
{
  int2 coord1 = {get_global_id(0)*2, get_global_id(1)*2};
  int2 coord2 = {get_global_id(0)*2+1, get_global_id(1)*2};
  int2 coord3 = {get_global_id(0)*2, get_global_id(1)*2+1};
  int2 coord4 = {get_global_id(0)*2+1, get_global_id(1)*2+1};
  float r1 = in[coord1.x + coord1.y*nItemsPerLine];
  float r2 = in[coord2.x + coord2.y*nItemsPerLine];
  float r3 = in[coord3.x + coord3.y*nItemsPerLine];
  float r4 = in[coord4.x + coord4.y*nItemsPerLine];
  barrier(CLK_GLOBAL_MEM_FENCE);
  if (isOKCoord(coord1, nItemsPerLine))
	  out[coord1.x + coord1.y*nItemsPerLine] = r1;
  if (isOKCoord(coord2, nItemsPerLine))
	  out[coord2.x + coord2.y*nItemsPerLine] = r2;
  if (isOKCoord(coord3, nItemsPerLine))
	  out[coord2.x + coord2.y*nItemsPerLine] = r3;
  if (isOKCoord(coord4, nItemsPerLine))
	  out[coord2.x + coord2.y*nItemsPerLine] = r4;
}

__kernel void copy4in1Buffer3(
	__global float* in,
	__global write_only image2d_t out,
	int nItemsPerLine)
{
  int2 coord1 = {get_global_id(0)*2, get_global_id(1)*2};
  int2 coord2 = {get_global_id(0)*2+1, get_global_id(1)*2};
  int2 coord3 = {get_global_id(0)*2, get_global_id(1)*2+1};
  int2 coord4 = {get_global_id(0)*2+1, get_global_id(1)*2+1};
  float r1 = in[coord1.x + coord1.y*nItemsPerLine];
  float r2 = in[coord2.x + coord2.y*nItemsPerLine];
  float r3 = in[coord3.x + coord3.y*nItemsPerLine];
  float r4 = in[coord4.x + coord4.y*nItemsPerLine];
  barrier(CLK_GLOBAL_MEM_FENCE);

  if (isOKCoordIm(coord1, out))
	  write_imagef(out, coord1, r1);
  if (isOKCoordIm(coord2, out))
	  write_imagef(out, coord2, r2);
  if (isOKCoordIm(coord3, out))
	  write_imagef(out, coord3, r3);
  if (isOKCoordIm(coord4, out))
	  write_imagef(out, coord4, r4);
}