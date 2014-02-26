#pragma once

#include "Base.h"
#include "Ptr.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Visitors.h"
#include "Vector.h"
#include "Error.h"

void checkGLError();


void printGLInfo ();

class GLTexture2DCL;

class GLTexture2D
{
	friend GLTexture2DCL;

protected:
	uint2 m_resolution;
	int m_internalFormat;
	int m_format;
	uint m_idTexture;
public:
	GLTexture2D (int width, int height, uint colorInternalFormat, uint colorFormat);
	uint2 getResolution ();
	void bind ();
	void unbind ();
	void bindToFramebuffer (int attachment);
	uint getGLuint() {return m_idTexture;}
protected:
	void writeData (uchar* data, int format, int type);
	void readData (uchar* data, int format, int type);
public:
	virtual ~GLTexture2D ();

	template<typename visitorType, typename visitorType1, int texFormat, int texType>
	void visitReadOnlyTemplate (Visitor2ReadOnly<visitorType>* v);
	void visitReadOnly (Visitor2ReadOnly<uchar3>* v);
	void visitReadOnly (Visitor2ReadOnly<float3>* v);
	void visitReadOnly (Visitor2ReadOnly<float>* v);

	template<typename visitorType, typename visitorType1, int texFormat, int texType>
	void visitWriteOnlyTemplate (Visitor2WriteOnly<visitorType>* v);
	void visitWriteOnly (Visitor2WriteOnly<uchar3>* v);
	void visitWriteOnly (Visitor2WriteOnly<float3>* v);
	void visitWriteOnly (Visitor2WriteOnly<float>* v);

	void writeToFile (string file);
};


class GLTextureEnvMap: public GLTexture2D
{
public:
	GLTextureEnvMap( int width, int height, uint depthInternalFormat);
	GLTextureEnvMap( int width, int height, uint depthInternalFormat, uint depthFormat);
};

class GLFramebuffer
{
	bool m_bind;
	uint m_framebuffer;
	Ptr<GLTexture2D> m_color;
	Ptr<GLTexture2D> m_depth;
public:
	GLFramebuffer (Ptr<GLTexture2D> color0 = NULL, Ptr<GLTexture2D> depth = NULL);
	void setColor (Ptr<GLTexture2D> color)
	{
		if (m_bind)
			error0(ERR_OPENGL, "Framebuffer is binded");
		m_color = color;
	}
	void setDepth (Ptr<GLTexture2D> depth)
	{
		if (m_bind)
			error0(ERR_OPENGL, "Framebuffer is binded");
		m_depth = depth;
	}
	void bind ();
	void unbind ();
	operator uint ()
	{		
		return m_framebuffer;
	}
	~GLFramebuffer();
};

class GLProgram
{
	string loadTxtFile (string filename);
	uint createShader(string shaderFile, string filename, uint shaderType);
	uint createProgram(Vector<uint> shaders);

	Vector<uint> m_vecShaders;
	uint m_program;
public:
	GLProgram (string vertexShaderFile, string fragmentShaderFile);
	GLProgram (bool ANY_VALUE, string vertexShaderCode, string fragmentShaderCode);
	~GLProgram();
	void use ();
	void setInt (string name, int value);
	void setUint (string name, uint value);
	void setFloat2( string name, float2 value);
	void setFloat3( string name, float3 value);
	void setFloat( string name, float v );
	void setArrFloat2( string name, Vector<float2>& offsets );
};

class GLSaveViewport
{
	int4 m_prevViewport;
public:
	GLSaveViewport(int newX, int newY, uint newSX, uint newSY);
	GLSaveViewport(uint newSX, uint newSY);
	GLSaveViewport(uint2 newSize);
	~GLSaveViewport ();
};