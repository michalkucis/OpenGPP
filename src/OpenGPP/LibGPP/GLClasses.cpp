#include "GLClasses.h"
#include "Error.h"
#include "VisitorsImpl.h"
#include "Vector3.h"
#include <assert.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <fstream>
#include <sstream>

void printGLInfo()
{
	std::cout << "OpenGL vendor:   " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "OpenGL version:  " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "OpenGL HLSL ver.:" << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}

GLTexture2D::GLTexture2D( int width, int height, uint colorInternalFormat, uint colorFormat ): 
	m_resolution(uint2(width,height)),
	m_internalFormat(colorInternalFormat),
	m_format(colorFormat)
{
	uint colorType = GL_UNSIGNED_BYTE;
	GLuint texture;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(GL_TEXTURE_2D, 0, colorInternalFormat, width, height, 0, colorFormat, colorType, 0); // place multisampling here too!
	checkGLError();
	glBindTexture(GL_TEXTURE_2D, 0);
	m_idTexture = texture;
}

uint2 GLTexture2D::getResolution ()
{
	return m_resolution;
}

void GLTexture2D::bindToFramebuffer (int attachment)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, m_idTexture, 0);
	checkGLError ();
}

void GLTexture2D::bind ()
{
	glBindTexture(GL_TEXTURE_2D, m_idTexture);
	checkGLError ();
}

void GLTexture2D::unbind ()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	checkGLError ();
}

void GLTexture2D::writeData (uchar* data, int format, int type)
{
	int width, height;
	glGetTexLevelParameteriv (GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH, &width);	
	glGetTexLevelParameteriv (GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT, &height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_resolution.x, m_resolution.y, 
		format, type, data);
	checkGLError();
}

void GLTexture2D::readData (uchar* data, int format, int type)
{
	glGetTexImage(GL_TEXTURE_2D, 0, format, type, data);
	checkGLError();
}

GLTexture2D::~GLTexture2D()
{
	checkGLError();
	glDeleteTextures( 1, &m_idTexture);
	checkGLError();
}

template<typename visitorType, typename visitorType1, int templateTexFormat, int texType>
void GLTexture2D::visitReadOnlyTemplate( Visitor2ReadOnly<visitorType>* v )
{
	int texFormat = templateTexFormat;
	if (m_format == GL_DEPTH_COMPONENT)
		texFormat = GL_DEPTH_COMPONENT;
	if (m_format == GL_RED)
		texFormat = GL_RED;
	int width = m_resolution.x;
	int height = m_resolution.y;
	glActiveTexture(GL_TEXTURE0);
	this->bind();
	checkGLError();
	int bytes = width*sizeof(visitorType) + (width*sizeof(visitorType))%4;
	uchar* data = new uchar[m_resolution.getArea()*4*sizeof(visitorType1)];
	readData (data, texFormat, texType);
	checkGLError();

	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
		{
			uchar* u = &(data[x*sizeof(visitorType) + y*bytes]);
			visitorType& u3 = *(visitorType*)u;
			v->visitRead(int2(x,y),u3);
		}
		delete [] data;
		this->unbind();
}

void GLTexture2D::visitReadOnly( Visitor2ReadOnly<uchar3>* v )
{
	visitReadOnlyTemplate<uchar3,uchar,GL_RGB,GL_UNSIGNED_BYTE> (v);
}

void GLTexture2D::visitReadOnly( Visitor2ReadOnly<float3>* v )
{
	visitReadOnlyTemplate<float3,float,GL_RGB,GL_FLOAT> (v);
}

void GLTexture2D::visitReadOnly( Visitor2ReadOnly<float>* v )
{
	visitReadOnlyTemplate<float,float,GL_DEPTH_COMPONENT,GL_FLOAT> (v);
}

