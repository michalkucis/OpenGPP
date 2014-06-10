#define ALPHA (-1.586134342f)
#define BETA  (-0.05298011854f)
#define GAMMA (0.8829110762f)
#define DELTA (0.4435068522)
#define ZETA1 (1.1496043988602418f)
#define ZETA2 (0.8697465511709978f)


float topGetFromSharedMem2(
	__local float* sharedMemory,
	int index)
{
	int nRow = get_local_id(1)*2+index-4;
	return sharedMemory[get_local_id(0) + abs(nRow)*get_local_size(0)];
}


void topTransform2(
	__global float* out,
	int stride,
	__local float* sharedMemory)
{
	int offset = get_local_size(0);
	int srcRow = get_local_id(1);
	float midres[7];
	
	for (int i = 0; i < 7; i++)
		midres[i]=topGetFromSharedMem2(sharedMemory,i+1);

	midres[0] += ALPHA*(topGetFromSharedMem2(sharedMemory, 0) + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5] + topGetFromSharedMem2(sharedMemory, 8));

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	
	out[get_global_id(0) + get_global_id(1)*2*stride] = midres[3]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*2+1)*stride] = midres[4]*ZETA2;
}

void topTransferMemoryFromBottomToTop2(
	__local float* sharedMemory)
{
	if (1/*get_local_id(1) >= 8*/)
	{
		int off = get_local_id(0) + get_local_id(1)*get_local_size(0);
		sharedMemory[off] = sharedMemory[off+(get_local_size(1)*2-4)*get_local_size(0)];
	}
}


void loadMemory2(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff = localOffX + localSizeX*(localOffY+8);
	int globalOff = get_global_id(0) + (get_global_id(1)+4+nWindowPos*get_local_size(1)*2)*stride;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localSizeX*localSizeY] = inout[globalOff + stride*localSizeY];
}



float middleGetFromSharedMem2(
	__local float* sharedMemory,
	int index)
{
	int nRow = get_local_id(1)*2+index;
	return sharedMemory[get_local_id(0) + nRow*get_local_size(0)];
}

void middleTransferMemoryFromBottomToTop2(
	__local float* sharedMemory)
{
	if (get_local_id(1) >= 4)
	{
		int off = get_local_id(0) + (get_local_id(1)-4)*2*get_local_size(0);
		int offOrg = off+(get_local_size(1)*2)*get_local_size(0);
		sharedMemory[off] = sharedMemory[offOrg];
		sharedMemory[off+get_local_size(0)] = sharedMemory[offOrg+get_local_size(0)];
	}
}

void middleTransform2(
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int offset = get_local_size(0);
	int srcRow = get_local_id(1);
	float midres[7];
	
	for (int i = 0; i < 7; i++)
		midres[i]=middleGetFromSharedMem2(sharedMemory,i+1);

	midres[0] += ALPHA*(middleGetFromSharedMem2(sharedMemory, 0) + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5] + middleGetFromSharedMem2(sharedMemory, 8));

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	
	out[get_global_id(0) + (get_global_id(1)*2+nWindowPos*get_local_size(1)*2)*stride] = midres[3]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*2+1+nWindowPos*get_local_size(1)*2)*stride] = midres[4]*ZETA2;
}


void bottomLoadMemory2(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff1 = get_local_id(0) + get_local_size(0)*(get_local_id(1)+4);
	int globalOff1 = get_global_id(0) + stride*(nWindowPos*get_local_size(1)*2+get_local_id(1));
	if(get_global_id(1) >= 4)
		sharedMemory[localOff1] = inout[globalOff1];

	int localOff2 = get_local_id(0) + get_local_size(0)*(get_local_id(1)+4+get_local_size(1));
	int globalOff2 = get_global_id(0) + stride*(nWindowPos*(get_local_size(1)*2)+get_local_size(1)+get_local_id(1));
	float loadedX = sharedMemory[localOff2] = inout[globalOff2];
	
	if(get_global_id(1) >= get_global_size(1)-5 && get_global_id(1) < get_global_size(1)-1)
	{
		int off = get_global_size(1)-get_global_id(1)-2;
		int localOff3 = get_local_id(0) + get_local_size(0)*(2*get_local_size(1)+4+off);
		sharedMemory[localOff3] = loadedX;
	}
}


__kernel void cdf97VertTwo(
	__global float* inout,
	__local float* sharedMemory,
	int nWindowPos,
	int stride)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);
	
	// load "top rectangle":
	int localOff = localOffX + localSizeX*localOffY;
	int globalOff = get_global_id(0) + get_global_id(1)*stride;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localSizeX*localSizeY] = inout[globalOff + stride*localSizeY];
	if(get_local_id(1) < 4)
		sharedMemory[localOff + localSizeX*localSizeY*2] = inout[globalOff + stride*localSizeY*2];
	barrier(CLK_LOCAL_MEM_FENCE);

	// transform & save to global memory
	topTransform2(inout, stride, sharedMemory);
	barrier(CLK_LOCAL_MEM_FENCE);
	topTransferMemoryFromBottomToTop2(sharedMemory);
	
	for(int i = 1; i < nWindowPos; i++)
	{
		loadMemory2(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);

		middleTransform2(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);
		middleTransferMemoryFromBottomToTop2(sharedMemory);
	}

	bottomLoadMemory2(inout, stride, sharedMemory, nWindowPos);
	barrier(CLK_LOCAL_MEM_FENCE);
	middleTransform2(inout, stride, sharedMemory, nWindowPos);
}


//=====================================================================




float topGetFromSharedMem8(
	__local float* sharedMemory,
	int index)
{
	int nRow = get_local_id(1)*8+index-4;
	return sharedMemory[get_local_id(0) + abs(nRow)*get_local_size(0)];
}



void topTransform8(
	__global float* out,
	int stride,
	__local float* sharedMemory)
{
	int offset = get_local_size(0);
	int srcRow = get_local_id(1);
	float midres[13];
	
	for (int i = 0; i < 13; i++)
		midres[i]=topGetFromSharedMem8(sharedMemory,i+1);

	midres[0] += ALPHA*(topGetFromSharedMem8(sharedMemory, 0) + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5]+midres[7]);
	midres[8] += ALPHA*(midres[7]+midres[9]);
	midres[10]+= ALPHA*(midres[9]+midres[11]);
	midres[12]+= ALPHA*(midres[11] + topGetFromSharedMem8(sharedMemory, 14));

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);
	midres[7] += BETA*(midres[6]+midres[8]);
	midres[9] += BETA*(midres[8]+midres[10]);
	midres[11] += BETA*(midres[10]+midres[12]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);
	midres[6] += GAMMA*(midres[5]+midres[7]);
	midres[8] += GAMMA*(midres[7]+midres[9]);
	midres[10] += GAMMA*(midres[9]+midres[11]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	midres[5] += DELTA*(midres[4]+midres[6]);	
	midres[7] += DELTA*(midres[6]+midres[8]);	
	midres[9] += DELTA*(midres[8]+midres[10]);

	out[get_global_id(0) + (get_global_id(1)*8+0)*stride] = midres[3]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*8+1)*stride] = midres[4]*ZETA2;
	out[get_global_id(0) + (get_global_id(1)*8+2)*stride] = midres[5]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*8+3)*stride] = midres[6]*ZETA2;	
	out[get_global_id(0) + (get_global_id(1)*8+4)*stride] = midres[7]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*8+5)*stride] = midres[8]*ZETA2;	
	out[get_global_id(0) + (get_global_id(1)*8+6)*stride] = midres[9]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*8+7)*stride] = midres[10]*ZETA2;
}

void topTransferMemoryFromBottomToTop8(
	__local float* sharedMemory)
{
	if (1/*get_local_id(1) < 4*/)
	{
		int off = get_local_id(0) + get_local_id(1)*get_local_size(0);
		sharedMemory[off] = sharedMemory[off+(get_local_size(1)*8-4)*get_local_size(0)];
		//sharedMemory[off+get_local_size(0)] = sharedMemory[off+(get_local_size(1)*8-4)*get_local_size(0)+get_local_size(0)];
	}
}


void loadMemory8(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff = localOffX + localSizeX*(localOffY+8);
	int globalOff = get_global_id(0) + (get_global_id(1)+4+nWindowPos*get_local_size(1)*8)*stride;
	int localAppend = localSizeX*localSizeY;
	int globalAppend = stride*localSizeY;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localAppend*1] = inout[globalOff + globalAppend*1];
	sharedMemory[localOff + localAppend*2] = inout[globalOff + globalAppend*2];
	sharedMemory[localOff + localAppend*3] = inout[globalOff + globalAppend*3];
	sharedMemory[localOff + localAppend*4] = inout[globalOff + globalAppend*4];
	sharedMemory[localOff + localAppend*5] = inout[globalOff + globalAppend*5];
	sharedMemory[localOff + localAppend*6] = inout[globalOff + globalAppend*6];
	sharedMemory[localOff + localAppend*7] = inout[globalOff + globalAppend*7];
}



float middleGetFromSharedMem8(
	__local float* sharedMemory,
	int index)
{
	int nRow = get_local_id(1)*8+index;
	return sharedMemory[get_local_id(0) + nRow*get_local_size(0)];
}

void middleTransferMemoryFromBottomToTop8(
	__local float* sharedMemory)
{
	if (1/*get_local_id(1) < 4*/)
	{
		int off = get_local_id(0) + get_local_id(1)*get_local_size(0);
		int offOrg = off+(get_local_size(1)*8)*get_local_size(0);
		sharedMemory[off] = sharedMemory[offOrg];
		/*sharedMemory[off+get_local_size(0)] = sharedMemory[offOrg+get_local_size(0)];*/
	}
}

