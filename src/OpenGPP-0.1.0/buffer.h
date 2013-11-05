/////////////////////////////////////////////////////////////////////////////////
//
//  buffer: obaluje funkcionalitu 2D bufferu
//  - sluzi ako pamatovo uspornejsi HDRImage oprosteny od 'vsetkej' funkcionality
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename T>
struct Buffer
{
public:
	T* data;
	uint2 size;

	void create (uint2 size)
	{
		this->size = size;
		data = new T [size.getArea()];
	}
	
	void clear ()
	{
		if (data)
			delete [] data;
		size = uint2 (0,0);
	}

	T getData (uint x, uint y)
	{
		return data[x + y*size.x];
	}

	T getData (uint2 pos)
	{
		return getData (pos.x, pos.y);
	}
	
	void setData (uint x, uint y, T newval)
	{
		data[x + y*size.x] = newval;
	}

	void setData (uint2 pos, T newval)
	{
		setData (pos.x, pos.y, newval);
	}

};