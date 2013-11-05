#pragma once

#include "Visitors.h"
#include "Error.h"

#include <boost/container/map.hpp>

template<
	typename KEY, 
	typename VALUE, 
	typename COMPARE = std::less<KEY>, 
	typename ALLOC = std::allocator<std::pair<const KEY, VALUE>>>
class Map: protected boost::container::map<KEY, VALUE, COMPARE, ALLOC>
{
	typedef KEY key_t;
	typedef const KEY ckey_t;
	typedef VALUE value_t;
	typedef boost::container::map<KEY, VALUE, COMPARE, ALLOC> map_t;
public:
	value_t& find(key_t& k)
	{
		map_t::iterator it = map_t::find(k);
		map_t::iterator itEnd = map_t::end();
		if (it == itEnd)
			error0(ERR_STRUCT, "The key cannot be found in the map");
		return it->second;
	}
	bool exist (key_t& k)
	{
		map_t::iterator it = map_t::find(k);
		map_t::iterator itEnd = map_t::end();
		return it != itEnd;
	}
	void insert (key_t& k, value_t& v)
	{
		bool isAdded = ! exist(k);
		if (isAdded)
			map_t::operator[](k) = v;
		else
			error0(ERR_STRUCT, "The key cannot be inserted into the map");
	}
	void erase (key_t& k)
	{
		map_t::erase(k);
	}
	bool isEmpty ()
	{
		return map_t::empty();
	}
	void visit (Visitor1ReadOnly<value_t>* visitor)
	{
		set_t::iterator it = map_t::begin();
		set_t::iterator itEnd = map_t::end();

		for (int index = 0; it != itEnd; it++, index++)
		{
			T& p = it->second;
			visitor->visitRead (index, p);
		}
	}
};