void middleTransform8(
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int offset = get_local_size(0);
	int srcRow = get_local_id(1);
	float midres[13];
	
	for (int i = 0; i < 13; i++)
		midres[i]=middleGetFromSharedMem8(sharedMemory,i+1);

	midres[0] += ALPHA*(middleGetFromSharedMem8(sharedMemory, 0) + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5]+midres[7]);
	midres[8] += ALPHA*(midres[7]+midres[9]);
	midres[10]+= ALPHA*(midres[9]+midres[11]);
	midres[12]+= ALPHA*(midres[11] + middleGetFromSharedMem8(sharedMemory, 14));

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);
	midres[7] += BETA*(midres[6]+midres[8]);
	midres[9] += BETA*(midres[8]+midres[10]);
	midres[11] += BETA*(midres[10]+midres[12]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);
	midres[6] += GAMMA*(midres[5]+midres[7]);
	midres[8] += GAMMA*(midres[7]+midres[9]);
	midres[10] += GAMMA*(midres[9]+midres[11]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	midres[5] += DELTA*(midres[4]+midres[6]);	
	midres[7] += DELTA*(midres[6]+midres[8]);	
	midres[9] += DELTA*(midres[8]+midres[10]);

	out[get_global_id(0) + (nWindowPos*get_global_size(1)*8+get_global_id(1)*8+0)*stride] = midres[3]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*8+get_global_id(1)*8+1)*stride] = midres[4]*ZETA2;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*8+get_global_id(1)*8+2)*stride] = midres[5]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*8+get_global_id(1)*8+3)*stride] = midres[6]*ZETA2;	
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*8+get_global_id(1)*8+4)*stride] = midres[7]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*8+get_global_id(1)*8+5)*stride] = midres[8]*ZETA2;	
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*8+get_global_id(1)*8+6)*stride] = midres[9]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*8+get_global_id(1)*8+7)*stride] = midres[10]*ZETA2;
}

void bottomLoadMemory8(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff1 = get_local_id(0) + get_local_size(0)*(get_local_id(1)+4);
	int globalOff1 = get_global_id(0) + stride*(nWindowPos*get_local_size(1)*8+get_local_id(1));
	if(get_global_id(1) >= 4)
		sharedMemory[localOff1] = inout[globalOff1];

	int localOff2 = get_local_id(0) + get_local_size(0)*(get_local_id(1)+4+get_local_size(1));
	int globalOff2 = get_global_id(0) + stride*(nWindowPos*(get_local_size(1)*8)+get_local_size(1)+get_local_id(1));
	int localAppend = get_local_size(0)*get_local_size(1);
	int globalAppend = stride*get_local_size(1);
	sharedMemory[localOff2 + localAppend*0] = inout[globalOff2 + globalAppend*0];
	sharedMemory[localOff2 + localAppend*1] = inout[globalOff2 + globalAppend*1];
	sharedMemory[localOff2 + localAppend*2] = inout[globalOff2 + globalAppend*2];
	sharedMemory[localOff2 + localAppend*3] = inout[globalOff2 + globalAppend*3];
	sharedMemory[localOff2 + localAppend*4] = inout[globalOff2 + globalAppend*4];
	sharedMemory[localOff2 + localAppend*5] = inout[globalOff2 + globalAppend*5];
	float loadedX = sharedMemory[localOff2 + localAppend*6] = inout[globalOff2 + globalAppend*6];
	
	if(get_global_id(1) >= get_global_size(1)-5 && get_global_id(1) < get_global_size(1)-1)
	{
		int off = get_global_size(1)-get_global_id(1)-2;
		int localOff3 = get_local_id(0) + get_local_size(0)*(8*get_local_size(1)+4+off);
		sharedMemory[localOff3] = loadedX;
	}
}


__kernel void cdf97VertTwo8Coef(
	__global float* inout,
	__local float* sharedMemory,
	int nWindowPos,
	int stride)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);
	
	// load "top rectangle":
	int localOff = localOffX + localSizeX*localOffY;
	int globalOff = get_global_id(0) + get_global_id(1)*stride;
	int localAppend = localSizeX*localSizeY;
	int globalAppend = stride*localSizeY;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localAppend] = inout[globalOff + globalAppend];
	sharedMemory[localOff + localAppend*2] = inout[globalOff + globalAppend*2];
	sharedMemory[localOff + localAppend*3] = inout[globalOff + globalAppend*3];
	sharedMemory[localOff + localAppend*4] = inout[globalOff + globalAppend*4];
	sharedMemory[localOff + localAppend*5] = inout[globalOff + globalAppend*5];
	sharedMemory[localOff + localAppend*6] = inout[globalOff + globalAppend*6];
	sharedMemory[localOff + localAppend*7] = inout[globalOff + globalAppend*7];
	if(get_local_id(1) < 4)
		sharedMemory[localOff + localAppend*8] = inout[globalOff + globalAppend*8];
	barrier(CLK_LOCAL_MEM_FENCE);

	// transform & save to global memory
	topTransform8(inout, stride, sharedMemory);
	barrier(CLK_LOCAL_MEM_FENCE);
	topTransferMemoryFromBottomToTop8(sharedMemory);

	for(int i = 1; i < nWindowPos; i++)
	{
		loadMemory8(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);

		middleTransform8(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);
		middleTransferMemoryFromBottomToTop8(sharedMemory);
	}
	
	bottomLoadMemory8(inout, stride, sharedMemory, nWindowPos);
	barrier(CLK_LOCAL_MEM_FENCE);
	middleTransform8(inout, stride, sharedMemory, nWindowPos);
}


//=============================================================


float topGetFromSharedMem4(
	__local float* sharedMemory,
	int index)
{
	int nRow = get_local_id(1)*4+index-4;
	return sharedMemory[get_local_id(0) + abs(nRow)*get_local_size(0)];
}



void topTransform4(
	__global float* out,
	int stride,
	__local float* sharedMemory)
{
	int offset = get_local_size(0);
	int srcRow = get_local_id(1);
	float midres[9];
	
	for (int i = 0; i < 9; i++)
		midres[i]=topGetFromSharedMem4(sharedMemory,i+1);

	midres[0] += ALPHA*(topGetFromSharedMem4(sharedMemory, 0) + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5]+midres[7]);
	midres[8] += ALPHA*(midres[7] + topGetFromSharedMem4(sharedMemory, 10));

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);
	midres[7] += BETA*(midres[6]+midres[8]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);
	midres[6] += GAMMA*(midres[5]+midres[7]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	midres[5] += DELTA*(midres[4]+midres[6]);

	out[get_global_id(0) + (get_global_id(1)*4+0)*stride] = midres[3]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*4+1)*stride] = midres[4]*ZETA2;
	out[get_global_id(0) + (get_global_id(1)*4+2)*stride] = midres[5]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*4+3)*stride] = midres[6]*ZETA2;	
}

void topTransferMemoryFromBottomToTop4(
	__local float* sharedMemory)
{
	if (1/*get_local_id(1) < 4*/)
	{
		int off = get_local_id(0) + get_local_id(1)*get_local_size(0);
		sharedMemory[off] = sharedMemory[off+(get_local_size(1)*4-4)*get_local_size(0)];
	}
}


void loadMemory4(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff = localOffX + localSizeX*(localOffY+8);
	int globalOff = get_global_id(0) + (get_global_id(1)+4+nWindowPos*get_local_size(1)*4)*stride;
	int localAppend = localSizeX*localSizeY;
	int globalAppend = stride*localSizeY;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localAppend*1] = inout[globalOff + globalAppend*1];
	sharedMemory[localOff + localAppend*2] = inout[globalOff + globalAppend*2];
	sharedMemory[localOff + localAppend*3] = inout[globalOff + globalAppend*3];
}



float middleGetFromSharedMem4(
	__local float* sharedMemory,
	int index)
{
	int nRow = get_local_id(1)*4+index;
	return sharedMemory[get_local_id(0) + nRow*get_local_size(0)];
}

void middleTransferMemoryFromBottomToTop4(
	__local float* sharedMemory)
{
	if (1/*get_local_id(1) < 4*/)
	{
		int off = get_local_id(0) + get_local_id(1)*get_local_size(0);
		int offOrg = off+(get_local_size(1)*4)*get_local_size(0);
		sharedMemory[off] = sharedMemory[offOrg];
		/*sharedMemory[off+get_local_size(0)] = sharedMemory[offOrg+get_local_size(0)];*/
	}
}

void middleTransform4(
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int offset = get_local_size(0);
	int srcRow = get_local_id(1);
	float midres[9];
	
	for (int i = 0; i < 9; i++)
		midres[i]=middleGetFromSharedMem4(sharedMemory,i+1);

	midres[0] += ALPHA*(middleGetFromSharedMem4(sharedMemory, 0) + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5]+midres[7]);
	midres[8] += ALPHA*(midres[7] + middleGetFromSharedMem4(sharedMemory, 10));

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);
	midres[7] += BETA*(midres[6]+midres[8]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);
	midres[6] += GAMMA*(midres[5]+midres[7]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	midres[5] += DELTA*(midres[4]+midres[6]);

	out[get_global_id(0) + (nWindowPos*get_global_size(1)*4+get_global_id(1)*4+0)*stride] = midres[3]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*4+get_global_id(1)*4+1)*stride] = midres[4]*ZETA2;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*4+get_global_id(1)*4+2)*stride] = midres[5]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*4+get_global_id(1)*4+3)*stride] = midres[6]*ZETA2;	
}

