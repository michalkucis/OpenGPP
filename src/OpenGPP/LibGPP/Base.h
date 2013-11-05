#pragma once

#include <string>
typedef std::string string;

typedef unsigned char uchar;
typedef unsigned int uint;

string int2str( int n );
string float2str( float f );
int round( float x );
float log2( float x );
float clamp (float v, float minimum, float maximum);