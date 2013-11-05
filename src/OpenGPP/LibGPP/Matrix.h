#pragma once

#include "Vector.h"
#include "Vector2.h"

template <typename T>
class Matrix
{
protected:
	uint2 m_size;
	Vector<Vector<T>> m_data;
public:
	Matrix (uint2 n = uint2(0,0))
	{
		setSize(n);
	}
	Matrix (Matrix<T>& mat)
	{
		this->operator = (mat);
	}
	void setSize (uint2 size)
	{
		m_data.setSize(size.x);
		for (uint i = 0; i < size.x; i++)
			m_data[i].setSize(size.y);
		m_size = size;
	}
	uint2 getSize () const
	{
		return m_size;
	}
	Vector<T>& operator[](uint nx)
	{
		return m_data[nx];
	}
	T& operator[](uint2 n)
	{
		return m_data[n.x][n.y];
	}
	Matrix<T>& operator=(Matrix<T>& mat)
	{
		setSize(mat.getSize());
		for (uint i = 0; i < m_data.getSize(); i++)
		{
			Vector<T>& vArg = mat[i];
			Vector<T>& vData = m_data[i];
			assert(vArg.getSize() == vData.getSize());
			vData = vArg;
		}
		return *this;
	}
	void visit(Visitor2WriteOnly<T>* visitor)
	{
		for(uint x = 0; x < m_size.x; x++)
			for(uint y = 0; y < m_size.y; y++)
			{
				m_data[x][y] = visitor->visitWrite(int2(x,y));
			}
	}
};