void bottomLoadMemory4(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff1 = get_local_id(0) + get_local_size(0)*(get_local_id(1)+4);
	int globalOff1 = get_global_id(0) + stride*(nWindowPos*get_local_size(1)*4+get_local_id(1));
	if(get_global_id(1) >= 4)
		sharedMemory[localOff1] = inout[globalOff1];

	int localOff2 = get_local_id(0) + get_local_size(0)*(get_local_id(1)+4+get_local_size(1));
	int globalOff2 = get_global_id(0) + stride*(nWindowPos*(get_local_size(1)*4)+get_local_size(1)+get_local_id(1));
	int localAppend = get_local_size(0)*get_local_size(1);
	int globalAppend = stride*get_local_size(1);
	sharedMemory[localOff2 + localAppend*0] = inout[globalOff2 + globalAppend*0];
	sharedMemory[localOff2 + localAppend*1] = inout[globalOff2 + globalAppend*1];
	float loadedX = sharedMemory[localOff2 + localAppend*2] = inout[globalOff2 + globalAppend*2];
	
	if(get_global_id(1) >= get_global_size(1)-5 && get_global_id(1) < get_global_size(1)-1)
	{
		int off = get_global_size(1)-get_global_id(1)-2;
		int localOff3 = get_local_id(0) + get_local_size(0)*(4*get_local_size(1)+4+off);
		sharedMemory[localOff3] = loadedX;
	}
}


__kernel void cdf97VertTwo4Coef(
	__global float* inout,
	__local float* sharedMemory,
	int nWindowPos,
	int stride)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);
	
	// load "top rectangle":
	int localOff = localOffX + localSizeX*localOffY;
	int globalOff = get_global_id(0) + get_global_id(1)*stride;
	int localAppend = localSizeX*localSizeY;
	int globalAppend = stride*localSizeY;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localAppend] = inout[globalOff + globalAppend];
	sharedMemory[localOff + localAppend*2] = inout[globalOff + globalAppend*2];
	sharedMemory[localOff + localAppend*3] = inout[globalOff + globalAppend*3];
	if(get_local_id(1) < 4)
		sharedMemory[localOff + localAppend*4] = inout[globalOff + globalAppend*4];
	barrier(CLK_LOCAL_MEM_FENCE);

	// transform & save to global memory
	topTransform4(inout, stride, sharedMemory);
	barrier(CLK_LOCAL_MEM_FENCE);
	topTransferMemoryFromBottomToTop4(sharedMemory);

	for(int i = 1; i < nWindowPos; i++)
	{
		loadMemory4(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);

		middleTransform4(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);
		middleTransferMemoryFromBottomToTop4(sharedMemory);
	}
	
	bottomLoadMemory4(inout, stride, sharedMemory, nWindowPos);
	barrier(CLK_LOCAL_MEM_FENCE);
	middleTransform4(inout, stride, sharedMemory, nWindowPos);
}



//=============================================================


float topGetFromSharedMem16(
	__local float* sharedMemory,
	int index)
{
	int nRow = get_local_id(1)*16+index-4;
	return sharedMemory[get_local_id(0) + abs(nRow)*get_local_size(0)];
}



void topTransform16(
	__global float* out,
	int stride,
	__local float* sharedMemory)
{
	int offset = get_local_size(0);
	int srcRow = get_local_id(1);
	float midres[21];
	
	for (int i = 0; i < 21; i++)
		midres[i]=topGetFromSharedMem16(sharedMemory,i+1);

	midres[0] += ALPHA*(topGetFromSharedMem16(sharedMemory, 0) + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5]+midres[7]);
	midres[8] += ALPHA*(midres[7]+midres[9]);
	midres[10] += ALPHA*(midres[9]+midres[11]);	
	midres[12] += ALPHA*(midres[11]+midres[13]);
	midres[14] += ALPHA*(midres[13]+midres[15]);
	midres[16] += ALPHA*(midres[15]+midres[17]);
	midres[18] += ALPHA*(midres[17]+midres[19]);
	midres[20] += ALPHA*(midres[19] + topGetFromSharedMem16(sharedMemory, 22));

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);
	midres[7] += BETA*(midres[6]+midres[8]);
	midres[9] += BETA*(midres[8]+midres[10]);
	midres[11] += BETA*(midres[10]+midres[12]);
	midres[13] += BETA*(midres[12]+midres[14]);
	midres[15] += BETA*(midres[14]+midres[16]);
	midres[17] += BETA*(midres[16]+midres[18]);
	midres[19] += BETA*(midres[18]+midres[20]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);
	midres[6] += GAMMA*(midres[5]+midres[7]);
	midres[8] += GAMMA*(midres[7]+midres[9]);
	midres[10] += GAMMA*(midres[9]+midres[11]);	
	midres[12] += GAMMA*(midres[11]+midres[13]);
	midres[14] += GAMMA*(midres[13]+midres[15]);
	midres[16] += GAMMA*(midres[15]+midres[17]);
	midres[18] += GAMMA*(midres[17]+midres[19]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	midres[5] += DELTA*(midres[4]+midres[6]);
	midres[7] += DELTA*(midres[6]+midres[8]);
	midres[9] += DELTA*(midres[8]+midres[10]);
	midres[11] += DELTA*(midres[10]+midres[12]);
	midres[13] += DELTA*(midres[12]+midres[14]);
	midres[15] += DELTA*(midres[14]+midres[16]);
	midres[17] += DELTA*(midres[16]+midres[18]);

	out[get_global_id(0) + (get_global_id(1)*16+0)*stride] = midres[3]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*16+1)*stride] = midres[4]*ZETA2;
	out[get_global_id(0) + (get_global_id(1)*16+2)*stride] = midres[5]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*16+3)*stride] = midres[6]*ZETA2;	
	out[get_global_id(0) + (get_global_id(1)*16+4)*stride] = midres[7]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*16+5)*stride] = midres[8]*ZETA2;
	out[get_global_id(0) + (get_global_id(1)*16+6)*stride] = midres[9]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*16+7)*stride] = midres[10]*ZETA2;	
	out[get_global_id(0) + (get_global_id(1)*16+8)*stride] = midres[11]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*16+9)*stride] = midres[12]*ZETA2;
	out[get_global_id(0) + (get_global_id(1)*16+10)*stride] = midres[13]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*16+11)*stride] = midres[14]*ZETA2;	
	out[get_global_id(0) + (get_global_id(1)*16+12)*stride] = midres[15]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*16+13)*stride] = midres[16]*ZETA2;
	out[get_global_id(0) + (get_global_id(1)*16+14)*stride] = midres[17]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*16+15)*stride] = midres[18]*ZETA2;	
}

void topTransferMemoryFromBottomToTop16(
	__local float* sharedMemory)
{
	if (1/*get_local_id(1) < 4*/)
	{
		int off = get_local_id(0) + get_local_id(1)*get_local_size(0);
		sharedMemory[off] = sharedMemory[off+(get_local_size(1)*16-4)*get_local_size(0)];
		//sharedMemory[off+get_local_size(0)] = sharedMemory[off+(get_local_size(1)*16-4)*get_local_size(0)+get_local_size(0)];
	}
}


void loadMemory16(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff = localOffX + localSizeX*(localOffY+8);
	int globalOff = get_global_id(0) + (get_global_id(1)+4+nWindowPos*get_local_size(1)*16)*stride;
	int localAppend = localSizeX*localSizeY;
	int globalAppend = stride*localSizeY;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localAppend*1] = inout[globalOff + globalAppend*1];
	sharedMemory[localOff + localAppend*2] = inout[globalOff + globalAppend*2];
	sharedMemory[localOff + localAppend*3] = inout[globalOff + globalAppend*3];
	sharedMemory[localOff + localAppend*4] = inout[globalOff + globalAppend*4];
	sharedMemory[localOff + localAppend*5] = inout[globalOff + globalAppend*5];
	sharedMemory[localOff + localAppend*6] = inout[globalOff + globalAppend*6];
	sharedMemory[localOff + localAppend*7] = inout[globalOff + globalAppend*7];
	sharedMemory[localOff + localAppend*8] = inout[globalOff + globalAppend*8];
	sharedMemory[localOff + localAppend*9] = inout[globalOff + globalAppend*9];
	sharedMemory[localOff + localAppend*10] = inout[globalOff + globalAppend*10];
	sharedMemory[localOff + localAppend*11] = inout[globalOff + globalAppend*11];
	sharedMemory[localOff + localAppend*12] = inout[globalOff + globalAppend*12];
	sharedMemory[localOff + localAppend*13] = inout[globalOff + globalAppend*13];
	sharedMemory[localOff + localAppend*14] = inout[globalOff + globalAppend*14];
	sharedMemory[localOff + localAppend*15] = inout[globalOff + globalAppend*15];
}



float middleGetFromSharedMem16(
	__local float* sharedMemory,
	int index)
{
	int nRow = get_local_id(1)*16+index;
	return sharedMemory[get_local_id(0) + nRow*get_local_size(0)];
}

void middleTransferMemoryFromBottomToTop16(
	__local float* sharedMemory)
{
	if (1/*get_local_id(1) < 4*/)
	{
		int off = get_local_id(0) + get_local_id(1)*get_local_size(0);
		int offOrg = off+(get_local_size(1)*16)*get_local_size(0);
		sharedMemory[off] = sharedMemory[offOrg];
		/*sharedMemory[off+get_local_size(0)] = sharedMemory[offOrg+get_local_size(0)];*/
	}
}

