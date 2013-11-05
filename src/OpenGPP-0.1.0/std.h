/////////////////////////////////////////////////////////////////////////////////
//
//  subor definuje logaritmus so zakladom 2
//  TODO
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <boost/shared_ptr.hpp>

float log2 (float value);

float clamp (float v, float minimum = 0.0f, float maximum = 1.0f);

template <typename T>
boost::shared_ptr<T> sharedNew ()
{
	return boost::shared_ptr<T> (new T); 
}
template <typename T, typename ARG>
boost::shared_ptr<T> sharedNew (ARG arg)
{
	return boost::shared_ptr<T> (new T(arg));
}
template <typename T, typename ARG1, typename ARG2>
boost::shared_ptr<T> sharedNew (ARG1 arg1, ARG2 arg2)
{
	return boost::shared_ptr<T> (new T(arg1, arg2));
}
template <typename T, typename ARG1, typename ARG2, typename ARG3>
boost::shared_ptr<T> sharedNew (ARG1 arg1, ARG2 arg2, ARG3 arg3)
{
	return boost::shared_ptr<T> (new T(arg1, arg2, arg3));
}
template <typename T, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
boost::shared_ptr<T> sharedNew (ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
{
	return boost::shared_ptr<T> (new T(arg1, arg2, arg3, arg4));
}
template <typename T>
boost::shared_ptr<T> sharedNull ()
{
	return boost::shared_ptr<T> ();
}