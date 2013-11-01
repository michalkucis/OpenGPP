/////////////////////////////////////////////////////////////////////////////////
//
//  DistortioWarpFunctor: obaluje algoritmus sietoveho warpingu
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "hdrimage.h"
#include "type.h"
#include <vector>

class DistortionWarpFunctor
{
	PtrHDRImage3f m_image;
	uint2 m_size;
	uint2 m_totalsize;
	PtrHDRImage1f m_gridX, m_gridY;

	typedef std::vector<float2> vec2F_t;
	typedef std::vector<float> vecF_t;
public:
	DistortionWarpFunctor (PtrHDRImage3f image, PtrHDRImage1f gridX, PtrHDRImage1f gridY)
	{
		m_image = image;
		m_size = image->getSize();
		m_totalsize = image->getTotalSize();
		m_gridX = gridX;
		m_gridY = gridY;
	}
private:
	enum GRID_TYPE {T_X, T_Y};
	float getGridF (GRID_TYPE type, uint x, uint y)
	{ 
		if (x > 0xf0000000)
			x = 0;
		if (y > 0xf0000000)
			y = 0;
		if (x > m_gridX->getSize().x-1)
			x = m_gridX->getSize().x-1;
		if (y > m_gridX->getSize().y-1)
			y = m_gridX->getSize().y-1;
		if (type == T_X)
			return m_gridX->getPixel (x,y);
		else
			return m_gridY->getPixel (x,y);
	}
	void getLineGridIntersects (uint nLine, vecF_t& intersects)
	{
		uint countX = m_gridX->getSize().x;
		uint countY = m_gridY->getSize().y;
		float stepY = 1.0f/m_size.y;
		float actualY = stepY*(nLine+0.5f);

		intersects.resize (countX);
		for (uint x = 0; x < countX; x++)
		{
			int indexY = 0;
			for (uint y = 0; y < countY; y++)
				if (m_gridY->getPixel (x, y) <= actualY)
					indexY = y+1;
			
			float lowerY = getGridF (T_Y, x, indexY-1);
			float higherY = getGridF (T_Y, x, indexY);
			float sizeY = higherY - lowerY;
			float rel = (actualY-lowerY) / sizeY;
			if (! sizeY)
				rel = 0;
			float lowerX = getGridF (T_X, x, indexY-1);
			float higherX = getGridF (T_X, x, indexY);
			intersects[x] = higherX*rel + lowerX*(1-rel);
		}
	}
	void getLineRegularIntersects (uint nColumn, vecF_t& intersects)
	{
		uint count = m_gridX->getSize().x;
		float step = 1.0f/(count-1);
		
		intersects.resize (count);
		for (uint i = 0; i < count; i++)
			intersects[i] = step*i;
	}
	// nazvy premennych su ech, fuj
	void getColumnGridIntersects (uint nColumn, vecF_t& intersects)
	{
		uint countX = m_gridX->getSize().y;
		uint countY = m_gridY->getSize().x;
		float stepY = 1.0f/m_size.x;
		float actualY = stepY*(nColumn+0.5f);

		intersects.resize (countX);
		for (uint x = 0; x < countX; x++)
		{
			int indexY = 0;
			for (uint y = 0; y < countY; y++)
				if (getGridF (T_X, y, x) <= actualY)
					indexY = y+1;

			float lowerY = getGridF (T_X, indexY-1, x);
			float higherY = getGridF (T_X, indexY, x);
			float sizeY = higherY - lowerY;
			float rel = (actualY-lowerY) / sizeY;
			if (! sizeY)
				rel = 0;
			float lowerX = getGridF (T_Y, indexY-1, x);
			float higherX = getGridF (T_Y, indexY, x);
			intersects[x] = higherX*rel + lowerX*(1-rel);
		}
	}
	void getColumnRegularIntersects (uint nColumn, vecF_t& intersects)
	{
		uint count = m_gridX->getSize().y;
		float step = 1.0f/(count-1);
		
		intersects.resize (count);
		for (uint i = 0; i < count; i++)
			intersects[i] = step*i;
	}
	float& getPixel (float rel, uint size, float* pixels)
	{
		uint index = (uint)(rel*(size));
		index = index > 0xf0000000 ? 0 : index;
		index = index >= size ? size-1 : index;
		//assert (index < size);
		return pixels[index];
	} 
	void getRelToIndexBlock (float x, vecF_t& intersect, float& relBlock, uint& indexOfBlock)
	{
		uint index = 0;
		for (uint i = 0; i < intersect.size(); i++)
		{
			if (x < intersect[i])
				break;
			index = i+1;
		}
		if (index <= 0)
		{
			relBlock = 0;
			indexOfBlock = 1;
			return;
		}
		else if (index >= intersect.size())
		{
			relBlock = 1;
			indexOfBlock = intersect.size()-1;
			return;
		}
		float lowX = intersect[index-1];
		float highX = intersect[index];
		float sizeX = highX - lowX;
		float rel = (x-lowX)/sizeX;
		relBlock = rel;
		indexOfBlock = index;
	}
	void getIndexBlockToRel (float relBlock, uint indexOfBlock, vecF_t& intersect, float& x)
	{
		float lowX = intersect[indexOfBlock-1];
		float highX = intersect[indexOfBlock];
		float sizeX = highX - lowX;
		x = (relBlock*sizeX)+lowX;
	}