void middleTransform16(
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int offset = get_local_size(0);
	int srcRow = get_local_id(1);
	float midres[21];
	
	for (int i = 0; i < 21; i++)
		midres[i]=middleGetFromSharedMem16(sharedMemory,i+1);

	midres[0] += ALPHA*(middleGetFromSharedMem16(sharedMemory, 0) + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5]+midres[7]);
	midres[8] += ALPHA*(midres[7]+midres[9]);
	midres[10] += ALPHA*(midres[9]+midres[11]);	
	midres[12] += ALPHA*(midres[11]+midres[13]);
	midres[14] += ALPHA*(midres[13]+midres[15]);
	midres[16] += ALPHA*(midres[15]+midres[17]);
	midres[18] += ALPHA*(midres[17]+midres[19]);
	midres[20] += ALPHA*(midres[19] + middleGetFromSharedMem16(sharedMemory, 22));

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);
	midres[7] += BETA*(midres[6]+midres[8]);
	midres[9] += BETA*(midres[8]+midres[10]);
	midres[11] += BETA*(midres[10]+midres[12]);
	midres[13] += BETA*(midres[12]+midres[14]);
	midres[15] += BETA*(midres[14]+midres[16]);
	midres[17] += BETA*(midres[16]+midres[18]);
	midres[19] += BETA*(midres[18]+midres[20]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);
	midres[6] += GAMMA*(midres[5]+midres[7]);
	midres[8] += GAMMA*(midres[7]+midres[9]);
	midres[10] += GAMMA*(midres[9]+midres[11]);	
	midres[12] += GAMMA*(midres[11]+midres[13]);
	midres[14] += GAMMA*(midres[13]+midres[15]);
	midres[16] += GAMMA*(midres[15]+midres[17]);
	midres[18] += GAMMA*(midres[17]+midres[19]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	midres[5] += DELTA*(midres[4]+midres[6]);
	midres[7] += DELTA*(midres[6]+midres[8]);
	midres[9] += DELTA*(midres[8]+midres[10]);
	midres[11] += DELTA*(midres[10]+midres[12]);
	midres[13] += DELTA*(midres[12]+midres[14]);
	midres[15] += DELTA*(midres[14]+midres[16]);
	midres[17] += DELTA*(midres[16]+midres[18]);

	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+0)*stride] = midres[3]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+1)*stride] = midres[4]*ZETA2;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+2)*stride] = midres[5]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+3)*stride] = midres[6]*ZETA2;	
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+4)*stride] = midres[7]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+5)*stride] = midres[8]*ZETA2;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+6)*stride] = midres[9]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+7)*stride] = midres[10]*ZETA2;	
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+8)*stride] = midres[11]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+9)*stride] = midres[12]*ZETA2;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+10)*stride] = midres[13]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+11)*stride] = midres[14]*ZETA2;	
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+12)*stride] = midres[15]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+13)*stride] = midres[16]*ZETA2;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+14)*stride] = midres[17]*ZETA1;
	out[get_global_id(0) + (nWindowPos*get_global_size(1)*16+get_global_id(1)*16+15)*stride] = midres[18]*ZETA2;	
}

void bottomLoadMemory16(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff1 = get_local_id(0) + get_local_size(0)*(get_local_id(1)+4);
	int globalOff1 = get_global_id(0) + stride*(nWindowPos*get_local_size(1)*16+get_local_id(1));
	if(get_global_id(1) >= 4)
		sharedMemory[localOff1] = inout[globalOff1];

	int localOff2 = get_local_id(0) + get_local_size(0)*(get_local_id(1)+4+get_local_size(1));
	int globalOff2 = get_global_id(0) + stride*(nWindowPos*(get_local_size(1)*16)+get_local_size(1)+get_local_id(1));
	int localAppend = get_local_size(0)*get_local_size(1);
	int globalAppend = stride*get_local_size(1);
	sharedMemory[localOff2 + localAppend*0] = inout[globalOff2 + globalAppend*0];
	sharedMemory[localOff2 + localAppend*1] = inout[globalOff2 + globalAppend*1];
	sharedMemory[localOff2 + localAppend*2] = inout[globalOff2 + globalAppend*2];
	sharedMemory[localOff2 + localAppend*3] = inout[globalOff2 + globalAppend*3];
	sharedMemory[localOff2 + localAppend*4] = inout[globalOff2 + globalAppend*4];
	sharedMemory[localOff2 + localAppend*5] = inout[globalOff2 + globalAppend*5];
	sharedMemory[localOff2 + localAppend*6] = inout[globalOff2 + globalAppend*6];
	sharedMemory[localOff2 + localAppend*7] = inout[globalOff2 + globalAppend*7];
	sharedMemory[localOff2 + localAppend*8] = inout[globalOff2 + globalAppend*8];
	sharedMemory[localOff2 + localAppend*9] = inout[globalOff2 + globalAppend*9];
	sharedMemory[localOff2 + localAppend*10] = inout[globalOff2 + globalAppend*10];
	sharedMemory[localOff2 + localAppend*11] = inout[globalOff2 + globalAppend*11];
	sharedMemory[localOff2 + localAppend*12] = inout[globalOff2 + globalAppend*12];
	sharedMemory[localOff2 + localAppend*13] = inout[globalOff2 + globalAppend*13];
	float loadedX = sharedMemory[localOff2 + localAppend*14] = inout[globalOff2 + globalAppend*14];
	
	if(get_global_id(1) >= get_global_size(1)-5 && get_global_id(1) < get_global_size(1)-1)
	{
		int off = get_global_size(1)-get_global_id(1)-2;
		int localOff3 = get_local_id(0) + get_local_size(0)*(16*get_local_size(1)+4+off);
		sharedMemory[localOff3] = loadedX;
	}
}


__kernel void cdf97VertTwo16Coef(
	__global float* inout,
	__local float* sharedMemory,
	int nWindowPos,
	int stride)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);
	
	// load "top rectangle":
	int localOff = localOffX + localSizeX*localOffY;
	int globalOff = get_global_id(0) + get_global_id(1)*stride;
	int localAppend = localSizeX*localSizeY;
	int globalAppend = stride*localSizeY;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localAppend] = inout[globalOff + globalAppend];
	sharedMemory[localOff + localAppend*2] = inout[globalOff + globalAppend*2];
	sharedMemory[localOff + localAppend*3] = inout[globalOff + globalAppend*3];
	sharedMemory[localOff + localAppend*4] = inout[globalOff + globalAppend*4];
	sharedMemory[localOff + localAppend*5] = inout[globalOff + globalAppend*5];
	sharedMemory[localOff + localAppend*6] = inout[globalOff + globalAppend*6];
	sharedMemory[localOff + localAppend*7] = inout[globalOff + globalAppend*7];
	sharedMemory[localOff + localAppend*8] = inout[globalOff + globalAppend*8];
	sharedMemory[localOff + localAppend*9] = inout[globalOff + globalAppend*9];
	sharedMemory[localOff + localAppend*10] = inout[globalOff + globalAppend*10];
	sharedMemory[localOff + localAppend*11] = inout[globalOff + globalAppend*11];
	sharedMemory[localOff + localAppend*12] = inout[globalOff + globalAppend*12];
	sharedMemory[localOff + localAppend*13] = inout[globalOff + globalAppend*13];
	sharedMemory[localOff + localAppend*14] = inout[globalOff + globalAppend*14];
	sharedMemory[localOff + localAppend*15] = inout[globalOff + globalAppend*15];
	if(get_local_id(1) < 4)
		sharedMemory[localOff + localAppend*16] = inout[globalOff + globalAppend*16];
	barrier(CLK_LOCAL_MEM_FENCE);

	// transform & save to global memory
	topTransform16(inout, stride, sharedMemory);
	barrier(CLK_LOCAL_MEM_FENCE);
	topTransferMemoryFromBottomToTop16(sharedMemory);
	
	for(int i = 1; i < nWindowPos; i++)
	{
		loadMemory16(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);

		middleTransform16(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);
		middleTransferMemoryFromBottomToTop16(sharedMemory);
	}
	
	bottomLoadMemory16(inout, stride, sharedMemory, nWindowPos);
	barrier(CLK_LOCAL_MEM_FENCE);
	middleTransform16(inout, stride, sharedMemory, nWindowPos);
}


//===================================================================

float topGetCoefS(
	local float* sharedMemory,
	int id)
{
	int nRow = id + get_local_id(1)*2;
	return sharedMemory[get_local_id(0) + (abs(nRow)) * get_local_size(0)];
}

void topSetCoefS(
	local float* sharedMemory,
	int id,
	float x)
{
	sharedMemory[get_local_id(0) + (abs(id+get_local_id(1)*2)) * get_local_size(0)] = x;
}

