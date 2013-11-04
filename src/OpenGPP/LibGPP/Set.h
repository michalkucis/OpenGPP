#pragma once

#include <boost/container/set.hpp>
#include "Base.h"
#include "Error.h"
#include "Visitors.h"

template <typename T, typename COMPARE = std::less<T>, typename ALLOC = std::allocator<T>>
class Set: protected boost::container::set<T, COMPARE, ALLOC>
{
public:
	typedef T key_t;
	typedef const T ckey_t;
	typedef boost::container::set<T, COMPARE, ALLOC> set_t;
	Set<T,COMPARE,ALLOC> ()
	{}
	bool exist (key_t val)
	{
		set_t::iterator it = set_t::find (val);
		set_t::iterator itEnd = set_t::end();
		return it != itEnd;
	}
	bool insert (key_t val)
	{
		bool isAdded = ! exist(val);
		set_t::insert(val);
		return isAdded;
	}
	void erase (key_t val)
	{
		set_t::erase(val);
	}
	bool isEmpty ()
	{
		return set_t::empty();
	}
	T getFirst ()
	{
		if (isEmpty())
			error0(ERR_STRUCT, "Set is empty");
		set_t::iterator it = set_t::begin();
		return *it;
	}
	T getAndPopFirst()
	{
		if (isEmpty())
			error0(ERR_STRUCT, "Set is empty");
		set_t::iterator it = set_t::begin();
		T value = *it;
		popFirst();
		return value;
	}
	void popFirst ()
	{
		if (isEmpty())
			error0(ERR_STRUCT, "Set is empty");
		erase (getFirst());
	}
	void clear ()
	{
		set_t::clear ();
	}
	// potencionally dangerous.. do not change results of comparisons
	void visit (Visitor1ReadWrite<T>* visitor)
	{
		set_t::iterator it = set_t::begin();
		set_t::iterator itEnd = set_t::end();

		for (int index = 0; it != itEnd; it++, index++)
		{
			T& p = *it;
			visitor->visitReadWrite (index, p);
		}
	}	
	void visit (Visitor1ReadOnly<T>* visitor)
	{
		set_t::iterator it = set_t::begin();
		set_t::iterator itEnd = set_t::end();

		for (int index = 0; it != itEnd; it++, index++)
		{
			T& p = *it;
			visitor->visitRead (index, p);
		}
	}
};