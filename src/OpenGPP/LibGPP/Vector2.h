#pragma once

#include "Base.h"


#define DEFINE_VEC2_LOGICOP2(OP)\
	inline vec_t& operator OP (const vec_t& arg) \
	{\
	return ((x OP arg.x) || (y OP arg.y));\
	}

#define DEFINE_VEC2_ARITHMOP2(OP)\
	inline vec_t operator OP (const vec_t& arg) const\
	{\
	return vec_t (x OP arg.x, y OP arg.y);\
	}\
	inline vec_t operator OP (type_t val) const\
	{\
	return vec_t (x OP val, y OP val);\
	}
#define DEFINE_VEC2_ARITHMOP2ASS(OP)\
	inline vec_t& operator OP (const vec_t& arg)\
	{\
	x OP arg.x;\
	y OP arg.y;\
	return *this;\
	}	
#define DEFINE_VEC2_ARITHMOP2_SCALAR(OP)\
	inline vec_t operator OP (type_t arg)\
	{\
	return vec_t (x OP arg, y OP arg);\
	}
#define DEFINE_VEC2_ARITHMOP2ASS_SCALAR(OP)\
	inline vec_t& operator OP (type_t arg)\
	{\
	x OP arg;\
	y OP arg;\
	return *this;\
	}
#define DEFINE_VEC2_CONSTRUCTOR\
	vec_t ()\
	{\
	}\
	\
	vec_t (type_t arg)\
	{\
	x = arg; y = arg;\
	}\
	vec_t (type_t argX, type_t argY)\
	{\
	x = argX;\
	y = argY;\
	}\
	\
	vec_t (const type_t* arg)\
	{\
	x = arg[0];\
	y = arg[1];\
	}\
	\
	vec_t (const vec_t& arg)\
	{\
	x = arg.x;\
	y = arg.y;\
	}


template <typename T>
class Vector2
{
private:
	typedef T type_t;
	typedef Vector2<type_t> vec_t;

public:
	type_t x,y;

	inline vec_t& operator= (const vec_t& arg)
	{
		x = arg.x;
		y = arg.y;
		return *this;
	}

	inline vec_t operator- () const
	{
		return vec_t (-x, -y);
	}

	inline type_t getLengthSq () const
	{
		return x*x + y*y;
	}

	inline type_t getLength () const
	{
		return std::sqrt (getLengthSq ());
	}

	inline vec_t getAbs () const
	{
		return vec_t (abs(x), abs(y));
	}

	inline vec_t getNormalised () const
	{
		return (*this) / getLength ();
	}

	inline type_t getArea () const
	{
		return x*y;
	}

	static inline vec_t getMin (const vec_t& v1, const vec_t& v2)
	{
		vec_t v (
			(v1.x > v2.x ? v2.x : v1.x), 
			(v1.y > v2.y ? v2.y : v1.y));
		return v;
	}

	static inline vec_t getMax (const vec_t& v1, const vec_t& v2)
	{
		vec_t v (
			(v1.x < v2.x ? v2.x : v1.x), 
			(v1.y < v2.y ? v2.y : v1.y));
		return v;
	}

	inline type_t getMaxValue ()
	{
		type_t v(x > y ? x : y);
		return v;
	}

	inline type_t getMinValue ()
	{
		type_t v(x < y ? x : y);
		return v;
	}

	template <typename T>
	Vector2<T> get ()
	{
		Vector2<T> v ((T) x, (T) y);
		return v;
	}

	template <typename VEC_T>
	VEC_T get2 ()
	{
		VEC_T v ((VEC_T::type_t) x, (VEC_T::type_t) y);
		return v;
	}

	template <typename VEC_T>
	VEC_T get3 ()
	{
		VEC_T v ((VEC_T::type_t) x, (VEC_T::type_t) y, (VEC_T::type_t) 0);
		return v;
	}

	template <typename VEC_T>
	VEC_T get4 ()
	{
		VEC_T v ((VEC_T::type_t) x, (VEC_T::type_t) y, (VEC_T::type_t) 0, (VEC_T::type_t) 0);
		return v;
	}

	DEFINE_VEC2_CONSTRUCTOR

		DEFINE_VEC2_LOGICOP2(==)
		DEFINE_VEC2_LOGICOP2(!=)
		DEFINE_VEC2_LOGICOP2(<=)
		DEFINE_VEC2_LOGICOP2(>=)
		DEFINE_VEC2_LOGICOP2(<)
		DEFINE_VEC2_LOGICOP2(>)

		DEFINE_VEC2_ARITHMOP2(+)
		DEFINE_VEC2_ARITHMOP2(-)
		DEFINE_VEC2_ARITHMOP2(*)
		DEFINE_VEC2_ARITHMOP2(/)
		DEFINE_VEC2_ARITHMOP2(%)

		DEFINE_VEC2_ARITHMOP2ASS(+=)
		DEFINE_VEC2_ARITHMOP2ASS(-=)
		DEFINE_VEC2_ARITHMOP2ASS(*=)
		DEFINE_VEC2_ARITHMOP2ASS(/=)
		DEFINE_VEC2_ARITHMOP2ASS(%=)

		DEFINE_VEC2_ARITHMOP2_SCALAR(+)
		DEFINE_VEC2_ARITHMOP2_SCALAR(-)
		DEFINE_VEC2_ARITHMOP2_SCALAR(*)
		DEFINE_VEC2_ARITHMOP2_SCALAR(/)
		DEFINE_VEC2_ARITHMOP2_SCALAR(%)

		DEFINE_VEC2_ARITHMOP2ASS_SCALAR(+=)
		DEFINE_VEC2_ARITHMOP2ASS_SCALAR(-=)
		DEFINE_VEC2_ARITHMOP2ASS_SCALAR(*=)
		DEFINE_VEC2_ARITHMOP2ASS_SCALAR(/=)
		DEFINE_VEC2_ARITHMOP2ASS_SCALAR(%=)
};

class int2: public Vector2<int>
{
public:
	typedef int type_t;
	typedef int2 vec_t;

	DEFINE_VEC2_CONSTRUCTOR
	int2 (const Vector2<int>& v2)
	{
		x = v2.x;
		y = v2.y;
	}
};

class uint2: public Vector2<uint>
{
public:
	typedef uint type_t;
	typedef uint2 vec_t;

	DEFINE_VEC2_CONSTRUCTOR
	uint2 (const Vector2<uint>& v2)
	{
		x = v2.x;
		y = v2.y;
	}
};

class char2: public Vector2<char>
{
public:
	typedef char type_t;
	typedef char2 vec_t;

	DEFINE_VEC2_CONSTRUCTOR
		char2 (const Vector2<char>& v2)
	{
		x = v2.x;
		y = v2.y;
	}
};

class uchar2: public Vector2<uchar>
{
public:
	typedef uchar type_t;
	typedef uchar2 vec_t;

	DEFINE_VEC2_CONSTRUCTOR
		uchar2 (const Vector2<uchar>& v2)
	{
		x = v2.x;
		y = v2.y;
	}
};

class float2: public Vector2<float>
{
public:
	typedef float type_t;
	typedef float2 vec_t;

	DEFINE_VEC2_CONSTRUCTOR
		float2 (const Vector2<float>& v2)
	{
		x = v2.x;
		y = v2.y;
	}
};