void topTransformS(
	__global float* out,
	int stride,
	__local float* sharedMemory)
{
	float midres[4];

	midres[1] = topGetCoefS(sharedMemory, 0);
	midres[2] = topGetCoefS(sharedMemory, 1);
	midres[3] = topGetCoefS(sharedMemory, 2);

	midres[2] += ALPHA*(midres[1]+midres[3]);
	topSetCoefS(sharedMemory, 1, midres[2]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[0] = topGetCoefS(sharedMemory, -1);
	midres[1] += BETA*(midres[0]+midres[2]);
	topSetCoefS(sharedMemory, 0, midres[1]);
	barrier(CLK_LOCAL_MEM_FENCE);

	if (get_local_id(1) < 7)
	{
		midres[3] = topGetCoefS(sharedMemory, 2);
		midres[2] += GAMMA*(midres[1]+midres[3]);
		topSetCoefS(sharedMemory, 1, midres[2]);
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	if (get_local_id(1) < 7)
	{
		midres[0] = topGetCoefS(sharedMemory, -1);
		midres[1] += DELTA*(midres[0]+midres[2]);
		topSetCoefS(sharedMemory, 0, midres[1]);
	}
	

	if (get_local_id(1) < 7)
	{
		out[get_global_id(0) + (get_global_id(1)*2+0)*stride] = midres[1]*ZETA1;
		out[get_global_id(0) + (get_global_id(1)*2+1)*stride] = midres[2]*ZETA2;
	}
}


void topTransferMemoryFromBottomToTopS(
	__local float* sharedMemory)
{
	if (get_local_id(1) < 4)
	{
		int off = get_local_id(0) + get_local_id(1)*get_local_size(0);
		sharedMemory[off] = sharedMemory[off+(get_local_size(1)*2-3)*get_local_size(0)];
	}
}



void loadMemoryS(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff = localOffX + localSizeX*(localOffY+4);
	int globalOff = get_global_id(0) + (get_global_id(1)+1+nWindowPos*get_local_size(1)*2)*stride;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localSizeX*localSizeY] = inout[globalOff + stride*localSizeY];
}


//float middleGetFromSharedMem2(
//	__local float* sharedMemory,
//	int index)
//{
//	int nRow = get_local_id(1)*2+index;
//	return sharedMemory[get_local_id(0) + nRow*get_local_size(0)];
//}

float middleGetCoefS(
	local float* sharedMemory,
	int id)
{
	int nRow = id + 3 + get_local_id(1)*2;
	return sharedMemory[get_local_id(0) + nRow * get_local_size(0)];
}

void middleSetCoefS(
	local float* sharedMemory,
	int id,
	float x)
{
	sharedMemory[get_local_id(0) + (abs(id + 3 + get_local_id(1)*2)) * get_local_size(0)] = x;
}

void middleTransformS(
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	float midres[3];

	midres[0] = middleGetCoefS(sharedMemory, 0);
	midres[1] = middleGetCoefS(sharedMemory, 1);
	midres[2] = middleGetCoefS(sharedMemory, 2);

	midres[1] += ALPHA*(midres[0]+midres[2]);
	middleSetCoefS(sharedMemory, 1, midres[1]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[2] = midres[1];
	midres[1] = midres[0];
	midres[0] = middleGetCoefS(sharedMemory, -1);
	midres[1] += BETA*(midres[0]+midres[2]);
	middleSetCoefS(sharedMemory, 0, midres[1]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[2] = midres[1];
	midres[1] = midres[0];
	midres[0] = middleGetCoefS(sharedMemory, -2);
	midres[1] += GAMMA*(midres[0]+midres[2]);
	middleSetCoefS(sharedMemory, -1, midres[1]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[2] = midres[1];
	midres[1] = midres[0];
	midres[0] = middleGetCoefS(sharedMemory, -3);
	midres[1] += DELTA*(midres[0]+midres[2]);
	//middleSetCoefS(sharedMemory, -2, midres[1]);

	out[get_global_id(0) + (get_global_id(1)*2-2+get_global_size(1)*2*nWindowPos)*stride] = midres[1]*ZETA1;
	out[get_global_id(0) + (get_global_id(1)*2-1+get_global_size(1)*2*nWindowPos)*stride] = midres[2]*ZETA2;
}


void bottomTransformS(
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	if (get_local_id(1)==0)
	{
		float midres[3];
		int offLocal = get_local_id(0) + get_local_size(0)*(2*get_local_size(1)+1);

		barrier(CLK_LOCAL_MEM_FENCE);
		midres[0] = sharedMemory[offLocal];
		midres[1] = sharedMemory[offLocal+get_local_size(0)];
		midres[2] = midres[0];

		midres[1] += GAMMA*(midres[0]+midres[2]);
		middleSetCoefS(sharedMemory, -1, midres[1]);
		barrier(CLK_LOCAL_MEM_FENCE);

		midres[2] = midres[1];
		midres[1] = midres[0];
		midres[0] = sharedMemory[offLocal-get_local_size(0)];
		midres[1] += DELTA*(midres[0]+midres[2]);
		//middleSetCoefS(sharedMemory, -2, midres[1]);

		out[get_global_id(0) + (-2+get_global_size(1)*2*(nWindowPos+1))*stride] = midres[1]*ZETA1;
		out[get_global_id(0) + (-1+get_global_size(1)*2*(nWindowPos+1))*stride] = midres[2]*ZETA2;
	}
}


void middleTransferMemoryFromBottomToTopS(
	__local float* sharedMemory)
{
	if (get_local_id(1) < 4)
	{
		int off = get_local_id(0) + get_local_id(1)*get_local_size(0);
		int offOrg = off + 2*get_local_size(1)*get_local_size(0);
		sharedMemory[off] = sharedMemory[offOrg];
	}
}




void bottomLoadMemoryS(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff1 = get_local_id(0) + get_local_size(0)*(get_local_id(1)+3);
	int globalOff1 = get_global_id(0) + stride*(nWindowPos*get_local_size(1)*2+get_local_id(1));
	if(get_global_id(1) >= 1)
		sharedMemory[localOff1] = inout[globalOff1];

	int localOff2 = get_local_id(0) + get_local_size(0)*(get_local_id(1)+3+get_local_size(1));
	int globalOff2 = get_global_id(0) + stride*(nWindowPos*(get_local_size(1)*2)+get_local_size(1)+get_local_id(1));
	float loadedX = sharedMemory[localOff2] = inout[globalOff2];
	
	if(get_global_id(1) == get_global_size(1)-2)
	{
		int off = get_global_size(1)-get_global_id(1)-1;
		int localOff3 = get_local_id(0) + get_local_size(0)*(2*get_local_size(1)+3);
		sharedMemory[localOff3] = loadedX;
	}
}


__kernel void cdf97VertTwoSync(
	__global float* inout,
	__local float* sharedMemory,
	int nWindowPos,
	int stride)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);
	
	// load "top rectangle":
	int localOff = localOffX + localSizeX*localOffY;
	int globalOff = get_global_id(0) + get_global_id(1)*stride;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localSizeX*localSizeY] = inout[globalOff + stride*localSizeY];
	if(get_local_id(1) < 1)
		sharedMemory[localOff + localSizeX*localSizeY*2] = inout[globalOff + stride*localSizeY*2];
	barrier(CLK_LOCAL_MEM_FENCE);

	// transform & save to global memory
	topTransformS(inout, stride, sharedMemory);
	barrier(CLK_LOCAL_MEM_FENCE);
	topTransferMemoryFromBottomToTopS(sharedMemory);
//	barrier(CLK_LOCAL_MEM_FENCE);
	barrier(CLK_LOCAL_MEM_FENCE);
	for(int i = 1; i < nWindowPos; i++)
	{
		loadMemoryS(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);

		middleTransformS(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);
		middleTransferMemoryFromBottomToTopS(sharedMemory);
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	bottomLoadMemoryS(inout, stride, sharedMemory, nWindowPos);
	barrier(CLK_LOCAL_MEM_FENCE);
	middleTransformS(inout, stride, sharedMemory, nWindowPos);
	bottomTransformS(inout, stride, sharedMemory, nWindowPos);
}


//================================================================================


__kernel void transposeBest(
	__global float* in,
	__global float* out,
	__local float* sharedMemory,
	int stride)
{
	int2 blockSize = {32, 32};
	int globalSrc = get_global_id(0)+stride*get_global_id(1)*4;
	int localSrc = get_local_id(0)+get_local_id(1)*blockSize.x*4;
	int globalOffsetSrc = stride;
	int localOffsetSrc  = get_local_size(0);

	sharedMemory[localSrc] = in[globalSrc];
	sharedMemory[localSrc + localOffsetSrc*1] = in[globalSrc + globalOffsetSrc*1];
	sharedMemory[localSrc + localOffsetSrc*2] = in[globalSrc + globalOffsetSrc*2];
	sharedMemory[localSrc + localOffsetSrc*3] = in[globalSrc + globalOffsetSrc*3];
	barrier(CLK_LOCAL_MEM_FENCE);


	int globalDst = get_group_id(1)*blockSize.x+get_local_id(0)
		+ stride * (get_group_id(0)*blockSize.y+get_local_id(1)*4);
	int localDst = get_local_id(1)*4 + get_local_id(0)*blockSize.x;
	int globalOffsetDst = stride;
	int localOffsetDst = 1;

	out[globalDst] = sharedMemory[localDst];
	out[globalDst + globalOffsetDst*1] = sharedMemory[localDst + localOffsetDst*1];
	out[globalDst + globalOffsetDst*2] = sharedMemory[localDst + localOffsetDst*2];
	out[globalDst + globalOffsetDst*3] = sharedMemory[localDst + localOffsetDst*3];
}


//================================================================================


__kernel void horizLifting2Coef (
	__global float* inout,
	int stride,
	__local float* sharedMemory)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	float f = sharedMemory[4 + id] = inout[id + nRow*stride];
	if (id > 0 && id < 5)
		sharedMemory[4-id] = f; 
	f = sharedMemory[4 + id + 1*count] = inout[id + 1*count + nRow*stride];
	if (id > count-6 && id < count-1)
		sharedMemory[4 + 3*count - id - 2] = f;

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[7];
	for (int i = 0; i < 7; i++)
		midres[i]=sharedMemory[i + 1 + 2*id];

	midres[0] += ALPHA*(sharedMemory[2*id] + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6]+= ALPHA*(midres[5] + sharedMemory[2*id+8]);

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);

	midres[3] += DELTA*(midres[2]+midres[4]);

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*2 + 0] = midres[3]*ZETA1;
	sharedMemory[id*2 + 1] = midres[4]*ZETA2;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 2; i++)
		inout[id + i*count + nRow*stride] = sharedMemory[id + i*count];
}


void horizLifting2CoefPre (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	float f = sharedMemory[4 + id] = in[id + nRow*stride];
	if (id > 0 && id < 5)
		sharedMemory[4-id] = f; 
	for (int i = 1; i < 2; i++)
		sharedMemory[4 + i*count + id] = in[id + i*count + nRow*stride];
	if (id < 4)
		sharedMemory[4 + 2*count + id] = in[id + 2*count + nRow*stride];

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[7];
	for (int i = 0; i < 7; i++)
		midres[i]=sharedMemory[i + 1 + 2*id];

	midres[0] += ALPHA*(sharedMemory[2*id] + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6]+= ALPHA*(midres[5] + sharedMemory[2*id+8]);

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);

	midres[3] += DELTA*(midres[2]+midres[4]);

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*2 + 0] = midres[3]*ZETA1;
	sharedMemory[id*2 + 1] = midres[4]*ZETA2;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 2; i++)
		out[id + i*count + nRow*stride] = sharedMemory[id + i*count];
}


