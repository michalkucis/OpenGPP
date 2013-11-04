#include "hdrimage.h"
#include "hdrloader.h"
#include "hdrimagefunctor.h"
#include <assert.h>
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>



uint HDRImageBase::getOffset (uint2 pos)
{
	assert (pos.x < m_availableSize.x);
	assert (pos.y < m_availableSize.y);
	return pos.x + pos.y*m_totalSize.x; 
}
uint HDRImageBase::getOffsetUseTotal (uint2 pos)
{
	assert (pos.x < m_totalSize.x);
	assert (pos.y < m_totalSize.y);
	return pos.x + pos.y*m_totalSize.x; 
}







boost::shared_ptr<HDRImage3f> HDRImage1f::getHDRImage3f ()
{
	boost::shared_ptr<HDRImage3f> hdri3f = boost::shared_ptr<HDRImage3f>(new HDRImage3f (getSize()));
	HDRIFunctor3fCopyFrom1f functor (this);
	hdri3f->applyFunctor (functor);
	return hdri3f;
}

uchar toUchar (float v1, float v2)
{
	float v = v1*v2;
	return (uchar) ((v<0 ? 0 : v) > 255 ? 255 : v);
}


int roundf (float x)
{
	return (int)std::floor (x + 0.5f);
}

float mysincf (float x)
{
	const float pi = 3.14159265f;

    if (x==0)
        return 1;
	return sinf (pi * x) / (pi * x);
}
template <typename int a>
float lanczos (float x)
{
	return mysincf (x / a) * mysincf (x);
	/*if (x > 0.5f || x < -0.5f)
		return 0;
	return 1;*/
}

void remapLanczosFilter (float3* in, uint sizeIn, float3* out, uint sizeOut, float2 offset)
{
	float off = offset.x * sizeIn;
	float size = sizeIn * (1.0f - offset.x - offset.y);
	float step = size / sizeOut;
	const int a = 3;

	for (uint i = 0; i < sizeOut; i++)
	{
		float centerX = off + step * (i+0.5f);
		float3 sum (0,0,0);
		for (int j = -a; j <= a; j++)
		{
			float x = centerX + j;
			int index = roundf(x);
			int clampindex = std::min<int> (std::max<int> (index,0), sizeIn-1); 
			sum += in[clampindex ] * lanczos<3>(centerX - index);

		}
		out[i] = sum;
	}
}

void remapAverage (float3* in, uint sizeIn, float3* out, uint sizeOut, float2 offset)
{
	float off = offset.x * sizeIn;
	float size = sizeIn * (1.0f - offset.x - offset.y);
	float step = size / sizeOut;
	assert (step >= 1.0f); // use remapLanczosFilter
	for (uint i = 0; i < sizeOut; i++)
	{
		float beginX = off + step * i;
		float endX = off + step * (i+1);
		float3 sum (0,0,0);
		float beginW = 1 - beginX + floor(beginX);
		float endW = endX - floor(endX);
		sum += in[(int)floor(beginX)] * beginW;
		sum += in[(int)floor(endX)] * endW;
		int nendX = (int) floor(endX);
		for (int x = (int) floor(beginX)+1; x < nendX; x++)
			sum += in[x];
		out[i] = sum / step;
	}
}

void HDRImage3c::rfftplanUpdate ()
{
	uint2 total = getTotalSize()-uint2(0,2);
	m_rfftplanR = fftwf_plan_dft_c2r_2d (total.y, total.x, 
			m_red, m_hdriRFFT->getRedBuffer(), FFTW_MEASURE);
	m_rfftplanG = fftwf_plan_dft_c2r_2d (total.y, total.x, 
			m_green, m_hdriRFFT->getGreenBuffer(), FFTW_MEASURE);
	m_rfftplanB = fftwf_plan_dft_c2r_2d (total.y, total.x, 
			m_blue, m_hdriRFFT->getBlueBuffer(), FFTW_MEASURE);
}

boost::shared_ptr<HDRImage3f> HDRImage3c::computeRFFT ()
{
	if (! m_hdriRFFT.get())
		m_hdriRFFT = boost::shared_ptr<HDRImage3f> (new HDRImage3f);
	uint2 size = (getSize()-uint2(0,1))*uint2(1,2);
	uint2 append = (getTotalSize()-uint2(0,1))*uint2(1,2)-size;
	if (size != m_hdriRFFT->getSize() || append != m_hdriRFFT->getAppendSize())
		m_hdriRFFT->setSize (size, append);
	if (m_rfftplanNeedUpdate)
	{
		rfftplanUpdate ();			
		m_rfftplanNeedUpdate = false;
	}

	m_hdriRFFT->setMult (getMult()/m_hdriRFFT->getTotalSize().getArea());

	fftwf_execute (m_rfftplanR);
	fftwf_execute (m_rfftplanG);
	fftwf_execute (m_rfftplanB);
	return m_hdriRFFT;
}

