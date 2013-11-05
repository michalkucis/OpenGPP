/////////////////////////////////////////////////////////////////////////////////
//
//  initgrid: inicialuje deformacnu mriezku na zaklade distortion modelu
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "hdrimage.h"
#include "type.h"

void initGrid (PtrHDRImage1f gridX, PtrHDRImage1f gridY, float m1, float m2, float m3)
{
	uint count = 16;

	gridX->setSize (uint2(count,count));
	gridY->setSize (uint2(count,count));

	for (uint y = 0; y < count; y++)
		for (uint x = 0; x < count; x++)
		{
			float rx = x/((float)count-1)*2-1;
			float ry = y/((float)count-1)*2-1;

			float prevr;
			float r = prevr = sqrtf(rx*rx+ry*ry) / sqrtf(2);
			r = m1*r + m2*r*r + m3*r*r*r;
			
			rx *= r/prevr;
			ry *= r/prevr;

			rx = (rx+1)/2;
			ry = (ry+1)/2;

			gridX->setPixel(uint2(x,y), rx);
			gridY->setPixel(uint2(x,y), ry);
		}
}

void initGrid (PtrHDRImage1f gridX, PtrHDRImage1f gridY, uint2 sizeOfGrid, float2 aspectratio, FuncFtoF* func)
{
	gridX->setSize (sizeOfGrid);
	gridY->setSize (sizeOfGrid);

	//float invAspectratioLen = 1.0f/aspectratio.getLength();

	for (uint y = 0; y < sizeOfGrid.y; y++)
		for (uint x = 0; x < sizeOfGrid.x; x++)
		{
			float rx = x/((float)sizeOfGrid.x-1)*2-1;
			float ry = y/((float)sizeOfGrid.y-1)*2-1;

			rx*=aspectratio.x;
			ry*=aspectratio.y;

			float prevr;
			float r = prevr = sqrtf(rx*rx+ry*ry) / sqrtf(2);
			r = (*func) (r);

			rx/=aspectratio.x;
			ry/=aspectratio.y;
			
			rx *= r/prevr;
			ry *= r/prevr;

			rx = (rx+1)/2;
			ry = (ry+1)/2;

			gridX->setPixel(uint2(x,y), rx);
			gridY->setPixel(uint2(x,y), ry);
		}
}