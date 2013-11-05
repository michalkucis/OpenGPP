#pragma once


#include <boost/container/vector.hpp>
#include "Base.h"
#include "Error.h"


template <typename Key>
class Vector
{
public:
	typedef Key key_t;
	typedef const key_t ckey_t;
protected:
	typedef boost::container::vector<Key> vec_t;
	vec_t m_vec;
public:
	Vector<key_t> ()
	{

	}
	Vector<key_t> (const key_t& k)
	{
		pushBack(k);
	}
	uint getSize ()
	{
		return m_vec.size();
	}
	void setSize (uint size)
	{
		m_vec.resize(size);
	}
	void pushBack (const key_t& k)
	{
		m_vec.push_back(k);
	}
	void insert (uint index, key_t& k)
	{
		if (index > getSize())
			error0(ERR_STRUCT, "Out of bound");

		m_vec.insert(index, 1, k);
	}
	void clear ()
	{
		m_vec.clear();
	}

	key_t& operator [] (int index)
	{
		return m_vec[index];
	}
	Vector<key_t>& operator = (Vector<key_t>& v)
	{
		this->setSize (v.getSize());
		for (uint i = 0; i < v.getSize(); i++)
			this->operator[](i) = v[i];
		return *this;
	}
};

template <typename Key>
class VectorNoInit
{
public:
	typedef Key key_t;
	typedef const key_t ckey_t;
protected:
	key_t* m_data;
	uint m_size;
	uint m_allocSize;
public:
	~VectorNoInit<key_t> ()
	{
		for (uint i = 0; i < m_size; i++)
			m_data[i].~key_t();
		if (m_data)
			delete [] (m_data);
	}
	key_t& operator [] (uint index)
	{
		if (index >= m_size)
			error (ERR_STRUCT);

		return m_data[index];
	}
	uint getSize ()
	{
		return m_size;
	}
	void setSize (uint size)
	{
		if (! m_data)
		{
			m_data = (key_t*) malloc (sizeof(key_t)*size);
			m_allocSize = size;
		}

		if (size > m_allocSize)
		{
			m_data = (key_t*) realloc (m_data, sizeof(key_t)*size);
			m_allocSize = size;
		}

		for (int i = (int)m_size; i <= (int)size-1; i++)
			m_data[i] = key_t();

		for (int i = (int)m_size; i >= (int) size; i--)
			m_data[i].~key_t();

		m_size = size;
	}
	void pushBack (const key_t& k)
	{
		setSize (m_size+1);
		m_data[m_size-1] = k;
	}
	void clear ()
	{
		setSize(0);
		m_size = 0;
	}
};