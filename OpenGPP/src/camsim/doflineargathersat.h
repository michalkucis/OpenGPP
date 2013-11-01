/////////////////////////////////////////////////////////////////////////////////
//
//  doflineargathersat: efekt hlbky ostrosti zalozeny na variabilnom fir filtry
//	  - tato verzia vyuziva intergralny obraz
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once


class DOFlinearGatherSAT: public DOF
{
	HDRImage1f m_hdriCoC;
public:
	DOFlinearGatherSAT (PtrFuncFtoF func): DOF (func)
	{
	}
	float& getItem (int x, int y, int width, int height, float* channel)
	{
		return channel[x + y*width];
	}
	int2 clampPos (int2 pos, uint2 size)
	{
		pos.x = pos.x<0 ? 0 : pos.x;
		pos.x = pos.x>=size.x ? size.x-1 : pos.x;
		
		pos.y = pos.y<0 ? 0 : pos.y;
		pos.y = pos.y>=size.y ? size.y-1 : pos.y;
		return pos;
	}
	void doDOF (PtrHDRImage3f ptr_in, PtrHDRImage1f ptr_depth, PtrHDRImage3f &ptr_out)
	{
		assert (ptr_in->getSize() == ptr_depth->getSize()); 
	
		uint2 size (ptr_in->getSize());
		uint2 total (ptr_in->getTotalSize());
		
		ptr_out = ptr_in;
		uint width = size.x;
		uint height = size.y;

		getHDRICoC (ptr_depth.get(), &m_hdriCoC);

		for (uint nColor = 0; nColor < 3; nColor++)
		{
			float* inChannel;
			float* outChannel;
			HDRImage1d sat (ptr_in->getSize());
			switch (nColor)
			{
			case 0:
				inChannel = ptr_in->getRedBuffer();
				outChannel = ptr_out->getRedBuffer();
				break;
			case 1:
				inChannel = ptr_in->getBlueBuffer();
				outChannel = ptr_out->getBlueBuffer();
				break;
			case 2:
				inChannel = ptr_in->getGreenBuffer();
				outChannel = ptr_out->getGreenBuffer();
			}
			createSummedAreaTable (inChannel, sat.getBuffer(), size, ptr_in->getAppendSize(), ptr_out->getAppendSize());
   			for (uint ny = 0; ny < height; ny++)
			{ 
				for (uint nx = 0; nx < width; nx++)
				{
				//float* pn = &(getItem (0, ny, total.x, total.y, inChannel));
					float radiusOfCoC = m_hdriCoC.getPixel (uint2(nx, ny));
					float& ptarget = outChannel[nx+ny*ptr_in->getTotalSize().x];
					int offset = ((int) radiusOfCoC)-1;
					offset = offset<0 ? 0 : offset;
					
					double v[4];
#define GET(OFF1, OFF2) sat.getPixel(clampPos(int2(nx+OFF1,ny+OFF2),size).get<uint>())
					v[0] = GET(-offset-1, -offset-1);
					v[1] = GET(offset, -offset-1);
					v[2] = GET(-offset-1, offset);
					v[3] = GET(offset, offset);
#undef GET
					double res = v[0] - v[1] - v[2] + v[3];
					int2 posMin (nx - offset - 1, ny - offset -1);
					int2 posMax (nx + offset, ny + offset);
					posMin = clampPos (posMin, size);
					posMax = clampPos (posMax, size);
					int2 diff = posMax - posMin;

					res /= (diff.x)*(diff.y);
					outChannel [nx + ny*total.x] = (float) res;
				}
			}
		}
	}
};