/////////////////////////////////////////////////////////////////////////////////
//
//  doflineargatherprimitive: efekt hlbky ostrosti zalozeny na variabilnom fir filtry
//	  - tato verzia nepodporuje intergralny obraz, preto je casovo velmi narocna pri velkom rozostreni
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once


class DOFlinearGatherPrimitive: public DOF
{
	HDRImage1f m_hdri1fTmp;
	HDRImage1f m_hdriCoC;
public:
	DOFlinearGatherPrimitive (PtrFuncFtoF func): DOF (func)
	{
	}
	float& getItem (int x, int y, int width, int height, float* channel)
	{
		return channel[x + y*width];
	}
	void doDOF (PtrHDRImage3f ptr_in, PtrHDRImage1f ptr_depth, PtrHDRImage3f &ptr_out)
	{
		assert (ptr_in->getSize() == ptr_depth->getSize()); 
	
		uint2 size (ptr_in->getSize());
		uint2 total (ptr_in->getTotalSize());
		uint2 sizeTmp (ptr_in->getSize().y, ptr_in->getSize().x);
		m_hdri1fTmp.setSize (sizeTmp);

		ptr_out = ptr_in;
		uint width = size.x;
		uint height = size.y;

		getHDRICoC (ptr_depth.get(), &m_hdriCoC);

		for (uint nColor = 0; nColor < 3; nColor++)
		{
			float* inChannel;
			float* tmpChannel;
			float* outChannel;
			tmpChannel = m_hdri1fTmp.getBuffer();
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
   			for (uint ny = 0; ny < height; ny++)
			{ 
				float* pn = &(getItem (0, ny, total.x, total.y, inChannel));
				float* radiusOfCoC = m_hdriCoC.getBuffer (uint2(0, ny));
				float* ptarget = &(getItem (0, ny, size.x, size.y, tmpChannel));
				for (uint nx = 0; nx < width; nx++)
				{
					int nsteps = (int) ceil (*radiusOfCoC);
					float weight = 1.0f/(nsteps*2-1);
					
					float acc = 0;
					
					float* p = pn;
					for (int x = nx, i = 0; i < nsteps; i++)
					{
						acc += *p;
						if (x!=width-1)
						{
							p++;
							x++;
						}
					}
					p = pn;
					for (int x = nx, i = 0; i < nsteps; i++)
					{
						acc += *p;
						if (x!=0)
						{
							p--;
							x--;
						}
					}
					acc -= *pn;
					float avg = acc * weight; 
					*ptarget = avg;
					ptarget++;
					pn++;
					radiusOfCoC++;
				}
			}
  			for (uint ny = 0; ny < height; ny++)
			{ 
				float* pn = &(getItem (0, ny, size.x, size.y, tmpChannel));
				float* radiusOfCoC = m_hdriCoC.getBuffer (uint2(0, ny));
				float* ptarget = &(getItem (0, ny, total.x, total.y, outChannel));
				for (uint nx = 0; nx < width; nx++)
				{
					int nsteps = (int) ceil (*radiusOfCoC);
					float weight = 1.0f/(nsteps*2-1);
					
					float acc = 0;
					
					float* p = pn;
					for (int y = ny, i = 0; i < nsteps; i++)
					{
						acc += *p;
						if (y!=height-1)
						{
							p+=size.x;
							y++;
						}
					}
					p = pn;
					for (int y = ny, i = 0; i < nsteps; i++)
					{
						acc += *p;
						if (y!=0)
						{
							p-=size.x;
							y--;
						}
					}
					acc -= *pn;
					float avg = acc * weight; 
					*ptarget = avg;
					ptarget++;
					pn++;
					radiusOfCoC++;
				}
			}
		}
	}
};