void horizLifting2CoefMiddle (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	if (id < 8)
		sharedMemory[id] = sharedMemory[id + 2*count];
	for (int i = 0; i < 2; i++)
		sharedMemory[8 + id + i*count] = in[4 + id + (i+2*nWindowPos)*count + nRow*stride];

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[7];
	for (int i = 0; i < 7; i++)
		midres[i]=sharedMemory[i + 1 + 2*id];

	midres[0] += ALPHA*(sharedMemory[2*id] + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5] + sharedMemory[2*id+8]);

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);

	midres[3] += DELTA*(midres[2]+midres[4]);

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*2 + 0] = midres[3]*ZETA1;
	sharedMemory[id*2 + 1] = midres[4]*ZETA2;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 2; i++)
		out[id + (i+nWindowPos*2)*count + nRow*stride] = sharedMemory[id + i*count];
}


void horizLifting2CoefPost (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	if (id < 8)
		sharedMemory[id] = sharedMemory[id + 2*count]; 

	sharedMemory[8 + id] = in[id + 4 + (nWindowPos*2)*count + nRow*stride];
	float f;
	if (id >= 4)
		f = sharedMemory[8 - 4 + id + 1*count] = in[id + (1 + nWindowPos*2)*count + nRow*stride];
	if (id > count-6 && id < count-1)
		sharedMemory[4 + 3*count - id - 2] = f;

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[7];
	for (int i = 0; i < 7; i++)
		midres[i]=sharedMemory[i + 1 + 2*id];

	midres[0] += ALPHA*(sharedMemory[2*id] + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5] + sharedMemory[2*id+8]);

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);

	midres[3] += DELTA*(midres[2]+midres[4]);

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*2 + 0] = midres[3]*ZETA1;
	sharedMemory[id*2 + 1] = midres[4]*ZETA2;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 2; i++)
		out[id + (i+nWindowPos*2)*count + nRow*stride] = sharedMemory[id + i*count];
}


__kernel void horizLifting2CoefWide (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	horizLifting2CoefPre(in, out, stride, sharedMemory);
	for (int i = 1; i < nWindowPos; i++)
		horizLifting2CoefMiddle(in, out, stride, sharedMemory, i);
	horizLifting2CoefPost(in, out, stride, sharedMemory, nWindowPos);
}


//================================================================================


__kernel void horizLifting8Coef (
	__global float* inout,
	int stride,
	__local float* sharedMemory)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	float f = sharedMemory[4 + id] = inout[id + nRow*stride];
	if (id > 0 && id < 5)
		sharedMemory[4-id] = f; 
	for (int i = 1; i < 7; i++)
		sharedMemory[4 + i*count + id] = inout[id + i*count + nRow*stride];
	f = sharedMemory[4 + id + 7*count] = inout[id + 7*count + nRow*stride];
	if (id > count-6 && id < count-1)
		sharedMemory[4 + 9*count - id - 2] = f;

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[13];
	for (int i = 0; i < 13; i++)
		midres[i]=sharedMemory[i + 1 + 8*id];

	midres[0] += ALPHA*(sharedMemory[8*id] + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5]+midres[7]);
	midres[8] += ALPHA*(midres[7]+midres[9]);
	midres[10]+= ALPHA*(midres[9]+midres[11]);
	midres[12]+= ALPHA*(midres[11] + sharedMemory[8*id+14]);

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);
	midres[7] += BETA*(midres[6]+midres[8]);
	midres[9] += BETA*(midres[8]+midres[10]);
	midres[11] += BETA*(midres[10]+midres[12]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);
	midres[6] += GAMMA*(midres[5]+midres[7]);
	midres[8] += GAMMA*(midres[7]+midres[9]);
	midres[10] += GAMMA*(midres[9]+midres[11]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	midres[5] += DELTA*(midres[4]+midres[6]);	
	midres[7] += DELTA*(midres[6]+midres[8]);	
	midres[9] += DELTA*(midres[8]+midres[10]);

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*8 + 0] = midres[3]*ZETA1;
	sharedMemory[id*8 + 1] = midres[4]*ZETA2;
	sharedMemory[id*8 + 2] = midres[5]*ZETA1;
	sharedMemory[id*8 + 3] = midres[6]*ZETA2;	
	sharedMemory[id*8 + 4] = midres[7]*ZETA1;
	sharedMemory[id*8 + 5] = midres[8]*ZETA2;	
	sharedMemory[id*8 + 6] = midres[9]*ZETA1;
	sharedMemory[id*8 + 7] = midres[10]*ZETA2;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 8; i++)
		inout[id + i*count + nRow*stride] = sharedMemory[id + i*count];
}


void horizLifting8CoefPre (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	float f = sharedMemory[4 + id] = in[id + nRow*stride];
	if (id > 0 && id < 5)
		sharedMemory[4-id] = f; 
	for (int i = 1; i < 8; i++)
		sharedMemory[4 + i*count + id] = in[id + i*count + nRow*stride];
	if (id < 4)
		sharedMemory[4 + 8*count + id] = in[id + 8*count + nRow*stride];

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[13];
	for (int i = 0; i < 13; i++)
		midres[i]=sharedMemory[i + 1 + 8*id];

	midres[0] += ALPHA*(sharedMemory[8*id] + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5]+midres[7]);
	midres[8] += ALPHA*(midres[7]+midres[9]);
	midres[10]+= ALPHA*(midres[9]+midres[11]);
	midres[12]+= ALPHA*(midres[11] + sharedMemory[8*id+14]);

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);
	midres[7] += BETA*(midres[6]+midres[8]);
	midres[9] += BETA*(midres[8]+midres[10]);
	midres[11] += BETA*(midres[10]+midres[12]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);
	midres[6] += GAMMA*(midres[5]+midres[7]);
	midres[8] += GAMMA*(midres[7]+midres[9]);
	midres[10] += GAMMA*(midres[9]+midres[11]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	midres[5] += DELTA*(midres[4]+midres[6]);	
	midres[7] += DELTA*(midres[6]+midres[8]);	
	midres[9] += DELTA*(midres[8]+midres[10]);

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*8 + 0] = midres[3]*ZETA1;
	sharedMemory[id*8 + 1] = midres[4]*ZETA2;
	sharedMemory[id*8 + 2] = midres[5]*ZETA1;
	sharedMemory[id*8 + 3] = midres[6]*ZETA2;	
	sharedMemory[id*8 + 4] = midres[7]*ZETA1;
	sharedMemory[id*8 + 5] = midres[8]*ZETA2;	
	sharedMemory[id*8 + 6] = midres[9]*ZETA1;
	sharedMemory[id*8 + 7] = midres[10]*ZETA2;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 8; i++)
		out[id + i*count + nRow*stride] = sharedMemory[id + i*count];
}


void horizLifting8CoefMiddle (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	if (id < 8)
		sharedMemory[id] = sharedMemory[id + 8*count];
	for (int i = 0; i < 8; i++)
		sharedMemory[8 + id + i*count] = in[4 + id + (i+8*nWindowPos)*count + nRow*stride];

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[13];
	for (int i = 0; i < 13; i++)
		midres[i]=sharedMemory[i + 1 + 8*id];

	midres[0] += ALPHA*(sharedMemory[8*id] + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5]+midres[7]);
	midres[8] += ALPHA*(midres[7]+midres[9]);
	midres[10]+= ALPHA*(midres[9]+midres[11]);
	midres[12]+= ALPHA*(midres[11] + sharedMemory[8*id+14]);

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);
	midres[7] += BETA*(midres[6]+midres[8]);
	midres[9] += BETA*(midres[8]+midres[10]);
	midres[11] += BETA*(midres[10]+midres[12]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);
	midres[6] += GAMMA*(midres[5]+midres[7]);
	midres[8] += GAMMA*(midres[7]+midres[9]);
	midres[10] += GAMMA*(midres[9]+midres[11]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	midres[5] += DELTA*(midres[4]+midres[6]);	
	midres[7] += DELTA*(midres[6]+midres[8]);	
	midres[9] += DELTA*(midres[8]+midres[10]);

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*8 + 0] = midres[3]*ZETA1;
	sharedMemory[id*8 + 1] = midres[4]*ZETA2;
	sharedMemory[id*8 + 2] = midres[5]*ZETA1;
	sharedMemory[id*8 + 3] = midres[6]*ZETA2;	
	sharedMemory[id*8 + 4] = midres[7]*ZETA1;
	sharedMemory[id*8 + 5] = midres[8]*ZETA2;	
	sharedMemory[id*8 + 6] = midres[9]*ZETA1;
	sharedMemory[id*8 + 7] = midres[10]*ZETA2;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 8; i++)
		out[id + (i+nWindowPos*8)*count + nRow*stride] = sharedMemory[id + i*count];
}


void horizLifting8CoefPost (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	if (id < 8)
		sharedMemory[id] = sharedMemory[id + 8*count]; 
	for (int i = 0; i < 7; i++)
		sharedMemory[8 + i*count + id] = in[id + 4 +(i+nWindowPos*8)*count + nRow*stride];
	float f;
	if (id >= 4)
		f = sharedMemory[8 - 4 + id + 7*count] = in[id + (7 + nWindowPos*8)*count + nRow*stride];
	if (id > count-6 && id < count-1)
		sharedMemory[4 + 9*count - id - 2] = f;

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[13];
	for (int i = 0; i < 13; i++)
		midres[i]=sharedMemory[i + 1 + 8*id];

	midres[0] += ALPHA*(sharedMemory[8*id] + midres[1]);
	midres[2] += ALPHA*(midres[1]+midres[3]);
	midres[4] += ALPHA*(midres[3]+midres[5]);
	midres[6] += ALPHA*(midres[5]+midres[7]);
	midres[8] += ALPHA*(midres[7]+midres[9]);
	midres[10]+= ALPHA*(midres[9]+midres[11]);
	midres[12]+= ALPHA*(midres[11] + sharedMemory[8*id+14]);

	midres[1] += BETA*(midres[0]+midres[2]);
	midres[3] += BETA*(midres[2]+midres[4]);
	midres[5] += BETA*(midres[4]+midres[6]);
	midres[7] += BETA*(midres[6]+midres[8]);
	midres[9] += BETA*(midres[8]+midres[10]);
	midres[11] += BETA*(midres[10]+midres[12]);

	midres[2] += GAMMA*(midres[1]+midres[3]);
	midres[4] += GAMMA*(midres[3]+midres[5]);
	midres[6] += GAMMA*(midres[5]+midres[7]);
	midres[8] += GAMMA*(midres[7]+midres[9]);
	midres[10] += GAMMA*(midres[9]+midres[11]);

	midres[3] += DELTA*(midres[2]+midres[4]);
	midres[5] += DELTA*(midres[4]+midres[6]);	
	midres[7] += DELTA*(midres[6]+midres[8]);	
	midres[9] += DELTA*(midres[8]+midres[10]);

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*8 + 0] = midres[3]*ZETA1;
	sharedMemory[id*8 + 1] = midres[4]*ZETA2;
	sharedMemory[id*8 + 2] = midres[5]*ZETA1;
	sharedMemory[id*8 + 3] = midres[6]*ZETA2;	
	sharedMemory[id*8 + 4] = midres[7]*ZETA1;
	sharedMemory[id*8 + 5] = midres[8]*ZETA2;	
	sharedMemory[id*8 + 6] = midres[9]*ZETA1;
	sharedMemory[id*8 + 7] = midres[10]*ZETA2;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 8; i++)
		out[id + (i+nWindowPos*8)*count + nRow*stride] = sharedMemory[id + i*count];
}


