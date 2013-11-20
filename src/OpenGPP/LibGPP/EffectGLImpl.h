#pragma once

#include "EffectGL.h"
#include "Matrix.h"
#include "Functions.h"
#include "PIDcontroller.h"
#include "SharedObjectsFactory.h"

class EffectDistortionFunc
{
protected:
	void getGridPre (Matrix<float2>& out, uint2 sizeGrid);
	void getGridFromFunc (Matrix<float2>& out, uint2 sizeGrid, Ptr<FuncFtoF> func);
	float2 getPos (Matrix<float2>& mat, int2 pos);
	void drawVertex(int2 index, Matrix<float2>& gridPre, Matrix<float2>& gridPost);
	void drawGridElement (int2 index, Matrix<float2>& gridPre, Matrix<float2>& gridPost);
	void drawGridInTheMiddle (Matrix<float2>& gridPre, Matrix<float2>& gridPost);
	void drawGridInTheBorders (Matrix<float2>& gridPre, Matrix<float2>& gridPost);
};

class EffectCopyColorAndDepthMaps: public EffectGL
{
public:
	Ptr<GLTexture2D> process(Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envmap);
	Ptr<GLTexture2D> processDepth (Ptr<GLTexture2D> depth);
};

class EffectDistortionGrid: public EffectGL, public EffectDistortionFunc
{
	Matrix<float2> m_gridPre;
	Matrix<float2> m_gridPost;
public:
	EffectDistortionGrid (Ptr<SharedObjectsFactory> sof, Matrix<float2>& grid);
	EffectDistortionGrid (Ptr<SharedObjectsFactory> sof, uint2 gridResolution, Ptr<FuncFtoF> func);
	EffectDistortionGrid (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed);
	EffectDistortionGrid (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Matrix<float2>& grid);
	EffectDistortionGrid (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, uint2 gridResolution, Ptr<FuncFtoF> func);
	void setGrid (Matrix<float2>& grid);
	void setGrid (uint2 gridResolution, Ptr<FuncFtoF> func);
	Ptr<GLTexture2D> process (Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
	Ptr<GLTexture2D> processDepth (Ptr<GLTexture2D> depth);
};

class EffectChromaticAberration: public EffectGL, public EffectDistortionFunc
{
	Matrix<float2> m_gridPre[3];
	Matrix<float2> m_gridPost[3];
	GLProgram m_programRed, m_programGreen, m_programBlue;
public:
	EffectChromaticAberration (Ptr<SharedObjectsFactory> sof);
	EffectChromaticAberration (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed);
	void setGrid (uint numChannel, Matrix<float2>& grid);
	void setGrid (uint numChannel, uint2 gridResolution, Ptr<FuncFtoF> func);
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
	Ptr<GLTexture2D> processDepth( Ptr<GLTexture2D> depth );
};

class EffectVignettingImageFile: public EffectGL
{
	string m_filename;
	bool m_dynamicLoaded;
	Ptr<GLTexture2D> m_texLoaded;
	GLProgram m_program;
public:
	EffectVignettingImageFile (Ptr<SharedObjectsFactory> sof, string filename, bool dynamicLoaded);
	EffectVignettingImageFile (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, string filename, bool dynamicLoaded);
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
};

class EffectVignettingImageFunc: public EffectGL
{
	bool m_dynamicLoaded;
	Ptr<GLTexture2D> m_texLoaded;
	GLProgram m_program;
	uint2 m_resolution;
	Ptr<FuncFtoF> m_func;
protected:
	Ptr<GLTexture2D> createTexture (uint2 resolution, Ptr<FuncFtoF> func);
public:
	EffectVignettingImageFunc(Ptr<SharedObjectsFactory> sof, uint2 resolution, Ptr<FuncFtoF> func, bool dynamicLoaded );
	EffectVignettingImageFunc(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, uint2 resolution, Ptr<FuncFtoF> func, bool dynamicLoaded );
	Ptr<GLTexture2D> process(  Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap );
};

class EffectVignettingFunc: public EffectGL
{
	GLProgram m_program;
	Ptr<FuncFtoF> m_func;
protected:
	string createTextSource (Ptr<FuncFtoF> func);
public:
	EffectVignettingFunc(Ptr<SharedObjectsFactory> sof, Ptr<FuncFtoF> func );
	EffectVignettingFunc(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Ptr<FuncFtoF> func );
	Ptr<GLTexture2D> process(  Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap );
};

class EffectBrightnessAdapter: public EffectGL
{
protected:
	float3 getAverage(Ptr<GLTexture2D> tex);
	float computeBrightness (float3 color);
	Ptr<PIDController> m_pid;
	GLProgram m_program;
public:
	EffectBrightnessAdapter(Ptr<SharedObjectsFactory> sof, PIDController& pid = PIDController(0.2f));
	EffectBrightnessAdapter(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, PIDController& pid = PIDController(0.2f));
	void setPIDcontroller(PIDController& pid);
	Ptr<GLTexture2D> process (Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
};

class EffectMotionBlurSimple: public EffectGL
{
	GLProgram m_program;
	int m_numSamples;
	float3 m_deltaPos;
	float2 m_deltaView;
public:
	EffectMotionBlurSimple (Ptr<SharedObjectsFactory> sof, float3 deltaPos, float2 deltaView, int numSamples);
	EffectMotionBlurSimple (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, float3 deltaPos, float2 deltaView, int numSamples);
	bool requireDepth()
		{return true;}
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
};

class EffectMotionBlur2Phases: public EffectGL
{
	EffectMotionBlurSimple m_effect1st, m_effect2nd;
public:
	EffectMotionBlur2Phases(Ptr<SharedObjectsFactory> sof, float3 deltaPos, float2 deltaView, int numSamples1Phase, int numSamples2Phase);
	EffectMotionBlur2Phases(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, float3 deltaPos, float2 deltaView, int numSamples1Phase, int numSamples2Phase);
	bool requireDepth()
		{return true;}
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap );
};

class EffectNoiseUniform: public EffectGL
{
	GLProgram m_program;
	float m_mappingBegin;
	float m_mappingSize;
public:
	EffectNoiseUniform(Ptr<SharedObjectsFactory> sof, float begin = 0.0f, float end = 1.0f);
	EffectNoiseUniform(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, float begin = 0.0f, float end = 1.0f);
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap );
};


