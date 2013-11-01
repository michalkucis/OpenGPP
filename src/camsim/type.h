/////////////////////////////////////////////////////////////////////////////////
//
//  Vector2: sablona dvojprvkoveho vektoru
//  Vector3: sablona trojprvkoveho vektoru
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifdef USE_OPENCV
#include <opencv/cv.h>
#endif

#include <algorithm>
#include <fftw3.h>

template <class Type>
struct Vector2
{
	typedef Type value_t;
	typedef Vector2<value_t> vec_t;

	value_t x,y;

	vec_t ()
	{
	}

	vec_t (value_t argX, value_t argY)
	{
		x = argX;
		y = argY;
	}

	vec_t (const value_t* arg)
	{
		x = arg[0];
		y = arg[1];
	}

	vec_t (const vec_t& arg)
	{
		x = arg.x;
		y = arg.y;
	}

	inline vec_t& operator= (const vec_t& arg)
	{
		x = arg.x;
		y = arg.y;
		return *this;
	}

	inline bool operator!= (const vec_t& arg) const
	{
		return ((x != arg.x) || (y != arg.y));
	}
	inline bool operator== (const vec_t& arg) const
	{
		return ! (*this != arg);
	}
	inline bool operator<= (const vec_t& arg) const
	{
		return ((x <= arg.x) || (y <= arg.y));
	}
	inline bool operator>= (const vec_t& arg) const
	{
		return ((x >= arg.x) || (y >= arg.y));
	}
	inline bool operator> (const vec_t& arg) const
	{
		return ((x > arg.x) || (y > arg.y));
	}
	inline bool operator< (const vec_t& arg) const
	{
		return ((x < arg.x) || (y < arg.y));
	}


	inline vec_t operator- () const
	{
		return vec_t (-x, -y);
	}

	inline vec_t operator+ (const vec_t& arg) const
	{
		return vec_t (x + arg.x, y + arg.y);
	}
	inline vec_t& operator+= (const vec_t& arg)
	{
		x += arg.x;
		y += arg.y;
		return *this;
	}

	inline vec_t operator+ (value_t val) const
	{
		return vec_t (x + val, y + val);
	}
	inline vec_t& operator+= (value_t val)
	{
		x += val;
		y += val;
		return *this;
	}

	inline vec_t operator- (const vec_t& arg) const
	{
		return vec_t (x - arg.x, y - arg.y);
	}
	inline vec_t& operator-= (const vec_t& arg)
	{
		x -= arg.x;
		y -= arg.y;
		return *this;
	}
	inline vec_t operator- (value_t val) const
	{
		return vec_t (x - val, y - val);
	}
	inline vec_t& operator-= (value_t val)
	{
		x -= val;
		y -= val;
		return *this;
	}

	inline vec_t operator* (const vec_t& arg) const
	{
		return vec_t (x * arg.x, y * arg.y);
	}
	inline vec_t& operator*= (const vec_t& arg)
	{
		x *= arg.x;
		y *= arg.y;
		return *this;
	}

	inline vec_t operator* (value_t val) const
	{
		return vec_t (x * val, y * val);
	}
	inline vec_t& operator*= (value_t val)
	{
		x *= val;
		y *= val;
		return *this;
	}

	inline vec_t operator/ (value_t arg) const
	{
		return vec_t (x/arg, y/arg);
	}
	inline vec_t& operator/= (value_t arg)
	{
		x /= arg;
		y /= arg;
		return *this;
	}

	inline vec_t operator/ (vec_t& arg) const
	{
		return vec_t (x/arg.x, y/arg.y);
	}
	inline vec_t& operator/= (vec_t& arg)
	{
		x /= arg.x;
		y /= arg.y;
		return *this;
	}

	inline vec_t operator% (value_t arg) const
	{
		return vec_t (x%arg, y%arg);
	}
	inline vec_t& operator%= (value_t arg)
	{
		x %= arg;
		y %= arg;
		return *this;
	}

	inline vec_t operator% (vec_t& arg) const
	{
		return vec_t (x%arg.x, y%arg.y);
	}
	inline vec_t& operator%= (vec_t& arg)
	{
		x %= arg.x;
		y %= arg.y;
		return *this;
	}

	inline value_t getLengthSq () const
	{
		return x*x + y*y;
	}

	inline value_t getLength () const
	{
		return std::sqrt (getLengthSq ());
	}

	inline vec_t getAbs () const
	{
		return vec_t (abs(x), abs(y));
	}

	inline vec_t getNormalized () const
	{
		return (*this) / Length ();
	}

	inline vec_t getSqrt () const
	{
		return vec_t (sqrt(x), sqrt(y));
	}
	
	inline value_t getArea () const
	{
		return x*y;
	}

#ifdef USE_OPENCV
	vec_t (cv::Point arg): 
		x(arg.x), 
		y(arg.y)
	{
	}

	vec_t (cv::Size arg): 
		x(arg.width), 
		y(arg.height)
	{
	}


	cv::Size getCvSize ()
	{
		cv::Size size;
		size.width = (uint) x;
		size.height = (uint) y;
		return size;
	}

	cv::Point getCvPoint ()
	{
		cv::Point point;
		point.x = (int) x;
		point.y = (int) y;
		return point;		
	}
#endif//USE_OPENCV

	template <typename type>
	Vector2<type> get ()
	{
		Vector2<type> res;
		res.x = (type) x;
		res.y = (type) y;
		return res;
	}