void HDRImage3f::getSubImage (PtrHDRImage3f& outimg, uint2 size, float2 reloffsetMin, float2 reloffsetMax)
{
	if (! outimg.get())
		outimg = PtrHDRImage3f (new HDRImage3f);
	outimg->setSize (size);
	outimg->setMult (getMult());
	float2 step = (getSize().get<float>() * (float2(1.0f,1.0f) - reloffsetMin- reloffsetMax)/ size.get<float>()) ;
	HDRImage3f midresult (uint2(outimg->getSize().x, getSize().y));
	{
		float3* in = new float3[getSize().x];
		float3* out = new float3[outimg->getSize().x];
		for (uint y = 0; y < getSize().y; y++)
		{
			for (uint x = 0; x < getSize().x; x++)
				in[x] = getPixel (uint2(x,y));
			
			if (step.x < 1.0f)
				remapLanczosFilter (in, getSize().x, out, size.x, float2(reloffsetMin.x, reloffsetMax.x));
			else
				remapAverage (in, getSize().x, out, size.x, float2(reloffsetMin.x, reloffsetMax.x));

			for (uint x = 0; x < size.x; x++)
				midresult.setPixel (uint2(x,y), out[x]);
		}
		delete [] (out);
		delete [] (in);
	}
	{
		float3* in = new float3[getSize().y];
		float3* out = new float3[outimg->getSize().y];
		for (uint x = 0; x < size.x; x++)
		{
			for (uint y = 0; y < getSize().y; y++)
				in[y] = midresult.getPixel (uint2(x,y));
			if (step.y < 1.0f)
				remapLanczosFilter (in, getSize().y, out, size.y, float2(reloffsetMin.y, reloffsetMax.y));
			else
				remapAverage (in, getSize().y, out, size.y, float2(reloffsetMin.y, reloffsetMax.y));

			for (uint y = 0; y < size.y; y++)
				outimg->setPixel (uint2(x,y), out[y]);
		}
		delete [] (out);
		delete [] (in);
	}
}

bool HDRImage3f::loadFromFile (cchar* pathname, uint2 appendMult, uint2 appendAdd)
{
	if ((strlen(pathname)>4) && !(strcmp (".hdr", pathname+strlen(pathname)-4)))
		return loadFromHDRFile (pathname, appendMult, appendAdd);
	
	SDL_Surface* surf = IMG_Load (pathname);
	if (surf == NULL)
		return false;

	assert (surf->format->BitsPerPixel == 24 || surf->format->BitsPerPixel == 32);
	uint2 available = uint2 (surf->w, surf->h);
	uint2 append = available - available*appendMult + appendAdd;
	setSize (available, append);
	uint countReals = m_totalSize.getArea();
	m_red = ::fftwf_alloc_real (countReals);
	m_green = ::fftwf_alloc_real (countReals);
	m_blue = ::fftwf_alloc_real (countReals);

	SDL_LockSurface (surf);
	uint width = surf->w;
	uint* outpix = (uint*) surf->pixels;
	uint2 size = getSize();
	float* r = m_red;
	float* g = m_green;
	float* b = m_blue;
	uchar* inpix = ((uchar*)surf->pixels);
	uint offX = surf->format->BitsPerPixel / 8;
	for (uint y = 0; y < size.y; y++)
	{
		for (uint x = 0; x < size.x; x++)
		{
			uint offsetSrc = offX*x + y*surf->pitch;
			*r = ((float) inpix[offsetSrc+0])/256;
			*g = ((float) inpix[offsetSrc+1])/256;
			*b = ((float) inpix[offsetSrc+2])/256;
			r++;
			g++;
			b++;
		}
		r += m_appendSize.x;
		g += m_appendSize.x;
		b += m_appendSize.x;
	}
	SDL_UnlockSurface (surf);
	SDL_FreeSurface (surf);

	return true;
}