template<typename visitorType, typename visitorType1, int templateTexFormat, int texType>
void GLTexture2D::visitWriteOnlyTemplate( Visitor2WriteOnly<visitorType>* v )
{
	int texFormat = templateTexFormat;
	if (m_format == GL_DEPTH_COMPONENT)
		texFormat = GL_DEPTH_COMPONENT;
	if (m_format == GL_RED)
		texFormat = GL_RED;
	glActiveTexture(GL_TEXTURE0);
	this->bind();
	checkGLError();
	int width = m_resolution.x;
	int height = m_resolution.y;
	int bytes = width*sizeof(visitorType) + (width*sizeof(visitorType))%4;
	uchar* data = new uchar[m_resolution.getArea()*4*sizeof(visitorType1)];
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
		{
			uchar* u = &(data[x*sizeof(visitorType) + y*bytes]);
			visitorType& u3 = *(visitorType*)u;
			u3 = v->visitWrite(int2(x,y));
		}
		writeData(data, texFormat, texType);
		checkGLError();
		delete [] data;
		this->unbind();
}

void GLTexture2D::visitWriteOnly( Visitor2WriteOnly<uchar3>* v )
{
	visitWriteOnlyTemplate<uchar3,uchar,GL_RGB,GL_UNSIGNED_BYTE> (v);
}

void GLTexture2D::visitWriteOnly( Visitor2WriteOnly<float3>* v )
{
	visitWriteOnlyTemplate<float3,float,GL_RGB,GL_FLOAT> (v);
}

void GLTexture2D::visitWriteOnly( Visitor2WriteOnly<float>* v )
{
	visitWriteOnlyTemplate<float,float,GL_RED,GL_FLOAT> (v);
}

void GLTexture2D::writeToFile( string file )
{
	if (m_format == GL_DEPTH_COMPONENT || m_format == GL_RED)
	{
		Visitor2ReadOnlyRedWriteToFile visitor(file);
		visitReadOnly(&visitor);
	}
	else if (m_format == GL_RGB)
	{
		Visitor2ReadOnlyRGBWriteToFile visitor(file);
		visitReadOnly(&visitor);
	}
	else
		assert(0);

}

GLTextureEnvMap::GLTextureEnvMap( int width, int height, uint depthInternalFormat): GLTexture2D(
	width, height, depthInternalFormat, GL_DEPTH_COMPONENT)
{

}

GLTextureEnvMap::GLTextureEnvMap( int width, int height, uint depthInternalFormat, uint depthFormat): GLTexture2D(
	width, height, depthInternalFormat, depthFormat)
{

}



//GLRenderbuffer::GLRenderbuffer( int width, int height, uint internalFormat ) : GLSurface
//	(uint2(width, height), internalFormat, GL_RENDERBUFFER)
//{
//	GLuint renderbuffer;
//	glGenRenderbuffers(1, &renderbuffer);
//	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
//	glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width, height); 
//	glBindRenderbuffer(GL_RENDERBUFFER, 0);
//	checkGLError();
//	m_idRenderbuffer = renderbuffer;
//}
//
//void GLRenderbuffer::bind ()
//{
//	glBindRenderbuffer(GL_RENDERBUFFER, m_idRenderbuffer);
//	checkGLError();
//}
//
//void GLRenderbuffer::unbind ()
//{
//	glBindRenderbuffer(GL_RENDERBUFFER, 0);
//	checkGLError();
//}
//
//void GLRenderbuffer::bindToFramebuffer (int attachment)
//{
//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment,
//		GL_RENDERBUFFER, m_idRenderbuffer);
//	checkGLError ();
//}
//
//void GLRenderbuffer::writeData (uchar* data, int format, int type)
//{
//	error(ERR_STRUCT);
//}
//
//void GLRenderbuffer::readData (uchar* data, int format, int type)
//{
//	error(ERR_STRUCT);
//}
//
//GLRenderbuffer::~GLRenderbuffer()
//{
//	glDeleteRenderbuffers(1, &m_idRenderbuffer);
//	m_idRenderbuffer = NULL;
//}