	int roundf (float x)
	{
		return (int)std::floor (x + 0.5f);
	}
	float mysincf (float x)
	{
		const float pi = 3.14159265f;

		if (x==0)
			return 1;
		return sinf (pi * x) / (pi * x);
	}
	template <typename int a>
	float lanczos (float x)
	{
		return mysincf (x / a) * mysincf (x);
		/*if (x > 0.5f || x < -0.5f)
			return 0;
		return 1;*/
	}
	float lanczosFilter (float x, uint size, float* arr)
	{
#define a 2
		float value = 0.0f;
		float centerX = x;
		float sum = 0.0f;
		for (int j = -a; j <= a; j++)
		{
			float x = centerX + j;
			int index = roundf(x);
			int clampindex = std::min<int> (std::max<int> (index,0), size-1); 
			sum += arr[clampindex] * lanczos<2>(centerX - index);
		}
		return sum;
#undef a
	}
	void warp (float* in, float* out, uint size, vecF_t& src, vecF_t& dst)
	{
		float step = 1.0f/size;
		for (uint index = 0; index < size; index++)
		{
			float x1 = step * (index + 0.5f);
			float x2;

			float relBlock; uint indexOfBlock;
			getRelToIndexBlock (x1, dst, relBlock, indexOfBlock);
			getIndexBlockToRel (relBlock, indexOfBlock, src, x2);
			x2 *= size;
			getPixel (x1, size, out) = lanczosFilter (x2, size, in);
		}
	}
	void loadLine (float* line, uint nChannel, uint nLine)
	{
		float* in;
		switch (nChannel)
		{
		case 0:
			in = m_image->getRedBuffer (); break;
		case 1:
			in = m_image->getGreenBuffer (); break;
		case 2:
			in = m_image->getBlueBuffer (); break;
		}
		memcpy (line, in + m_totalsize.x*nLine, m_size.x*sizeof(float));
	}
	void loadColumn (float* column, uint nChannel, uint nColumn)
	{
		float* in;
		switch (nChannel)
		{
		case 0:
			in = m_image->getRedBuffer (); break;
		case 1:
			in = m_image->getGreenBuffer (); break;
		case 2:
			in = m_image->getBlueBuffer (); break;
		}
		for (uint i = 0; i < m_size.y; i++)
			column[i] = in[nColumn + m_totalsize.x*i];
	}
	void storeLine (float* line, uint nChannel, uint nLine)
	{
		float* in;
		switch (nChannel)
		{
		case 0:
			in = m_image->getRedBuffer (); break;
		case 1:
			in = m_image->getGreenBuffer (); break;
		case 2:
			in = m_image->getBlueBuffer (); break;
		}
		memcpy (in + m_totalsize.x*nLine, line, m_size.x*sizeof(float));
	}
	void storeColumn (float* column, uint nChannel, uint nColumn)
	{
		float* in;
		switch (nChannel)
		{
		case 0:
			in = m_image->getRedBuffer (); break;
		case 1:
			in = m_image->getGreenBuffer (); break;
		case 2:
			in = m_image->getBlueBuffer (); break;
		}
		for (uint i = 0; i < m_size.y; i++)
			in[nColumn + m_totalsize.x*i] = column[i];
	}
public:
	void applyToLine (uint nChannel, uint nLine)
	{
		assert (nChannel < 3);
		assert (nLine < m_size.y);
			
		float* in = new float [m_size.x];
		float* out = new float [m_size.x];
		//std::vector<float> in (m_size.x);
		//std::vector<float> out (m_size.x);

		loadLine (in, nChannel, nLine);
		vecF_t srcIntersect;
		vecF_t dstIntersect;
		getLineGridIntersects (nLine, dstIntersect);
		getLineRegularIntersects (nLine, srcIntersect);
		warp (in, out, m_size.x, srcIntersect, dstIntersect);
		storeLine (out, nChannel, nLine);
		
		delete [] in;
		delete [] out;
	}
	void applyToColumn (uint nChannel, uint nColumn)
	{
		assert (nChannel < 3);
		assert (nColumn < m_size.x);
			
		float* in = new float [m_size.y];
		float* out = new float [m_size.y];

		loadColumn (in, nChannel, nColumn);
		vecF_t srcIntersect;
		vecF_t dstIntersect;
		getColumnGridIntersects (nColumn, dstIntersect);
		getColumnRegularIntersects (nColumn, srcIntersect);
		warp (in, out, m_size.y, srcIntersect, dstIntersect);
		storeColumn (out, nChannel, nColumn);
		
		delete [] in;
		delete [] out;
	}
	void apply (uint nChannel)
	{
		for (uint nLine = 0; nLine < m_size.y; nLine++)
			applyToLine (nChannel, nLine);
		for (uint nColumn = 0; nColumn < m_size.x; nColumn++)
			applyToColumn (nChannel, nColumn);
	}
};