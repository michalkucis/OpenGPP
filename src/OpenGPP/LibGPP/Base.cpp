#include "Base.h"

#pragma comment(lib, "cv210d.lib")
#pragma comment(lib, "cvaux210d.lib")
#pragma comment(lib, "cvhaartraining210d.lib")
#pragma comment(lib, "cxcore210d.lib")
#pragma comment(lib, "cxts210d.lib")
#pragma comment(lib, "highgui210d.lib")
#pragma comment(lib, "ml210d.lib") 
//#pragma comment(lib, "opencv_ffmpeg210d.lib")


#pragma warning(disable : 4996)

string int2str( int n )
{
	char buffer [1024];
	sprintf (buffer, "%d", n);
	string str = buffer;
	return buffer;
}

string float2str( float f )
{
	char buffer [1204];
	sprintf (buffer, "%f", f);
	string str = buffer;
	return str;
}

int round(float x)
{
	return (int)ceil(x + 0.5f);
}

float log2(float x)
{
	float exponent = logf(x) / logf(2);
	return exponent;
}

float clamp( float v, float minimum, float maximum )
{
	if (v < minimum)
		v = minimum;
	if (v > maximum)
		v = maximum;
	return v;
}
