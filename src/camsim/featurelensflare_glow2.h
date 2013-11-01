/////////////////////////////////////////////////////////////////////////////////
//
//  featureglow2: efekt, ktory pridava 'ziaru' okolo jasnych bodov v obraze
//    - efekt pridava bielu ziaru okolo pixelov 
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

class FeatureLensFlare_Glow2: public Feature
{
public:
	FeatureLensFlare_Glow2 ()
	{
	}
	type getType ()
	{
		return Feature::FEATURE_3C_TO_3F;
	}
	void featureCtoF (PtrHDRImage3c in, PtrHDRImage3f &out)
	{
		//hdriFreq:
		PtrHDRImage3c hdriFreq = PtrHDRImage3c (new HDRImage3c);
		{ 
			if (in->getSize() != hdriFreq->getSize() || in->getAppendSize() != hdriFreq->getAppendSize())
				hdriFreq->setSize (in->getSize(), in->getAppendSize());
			copyHDRImage (hdriFreq.get(), in.get());

			FeatureStaticFFT imperf (PtrFuncI2I2toF (new FuncGlow2));
			imperf.featureCtoC (hdriFreq, hdriFreq);
		}
		
		//out:
		// rfft musi byt na tomto mieste... preco? kto vie, suvisi to ale s fftw...
		out = in->computeRFFT ();
		
		// clear in:
		in.reset();

		// hdri:
		boost::shared_ptr<HDRImage3f> hdri = hdriFreq->computeRFFT ();
		
		// clear hdriFreq:
		hdriFreq.reset();

		// hdriGray:
		HDRImage1f hdriGray;
		{
			if (out->getSize() != hdriGray.getSize())
				hdriGray.setSize (out->getSize());
			FuncF3toFsumChannel funcSum;
			hdri->getHDRImage1f (&hdriGray, &funcSum);
		}

		// clear hdri:
		hdri.reset();

		// add out:
		addHDRImage (out.get(), &hdriGray);
	}

};