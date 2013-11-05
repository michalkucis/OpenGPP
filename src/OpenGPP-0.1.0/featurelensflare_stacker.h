/////////////////////////////////////////////////////////////////////////////////
//
//  FeatureStacker: trieda umoznuje vkladanie efektov do fronty
//    metoda 'show' bude transformovat vstupnu mapu pomocou efektov vlozenych do fronty
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <list>
#include "feature.h"


class FeatureLensFlare_Stacker
{
	typedef std::list<PtrFeature> list_t;
	typedef list_t::iterator it_t;
	list_t m_list;
	
	enum state_t {STATE_HDRI3F, STATE_HDRI3C};
public:
	void add (PtrFeature ptr)
	{
		m_list.push_back (ptr);

	}
	// out_hdri = (in_hdri->computeFFT())->computeRFFT ();
	void show (PtrHDRImage3f in_hdri, PtrHDRImage3f &out_hdri)
	{
		PtrHDRImage3f hdri3f = in_hdri;
		PtrHDRImage3c hdri3c;
		state_t state = STATE_HDRI3F;
		it_t it = m_list.begin ();
		it_t end = m_list.end ();
		for (; it != end; it++)
		{
			Feature* feature = (*it).get();
			if (state == STATE_HDRI3F)
			{
				switch (feature->getType())
				{
				case Feature::FEATURE_3F_TO_3F:
					{
					PtrHDRImage3f out;
					feature->featureFtoF (hdri3f, out);
					hdri3f = out;
					break;
					}					
				case Feature::FEATURE_3F_TO_3C:
					feature->featureFtoC (hdri3f, hdri3c);
					state = STATE_HDRI3C;
					break;
				case Feature::FEATURE_3C_TO_3F:
					hdri3f->clearAppendArea();					
					hdri3c = hdri3f->computeFFT();
					feature->featureCtoF (hdri3c, hdri3f);
					break;
				case Feature::FEATURE_3C_TO_3C:
					{
					hdri3f->clearAppendArea();
					hdri3c = hdri3f->computeFFT();
					PtrHDRImage3c hdriOut;
					feature->featureCtoC (hdri3c, hdriOut);
					hdri3c = hdriOut;
					state = STATE_HDRI3C;
					break;
					}
				}
			}
			else if (state == STATE_HDRI3C)
			{
				switch (feature->getType())
				{
				case Feature::FEATURE_3F_TO_3F:
					{
					PtrHDRImage3f in = hdri3c->computeRFFT ();
					in->clearAppendArea ();
					PtrHDRImage3f out;
					feature->featureFtoF (in, out);
					hdri3f = out;
					state = STATE_HDRI3F;
					break;
					}					
				case Feature::FEATURE_3F_TO_3C:
					{
					PtrHDRImage3f in = hdri3c->computeRFFT ();
					in->clearAppendArea ();
					PtrHDRImage3c out;
					feature->featureFtoC (in, out);
					hdri3c = out;
					state = STATE_HDRI3C;
					break;
					}
				case Feature::FEATURE_3C_TO_3F:
					{
					PtrHDRImage3c in = hdri3c;
					PtrHDRImage3f out;
					feature->featureCtoF (in, out);
					hdri3f = out;
					state = STATE_HDRI3F;
					break;
					}
				case Feature::FEATURE_3C_TO_3C:
					{
					PtrHDRImage3c in = hdri3c;
					PtrHDRImage3c out;
					feature->featureCtoC (in, out);
					hdri3c = out;
					state = STATE_HDRI3C;
					break;
					}
				}
			}
		}
		if (state == STATE_HDRI3F)
			out_hdri = hdri3f;
		else if (state == STATE_HDRI3C)
			out_hdri = hdri3c->computeRFFT ();
		out_hdri->clearAppendArea ();
	}

	void showFtoC (PtrHDRImage3f in_hdri, PtrHDRImage3c &out_hdri)
	{
		PtrHDRImage3f hdri3f = in_hdri;
		PtrHDRImage3c hdri3c;
		state_t state = STATE_HDRI3F;
		it_t it = m_list.begin ();
		it_t end = m_list.end ();
		for (; it != end; it++)
		{
			Feature* feature = (*it).get();
			if (state == STATE_HDRI3F)
			{
				switch (feature->getType())
				{
				case Feature::FEATURE_3F_TO_3F:
					{
					PtrHDRImage3f out;
					feature->featureFtoF (hdri3f, out);
					hdri3f = out;
					break;
					}					
				case Feature::FEATURE_3F_TO_3C:
					feature->featureFtoC (hdri3f, hdri3c);
					state = STATE_HDRI3C;
					break;
				case Feature::FEATURE_3C_TO_3F:
					hdri3f->clearAppendArea();					
					hdri3c = hdri3f->computeFFT();
					feature->featureCtoF (hdri3c, hdri3f);
					break;
				case Feature::FEATURE_3C_TO_3C:
					{
					hdri3f->clearAppendArea();
					hdri3c = hdri3f->computeFFT();
					PtrHDRImage3c hdriOut;
					feature->featureCtoC (hdri3c, hdriOut);
					hdri3c = hdriOut;
					state = STATE_HDRI3C;
					break;
					}
				}
			}
			else if (state == STATE_HDRI3C)
			{
				switch (feature->getType())
				{
				case Feature::FEATURE_3F_TO_3F:
					{
					PtrHDRImage3f in = hdri3c->computeRFFT ();
					in->clearAppendArea ();
					PtrHDRImage3f out;
					feature->featureFtoF (in, out);
					hdri3f = out;
					state = STATE_HDRI3F;
					break;
					}					
				case Feature::FEATURE_3F_TO_3C:
					{
					PtrHDRImage3f in = hdri3c->computeRFFT ();
					in->clearAppendArea ();
					PtrHDRImage3c out;
					feature->featureFtoC (in, out);
					hdri3c = out;
					state = STATE_HDRI3C;
					break;
					}
				case Feature::FEATURE_3C_TO_3F:
					{
					PtrHDRImage3c in = hdri3c;
					PtrHDRImage3f out;
					feature->featureCtoF (in, out);
					hdri3f = out;
					state = STATE_HDRI3F;
					break;
					}
				case Feature::FEATURE_3C_TO_3C:
					{
					PtrHDRImage3c in = hdri3c;
					PtrHDRImage3c out;
					feature->featureCtoC (in, out);
					hdri3c = out;
					state = STATE_HDRI3C;
					break;
					}
				}
			}
		}
		if (state == STATE_HDRI3F)
			out_hdri = hdri3f->computeFFT ();
		else if (state == STATE_HDRI3C)
			out_hdri = hdri3c;
		//out_hdri->clearAppendArea ();
	}
};