__kernel void horizLifting8CoefWide (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	horizLifting8CoefPre(in, out, stride, sharedMemory);
	for (int i = 1; i < nWindowPos; i++)
		horizLifting8CoefMiddle(in, out, stride, sharedMemory, i);
	horizLifting8CoefPost(in, out, stride, sharedMemory, nWindowPos);
}


//============================================================



__kernel void horizConv2Coef (
	__global float* inout,
	int stride,
	__local float* sharedMemory)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	float f = sharedMemory[4 + id] = inout[id + nRow*stride];
	if (id > 0 && id < 5)
		sharedMemory[4-id] = f; 
	f = sharedMemory[4 + id + 1*count] = inout[id + 1*count + nRow*stride];
	if (id > count-6 && id < count-1)
		sharedMemory[4 + 3*count - id - 2] = f;

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[9];
	for (int i = 0; i < 9; i++)
		midres[i]=sharedMemory[i + 2*id];

	float a = 0;
	a = midres[0] * 0.026748757411f;
	a += midres[1] * -0.016864118443f;
	a += midres[2] * -0.078223266529f;
	a += midres[3] * 0.266864118443f;
	a += midres[4] * 0.602949018236f;
	a += midres[5] * 0.266864118443f;
	a += midres[6] * -0.078223266529f;
	a += midres[7] * -0.016864118443f;
	a += midres[8] * 0.026748757411f;

	float d = 0;
	d = midres[2] * 0.091271763114f;
	d += midres[3] * -0.057543526229f;
	d += midres[4] * -0.591271763114f;
	d += midres[5] * 1.11508705f;
	d += midres[6] * -0.591271763114f;
	d += midres[7] * -0.057543526229f;
	d += midres[8] * 0.091271763114f;

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*2 + 0] = a;
	sharedMemory[id*2 + 1] = d;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 2; i++)
		inout[id + i*count + nRow*stride] = sharedMemory[id + i*count];
}


void horizConv2CoefPre (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	float f = sharedMemory[4 + id] = in[id + nRow*stride];
	if (id > 0 && id < 5)
		sharedMemory[4-id] = f; 
	for (int i = 1; i < 2; i++)
		sharedMemory[4 + i*count + id] = in[id + i*count + nRow*stride];
	if (id < 4)
		sharedMemory[4 + 2*count + id] = in[id + 2*count + nRow*stride];

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[9];
	for (int i = 0; i < 9; i++)
		midres[i]=sharedMemory[i + 2*id];

	float a = 0;
	a = midres[0] * 0.026748757411f;
	a += midres[1] * -0.016864118443f;
	a += midres[2] * -0.078223266529f;
	a += midres[3] * 0.266864118443f;
	a += midres[4] * 0.602949018236f;
	a += midres[5] * 0.266864118443f;
	a += midres[6] * -0.078223266529f;
	a += midres[7] * -0.016864118443f;
	a += midres[8] * 0.026748757411f;

	float d = 0;
	d = midres[2] * 0.091271763114f;
	d += midres[3] * -0.057543526229f;
	d += midres[4] * -0.591271763114f;
	d += midres[5] * 1.11508705f;
	d += midres[6] * -0.591271763114f;
	d += midres[7] * -0.057543526229f;
	d += midres[8] * 0.091271763114f;

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*2 + 0] = a;
	sharedMemory[id*2 + 1] = d;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 2; i++)
		out[id + i*count + nRow*stride] = sharedMemory[id + i*count];
}


void horizConv2CoefMiddle (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	if (id < 8)
		sharedMemory[id] = sharedMemory[id + 2*count];
	for (int i = 0; i < 2; i++)
		sharedMemory[8 + id + i*count] = in[4 + id + (i+2*nWindowPos)*count + nRow*stride];

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[9];
	for (int i = 0; i < 9; i++)
		midres[i]=sharedMemory[i + 2*id];

	float a = 0;
	a = midres[0] * 0.026748757411f;
	a += midres[1] * -0.016864118443f;
	a += midres[2] * -0.078223266529f;
	a += midres[3] * 0.266864118443f;
	a += midres[4] * 0.602949018236f;
	a += midres[5] * 0.266864118443f;
	a += midres[6] * -0.078223266529f;
	a += midres[7] * -0.016864118443f;
	a += midres[8] * 0.026748757411f;

	float d = 0;
	d = midres[2] * 0.091271763114f;
	d += midres[3] * -0.057543526229f;
	d += midres[4] * -0.591271763114f;
	d += midres[5] * 1.11508705f;
	d += midres[6] * -0.591271763114f;
	d += midres[7] * -0.057543526229f;
	d += midres[8] * 0.091271763114f;

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*2 + 0] = a;
	sharedMemory[id*2 + 1] = d;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 2; i++)
		out[id + (i+nWindowPos*2)*count + nRow*stride] = sharedMemory[id + i*count];
}


void horizConv2CoefPost (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);
	
	// Nacitani z globalni do lokalni pamati:
	if (id < 8)
		sharedMemory[id] = sharedMemory[id + 2*count]; 

	sharedMemory[8 + id] = in[id + 4 + (nWindowPos*2)*count + nRow*stride];
	float f;
	if (id >= 4)
		f = sharedMemory[8 - 4 + id + 1*count] = in[id + (1 + nWindowPos*2)*count + nRow*stride];
	if (id > count-6 && id < count-1)
		sharedMemory[4 + 3*count - id - 2] = f;

	barrier(CLK_LOCAL_MEM_FENCE);

	// Nacitanie z riadku:
	float midres[9];
	for (int i = 0; i < 9; i++)
		midres[i]=sharedMemory[i + 2*id];

	float a = 0;
	a = midres[0] * 0.026748757411f;
	a += midres[1] * -0.016864118443f;
	a += midres[2] * -0.078223266529f;
	a += midres[3] * 0.266864118443f;
	a += midres[4] * 0.602949018236f;
	a += midres[5] * 0.266864118443f;
	a += midres[6] * -0.078223266529f;
	a += midres[7] * -0.016864118443f;
	a += midres[8] * 0.026748757411f;

	float d = 0;
	d = midres[2] * 0.091271763114f;
	d += midres[3] * -0.057543526229f;
	d += midres[4] * -0.591271763114f;
	d += midres[5] * 1.11508705f;
	d += midres[6] * -0.591271763114f;
	d += midres[7] * -0.057543526229f;
	d += midres[8] * 0.091271763114f;

	barrier(CLK_LOCAL_MEM_FENCE);
	sharedMemory[id*2 + 0] = a;
	sharedMemory[id*2 + 1] = d;
	barrier(CLK_LOCAL_MEM_FENCE);

	for(int i = 0; i < 2; i++)
		out[id + (i+nWindowPos*2)*count + nRow*stride] = sharedMemory[id + i*count];
}


__kernel void horizConv2CoefWide (
	__global float* in,
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	horizConv2CoefPre(in, out, stride, sharedMemory);
	for (int i = 1; i < nWindowPos; i++)
		horizConv2CoefMiddle(in, out, stride, sharedMemory, i);
	horizConv2CoefPost(in, out, stride, sharedMemory, nWindowPos);
}




//=========================================================================================

float smallGetCoefS1(
	local float* sharedMemory,
	int stride,
	int id)
{
	int offset = id + get_local_id(0)*2;
	offset = ((offset>=stride) ? (2*stride-offset-2) : offset);
	return sharedMemory[abs(offset)];
}

void smallSetCoefS1(
	local float* sharedMemory,
	int id,
	float x)
{
	sharedMemory[get_local_id(0)*2 + id] = x;
}

