#pragma once

#include <boost/shared_ptr.hpp>
#include "Error.h"

template<typename T>
class Ptr: public boost::shared_ptr<T>
{
public:
	Ptr ()
	{
	}
	Ptr (const Ptr& ptr): boost::shared_ptr<T>(ptr)
	{
	}
	Ptr (T* ptr): boost::shared_ptr<T>(ptr)
	{
	}
	T& operator* ()
	{
		if (isNull())
			error0(ERR_STRUCT, "Invalid derefence");
		return boost::shared_ptr<T>::operator*();
	}
	T* operator-> ()
	{
		if (isNull())
			error0(ERR_STRUCT, "Invalid");
		return boost::shared_ptr<T>::operator->();
	}
	bool isNull ()
	{
		return boost::shared_ptr<T>::get() == NULL;
	}
	~Ptr ()
	{
	}
};