/////////////////////////////////////////////////////////////////////////////////
//
//  HDRILoader: umoznuje nacitavat HDR obrazky zo suboru formatu HDR
//    tento subor je upravena verzia hdrloaderu od Igor-a Kravtchenka
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once


class HDRLoaderResult {
public:
	int width, height;
	// each pixel takes 3 float32, each component can be of any value...
	float *cols;
};

class HDRLoader {
public:
	static bool load(const char *fileName, HDRLoaderResult &res);
};