	static const vec_t ZERO;

	static vec_t getMin (vec_t v1, vec_t v2)
	{
		vec_t v = vec_t (std::min(v1.x, v2.x), std::min(v1.y, v2.y));
		return v;
	}

	static vec_t getMax (vec_t v1, vec_t v2)
	{
		vec_t v = vec_t (std::max(v1.x, v2.x), std::max(v1.y, v2.y));
		return v;
	}
};



//==============================================================


template <class Type>
struct Vector3
{
	typedef Type value_t;
	typedef Vector3<value_t> vec_t;

	value_t x,y,z;

	vec_t ()
	{
	}

	vec_t (value_t arg)
	{
		x = arg;
		y = arg;
		z = arg;
	}

	vec_t (value_t argX, value_t argY, value_t argZ)
	{
		x = argX;
		y = argY;
		z = argZ;
	}

	vec_t (const value_t* arg)
	{
		x = arg[0];
		y = arg[1];
		z = arg[2];
	}

	vec_t (const vec_t& arg)
	{
		x = arg.x;
		y = arg.y;
		z = arg.z;
	}

	vec_t (const Vector2<value_t>& arg)
	{
		x = arg.x;
		y = arg.y;
		z = 0;
	}

	inline vec_t& operator= (const vec_t& arg)
	{
		x = arg.x;
		y = arg.y;
		z = arg.z;
		return *this;
	}

	inline bool operator!= (const vec_t& arg) const
	{
		return ((x != arg.x) || (y != arg.y) || (z != arg.z));
	}
	inline bool operator== (const vec_t& arg) const
	{
		return ! (*this != arg);
	}

	inline vec_t operator- () const
	{
		return vec_t (-x, -y, -z);
	}

	inline vec_t operator+ (const vec_t& arg) const
	{
		return vec_t (x + arg.x, y + arg.y, z + arg.z);
	}
	inline vec_t& operator+= (const vec_t& arg)
	{
		x += arg.x;
		y += arg.y;
		z += arg.z;
		return *this;
	}

	inline vec_t operator- (const vec_t& arg) const
	{
		return vec_t (x - arg.x, y - arg.y, z - arg.z);
	}
	inline vec_t& operator-= (const vec_t& arg)
	{
		x -= arg.x;
		y -= arg.y;
		z -= arg.z;
		return *this;
	}

	inline vec_t operator* (const vec_t& arg) const
	{
		return vec_t (x * arg.x, y * arg.y, z * arg.z);
	}
	inline vec_t& operator*= (const vec_t& arg)
	{
		x *= arg.x;
		y *= arg.y;
		z *= arg.z;
		return *this;
	}

	inline vec_t operator* (value_t arg) const
	{
		return vec_t (x * arg, y * arg, z * arg);
	}
	inline vec_t& operator*= (value_t arg)
	{
		x *= arg;
		y *= arg;
		z *= arg;
		return *this;
	}

	inline vec_t operator/ (value_t arg) const
	{
		return vec_t (x/arg, y/arg, z/arg);
	}
	inline vec_t& operator/= (value_t arg)
	{
		x /= arg;
		y /= arg;
		z /= arg;
		return *this;
	}


	inline value_t getLengthSq () const
	{
		return x*x + y*y + z*z;
	}

	inline value_t getLength () const
	{
		return sqrt (GetLengthSq ());
	}

	inline vec_t getAbs () const
	{
		return vec_t (abs(x), abs(y), abs(z));
	}

	inline vec_t getNormalized () const
	{
		return (*this) / GetLength ();
	}

	inline vec_t getSqrt () const
	{
		return vec_t (sqrt(x), sqrt(y), sqrt(z));
	}

	inline value_t getArea () const
	{
		return x*y;
	}

	static const vec_t ZERO;

	static vec_t getMin (vec_t v1, vec_t v2)
	{
		vec_t v = vec_t (std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z));
		return v;
	}

	static vec_t getMax (vec_t v1, vec_t v2)
	{
		vec_t v = vec_t (std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z));
		return v;
	}

	template <typename type>
	Vector3<type> get ()
	{
		Vector3<type> res;
		res.x = (type) x;
		res.y = (type) y;
		res.z = (type) z;
		return res;
	}
};

template <class Type>
Vector3<Type> vec3Cross (Vector3<Type> &v1, Vector3<Type> &v2)
{
	typedef Vector3<Type> vec_t;
	vec_t res = vec_t (v1.y*v2.z-v2.y*v1.z, v1.z*v2.x-v2.z*v1.x, v1.x*v2.y-v1.y*v2.x);
	return res;
}



//==============================================================

typedef const char cchar;
typedef unsigned int uint;
typedef unsigned char uchar;

typedef Vector2<char> char2;
typedef Vector2<uchar> uchar2;
typedef Vector2<uint> uint2;
typedef Vector2<int> int2;
typedef Vector2<float> float2;
typedef Vector2<double> double2;


typedef Vector3<char> char3;
typedef Vector3<uchar> uchar3;
typedef Vector3<int> int3;
typedef Vector3<uint> uint3;
typedef Vector3<float> float3;
typedef Vector3<double> double3;


inline void mulAndStore (fftwf_complex& c1, const fftwf_complex& c2)
{
	float v1 = c1[0]*c2[0] - c1[1]*c2[1];
	float v2 = c1[0]*c2[1] + c1[1]*c2[0];
	c1[0] = v1;
	c1[1] = v2;
}