class EffectNoiseEmva: public EffectGL
{
	float m_fItoP, m_totalQuantumEfficiently,
		m_readNoise, m_darkCurrent, m_inverseOfOverallSystemGain, 
		m_expositionTime, m_fDNtoI;
	int m_saturationCapacity;
	GLProgram m_program;
public:
	EffectNoiseEmva(Ptr<SharedObjectsFactory> sof,
		   float fItoP, float totalQuantumEfficiently,
		   float readNoise, float darkCurrent, float inverseOfOverallSystemGain, 
		   int saturationCapacity, float expositionTime, float fDNtoI);
	EffectNoiseEmva(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed,
		float fItoP, float totalQuantumEfficiently,
		float readNoise, float darkCurrent, float inverseOfOverallSystemGain, 
		int saturationCapacity, float expositionTime, float fDNtoI);
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap );
};

class EffectResizeImages: public EffectGL
{
public:
	EffectResizeImages(Ptr<SharedObjectsFactory> sof);
	EffectResizeImages(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed);
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap );
	Ptr<GLTexture2D> processDepth(Ptr<GLTexture2D> depth);
};

class EffectDOF: public EffectGL
{
public:
	EffectDOF(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed):
		EffectGL(factoryRGB, factoryRed) {}

	virtual Ptr<GLTexture2D> process (Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap) = 0;
	virtual void setFuncCoC (FuncFtoFCompCoC& func) = 0;
	bool requireDepth()
	{
		return true;
	}
};

class EffectDOFDrawQuad: public EffectDOF
{
protected:
	virtual Ptr<GLTexture2D> createTexTarget () = 0;
	virtual void predraw( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth ) = 0;
	virtual void postdraw() = 0;
public:
	Ptr<GLTexture2D> process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
	EffectDOFDrawQuad(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed):
		EffectDOF(factoryRGB, factoryRed)
	{}
	EffectDOFDrawQuad(Ptr<SharedObjectsFactory> sof):
		EffectDOF(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture())
	{}
protected:
	void bindTexture (GLenum nTex, Ptr<GLTexture2D> tex);
	void unbindTexture(GLenum nTex);
};

class EffectDOFCircleOfConfusionInternal: public EffectDOFDrawQuad
{
protected:
	FuncFtoFCompCoC m_func;
	GLProgram m_program;
public:
	EffectDOFCircleOfConfusionInternal (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, string fragmentShaderFilename);
	EffectDOFCircleOfConfusionInternal (Ptr<SharedObjectsFactory> sof, string fragmentShaderFilename);
public:
	void setFuncCoC (FuncFtoFCompCoC& func)
	{
		m_func = func;
	}
	bool requireDepth() 
	{
		return true;
	}
protected:
	void predraw( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth );
	virtual void setProgramVariables(GLProgram& program) = 0;
	virtual void postdraw ();
};

class EffectDOFShowCoC: public EffectDOFCircleOfConfusionInternal
{
public:
	EffectDOFShowCoC (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed): 
		EffectDOFCircleOfConfusionInternal (factoryRGB, factoryRed, "shaders\\computeCoC.fs")
	{}
	EffectDOFShowCoC (Ptr<SharedObjectsFactory> sof):
		EffectDOFCircleOfConfusionInternal(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), "shader\\computeCoC.fs")
	{}
protected:
	Ptr<GLTexture2D> createTexTarget ()
	{
		return m_factoryTexRed->createProduct();
	}
	void setProgramVariables(GLProgram& program)
	{

	}
};

class EffectDOFRegularHor;
class EffectDOFRegularVert;

class EffectDOFRegular: public EffectDOF
{
	Ptr<EffectDOFRegularHor> m_dofHor;
	Ptr<EffectDOFRegularVert> m_dofVert;
public:
	EffectDOFRegular (Ptr<SharedObjectsFactory> sof, int countSamples, float mult, float aspectRatio);
	EffectDOFRegular (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, int countSamples, float mult, float aspectRatio);
	Ptr<GLTexture2D> process (Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
	void setFuncCoC (FuncFtoFCompCoC& func);
};

class EffectDOFImportanceHor;
class EffectDOFImportanceVert;

class EffectDOFImportance: public EffectDOF
{
	Ptr<EffectDOFImportanceHor> m_dofHor;
	Ptr<EffectDOFImportanceVert> m_dofVert;
	float m_mult;
	int m_countSamples;
	float m_actualAspectRatio;
public:
	EffectDOFImportance(Ptr<SharedObjectsFactory> sof, int countSamples, float mult, float aspectRatio);
	EffectDOFImportance(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, int countSamples, float mult, float aspectRatio);
	Ptr<GLTexture2D> process(Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap);
	void setFuncCoC(FuncFtoFCompCoC& func);
	void setAspectRatio(float aspectRatio);
	bool requireDepth();
};

