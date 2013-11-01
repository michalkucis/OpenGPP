/////////////////////////////////////////////////////////////////////////////////
//
//  dofdiffusion: efekt hlbky ostrosti zalozeny na diffuzii
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "buffer.h"

#include <algorithm>
class DOFdiffusion: public DOF
{
	uint steps;
	typedef Buffer<float> buffCOC_t, buffDepth_t;
	typedef Buffer<float3> buffABC_t, buffRGB_t;
	void createCOC (buffDepth_t in_depth, buffCOC_t& out_coc)
	{
		out_coc.create (in_depth.size);
		
		int area = in_depth.size.getArea();
		for (int i = 0; i < area; i++)
		{
			float z = in_depth.data[i];

			float coc = getRadiusOfCoC (z);
			out_coc.data[i] = coc;
		}
	}
	void createLineABC (buffCOC_t buffCOC, buffABC_t& out_buffABC)
	{
		uint width = buffCOC.size.x;
		out_buffABC.size = uint2(width, 1);
		out_buffABC.data = new float3 [out_buffABC.size.getArea()];
	}
	void initABCFromLine (uint y, buffCOC_t in_coc, buffABC_t& out_abc)
	{
		for (int x = 0; x < in_coc.size.x; x++)
		{
			float* coc = & (in_coc.data [x + y*in_coc.size.x]);
			float coc1 = x-1 < 0 ? 0 : coc[-1];
			float coc2 = coc[0];
			float coc3 = x+1 >= in_coc.size.x ? 0 : coc[1];
			float coc12 = std::min (coc1, coc2);
			float coc23 = std::min (coc2, coc3);
			float beta1 = coc12*coc12;
			float beta2 = coc23*coc23;

			float a = -beta1/steps;
			float b = (steps+beta1+beta2)/steps;
			float c = -beta2/steps;
			out_abc.data [x] = float3 (a, b, c);
		}
	}
	void initABCFromColumn (uint x, buffCOC_t in_coc, buffABC_t& out_abc)
	{
		uint width = in_coc.size.x;
		for (int y = 0; y < in_coc.size.y; y++)
		{
			float* coc = & (in_coc.data [x + y*width]);
			float coc1 = y-1 < 0 ? 0 : coc[-(int)width];
			float coc2 = coc[0];
			float coc3 = y+1 < in_coc.size.y ? coc[width] : 0;
			float coc12 = std::min (coc1, coc2);
			float coc23 = std::min (coc2, coc3);
			float beta1 = coc12*coc12;
			float beta2 = coc23*coc23;
			
			float a = -beta1/steps;
			float b = (steps+beta1+beta2)/steps;
			float c = -beta2/steps;
			out_abc.data [y] = float3 (a, b, c);
		}
	}
	void computeRGBline (buffABC_t in_abc, uint y, buffRGB_t& in_rgb, buffRGB_t& out_rgb)
	{
		const uint width = in_abc.size.x;
		float3* abc = in_abc.data;
		float3* d = in_rgb.data + width*y;//&(in_rgb.getData (0, y));
		float3* rgbout = out_rgb.data + width*y;
		float* a1 = new float [width];
		float* b1 = new float [width];
		float3* d1 = new float3 [width];

		for (int i = 0; i < steps; i++)
		{
			b1[0] = abc[0].y;
			for (int i = 1; i < width; i++)
			{
				a1[i] = abc[i].x/b1[i-1];
				b1[i] = abc[i].y - abc[i].x/b1[i-1]*abc[i-1].z;
			}
			d1[0] = d[0];
			for (int i = 1; i < width; i++)
				d1[i] = d[i] - d1[i-1]*a1[i];

			rgbout[width-1] = d1[width-1]/b1[width-1];
			for (int i = width-2; i >= 0; i--)
				rgbout[i] = (d1[i]-rgbout[i+1]*abc[i].z)/b1[i];
		}
		
		delete [] d1;
		delete [] b1;
		delete [] a1;
	}
	void computeRGBcol (buffABC_t in_abc, uint x, buffRGB_t& in_rgb, buffRGB_t& out_rgb)
	{
		const uint width = in_rgb.size.x;
		const uint height = in_rgb.size.y;
		float3* abc = in_abc.data;

		float3* _d = in_rgb.data + x;//&(in_rgb.getData (0, y));
		float3* _rgbout = out_rgb.data + x;
		
		float3* d = new float3 [height];
		for (uint i = 0; i < height; i++)
			d[i] = _d[i*width];

		float3* rgbout = new float3 [height];
		for (uint i = 0; i < height; i++)
			rgbout[i] = _rgbout[i*width];

		float* a1 = new float [height];
		float* b1 = new float [height];
		float3* d1 = new float3 [height];

		for (uint i = 0; i < steps; i++)
		{
			b1[0] = abc[0].y;
			for (int i = 1; i < height; i++)
			{
				a1[i] = abc[i].x/b1[i-1];
				b1[i] = abc[i].y - abc[i].x/b1[i-1]*abc[i-1].z;
			}
			d1[0] = d[0];
			for (int i = 1; i < height; i++)
				d1[i] = d[i] - d1[i-1]*a1[i];

			rgbout[height-1] = d1[height-1]/b1[height-1];
			for (int i = height-2; i >= 0; i--)
				rgbout[i] = (d1[i]-rgbout[i+1]*abc[i].z)/b1[i];
		}
		for (int i = 0; i < height; i++)
			_d[i*width] = d[i];

		for (int i = 0; i < height; i++)
			_rgbout[i*width] = rgbout[i];		
		
		delete [] d;
		delete [] rgbout;
		delete [] d1;
		delete [] b1;
		delete [] a1;
	}
	void fillRGBBuffer (HDRImage3f* in, buffRGB_t& buffRGB)
	{
		uint2 size = in->getSize();
		for (int nChannel = 0; nChannel < 3; nChannel++)
		{
			float* inBuff;
			switch (nChannel)
			{
			case 0:
				inBuff = in->getRedBuffer ();
				break;
			case 1:
				inBuff = in->getGreenBuffer ();
				break;
			case 2:
				inBuff = in->getBlueBuffer ();
				break;
			}
			float* outBuff = (((float*)(buffRGB.data))+nChannel);

			for (int y = 0; y < size.y; y++)
			{
				for (int x = 0; x < size.x; x++)
				{
					*outBuff = *inBuff;
					outBuff+=3;
					inBuff++;
				}
				inBuff += in->getAppendSize().x;
			}
		}
	}
	void fillHDR (buffRGB_t& in, HDRImage3f* out)
	{
		uint2 size = in.size;
		for (int nChannel = 0; nChannel < 3; nChannel++)
		{
			float* outChannel;
			switch (nChannel)
			{
			case 0:
				outChannel = out->getRedBuffer ();
				break;
			case 1:
				outChannel = out->getGreenBuffer ();
				break;
			case 2:
				outChannel = out->getBlueBuffer ();
				break;
			}
			float* inData =  ((float*)(in.data))+nChannel;

			for (uint y = 0; y < size.y; y++)
			{
				for (uint x = 0; x < size.x; x++)
				{
					*outChannel = *inData;
					inData+=3;
					outChannel++;
				}
				outChannel += out->getAppendSize().x;
			}
		}
	}
public:
	DOFdiffusion (PtrFuncRadiusOfCoC func): DOF(func)
	{
		steps = 1;
	}