GLFramebuffer::GLFramebuffer( Ptr<GLTexture2D> color0, Ptr<GLTexture2D> depth )
{
	glGenFramebuffers(1, &m_framebuffer);
	checkGLError ();

	m_color = color0;
	m_depth = depth;
	m_bind = false;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	checkGLError();
}

void GLFramebuffer::bind()
{	
	m_bind = true;
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	checkGLError ();
	m_color->bindToFramebuffer(GL_COLOR_ATTACHMENT0);
	
	if (! m_depth.isNull())
		m_depth->bindToFramebuffer(GL_DEPTH_ATTACHMENT);
	else
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);

	GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, 0};
	glDrawBuffers(1, drawBuffers);

	checkGLError ();
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
		error0(ERR_OPENGL, (string) (const char*) gluErrorString(status));
	checkGLError ();
}

void GLFramebuffer::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	checkGLError ();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	checkGLError ();
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	checkGLError ();
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	checkGLError ();
	m_bind = false;
}

GLFramebuffer::~GLFramebuffer()
{
	if (m_bind)
		error0(ERR_OPENGL, "Framebuffer is binded");
	glDeleteFramebuffers(1, &m_framebuffer);
	checkGLError();
}

GLProgram::GLProgram( string vertexShaderFile, string fragmentShaderFile )
{
	GLuint vs = 0;
	if (vertexShaderFile != "")
	{
		string code = loadTxtFile(vertexShaderFile);
		vs = createShader(vertexShaderFile, code, GL_VERTEX_SHADER);
	}
	GLuint fs = 0;
	if (fragmentShaderFile != "")
	{
		string code = loadTxtFile(fragmentShaderFile);
		fs = createShader(fragmentShaderFile, code, GL_FRAGMENT_SHADER);
	}

	if (vs != 0)
		m_vecShaders.pushBack(vs);
	if (fs != 0)
		m_vecShaders.pushBack(fs);
	m_program = createProgram(m_vecShaders);
}

GLProgram::GLProgram (bool ANY_VALUE, string vertexShaderCode, string fragmentShaderCode)
{
	GLuint vs = 0;
	if (vertexShaderCode != "")
		vs = createShader("Unknown", vertexShaderCode, GL_VERTEX_SHADER);
	GLuint fs = 0;
	if (fragmentShaderCode != "")
		fs = createShader("Unknown", fragmentShaderCode, GL_FRAGMENT_SHADER);

	if (vs != 0)
		m_vecShaders.pushBack(vs);
	if (fs != 0)
		m_vecShaders.pushBack(fs);
	m_program = createProgram(m_vecShaders);
}


GLProgram::~GLProgram()
{
	for (uint i = 0; i < m_vecShaders.getSize(); i++)
	{
		glDeleteShader(m_vecShaders[i]);
		checkGLError();
	}
	glDeleteProgram(m_program);
	checkGLError();
}

string GLProgram::loadTxtFile( string filename )
{
	std::ifstream file;
	file.open(filename, std::ios::in); // opens as ASCII!
	if (! file.is_open())
		error1(ERR_STRUCT, "File '%s' cannot be opened", filename.c_str());
	std::stringstream ss;
	while (! file.eof())
	{
		string s;
		getline(file, s);
		ss << s << std::endl;
	}
	return ss.str();
}

uint GLProgram::createShader( string shaderFile, string shaderCode, uint shaderType )
{
	GLuint fs = glCreateShader(shaderType);
	const char* szSources = (shaderCode.c_str());
	glShaderSource(fs, 1, &szSources, NULL);
	glCompileShader(fs);

	GLint shaderRes;
	glGetShaderiv(fs, GL_COMPILE_STATUS, &shaderRes);
	if (! shaderRes)
	{
		int len;
		glGetShaderiv (fs, GL_INFO_LOG_LENGTH, &len);
		char* szLog = new char[len+1];
		glGetShaderInfoLog (fs, len+1, &len, szLog);
		string log = szLog;
		delete[] szLog;
		glDeleteShader(fs);
		error2(ERR_OPENGL, "Shader '%s' cannot be compiled. Desc.: %s", shaderFile.c_str(),log.c_str());
	}
	checkGLError();
	return fs;
}

