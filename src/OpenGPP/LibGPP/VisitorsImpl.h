#pragma once

#include "Visitors.h"
#include "Functions.h"
#include "Vector2.h"
#include <fstream>

class Visitor2WriteGridByFuncFtoF: public Visitor2WriteOnly<float2>
{
	Ptr<FuncFtoF> m_func;
	uint2 m_gridSize;
	float2 m_fgridSize;
public:
	Visitor2WriteGridByFuncFtoF (uint2 gridSize, Ptr<FuncFtoF> func)
	{
		m_func = func;
		m_gridSize = gridSize;
		m_fgridSize = gridSize.get2<float2>();
	}
	float2 visitWrite(int2 n)
	{
		float2 norm = n.get2<float2>()/(m_fgridSize-1);
		float2 pos = norm * 2 - 1;
		float2 dir = pos.getNormalised();

		float xPrev = pos.getLength()/sqrtf(2);
		float xNew = (*m_func)(xPrev);
		if (xPrev == 0)
			return float2(0,0);
		return dir*xNew*sqrtf(2);
	}
};

class Visitor2WriteGridByLinearInterp: public Visitor2WriteOnly<float2>
{
	uint2 m_gridSize;
	float2 m_fgridSize;
public:	
	Visitor2WriteGridByLinearInterp (uint2 gridSize)
	{
		m_gridSize = gridSize;
		m_fgridSize = gridSize.get2<float2>();
	}
	float2 visitWrite(int2 n)
	{
		float2 norm = n.get2<float2>()/(m_fgridSize-1);
		float2 pos = norm * 2 - 1;
		return pos;
	}
};

class Visitor2WriteRGBByFuncFtoF: public Visitor2WriteOnly<float3>
{
	Ptr<FuncFtoF> m_func;
	uint2 m_gridSize;
	float2 m_fgridSize;
public:
	Visitor2WriteRGBByFuncFtoF (uint2 gridSize, Ptr<FuncFtoF> func)
	{
		assert (! func.isNull());
		m_func = func;
		m_gridSize = gridSize;
		m_fgridSize = gridSize.get2<float2>();
	}
	float3 visitWrite(int2 n)
	{
		float2 fn = n.get2<float2>();
		float2 pos = fn - m_fgridSize/2 ;

		float xPrev = (pos/(m_fgridSize.x/2)).getLength();
		float xNew = (*m_func)(xPrev);
		float3 rgb (xNew);
		return rgb;
	}
};


class Visitor2ReadOnlyRedWriteToFile: public Visitor2ReadOnly<float>
{
	std::ofstream m_fs;
public:
	Visitor2ReadOnlyRedWriteToFile (string filename)
	{
		m_fs.open(filename);
		if (! m_fs.is_open())
			error1(ERR_NOT_FOUND, "File '%s' cannot be opened.", filename.c_str());
	}
	void visitRead (int2 n, const float& value)
	{
		if (n.x == 0)
			m_fs << std::endl;
		m_fs << "(" << int2str(n.x) << "," << int2str(n.y) << "):\t" << value << std::endl;
	}
};

class Visitor2ReadOnlyRGBWriteToFile: public Visitor2ReadOnly<float3>
{
	std::ofstream m_fs;
public:
	Visitor2ReadOnlyRGBWriteToFile (string filename)
	{
		m_fs.open(filename);
		if (! m_fs.is_open())
			error1(ERR_NOT_FOUND, "File '%s' cannot be opened.", filename.c_str());
	}
	void visitRead (int2 n, const float3& value)
	{
		if (n.x == 0)
			m_fs << std::endl;
		m_fs << "(" << int2str(n.x) << "," << int2str(n.y) << "):" 
			<< "\t" << value.x 
			<< "\t" << value.y
			<< "\t" << value.z << std::endl;
	}
};