bool HDRImage3f::loadFromHDRFile (cchar* pathname, uint2 appendMult, 
				uint2 appendAdd)
{
	HDRLoaderResult res;
	bool bres = HDRLoader::load (pathname, res);
	if (false == bres)
		return false;
	uint2 available = uint2 (res.width, res.height);
	uint2 append = available*appendMult + appendAdd;
	setSize (available, append);

	uint countReals = m_totalSize.getArea();
	m_red = ::fftwf_alloc_real (countReals);
	m_green = ::fftwf_alloc_real (countReals);
	m_blue = ::fftwf_alloc_real (countReals);
	for (uint y = 0; y < getSize().y; y++)
		for (uint x = 0; x < getSize().x; x++)
		{
			uint offsetSrc = x + y*res.width;
			uint offsetDst = getOffset (x, y);
			m_red[offsetDst] = res.cols [offsetSrc*3];
			m_green[offsetDst] = res.cols [offsetSrc*3+1];
			m_blue[offsetDst] = res.cols [offsetSrc*3+2];
		}
	clearAppendArea ();
	return true;
}

void HDRImage3f::getHDRImage1f (HDRImage1f* out, FuncF3toF* func, uint2 append)
{
	uint2 sizeAvailable = out->getSize ();
	uint2 sizeAppend = out->getAppendSize ();
	if (sizeAvailable != getSize() || sizeAppend != append)
		out->setSize (sizeAvailable, sizeAppend);

	uint2 sizeTotal = out->getTotalSize ();
		
	out->setMult (m_mult);
	for (uint y = 0; y < sizeAvailable.y; y++)
	{
		float* r = getRedBuffer (uint2(0,y));
		float* g = getGreenBuffer (uint2(0,y));
		float* b = getBlueBuffer (uint2(0,y));
		float* a = out->getBuffer (uint2(0,y));
		for (uint x = 0; x < sizeAvailable.x; x++)
		{
			*a = (*func) (float3(*r, *g, *b));
			a++;
			r++;
			g++;
			b++;
		}
	}
}

SDL_Surface* HDRImage3f::getSDLSurface (float multCoef, SDL_Surface* surface, uint2 surfaceSize)
{
	uint2 size = uint2::getMin (getSize(), surfaceSize);
	if (surface != NULL)
	{
		if ((surface->w != size.x) || (surface->h != size.y) || (surface->format->BitsPerPixel != 32))
		{	
			SDL_FreeSurface (surface);
			surface = NULL;
		}
	}
	if (! surface)
	{
		surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 
			size.x, size.y, 32,
			0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);
	}
	float* inr (m_red);
	float* ing (m_green);
	float* inb (m_blue);

	SDL_LockSurface (surface);
	uint* outpix = (uint*) surface->pixels;
	multCoef *= 256.0;
	//uint2 size = getSize();
	for (uint y = 0; y < size.y; y++)
	{
		inr = getRedBuffer (uint2(0, y));
		ing = getGreenBuffer (uint2(0, y));
		inb = getBlueBuffer (uint2(0, y));
		outpix = (uint*) ((uchar*) surface->pixels + surface->pitch*y);
		for (uint x = 0; x < size.x; x++)
		{
			uchar ucr = toUchar (*inr, multCoef);
			uchar ucg = toUchar (*ing, multCoef);
			uchar ucb = toUchar (*inb, multCoef);
			uchar uca = 0;
			*outpix = ucr << 0 | ucg <<  8 | ucb << 16 | uca << 24;
			inr++;
			ing++;
			inb++;
			outpix++;
		}
		//inr += m_appendSize.x;
		//ing += m_appendSize.x;
		//inb += m_appendSize.x;
	}
	SDL_UnlockSurface (surface);
	return surface;
}

SDL_Surface* HDRImage1f::getSDLSurface (float multCoef, SDL_Surface* surface)
{
	if (surface != NULL)
	{
		if ((surface->w != getSize().x) || (surface->h != getSize().y) || (surface->format->BitsPerPixel != 32))
		{	
			SDL_FreeSurface (surface);
			surface = NULL;
		}
	}
	if (! surface)
	{
		surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 
			getSize().x, getSize().y, 32,
			0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000);
	}
	float* in (m_alpha);

	SDL_LockSurface (surface);
	uint* outpix = (uint*) surface->pixels;
	multCoef *= 256.0;
	uint2 size = getSize();
	for (uint y = 0; y < size.y; y++)
	{
		for (uint x = 0; x < size.x; x++)
		{
			uchar uc = toUchar (*in, multCoef);
			uchar uca = 0;
			*outpix = uc << 0 | uc <<  8 | uc << 16 | uca << 24;
			in++;
			outpix++;
		}
		in += m_appendSize.x;
	}
	SDL_UnlockSurface (surface);
	return surface;
}

