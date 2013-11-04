/////////////////////////////////////////////////////////////////////////////////
//
//  featuremotionblur: efekt pridava pohybove rozostrenie kamery
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "std.h"
#include "type.h"
#include "feature.h"
#include "hdrimage.h"

struct FeatureMotionBlurFullscreenParam
{
	float2 depthIncreases;
	float3 deltaCameraPos;
	float targetDist;
	float2 deltaTarget;
	float2 fovAngle;
};
typedef boost::shared_ptr<FeatureMotionBlurFullscreenParam> PtrFeatureMotionBlurFullscreenParam;


class FeatureMotionBlur: public Feature
{
	PtrFeatureMotionBlurFullscreenParam m_paramFullscreen;
	HDRImage3f m_motionVector;
	PtrHDRImage3f m_hdriOut;
	uint m_nSamples;
	PtrHDRImage1f m_depth;
public:
	// nSamples > 1  - const samples per output pixel
	// nSamples < -1 - nSamples are dynamicly computer per pixel. Max(computed nSamples) = -nSamples
	FeatureMotionBlur (uint nSamples, PtrFeatureMotionBlurFullscreenParam param)
	{
		assert (nSamples > 1 || nSamples < -1);
		m_nSamples = nSamples;
		m_hdriOut = PtrHDRImage3f (new HDRImage3f);
		m_paramFullscreen = param;
	}
	type getType ()
	{
		return FEATURE_3F_TO_3F;
	}
private:
	void computeMotionVectorMap ()
	{
		uint2 size = m_depth->getSize();
		float3 deltaCameraPos = m_paramFullscreen->deltaCameraPos;
		float targetDist = m_paramFullscreen->targetDist;
		float2 deltaTarget = m_paramFullscreen->deltaTarget;
		float2 fovangle = m_paramFullscreen->fovAngle;

		//deltaTarget *= float2 ((float) size.x, (float) size.y);

		m_motionVector.setSize (size);
		float* depth = m_depth->getBuffer ();
		float* motionX = m_motionVector.getRedBuffer ();
		float* motionY = m_motionVector.getGreenBuffer ();
		float* motionLen = m_motionVector.getBlueBuffer ();
		
		float2 fsize = size.get<float>();
		float2 invsize = float2(1.0f,1.0f)/(size.get<float>());
		float2 halfstep = invsize/2;
		float2 depthIncreases = float2 (sinf (fovangle.x), sinf (fovangle.y));
		for (uint y = 0; y < size.y; y++)
		{
			for (uint x = 0; x < size.x; x++)
			{
				float2 delta (deltaTarget);
				float2 org = (float2((float)x,(float)y)*invsize + halfstep)*2-1;
				float z = *depth;

				float2 world =  org * (depthIncreases*z);
				float2 newworld = world + float2(deltaCameraPos.x,deltaCameraPos.y)
					- org * depthIncreases * deltaCameraPos.z;

				float2 dst = newworld / (depthIncreases*z);
				
				delta += dst - org;
				delta *= fsize;

				depth++;
				(*motionX++) = delta.x;
				(*motionY++) = delta.y;
				(*motionLen++) = sqrtf(delta.x*delta.x + delta.y*delta.y);
			}
			depth += m_depth->getAppendSize().x;
		}
	}
	class HDRImageFunctor3fMotion: public HDRImageFunctor
	{
		bool m_bConstSamples;
		uint m_nSamples;
		HDRImage3f* m_motion;
		HDRImage3f* m_in;

	public:
		HDRImageFunctor3fMotion (int nSamples, HDRImage3f* in, HDRImage3f* motion)
		{
			m_bConstSamples = nSamples > 1;
			m_nSamples = (nSamples > 0) ? nSamples : -nSamples;
			m_motion = motion;
			m_in = in;
		}

	private:
		int round (float in)
		{
			return (int) std::floor (in+0.5f);
		}
		float sample (float x, float y, float* in)
		{
			int nx = round(x);
			int ny = round(y);
			uint2 size = m_motion->getSize();
			nx = std::max<int> (0, std::min<int> (size.x-1, nx));
			ny = std::max<int> (0, std::min<int> (size.y-1, ny));

			uint2 total = m_in->getTotalSize();
			return in[nx+total.x*ny];
		}
		float comp (uint x, uint y, float* in)
		{
			float3 motion3f = m_motion->getPixel (uint2(x,y));
			float2 motion (motion3f.x, motion3f.y);
			float motionLen (motion3f.z);

			float2 src ((float) x, (float) y);

			uint nSamples = m_nSamples;
			if (! m_bConstSamples)
				nSamples = std::min (nSamples, (uint) ceil (motionLen));

			float step = 1.0f/(nSamples-1);
			float inter = 0.0f;
			float sum = 0.0f;
			for (uint i = 0; i < nSamples; i++, inter += step)
			{
				float2 coord (src + motion*inter);
				sum += sample (coord.x, coord.y, in);
			}
			
			return sum / nSamples;
		}
	public:
		float getRed (uint x, uint y, float value)
		{
			return comp (x, y, m_in->getRedBuffer());
		}
		float getBlue (uint x, uint y, float value)
		{
			return comp (x, y, m_in->getBlueBuffer());
		}
		float getGreen (uint x, uint y, float value)
		{
			return comp (x, y, m_in->getGreenBuffer());
		}
	};
	void applyMotion (HDRImage3f* in, HDRImage3f* out, HDRImage3f* motion)
	{
		out->setSize (in->getSize(), in->getAppendSize());
		assert (motion->getSize() == in->getSize());

		HDRImageFunctor3fMotion functor (m_nSamples, in, motion);
		out->applyFunctor (functor);
		
	}
	void setDepthMap (PtrHDRImage1f depth)
	{
		m_depth = depth;
	}
public:
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f &out)
	{
		out = m_hdriOut;
		computeMotionVectorMap ();
		m_hdriOut->setSize (in->getSize());
		assert (m_hdriOut->getSize() == m_motionVector.getSize());
		applyMotion (in.get(), out.get(), &m_motionVector);
	}
};