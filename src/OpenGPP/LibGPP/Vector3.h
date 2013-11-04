#pragma once

#include "Base.h"

#define DEFINE_VEC3_LOGICOP2(OP)\
	inline vec_t& operator OP (const vec_t& arg) \
	{\
	return ((x OP arg.x) || (y OP arg.y) || (z OP arg.z));\
	}
#define DEFINE_VEC3_ARITHMOP2(OP)\
	inline vec_t operator OP (const vec_t& arg) const\
	{\
	return vec_t (x OP arg.x, y OP arg.y, z OP arg.z);\
	}\
	inline vec_t operator OP (type_t val) const\
	{\
	return vec_t (x OP val, y OP val, z OP val);\
	}
#define DEFINE_VEC3_ARITHMOP2ASS(OP)\
	inline vec_t& operator OP (const vec_t& arg)\
	{\
	x OP arg.x;\
	y OP arg.y;\
	z OP arg.z;\
	return *this;\
	}
#define DEFINE_VEC3_ARITHMOP2_SCALAR(OP)\
	inline vec_t operator OP (type_t arg)\
	{\
	return vec_t (x OP arg, y OP arg, z OP arg);\
	}
#define DEFINE_VEC3_ARITHMOP2ASS_SCALAR(OP)\
	inline vec_t& operator OP (type_t arg)\
	{\
	x OP arg;\
	y OP arg;\
	z OP arg;\
	return *this;\
	}
#define DEFINE_VEC3_CONSTRUCTOR\
	vec_t ()\
	{\
	}\
	\
	vec_t (type_t arg)\
	{\
	x = arg; y = arg; z = arg;\
	}\
	vec_t (type_t argX, type_t argY, type_t argZ)\
	{\
	x = argX;\
	y = argY;\
	z = argZ;\
	}\
	\
	vec_t (const type_t* arg)\
	{\
	x = arg[0];\
	y = arg[1];\
	z = arg[2];\
	}\
	\
	vec_t (const vec_t& arg)\
	{\
	x = arg.x;\
	y = arg.y;\
	z = arg.z;\
	}

template <typename T>
class Vector3
{
private:
	typedef T type_t;
	typedef Vector3<type_t> vec_t;

public:
	type_t x,y,z;

	inline vec_t& operator= (const vec_t& arg)
	{
		x = arg.x;
		y = arg.y;
		z = arg.z;
		return *this;
	}

	inline vec_t operator- () const
	{
		return vec_t (-x, -y, -z);
	}

	inline type_t getLengthSq () const
	{
		return x*x + y*y + z*z;
	}

	inline type_t getLength () const
	{
		return std::sqrt (getLengthSq ());
	}

	inline vec_t getAbs () const
	{
		return vec_t (abs(x), abs(y), abs(z));
	}

	inline vec_t getNormalised () const
	{
		return (*this) / getLength ();
	}

	inline type_t getArea () const
	{
		return x*y*z;
	}

	inline static vec_t cross (vec_t a, vec_t b)
	{
		vec_t ret;
		ret.x = a.y * b.z - a.z * b.y;
		ret.y = a.z * b.x - a.x * b.z;
		ret.z = a.x * b.y - a.y * b.x;
		return ret;
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
		VEC_T v ((VEC_T::type_t) x, (VEC_T::type_t) y, (VEC_T::type_t) z, (VEC_T::type_t) 0);
		return v;
	}

	DEFINE_VEC3_CONSTRUCTOR

		DEFINE_VEC3_LOGICOP2(==)
		DEFINE_VEC3_LOGICOP2(!=)
		DEFINE_VEC3_LOGICOP2(<=)
		DEFINE_VEC3_LOGICOP2(>=)
		DEFINE_VEC3_LOGICOP2(<)
		DEFINE_VEC3_LOGICOP2(>)

		DEFINE_VEC3_ARITHMOP2(+)
		DEFINE_VEC3_ARITHMOP2(-)
		DEFINE_VEC3_ARITHMOP2(*)
		DEFINE_VEC3_ARITHMOP2(/)
		DEFINE_VEC3_ARITHMOP2(%)