void copyHDRImage (HDRImage1f* hdriDst, HDRImage1f* hdriSrc,
		uint2 dstOrg, uint2 srcOrg, uint2 size)
{
	uint offminx = -std::min<int>(dstOrg.x, std::min<int>(srcOrg.x,0));
	uint offmaxx = size.x-std::min<uint>(hdriDst->getSize().x-dstOrg.x, std::min<uint> (hdriSrc->getSize().x-srcOrg.x, size.x));
	uint sizex = size.x - offminx - offmaxx;
	uint sx = srcOrg.x + offminx;
	uint dx = dstOrg.x + offminx;
	uint sizeofLine = sizex*sizeof(float);

	for (uint y = 0; y < size.y; y++)
	{
		uint dy = dstOrg.y + y;
		uint sy = srcOrg.y + y;
		if (dy < 0 || dy >= hdriDst->getSize().y)
			continue;
		if (sy < 0 || sy >= hdriSrc->getSize().y)
			continue;
		
		memcpy (hdriDst->getBuffer (uint2(dx, dy)),  hdriSrc->getBuffer (uint2 (sx, sy)),  sizeofLine);
	}
}


void copyHDRImage (HDRImage3f* hdriDst, HDRImage3f* hdriSrc,
		uint2 dstOrg, uint2 srcOrg, uint2 size)
{
	uint offminx = -std::min<int>(dstOrg.x, std::min<int>(srcOrg.x,0));
	uint offmaxx = size.x-std::min<uint>(hdriDst->getSize().x-dstOrg.x, std::min<uint> (hdriSrc->getSize().x-srcOrg.x, size.x));
	uint sizex = size.x - offminx - offmaxx;
	uint sx = srcOrg.x + offminx;
	uint dx = dstOrg.x + offminx;
	uint sizeofLine = sizex*sizeof(float);

	for (uint y = 0; y < size.y; y++)
	{
		uint dy = dstOrg.y + y;
		uint sy = srcOrg.y + y;
		if (dy < 0 || dy >= hdriDst->getSize().y)
			continue;
		if (sy < 0 || sy >= hdriSrc->getSize().y)
			continue;
		
		memcpy (hdriDst->getRedBuffer (uint2(dx, dy)),  hdriSrc->getRedBuffer (uint2 (sx, sy)),  sizeofLine);
		memcpy (hdriDst->getBlueBuffer (uint2(dx, dy)), hdriSrc->getBlueBuffer (uint2 (sx, sy)), sizeofLine);
		memcpy (hdriDst->getGreenBuffer (uint2(dx, dy)),hdriSrc->getGreenBuffer (uint2 (sx, sy)),sizeofLine);
	}
}

void copyHDRImage (HDRImage3c* hdriDst, HDRImage3c* hdriSrc)
{
	assert (hdriDst->getTotalSize () == hdriSrc->getTotalSize());

	uint2 size = hdriDst->getTotalSize ();
	{
		float* dst = (float*) hdriDst->getRedBuffer ();
		float* src = (float*) hdriSrc->getRedBuffer ();
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
				for (uint i = 0; i < 2; i++)
				{
					*dst = *src;
					dst++;
					src++;
				}
	}
	{
		float* dst = (float*) hdriDst->getGreenBuffer ();
		float* src = (float*) hdriSrc->getGreenBuffer ();
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
				for (uint i = 0; i < 2; i++)
				{
					*dst = *src;
					dst++;
					src++;
				}
	}
	{
		float* dst = (float*) hdriDst->getBlueBuffer ();
		float* src = (float*) hdriSrc->getBlueBuffer ();
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
				for (uint i = 0; i < 2; i++)
				{
					*dst = *src;
					dst++;
					src++;
				}
	}
}


void copyHDRImage (HDRImage3c* hdriDst, HDRImage1f* hdriSrc)
{
	assert (hdriDst->getTotalSize () == hdriSrc->getTotalSize()/uint2(1,2));

	uint2 size = hdriDst->getTotalSize ();
	{
		float* dst = (float*) hdriDst->getRedBuffer ();
		float* src = (float*) hdriSrc->getBuffer ();
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
				for (uint i = 0; i < 2; i++)
				{
					*dst = *src;
					dst++;
					src++;
				}
	}
	{
		float* dst = (float*) hdriDst->getGreenBuffer ();
		float* src = (float*) hdriSrc->getBuffer ();
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
				for (uint i = 0; i < 2; i++)
				{
					*dst = *src;
					dst++;
					src++;
				}
	}
	{
		float* dst = (float*) hdriDst->getBlueBuffer ();
		float* src = (float*) hdriSrc->getBuffer ();
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
				for (uint i = 0; i < 2; i++)
				{
					*dst = *src;
					dst++;
					src++;
				}
	}
}

