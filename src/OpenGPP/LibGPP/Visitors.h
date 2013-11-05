#pragma once

#include "Vector2.h"

template <typename T>
class Visitor1ReadOnly
{
public:
	typedef T key_t;
	virtual void visitRead (int n, const T& value) = 0;
};

template <typename T>
class Visitor1WriteOnly
{
public:
	typedef T key_t;
	virtual T visitWrite (int n) = 0;
};

template <typename T>
class Visitor1ReadWrite
{
public:
	typedef T key_t;
	virtual void visitReadWrite (int n, T& value) = 0;
};

template <typename T>
class Visitor2ReadOnly
{
public:
	typedef T key_t;
	virtual void visitRead (int2 n, const T& value) = 0;
};

template <typename T>
class Visitor2WriteOnly
{
public:
	typedef T key_t;
	virtual T visitWrite (int2 n) = 0;
};

template <typename T>
class Visitor2ReadWrite
{
public:
	typedef T key_t;
	virtual void visitReadWrite (int2 n, T& value) = 0;
};