		DEFINE_VEC3_ARITHMOP2ASS(+=)
		DEFINE_VEC3_ARITHMOP2ASS(-=)
		DEFINE_VEC3_ARITHMOP2ASS(*=)
		DEFINE_VEC3_ARITHMOP2ASS(/=)
		DEFINE_VEC3_ARITHMOP2ASS(%=)

		DEFINE_VEC3_ARITHMOP2_SCALAR(+)
		DEFINE_VEC3_ARITHMOP2_SCALAR(-)
		DEFINE_VEC3_ARITHMOP2_SCALAR(*)
		DEFINE_VEC3_ARITHMOP2_SCALAR(/)
		DEFINE_VEC3_ARITHMOP2_SCALAR(%)

		DEFINE_VEC3_ARITHMOP2ASS_SCALAR (+=)
		DEFINE_VEC3_ARITHMOP2ASS_SCALAR (-=)
		DEFINE_VEC3_ARITHMOP2ASS_SCALAR (*=)
		DEFINE_VEC3_ARITHMOP2ASS_SCALAR (/=)
		DEFINE_VEC3_ARITHMOP2ASS_SCALAR (%=)
};

class int3: public Vector3<int>
{
public:	
	typedef int type_t;
	typedef int3 vec_t;

	DEFINE_VEC3_CONSTRUCTOR
		int3 (const Vector3<int>& v3)
	{
		x = v3.x;
		y = v3.y;
		z = v3.z;
	}
};

class uint3: public Vector3<uint>
{
public:	
	typedef uint type_t;
	typedef uint3 vec_t;

	DEFINE_VEC3_CONSTRUCTOR
	uint3 (const Vector3<uint>& v3)
	{
		x = v3.x;
		y = v3.y;
		z = v3.z;
	}
};

class char3: public Vector3<char>
{
public:	
	typedef char type_t;
	typedef char3 vec_t;

	DEFINE_VEC3_CONSTRUCTOR
	char3 (const Vector3<char>& v3)
	{
		x = v3.x;
		y = v3.y;
		z = v3.z;
	}
};

class uchar3: public Vector3<uchar>
{
public:	
	typedef uchar type_t;
	typedef uchar3 vec_t;

	DEFINE_VEC3_CONSTRUCTOR
	uchar3 (const Vector3<uchar>& v3)
	{
		x = v3.x;
		y = v3.y;
		z = v3.z;
	}
};

class float3: public Vector3<float>
{
public:	
	typedef float type_t;
	typedef float3 vec_t;

	DEFINE_VEC3_CONSTRUCTOR
	
	float3 (const Vector3<float>& v3)
	{
		x = v3.x;
		y = v3.y;
		z = v3.z;
	}


	static float3 rotateAboutAxis (float3 vec, float3 axis, float angle)
	{
		//ref: http://inside.mines.edu/~gmurray/ArbitraryAxisRotation/
		float x(vec.x);
		float y(vec.y);
		float z(vec.z);

		float u(axis.x);
		float v(axis.y);
		float w(axis.z);

		float cosO(cosf(angle));
		float sinO(sinf(angle));

		float uxPvyPwz(x*u+v*y+w*z);
		float uuPvvPww(u*u+v*v+w*w);

		float outX=u*uxPvyPwz*(1-cosO)+uuPvvPww*x*cosO+sqrtf(uuPvvPww)*(-w*y+v*z)*sinO;
		float outY=v*uxPvyPwz*(1-cosO)+uuPvvPww*y*cosO+sqrtf(uuPvvPww)*(+w*x-u*z)*sinO;
		float outZ=w*uxPvyPwz*(1-cosO)+uuPvvPww*z*cosO+sqrtf(uuPvvPww)*(-v*x+u*y)*sinO;

		float coef =  1/uuPvvPww;

		float3 out = float3(outX,outY,outZ)*coef;
		return out;
	}
};