__kernel void horizLiftSync2Coef (
	__global float* inout,
	int stride,
	__local float* sharedMemory)
{
	int id = get_local_id(0);
	int count = get_local_size(0);
	int nRow = get_global_id(1);

	sharedMemory[id] = inout[id + nRow*stride];
	sharedMemory[id + count] = inout[id + count + nRow*stride];
	barrier(CLK_LOCAL_MEM_FENCE);
	
	// Nacitanie z riadku:
	float midres[4];

	midres[1] = smallGetCoefS1(sharedMemory, stride, 0);
	midres[2] = smallGetCoefS1(sharedMemory, stride, 1);
	midres[3] = smallGetCoefS1(sharedMemory, stride, 2);

	midres[2] += ALPHA*(midres[1]+midres[3]);
	smallSetCoefS1(sharedMemory, 1, midres[2]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[0] = smallGetCoefS1(sharedMemory, stride, -1);
	midres[1] += BETA*(midres[0]+midres[2]);
	smallSetCoefS1(sharedMemory, 0, midres[1]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[3] = smallGetCoefS1(sharedMemory, stride, 2);
	midres[2] += GAMMA*(midres[1]+midres[3]);
	smallSetCoefS1(sharedMemory, 1, midres[2]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[0] = smallGetCoefS1(sharedMemory, stride, -1);
	midres[1] += DELTA*(midres[0]+midres[2]);
	smallSetCoefS1(sharedMemory, 0, midres[1]);

	sharedMemory[get_global_id(0)*2] = midres[1]*ZETA1;
	sharedMemory[get_global_id(0)*2+1] = midres[2]*ZETA2;
	barrier(CLK_LOCAL_MEM_FENCE);

	inout[get_global_id(0) + nRow*stride] = sharedMemory[get_global_id(0)];
	inout[get_global_id(0) + get_global_size(0) + nRow*stride] = sharedMemory[get_global_id(0)+get_global_size(0)];
}


//===================================================================

float topGetCoefS1(
	local float* sharedMemory,
	int id)
{
	int index = get_local_id(0)*2 + id;
	return sharedMemory[abs(index)];
}

void topSetCoefS1(
	local float* sharedMemory,
	int id,
	float x)
{
	sharedMemory[abs(get_local_id(0)*2+id)] = x;
}

void topTransformS1(
	__global float* out,
	int stride,
	__local float* sharedMemory)
{
	float midres[4];

	midres[1] = topGetCoefS1(sharedMemory, 0);
	midres[2] = topGetCoefS1(sharedMemory, 1);
	midres[3] = topGetCoefS1(sharedMemory, 2);

	midres[2] += ALPHA*(midres[1]+midres[3]);
	topSetCoefS1(sharedMemory, 1, midres[2]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[0] = topGetCoefS1(sharedMemory, -1);
	midres[1] += BETA*(midres[0]+midres[2]);
	topSetCoefS1(sharedMemory, 0, midres[1]);
	barrier(CLK_LOCAL_MEM_FENCE);
	
	if (get_local_id(0) < get_global_size(0)-1)
	{
		midres[3] = topGetCoefS1(sharedMemory, 2);
		midres[2] += GAMMA*(midres[1]+midres[3]);
		topSetCoefS1(sharedMemory, 1, midres[2]);
	}
	barrier(CLK_LOCAL_MEM_FENCE);
	
	if (get_local_id(0) < get_global_size(0)-1)
	{
		midres[0] = topGetCoefS1(sharedMemory, -1);
		midres[1] += DELTA*(midres[0]+midres[2]);
		topSetCoefS1(sharedMemory, 0, midres[1]);
	}

	if (get_local_id(0) < get_global_size(0)-1)
	{
		sharedMemory[get_global_id(0)*2] = midres[1]*ZETA1;
		sharedMemory[get_global_id(0)*2+1] = midres[2]*ZETA2;
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	
	out[get_global_id(0) + get_global_id(1)*stride] = sharedMemory[get_global_id(0)];
	if (get_local_id(0) < get_global_size(0)-2)
	{
		out[get_global_id(0) + get_global_size(0) + get_global_id(1)*stride] = sharedMemory[get_global_id(0)+get_global_size(0)];
	}
}


void topTransferMemoryFromBottomToTopS1(
	__local float* sharedMemory)
{
	if (get_local_id(0) < 4)
	{
		int off = get_local_id(0);
		sharedMemory[off] = sharedMemory[off+2*get_local_size(0)-3];
	}
}



void loadMemoryS1(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localOffX = get_local_id(0);

	int localOff = localOffX+4;
	int globalOff = get_global_id(0) + 1 + nWindowPos*2*get_global_size(0) + get_global_id(1)*stride;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localSizeX] = inout[globalOff + localSizeX];
}


//float middleGetFromSharedMem2(
//	__local float* sharedMemory,
//	int index)
//{
//	int nRow = get_local_id(1)*2+index;
//	return sharedMemory[get_local_id(0) + nRow*get_local_size(0)];
//}

float middleGetCoefS1(
	local float* sharedMemory,
	int id)
{
	return sharedMemory[id + 3 + get_local_id(0)*2];
}

void middleSetCoefS1(
	local float* sharedMemory,
	int id,
	float x)
{
	sharedMemory[get_local_id(0)*2 + id + 3] = x;
}

void middleTransformS1(
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	float midres[3];

	midres[0] = middleGetCoefS1(sharedMemory, 0);
	midres[1] = middleGetCoefS1(sharedMemory, 1);
	midres[2] = middleGetCoefS1(sharedMemory, 2);

	midres[1] += ALPHA*(midres[0]+midres[2]);
	middleSetCoefS1(sharedMemory, 1, midres[1]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[2] = midres[1];
	midres[1] = midres[0];
	midres[0] = middleGetCoefS1(sharedMemory, -1);
	midres[1] += BETA*(midres[0]+midres[2]);
	middleSetCoefS1(sharedMemory, 0, midres[1]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[2] = midres[1];
	midres[1] = midres[0];
	midres[0] = middleGetCoefS1(sharedMemory, -2);
	midres[1] += GAMMA*(midres[0]+midres[2]);
	middleSetCoefS1(sharedMemory, -1, midres[1]);
	barrier(CLK_LOCAL_MEM_FENCE);

	midres[2] = midres[1];
	midres[1] = midres[0];
	midres[0] = middleGetCoefS1(sharedMemory, -3);
	midres[1] += DELTA*(midres[0]+midres[2]);
	middleSetCoefS1(sharedMemory, -2, midres[1]);

	out[get_global_id(0)*2 - 2 + nWindowPos*2*get_global_size(0) + get_global_id(1)*stride] = midres[1]*ZETA1;
	out[get_global_id(0)*2+1 - 2 + nWindowPos*2*get_global_size(0) + get_global_id(1)*stride] = midres[2]*ZETA2;
}


void bottomTransformS1(
	__global float* out,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	if (get_local_id(0)==0)
	{
		float midres[3];
		int offLocal = get_local_size(0)*2;

		midres[0] = sharedMemory[offLocal+1];
		midres[1] = sharedMemory[offLocal+2];
		midres[2] = midres[0];

		midres[1] += GAMMA*(midres[0]+midres[2]);
		middleSetCoefS1(sharedMemory, -1, midres[1]);
		barrier(CLK_LOCAL_MEM_FENCE);

		midres[2] = midres[1];
		midres[1] = midres[0];
		midres[0] = sharedMemory[offLocal-get_local_size(0)];
		midres[1] += DELTA*(midres[0]+midres[2]);
		//middleSetCoefS(sharedMemory, -2, midres[1]);

		out[(nWindowPos+1)*2*get_global_size(0)-2 + get_global_id(1)*stride] = midres[1]*ZETA1;
		out[(nWindowPos+1)*2*get_global_size(0)-1 + get_global_id(1)*stride] = midres[2]*ZETA2;
	}
}


void middleTransferMemoryFromBottomToTopS1(
	__local float* sharedMemory)
{
	if (get_local_id(0) < 4)
	{
		int off = get_local_id(0);
		int offOrg = off + 2*get_local_size(0);
		sharedMemory[off] = sharedMemory[offOrg];
	}
}




void bottomLoadMemoryS1(
	__global float* inout,
	int stride,
	__local float* sharedMemory,
	int nWindowPos)
{
	int localSizeX = get_local_size(0);
	int localSizeY = get_local_size(1);
	int localOffX = get_local_id(0);
	int localOffY = get_local_id(1);

	int localOff1 = get_local_id(0) + 3;
	int globalOff1 = get_global_id(0) + get_global_size(0)*(nWindowPos*2) + stride*get_global_id(1);
	if(get_global_id(0) >= 1)
		sharedMemory[localOff1] = inout[globalOff1];

	int localOff2 = get_local_id(0) + get_global_size(0) + 3;
	int globalOff2 = get_global_id(0) + get_global_size(0)*(nWindowPos*2+1) + stride*get_global_id(1);
	float loadedX = sharedMemory[localOff2] = inout[globalOff2];
	
	if(get_global_id(0) == get_global_size(0)-2)
	{
		int localOff3 = get_local_size(0)*2 + 3;
		sharedMemory[localOff3] = loadedX;
	}
}


__kernel void cdf97horLiftTwoSync(
	__global float* inout,
	__local float* sharedMemory,
	int nWindowPos,
	int stride)
{
	int localSizeX = get_global_size(0);

	// load "top rectangle":
	int localOff = get_global_id(0);
	int globalOff = get_global_id(0) + get_global_id(1)*stride;
	sharedMemory[localOff] = inout[globalOff];
	sharedMemory[localOff + localSizeX] = inout[globalOff + localSizeX];
	if(get_local_id(0) < 1)
		sharedMemory[localOff + localSizeX*2] = inout[globalOff + localSizeX*2];
	barrier(CLK_LOCAL_MEM_FENCE);

	// transform & save to global memory
	topTransformS1(inout, stride, sharedMemory);
	barrier(CLK_LOCAL_MEM_FENCE);
	topTransferMemoryFromBottomToTopS1(sharedMemory);

	barrier(CLK_LOCAL_MEM_FENCE);
	for(int i = 1; i < nWindowPos; i++)
	{
		loadMemoryS1(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);

		middleTransformS1(inout, stride, sharedMemory, i);
		barrier(CLK_LOCAL_MEM_FENCE);
		middleTransferMemoryFromBottomToTopS1(sharedMemory);
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	bottomLoadMemoryS1(inout, stride, sharedMemory, nWindowPos);
	barrier(CLK_LOCAL_MEM_FENCE);
	middleTransformS1(inout, stride, sharedMemory, nWindowPos);
	bottomTransformS1(inout, stride, sharedMemory, nWindowPos);
}


//=========================================================================================







__kernel void empty()
{
}


//=============================================================