void addHDRImage (HDRImage3f* hdriDst, HDRImage1f* hdriSrc)
{
	assert (hdriDst->getSize () == hdriSrc->getSize());
	uint2 appendDst = hdriDst->getAppendSize ();
	uint2 appendSrc = hdriSrc->getAppendSize ();
	uint2 size = hdriDst->getSize ();
	{
		float* dst = (float*) hdriDst->getRedBuffer ();
		float* src = (float*) hdriSrc->getBuffer ();
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				*dst += *src;
				dst++;
				src++;
			}
			dst += appendDst.x;
			src += appendSrc.x;
		}
	}
	{
		float* dst = (float*) hdriDst->getGreenBuffer ();
		float* src = (float*) hdriSrc->getBuffer ();
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				*dst += *src;
				dst++;
				src++;
			}
			dst += appendDst.x;
			src += appendSrc.x;
		}
	}
	{
		float* dst = (float*) hdriDst->getBlueBuffer ();
		float* src = (float*) hdriSrc->getBuffer ();
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				*dst += *src;
				dst++;
				src++;
			}
			dst += appendDst.x;
			src += appendSrc.x;
		}
}
}


void createSummedAreaTable (HDRImage3f* in, HDRImage3d* out)
{
	uint2 size = in->getSize();
	out->setSize (size);
	
	double3 first = in->getPixel(uint2(0,0)).get<double>();
	out->setPixel (uint2(0,0), first);
	double3 current = first;
	for (uint x = 1; x < size.x; x++)
	{
		current += in->getPixel (uint2(x,0)).get<double>();
		out->setPixel (uint2 (x,0), current);
	}
	for (uint y = 1; y < size.x; y++)
	{
		current += in->getPixel (uint2(0,y)).get<double>();
		out->setPixel (uint2 (0,y), current);
	}
	for (uint y = 1; y < size.y; y++)
		for (uint x = 1; x < size.x; x++)
		{
			float3 current = in->getPixel (uint2 (x,y));
			double3 up = out->getPixel (uint2 (x, y-1));
			double3 left = out->getPixel (uint2 (x-1, y));
			double3 upleft = out->getPixel (uint2 (x-1, y-1));
			double3 sum = current.get<double>() + up + left - upleft; 
			out->setPixel (uint2(x,y), sum);
		}
}


void createSummedAreaTable (HDRImage1f* in, HDRImage1d* out)
{
	uint2 size = in->getSize();
	out->setSize (size);
	
	double first = (double) in->getPixel(uint2(0,0));
	out->setPixel (uint2(0,0), first);
	double current = first;
	for (uint x = 1; x < size.x; x++)
	{
		current += (double) in->getPixel (uint2(x,0));
		out->setPixel (uint2 (x,0), current);
	}
	for (uint y = 1; y < size.x; y++)
	{
		current += (double) in->getPixel (uint2(0,y));
		out->setPixel (uint2 (0,y), current);
	}
	for (uint y = 1; y < size.y; y++)
		for (uint x = 1; x < size.x; x++)
		{
			float current = in->getPixel (uint2 (x,y));
			double up = out->getPixel (uint2 (x, y-1));
			double left = out->getPixel (uint2 (x-1, y));
			double upleft = out->getPixel (uint2 (x-1, y-1));
			double sum = ((double)current) + up + left - upleft; 
			out->setPixel (uint2(x,y),sum);
		}
}

#define OFFIN(X,Y) ((X)+(Y)*(size.x+appendIn.x))
#define OFFOUT(X,Y) ((X)+(Y)*(size.x+appendOut.x))
void createSummedAreaTable (float* in, double* out, uint2 size, uint2 appendIn, uint2 appendOut)
{
	double first = (double) in[OFFIN(0,0)];
	out[OFFOUT(0,0)] = first;
	double current = first;
	for (uint x = 1; x < size.x; x++)
	{
		current += (double) in[OFFIN(x,0)];
		out[OFFOUT(x,0)] = current;
	}
	for (uint y = 1; y < size.y; y++)
	{
		current += (double) in[OFFIN(0,y)];
		out[OFFOUT(0,y)] = current;
	}
	for (uint y = 1; y < size.y; y++)
		for (uint x = 1; x < size.x; x++)
		{
			float current = in[OFFIN(x,y)];
			double up = out[OFFOUT(x, y-1)];
			double left = out[OFFOUT(x-1, y)];
			double upleft = out[OFFOUT(x-1, y-1)];
			double sum = ((double)current) + up + left - upleft;
			out[OFFOUT(x,y)]=sum;
		}
}