	void doDOF (PtrHDRImage3f in, PtrHDRImage1f depth, PtrHDRImage3f &out)
	{
		uint width = in->getSize().x;
		uint height = in->getSize().y;
		uint2 size = in->getSize();
		//float* in_rgb, float* in_depth, uint width, uint height, float* out_rgb;

		buffDepth_t buffZ;
		buffZ.data = depth->getBuffer();
		buffZ.size = size;

		buffCOC_t buffCOC;
		createCOC (buffZ, buffCOC);
		
/*		out = in;
		memcpy (out->getRedBuffer(), buffZ.data, out->getSize().getArea()*sizeof(float));
		memcpy (out->getGreenBuffer(), buffZ.data, out->getSize().getArea()*sizeof(float));
		memcpy (out->getBlueBuffer(), buffZ.data, out->getSize().getArea()*sizeof(float));
		for (uint i = 0; i < out->getSize().getArea(); i++)
		{
			float add = -0.5;
			float mult = 1.0/2;
			*(out->getRedBuffer()+i) = add + mult * *(out->getRedBuffer()+i) ;
			*(out->getGreenBuffer()+i) = add + mult * *(out->getGreenBuffer()+i);
			*(out->getBlueBuffer()+i) = add + mult * *(out->getBlueBuffer()+i);
		}
		return; 
		
*/
		// show coc:
	/*	out = in;
		memcpy (out->getRedBuffer(), buffCOC.data, out->getSize().getArea()*sizeof(float));
		memcpy (out->getGreenBuffer(), buffCOC.data, out->getSize().getArea()*sizeof(float));
		memcpy (out->getBlueBuffer(), buffCOC.data, out->getSize().getArea()*sizeof(float));
		for (uint i = 0; i < out->getSize().getArea(); i++)
		{
			float coef = 0.05;
			*(out->getRedBuffer()+i) *= coef;
			*(out->getGreenBuffer()+i) *= coef;
			*(out->getBlueBuffer()+i) *= coef;
		}
		return; 
		*/
		buffABC_t buffABCline;
		createLineABC (buffCOC, buffABCline);
	
		buffRGB_t buffRGB;
		buffRGB.data = new float3 [size.getArea()];
		buffRGB.size = size;

		fillRGBBuffer (in.get(), buffRGB);

		for (uint y = 0; y < height; y++)
		{
			initABCFromLine (y, buffCOC, buffABCline);
			computeRGBline (buffABCline, y, buffRGB, buffRGB);
		}

		buffABC_t buffABCcol;
		createLineABC (buffCOC, buffABCcol);
		for (uint x = 0; x < width; x++)
		{
			initABCFromColumn (x, buffCOC, buffABCcol);
			computeRGBcol (buffABCcol, x, buffRGB, buffRGB);
		}

		out = in;
		fillHDR (buffRGB, out.get());

		buffABCcol.clear ();
		buffABCline.clear ();
		buffCOC.clear ();
		buffRGB.clear ();
	}

};