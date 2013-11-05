#pragma once

#include "Base.h"

#define DEFINE_VEC4_LOGICOP2(OP)\
	inline vec_t& operator OP (const vec_t& arg) \
	{\
	return ((x OP arg.x) || (y OP arg.y) || (z OP arg.z) || (w OP arg.w));\
	}
#define DEFINE_VEC4_ARITHMOP2(OP)\
	inline vec_t operator OP (const vec_t& arg) const\
	{\
	return vec_t (x OP arg.x, y OP arg.y, z OP arg.z, w OP arg.w);\
	}
#define DEFINE_VEC4_ARITHMOP2ASS(OP)\
	inline vec_t& operator OP (const vec_t& arg)\
	{\
	x OP arg.x;\
	y OP arg.y;\
	z OP arg.z;\
	w OP arg.w;\
	return *this;\
	}\
	inline vec_t& operator OP (vec_t arg)\
	{\
	x OP arg.x;\
	y OP arg.y;\
	z OP arg.z;\
	w OP arg.w;\
	return *this;\
	}
#define DEFINE_VEC4_ARITHMOP2_SCALAR(OP)\
	inline vec_t operator OP (type_t arg)\
	{\
	return vec_t (x OP arg, y OP arg, z OP arg);\
	}
#define DEFINE_VEC4_ARITHMOP2ASS_SCALAR(OP)\
	inline vec_t& operator OP (type_t arg)\
	{\
	x OP arg;\
	y OP arg;\
	z OP arg;\
	return *this;\
	}
#define DEFINE_VEC4_CONSTRUCTOR \
	vec_t ()\
	{\
	}\
	\
	vec_t (type_t arg)\
	{\
	x = arg; y = arg; z = arg; w = arg;\
	}\
	vec_t (type_t argX, type_t argY, type_t argZ, type_t argW)\
	{\
	x = argX;\
	y = argY;\
	z = argZ;\
	w = argW;\
	}\
	\
	vec_t (const type_t* arg)\
	{\
	x = arg[0];\
	y = arg[1];\
	z = arg[2];\
	w = arg[3];\
	}\
	\
	vec_t (const vec_t& arg)\
	{\
	x = arg.x;\
	y = arg.y;\
	z = arg.z;\
	w = arg.w;\
	}

template <typename T>
class Vector4
{
public:
	typedef T type_t;
	typedef Vector4<type_t> vec_t;

public:
	type_t x,y,z,w;

	inline vec_t& operator= (const vec_t& arg)
	{
		x = arg.x;
		y = arg.y;
		z = arg.z;
		w = arg.w;
		return *this;
	}

	inline vec_t operator- () const
	{
		return vec_t (-x, -y, -z, -w);
	}

	inline type_t getLengthSq () const
	{
		return x*x + y*y + z*z + w*w;
	}

	inline type_t getLength () const
	{
		return std::sqrt (getLengthSq ());
	}

	inline vec_t getAbs () const
	{
		return vec_t (abs(x), abs(y), abs(z), abs(w));
	}

	inline vec_t getNormalised () const
	{
		return (*this) / getLength ();
	}

	inline type_t getArea () const
	{
		return x*y*z*w;
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
		VEC_T v ((VEC_T::type_t) x, (VEC_T::type_t) y, (VEC_T::type_t) z);
		return v;
	}

	template <typename VEC_T>
	VEC_T get4 ()
	{
		VEC_T v ((VEC_T::type_t) x, (VEC_T::type_t) y, (VEC_T::type_t) z, (VEC_T::type_t) w);
		return v;
	}


	DEFINE_VEC4_CONSTRUCTOR

		DEFINE_VEC4_LOGICOP2(==)
		DEFINE_VEC4_LOGICOP2(!=)
		DEFINE_VEC4_LOGICOP2(<=)
		DEFINE_VEC4_LOGICOP2(>=)
		DEFINE_VEC4_LOGICOP2(<)
		DEFINE_VEC4_LOGICOP2(>)

		DEFINE_VEC4_ARITHMOP2(+)
		DEFINE_VEC4_ARITHMOP2(-)
		DEFINE_VEC4_ARITHMOP2(*)
		DEFINE_VEC4_ARITHMOP2(/)
		DEFINE_VEC4_ARITHMOP2(%)

		DEFINE_VEC4_ARITHMOP2ASS(+=)
		DEFINE_VEC4_ARITHMOP2ASS(-=)
		DEFINE_VEC4_ARITHMOP2ASS(*=)
		DEFINE_VEC4_ARITHMOP2ASS(/=)
		DEFINE_VEC4_ARITHMOP2ASS(%=)

		DEFINE_VEC4_ARITHMOP2_SCALAR(+)
		DEFINE_VEC4_ARITHMOP2_SCALAR(-)
		DEFINE_VEC4_ARITHMOP2_SCALAR(*)
		DEFINE_VEC4_ARITHMOP2_SCALAR(/)
		DEFINE_VEC4_ARITHMOP2_SCALAR(%)

		DEFINE_VEC4_ARITHMOP2ASS_SCALAR (+=)
		DEFINE_VEC4_ARITHMOP2ASS_SCALAR (-=)
		DEFINE_VEC4_ARITHMOP2ASS_SCALAR (*=)
		DEFINE_VEC4_ARITHMOP2ASS_SCALAR (/=)
		DEFINE_VEC4_ARITHMOP2ASS_SCALAR (%=)
};

class int4: public Vector4<int>
{
public:	
	typedef int type_t;
	typedef int4 vec_t;

	DEFINE_VEC4_CONSTRUCTOR
	int4 (const Vector4<int>& v4)
	{
		x = v4.x;
		y = v4.y;
		z = v4.z;
		w = v4.w;
	}
};

class uint4: public Vector4<uint>
{
public:	
	typedef uint type_t;
	typedef uint4 vec_t;

	DEFINE_VEC4_CONSTRUCTOR
	uint4 (const Vector4<uint>& v4)
	{
		x = v4.x;
		y = v4.y;
		z = v4.z;
		w = v4.w;
	}
};

class char4: public Vector4<char>
{
public:	
	typedef char type_t;
	typedef char4 vec_t;

	DEFINE_VEC4_CONSTRUCTOR
	char4 (const Vector4<char>& v4)
	{
		x = v4.x;
		y = v4.y;
		z = v4.z;
		w = v4.w;
	}
};

class uchar4: public Vector4<uchar>
{
public:	
	typedef uchar type_t;
	typedef uchar4 vec_t;

	DEFINE_VEC4_CONSTRUCTOR
	uchar4 (const Vector4<uchar>& v4)
	{
		x = v4.x;
		y = v4.y;
		z = v4.z;
		w = v4.w;
	}
};

class float4: public Vector4<float>
{
public:	
	typedef float type_t;
	typedef float4 vec_t;

	DEFINE_VEC4_CONSTRUCTOR
	float4 (const Vector4<float>& v4)
	{
		x = v4.x;
		y = v4.y;
		z = v4.z;
		w = v4.w;
	}
};