/////////////////////////////////////////////////////////////////////////////////
//
//  featurecentralsymmetry: efekt pridava centralne symetricky obraz
//
/////////////////////////////////////////////////////////////////////////////////
#pragma once


#define INIT_PPIX(HDRI,X,Y,V)\
	float* V##1 = (HDRI)->getRedBuffer (uint2(X,Y));\
	float* V##2 = (HDRI)->getGreenBuffer (uint2(X,Y));\
	float* V##3 = (HDRI)->getBlueBuffer (uint2(X,Y));
#define INC_PPIX(V)\
{\
	(V##1)++;\
	(V##2)++;\
	(V##3)++;\
}
#define DEC_PPIX(V)\
{\
	(V##1)--;\
	(V##2)--;\
	(V##3)--;\
}
#define GET_PPIX(V,V3)\
	float3 V3;\
	V3.x = *(V##1);\
	V3.y = *(V##2);\
	V3.z = *(V##3);

#define SET_PPIX(V,V3)\
{\
	*(V##1) = V3.x;\
	*(V##2) = V3.y;\
	*(V##3) = V3.z;\
}






class FeatureCentralSymmetry: public Feature
{
protected:
	HDRImage3f m_hdriMult;
public:
	PtrFuncFtoF3 m_func;
	FeatureCentralSymmetry (PtrFuncFtoF3 func)
	{
		m_func = func;
	}
	type getType ()
	{
		return Feature::FEATURE_3F_TO_3F;
	}
protected:
	void initHDRIMult (uint2 hdriSize)
	{
		uint2 size = hdriSize/2+hdriSize%2;
		if (m_hdriMult.getSize() == size)
			return;
		float2 fhdriSize = hdriSize.get<float>();
		float2 center = hdriSize.get<float>()/2;
		m_hdriMult.setSize (size);
		float multDist = 1 / sqrtf(center.x*center.x+center.y*center.y);
		for (uint y = 0; y < size.y; y++)
			for (uint x = 0; x < size.x; x++)
			{
				float2 pos = center-float2((float)x,(float)y)-float2(0.5f,0.5f);
				float dist = sqrtf(pos.x*pos.x + pos.y*pos.y) * multDist;
				float3 out = (*m_func.get()) (dist); 
				m_hdriMult.setPixel (uint2 (x,y), out);
			}
	}
public:
	void featureFtoF (PtrHDRImage3f in, PtrHDRImage3f &out)
	{
		out = in;
		HDRImage3f* inout_hdri = in.get();

		initHDRIMult (in->getSize());

		uint2 size = inout_hdri->getSize ();
		float2 center = size.get<float>()/2;
		uint countY = m_hdriMult.getSize().y;
		uint countX = m_hdriMult.getSize().x;
		for (uint i = 0, y1 = 0, y2 = size.y-1; i < countY; i++, y1++, y2--)
		{
			INIT_PPIX (inout_hdri, 0, y1, io1);
			INIT_PPIX (inout_hdri, size.x-1, y2, io2);
			INIT_PPIX ((&m_hdriMult), 0, y1, pcoef);
			for (uint i = 0; i < countX; i++)
			{
			    GET_PPIX (io1, i1);
				GET_PPIX (io2, i2);
				GET_PPIX (pcoef, coef);

				float3 o1, o2;

				o1 = i1 + i2 * coef;
				o2 = i2 + i1 * coef;

				SET_PPIX (io1, o1);
				SET_PPIX (io2, o2);

				INC_PPIX (pcoef);
				INC_PPIX (io1);
				DEC_PPIX (io2);
			}
			{
				INIT_PPIX ((&m_hdriMult), countX-1, y1, pcoef);
				for (uint i = countX; i < size.x; i++)
				{
					GET_PPIX (io1, i1);
					GET_PPIX (io2, i2);
					GET_PPIX (pcoef, coef);

					float3 o1, o2;

					o1 = i1 + i2 * coef;
					o2 = i2 + i1 * coef;

					SET_PPIX (io1, o1);
					SET_PPIX (io2, o2);

					DEC_PPIX (pcoef);
					INC_PPIX (io1);
					DEC_PPIX (io2);
				}
			}
		}
	}
};