uint GLProgram::createProgram( Vector<uint> shaders )
{
	GLint programRes;

	GLuint program = glCreateProgram();
	for (uint i = 0; i < shaders.getSize(); i++)
		glAttachShader(program, shaders[i]);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &programRes);
	if (! programRes)
	{
		int len;
		glGetProgramiv (program, GL_INFO_LOG_LENGTH, &len);
		char* szLog = new char[len+1];
		glGetProgramInfoLog (program, len+1, &len, szLog);
		string log = szLog;
		delete[] szLog;
		glDeleteProgram(program);
		error1(ERR_OPENGL, "Program cannot be compiled. Desc.: %s", log.c_str());
	}
	for (uint i = 0; i < shaders.getSize(); i++)
		glDetachShader(program, shaders[i]);
	checkGLError();

	return program;
}

void GLProgram::use()
{
	glUseProgram(m_program);
	checkGLError();
}

void GLProgram::setInt (string name, int value)
{
	int loc = glGetUniformLocation(m_program, name.c_str());
	checkGLError();
	glUniform1i(loc, value);
	checkGLError();
}

void GLProgram::setUint (string name, uint value)
{
	int loc = glGetUniformLocation(m_program, name.c_str());
	checkGLError();
	glUniform1ui(loc, value);
	checkGLError();
}

void GLProgram::setFloat2( string name, float2 value)
{
	int loc = glGetUniformLocation(m_program, name.c_str());
	//if (loc == -1)
	//	error0(ERR_STRUCT, "Unknown uniform name");
	checkGLError();
	glUniform2f(loc,value.x,value.y);
	checkGLError();
}

void GLProgram::setFloat3( string name, float3 value)
{
	int loc = glGetUniformLocation(m_program, name.c_str());
	//if (loc == -1)
	//	error0(ERR_STRUCT, "Unknown uniform name");
	checkGLError();
	glUniform3f(loc,value.x,value.y,value.z);
	checkGLError();
}

void GLProgram::setFloat( string name, float v )
{
	int loc = glGetUniformLocation(m_program, name.c_str());
	//if (loc == -1)
	//	error0(ERR_STRUCT, "Unknown uniform name");
	checkGLError();
	glUniform1f(loc,v);
	checkGLError();
}

void GLProgram::setArrFloat2( string name, Vector<float2>& offsets )
{
	int loc = glGetUniformLocation(m_program, name.c_str());
	//if (loc == -1)
	//	error0(ERR_STRUCT, "Unknown uniform name");
	checkGLError();
	uint size = offsets.getSize();
	float2* arr = new float2[size];
	for (uint i = 0; i < size; i++)
		arr[i] = offsets[i];
	glUniform2fv(loc, size, (float*) arr);
	delete [] (arr);
	checkGLError();
}

GLSaveViewport::GLSaveViewport( int newX, int newY, uint newSX, uint newSY )
{
	glGetIntegerv(GL_VIEWPORT, (GLint*)&m_prevViewport);
	glViewport(newX, newY, newSX, newSY);
	checkGLError();
}

GLSaveViewport::GLSaveViewport(uint newSX, uint newSY)
{
	glGetIntegerv(GL_VIEWPORT, (GLint*)&m_prevViewport);
	glViewport(0, 0, newSX, newSY);
	checkGLError();
}

GLSaveViewport::GLSaveViewport(uint2 newSize)
{
	glGetIntegerv(GL_VIEWPORT, (GLint*)&m_prevViewport);
	glViewport(0, 0, newSize.x, newSize.y);
	checkGLError();
}

GLSaveViewport::~GLSaveViewport()
{
	glViewport(m_prevViewport.x,m_prevViewport.y,m_prevViewport.z,m_prevViewport.w);
	checkGLError();
}
