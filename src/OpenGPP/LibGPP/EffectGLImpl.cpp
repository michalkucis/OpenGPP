#include "EffectGLImpl.h"
#include <SDL_timer.h>
#include "Ptr.h"
#include "Visitors.h"
#include "Vector2.h"
#include "VisitorsImpl.h"
#include <sstream>

Ptr<GLTexture2D> EffectCopyColorAndDepthMaps::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envmap )
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	tex->bind();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	drawQuad();

	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	return texTarget;
}

Ptr<GLTexture2D> EffectCopyColorAndDepthMaps::processDepth( Ptr<GLTexture2D> depth)
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRed->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	depth->bind();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();
	drawQuad();
	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	return texTarget;
}

void EffectDistortionFunc::getGridPre( Matrix<float2>& out, uint2 sizeGrid )
{
	Visitor2WriteGridByLinearInterp visitor(sizeGrid);
	out.setSize(sizeGrid);
	out.visit (&visitor);
}

void EffectDistortionFunc::getGridFromFunc( Matrix<float2>& out, uint2 sizeGrid, Ptr<FuncFtoF> func )
{
	Visitor2WriteGridByFuncFtoF visitor(sizeGrid, func);
	out.setSize(sizeGrid);
	out.visit(&visitor);
}

float2 EffectDistortionFunc::getPos( Matrix<float2>& mat, int2 pos )
{
	int2 size = mat.getSize().get2<int2>();
	if ((pos.x >= 0) && (pos.x < size.x) && (pos.y >= 0) && (pos.y < size.y))
		return mat[pos.x][pos.y];
	else if (pos.x < 0)
	{
		if (pos.y < 0)
			return float2(-1,-1);
		else if (pos.y >= size.y)
			return float2(-1,1);
		else 
			return float2(-1,mat[0][pos.y].y);
	}
	else if (pos.x >= size.x)
	{
		if (pos.y < 0)
			return float2(1,-1);
		else if (pos.y >= size.y)
			return float2(1,1);
		else 
			return float2(1,mat[size.x-1][pos.y].y);
	}
	else
	{
		if (pos.y < 0)
			return float2(mat[pos.x][0].x, -1);
		else
			return float2(mat[pos.x][size.y-1].x, 1);
	}
}

void EffectDistortionFunc::drawVertex( int2 index, Matrix<float2>& gridPre, Matrix<float2>& gridPost )
{
	float2 nposPre, nposPost;
	nposPre = getPos (gridPre, index);
	nposPost = getPos (gridPost, index);

	nposPre = (nposPre + 1) / 2;
	nposPost = (nposPost + 1) / 2;
// 	std::cout << "glTexCoord2f.. x=" << nposPre.x << "  y=" << nposPre.y << "\n";
// 	std::cout << "glVertex2f.... x=" << nposPost.x << "  y=" << nposPost.y << "\n";
	glTexCoord2f(nposPre.x, 1-nposPre.y);	
	glVertex2f(nposPost.x,  nposPost.y);
}

void EffectDistortionFunc::drawGridElement( int2 index, Matrix<float2>& gridPre, Matrix<float2>& gridPost )
{
	int2 indices [4];
	indices[0] = index;
	indices[1] = int2(index.x, index.y+1);
	indices[2] = index + 1;
	indices[3] = int2(index.x+1, index.y);

	for (int i = 0; i < 4; i++)
		drawVertex(indices[i], gridPre, gridPost);

	//std::cout << "\n\n";
}

void EffectDistortionFunc::drawGridInTheMiddle (Matrix<float2>& gridPre, Matrix<float2>& gridPost)
{
	glBegin(GL_QUADS); 
	for (int x = 0; x < (int) gridPre.getSize().x-1; x++)
		for (int y = 0; y < (int) gridPre.getSize().y-1; y++)
			drawGridElement(int2(x,y), gridPre, gridPost);
	glEnd();
}

void EffectDistortionFunc::drawGridInTheBorders (Matrix<float2>& gridPre, Matrix<float2>& gridPost)
{
	glBegin(GL_QUADS); 
	for (int x = 0; x < (int) gridPre.getSize().x-1; x++)
	{
		drawGridElement(int2(x,-1), gridPre, gridPost);
		drawGridElement(int2(x,gridPre.getSize().y-1), gridPre, gridPost);
	}
	for (int y = -1; y < (int) gridPre.getSize().y; y++)
	{
		drawGridElement(int2(-1,y), gridPre, gridPost);
		drawGridElement(int2(gridPre.getSize().x-1,y), gridPre, gridPost);
	}
	glEnd();
}

EffectDistortionGrid::EffectDistortionGrid (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed):
	EffectGL(factoryRGB, factoryRed)
{
	Matrix<float2> mat (uint2(2,2));
	mat[0][0] = float2(-1,-1);
	mat[0][1] = float2(-1,+1);
	mat[1][0] = float2(+1,-1);
	mat[1][1] = float2(+1,+1);
	setGrid(mat);
}

EffectDistortionGrid::EffectDistortionGrid( Ptr<SharedObjectsFactory> sof, Matrix<float2>& grid ):
	EffectGL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture())
{
	setGrid(grid);
}

EffectDistortionGrid::EffectDistortionGrid( Ptr<SharedObjectsFactory> sof, uint2 gridResolution, Ptr<FuncFtoF> func ):
	EffectGL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture())
{
	setGrid(gridResolution, func);
}

EffectDistortionGrid::EffectDistortionGrid (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, Matrix<float2>& grid):
	EffectGL(factoryRGB, factoryRed)
{
	setGrid(grid);
}

EffectDistortionGrid::EffectDistortionGrid (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, uint2 gridResolution, Ptr<FuncFtoF> func):
	EffectGL(factoryRGB, factoryRed)
{
	setGrid(gridResolution, func);
}

void EffectDistortionGrid::setGrid (Matrix<float2>& grid)
{
	getGridPre (m_gridPre, grid.getSize());

	m_gridPost = grid;
}

void EffectDistortionGrid::setGrid (uint2 gridResolution, Ptr<FuncFtoF> func)
{
	uint2 gridSize = gridResolution+1;
	getGridPre (m_gridPre, gridSize);
	getGridFromFunc (m_gridPost, gridSize, func);
}

Ptr<GLTexture2D> EffectDistortionGrid::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	tex->bind();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();
	drawGridInTheBorders(m_gridPre, m_gridPost);
	drawGridInTheMiddle(m_gridPre, m_gridPost);
	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	return texTarget;
}

Ptr<GLTexture2D> EffectDistortionGrid::processDepth( Ptr<GLTexture2D> depth )
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRed->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	depth->bind();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();
	drawGridInTheBorders(m_gridPre, m_gridPost);
	drawGridInTheMiddle(m_gridPre, m_gridPost);
	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	return texTarget;
}


template<int FUNC>
class RestorerGLBool
{
	bool m_bStatus;
public:
	RestorerGLBool ()
	{
		m_bStatus = glIsEnabled(FUNC) ? true : false;
	}
	~RestorerGLBool ()
	{
		if (m_bStatus)
			glEnable(FUNC);
		else
			glDisable(FUNC);
	}
};

EffectChromaticAberration::EffectChromaticAberration(Ptr<SharedObjectsFactory> sof): 
	m_programRed ("", "shaders\\distortionGridRed.fs"),
	m_programGreen ("", "shaders\\distortionGridGreen.fs"),
	m_programBlue ("", "shaders\\distortionGridBlue.fs"),
	EffectGL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture())
{
	Matrix<float2> mat (uint2(2,2));
	mat[0][0] = float2(-1,-1);
	mat[0][1] = float2(-1,+1);
	mat[1][0] = float2(+1,-1);
	mat[1][1] = float2(+1,+1);
	setGrid(0, mat);
	setGrid(1, mat);
	setGrid(2, mat);
}

EffectChromaticAberration::EffectChromaticAberration(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed): 
	m_programRed ("", "shaders\\distortionGridRed.fs"),
	m_programGreen ("", "shaders\\distortionGridGreen.fs"),
	m_programBlue ("", "shaders\\distortionGridBlue.fs"),
	EffectGL(factoryRGB, factoryRed)
{
	Matrix<float2> mat (uint2(2,2));
	mat[0][0] = float2(-1,-1);
	mat[0][1] = float2(-1,+1);
	mat[1][0] = float2(+1,-1);
	mat[1][1] = float2(+1,+1);
	setGrid(0, mat);
	setGrid(1, mat);
	setGrid(2, mat);
}

void EffectChromaticAberration::setGrid (uint numChannel, Matrix<float2>& grid)
{
	assert (numChannel < 3);
	getGridPre(m_gridPre[numChannel], grid.getSize());
	m_gridPost[numChannel] = grid;
}

void EffectChromaticAberration::setGrid (uint numChannel, uint2 gridResolution, Ptr<FuncFtoF> func)
{
	assert (numChannel < 3);
	getGridPre(m_gridPre[numChannel], gridResolution);
	getGridFromFunc(m_gridPost[numChannel], gridResolution, func);
}

Ptr<GLTexture2D> EffectChromaticAberration::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	glActiveTexture(GL_TEXTURE0);
	tex->bind();
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	RestorerGLBool<GL_DEPTH_TEST> stateDepthTest;
	glDisable(GL_DEPTH_TEST);
	RestorerGLBool<GL_TEXTURE_2D> stateTexture2D;
	glEnable(GL_TEXTURE_2D);

	m_programRed.use();
	drawGridInTheBorders(m_gridPre[0], m_gridPost[0]);

	RestorerGLBool<GL_BLEND> stateBlend;
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE,GL_ONE);
	m_programGreen.use();
	drawGridInTheBorders(m_gridPre[1], m_gridPost[1]);
	m_programBlue.use();
	drawGridInTheBorders(m_gridPre[2], m_gridPost[2]);

	m_programRed.use();
	drawGridInTheMiddle(m_gridPre[0], m_gridPost[0]);
	m_programGreen.use();
	drawGridInTheMiddle(m_gridPre[1], m_gridPost[1]);
	m_programBlue.use();
	drawGridInTheMiddle(m_gridPre[2], m_gridPost[2]);
	
	glUseProgram(0);
	
	glDisable(GL_BLEND);
	//glBlendEquation(GL_BLEND_COLOR);
	//glBlendFunc(GL_ONE, GL_ZERO);
	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);
	return texTarget;
}

Ptr<GLTexture2D> EffectChromaticAberration::processDepth( Ptr<GLTexture2D> depth )
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRed->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	glActiveTexture(GL_TEXTURE0);
	depth->bind();
	RestorerGLBool<GL_DEPTH_TEST> stateDepthTest;
	glDisable(GL_DEPTH_TEST);
	RestorerGLBool<GL_TEXTURE_2D> stateTexture2D;
	glEnable(GL_TEXTURE_2D);

	drawGridInTheBorders(m_gridPre[0], m_gridPost[0]);
	drawGridInTheMiddle(m_gridPre[0], m_gridPost[0]);

	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);
	return texTarget;
}

EffectVignettingImageFile::EffectVignettingImageFile(Ptr<SharedObjectsFactory> sof, string filename, bool dynamicLoaded ) : 
	EffectGL(sof),
	m_program("", "shaders\\vignettingImage.fs")
{
	m_filename = filename;
	m_dynamicLoaded = dynamicLoaded;
	if (! m_dynamicLoaded)
		m_texLoaded = loadTextureFromFile(m_filename); 
}

EffectVignettingImageFile::EffectVignettingImageFile(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, string filename, bool dynamicLoaded ) : 
	EffectGL(factoryRGB, factoryRed),
	m_program("", "shaders\\vignettingImage.fs")
{
	m_filename = filename;
	m_dynamicLoaded = dynamicLoaded;
	if (! m_dynamicLoaded)
		m_texLoaded = loadTextureFromFile(m_filename); 
}

Ptr<GLTexture2D> EffectVignettingImageFile::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	Ptr<GLTexture2D> texVignetting;
	if (m_dynamicLoaded)
		texVignetting = loadTextureFromFile(m_filename);
	else
		texVignetting = m_texLoaded;

	glActiveTexture(GL_TEXTURE0);
	tex->bind();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	texVignetting->bind();
	glEnable(GL_TEXTURE_2D);

	m_program.use();
	m_program.setInt("tex", 0);
	m_program.setInt("texVignetting",1);
	drawQuad();
	glUseProgram(0);
	checkGLError();

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	checkGLError();

	framebufferTarget->unbind();
	return texTarget;
}


Ptr<GLTexture2D> EffectVignettingImageFunc::createTexture (uint2 resolution, Ptr<FuncFtoF> func)
{
	Ptr<GLTexture2D> tex = new GLTexture2D(resolution.x, resolution.y, GL_RGB32F, GL_RGB);
	Visitor2WriteRGBByFuncFtoF visitor(resolution, func);
	tex->visitWriteOnly(&visitor);
	return tex;
}

EffectVignettingImageFunc::EffectVignettingImageFunc(Ptr<SharedObjectsFactory> sof, 
	uint2 resolution, Ptr<FuncFtoF> func, bool dynamicLoaded ) :
	EffectGL(sof),
	m_program("", "shaders\\vignettingImage.fs")
{
	m_dynamicLoaded = dynamicLoaded;
	m_resolution = resolution;
	if (m_dynamicLoaded)
	{
		m_func = func;
		m_resolution = resolution;
	}
	else
	{
		m_texLoaded = createTexture(resolution, func);
	}
}

EffectVignettingImageFunc::EffectVignettingImageFunc(Ptr<GLTexturesRGBFactory> factoryRGB,  Ptr<GLTexturesRedFactory> factoryRed, 
	uint2 resolution, Ptr<FuncFtoF> func, bool dynamicLoaded ) :
	EffectGL(factoryRGB, factoryRed),
	m_program("", "shaders\\vignettingImage.fs")
{
	m_dynamicLoaded = dynamicLoaded;
	m_resolution = resolution;
	if (m_dynamicLoaded)
	{
		m_func = func;
		m_resolution = resolution;
	}
	else
	{
		m_texLoaded = createTexture(resolution, func);
	}
}

Ptr<GLTexture2D> EffectVignettingImageFunc::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap )
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	Ptr<GLTexture2D> texVignetting;
	if (m_dynamicLoaded)
		texVignetting = createTexture(m_resolution, m_func);
	else
		texVignetting = m_texLoaded;

	glActiveTexture(GL_TEXTURE0);
	tex->bind();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	texVignetting->bind();
	glEnable(GL_TEXTURE_2D);

	m_program.use();
	m_program.setInt("tex", 0);
	m_program.setInt("texVignetting",1);
	drawQuad();
	glUseProgram(0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	framebufferTarget->unbind();
	return texTarget;
}


string EffectVignettingFunc::createTextSource( Ptr<FuncFtoF> func )
{
	std::stringstream ss;
	ss << 
		"uniform sampler2D tex;\n"
		"\n"
		"float mask (float x)\n"
		"{\n"
		"  float y;\n"
		"  " << func->getString() << ";\n"
		"  return y;\n"
		"\n"
		"}\n"
		"void main ()\n"
		"{\n"
		"  vec4 color = texture2D(tex, gl_TexCoord[0].st);\n"
		"  vec2 coord = {gl_TexCoord[0].s, gl_TexCoord[0].t};\n"
		"  coord = coord * 2 - 1;\n"
		"  float distance = sqrt(coord.s*coord.s+coord.t*coord.t) / sqrt(2);\n"
		"  float m = mask(distance);"
		"  m = m < 0 ? 0 : m;\n"
		"  gl_FragColor = color * m;\n"
		"}\n";
	string str = ss.str();
	//std::cout << str;
	return str;
}

EffectVignettingFunc::EffectVignettingFunc(Ptr<SharedObjectsFactory> sof, Ptr<FuncFtoF> func ): 
	EffectGL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture()),
	m_program(true, "", createTextSource(func))
{
}

EffectVignettingFunc::EffectVignettingFunc(Ptr<GLTexturesRGBFactory> factoryRGB,  Ptr<GLTexturesRedFactory> factoryRed, Ptr<FuncFtoF> func ): 
EffectGL(factoryRGB, factoryRed),
	m_program(true, "", createTextSource(func))
{
}


Ptr<GLTexture2D> EffectVignettingFunc::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap )
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	tex->bind();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	m_program.use();

	drawQuad();
	glUseProgram(0);

	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	return texTarget;
}


float3 EffectBrightnessAdapter::getAverage( Ptr<GLTexture2D> tex )
{
	glGenerateMipmap (GL_TEXTURE_2D);
	checkGLError();

	int max_level;
	glGetTexParameteriv( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &max_level );
	checkGLError();
	int max_mipmap = -1;
	for ( int i = 0; i < max_level; ++i )
	{
		int width;
		int height;
		glGetTexLevelParameteriv( GL_TEXTURE_2D, i, GL_TEXTURE_WIDTH, &width );
		glGetTexLevelParameteriv( GL_TEXTURE_2D, i, GL_TEXTURE_HEIGHT, &height );
		if ( 1 == height && 1 == width )
		{
			max_mipmap = i;
			break;
		}
		else if ( 0 == height || 0 == width )
		{
			max_mipmap = i-1;
			break;
		}
		checkGLError ();
	}
	float3 avg;
	glGetTexImage(GL_TEXTURE_2D, max_mipmap, GL_RGB, GL_FLOAT, &avg);
	checkGLError ();

	return avg;
}

float EffectBrightnessAdapter::computeBrightness( float3 color )
{
	return (color.x + color.y + color.z) / 3;
}

EffectBrightnessAdapter::EffectBrightnessAdapter(Ptr<SharedObjectsFactory> sof, PIDController& pid ) : 
	EffectGL(sof),
	m_program("", "shaders\\imageMult.fs"),
	m_pid(new PIDController(pid))
{
}

EffectBrightnessAdapter::EffectBrightnessAdapter(Ptr<GLTexturesRGBFactory> factoryRGB,  Ptr<GLTexturesRedFactory> factoryRed, PIDController& pid ) : 
	EffectGL(factoryRGB, factoryRed),
	m_program("", "shaders\\imageMult.fs"),
	m_pid(new PIDController(pid))
{
}

void EffectBrightnessAdapter::setPIDcontroller( PIDController& pid )
{
	*m_pid = pid;
}

Ptr<GLTexture2D> EffectBrightnessAdapter::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap )
{
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();
	tex->bind();
	m_program.use();

	float3 avg = getAverage(tex);
	float brightness = computeBrightness(avg);
	float mult = m_pid->recomputeLog2(brightness);
	m_program.setFloat3("mult", float3(mult,mult,mult));
	drawQuad();

	glUseProgram(0);
	tex->unbind();
	framebufferTarget->unbind();
	return texTarget;
}

EffectMotionBlurSimple::EffectMotionBlurSimple(Ptr<SharedObjectsFactory> sof, float3 deltaPos, float2 deltaView, int numSamples ) : 
	EffectGL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture()),
	m_program("", "shaders\\motionBlur.fs")
{
	m_numSamples = numSamples;
	m_deltaPos = deltaPos;
	m_deltaView = deltaView;
}

EffectMotionBlurSimple::EffectMotionBlurSimple(Ptr<GLTexturesRGBFactory> factoryRGB,  Ptr<GLTexturesRedFactory> factoryRed,
	float3 deltaPos, float2 deltaView, int numSamples ) : 
	EffectGL(factoryRGB, factoryRed),
	m_program("", "shaders\\motionBlur.fs")
{
	m_numSamples = numSamples;
	m_deltaPos = deltaPos;
	m_deltaView = deltaView;
}

Ptr<GLTexture2D> EffectMotionBlurSimple::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	tex->bind();
	glActiveTexture(GL_TEXTURE1);
	depth->bind();
	glEnable(GL_TEXTURE_2D);

	m_program.use();
	m_program.setUint("numSamples", m_numSamples);
	m_program.setInt("tex", 0);
	m_program.setInt("depth",1);
	m_program.setFloat3("deltaPos", m_deltaPos);
	m_program.setFloat2("deltaView", m_deltaView);
	drawQuad();
	glUseProgram(0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	framebufferTarget->unbind();
	return texTarget;
}

EffectMotionBlur2Phases::EffectMotionBlur2Phases(Ptr<SharedObjectsFactory> sof,
	float3 deltaPos, float2 deltaView, int numSamples1Phase, int numSamples2Phase ): 
EffectGL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture()),
	m_effect1st(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(),
	deltaPos/(numSamples2Phase+1.0f), 
	deltaView/(numSamples2Phase+1.0f), 
	numSamples1Phase),
	m_effect2nd(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(),
	deltaPos*(numSamples2Phase-1.0f)/(float)numSamples2Phase, 
	deltaView*(numSamples2Phase-1.0f)/(float)numSamples2Phase, 
	numSamples2Phase)
{

}

EffectMotionBlur2Phases::EffectMotionBlur2Phases(Ptr<GLTexturesRGBFactory> factoryRGB,  Ptr<GLTexturesRedFactory> factoryRed,
	float3 deltaPos, float2 deltaView, int numSamples1Phase, int numSamples2Phase ): 
	EffectGL(factoryRGB, factoryRed),
	m_effect1st(factoryRGB, factoryRed,
			deltaPos/(numSamples2Phase+1.0f), 
			deltaView/(numSamples2Phase+1.0f), 
			numSamples1Phase),
	m_effect2nd(factoryRGB, factoryRed,
			deltaPos*(numSamples2Phase-1.0f)/(float)numSamples2Phase, 
			deltaView*(numSamples2Phase-1.0f)/(float)numSamples2Phase, 
			numSamples2Phase)
{

}

Ptr<GLTexture2D> EffectMotionBlur2Phases::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	tex = m_effect1st.process(tex,depth, envMap);
	tex = m_effect2nd.process(tex,depth, envMap);
	return tex;
}

EffectNoiseUniform::EffectNoiseUniform(Ptr<SharedObjectsFactory> sof, float begin, float end) : 
	EffectGL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture()),
	m_program("", "shaders\\noiseUniform.fs")
{
	m_mappingBegin = begin;
	m_mappingSize = end - begin;
}

EffectNoiseUniform::EffectNoiseUniform(Ptr<GLTexturesRGBFactory> factoryRGB,  Ptr<GLTexturesRedFactory> factoryRed, float begin, float end) : 
	EffectGL(factoryRGB, factoryRed),
	m_program("", "shaders\\noiseUniform.fs")
{
	m_mappingBegin = begin;
	m_mappingSize = end - begin;
}

Ptr<GLTexture2D> EffectNoiseUniform::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	tex->bind();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	m_program.use();
	uint ticks = SDL_GetTicks();
	float fticks = ((float)(ticks%1000))/1000;

	m_program.setFloat2("vecOffsetRed", float2(0,0) + fticks);
	m_program.setFloat2("vecOffsetGreen", float2(0.5f,0) + fticks);
	m_program.setFloat2("vecOffsetBlue", float2(0,0.5f) + fticks);
	m_program.setFloat("mappingBegin", m_mappingBegin);
	m_program.setFloat("mappingSize", m_mappingSize);
	drawQuad();
	glUseProgram(0);

	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	return texTarget;
}

EffectNoiseEmva::EffectNoiseEmva(Ptr<SharedObjectsFactory> sof,
	float fItoP, float totalQuantumEfficiently, float readNoise, float darkCurrent, float inverseOfOverallSystemGain, 
	int saturationCapacity, float expositionTime, float fDNtoI) : 
EffectGL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture()),
	m_program("", "shaders\\noiseEmva.fs")
{
	m_fItoP = fItoP;
	m_totalQuantumEfficiently = totalQuantumEfficiently;
	m_readNoise = readNoise;
	m_darkCurrent = darkCurrent;
	m_inverseOfOverallSystemGain = inverseOfOverallSystemGain; 
	m_expositionTime = expositionTime;
	m_fDNtoI = fDNtoI;
	m_saturationCapacity = saturationCapacity;
}

EffectNoiseEmva::EffectNoiseEmva(Ptr<GLTexturesRGBFactory> factoryRGB,  Ptr<GLTexturesRedFactory> factoryRed,
	float fItoP, float totalQuantumEfficiently, float readNoise, float darkCurrent, float inverseOfOverallSystemGain, 
	int saturationCapacity, float expositionTime, float fDNtoI) : 
		EffectGL(factoryRGB, factoryRed),
		m_program("", "shaders\\noiseEmva.fs")
{
	m_fItoP = fItoP;
	m_totalQuantumEfficiently = totalQuantumEfficiently;
	m_readNoise = readNoise;
	m_darkCurrent = darkCurrent;
	m_inverseOfOverallSystemGain = inverseOfOverallSystemGain; 
	m_expositionTime = expositionTime;
	m_fDNtoI = fDNtoI;
	m_saturationCapacity = saturationCapacity;
}

Ptr<GLTexture2D> EffectNoiseEmva::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	tex->bind();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	m_program.use();
	uint ticks = SDL_GetTicks();
	float fticks = ((float)(ticks%1000))/1000;

	m_program.setFloat2("vecOffsetRed", float2(0,0) + fticks);
	m_program.setFloat2("vecOffsetGreen", float2(0.5f,0) + fticks);
	m_program.setFloat2("vecOffsetBlue", float2(0,0.5f) + fticks);

	m_program.setFloat("fItoP", m_fItoP);
	m_program.setFloat("totalQuantumEfficiently", m_totalQuantumEfficiently);
	m_program.setFloat("readNoise", m_readNoise);
	m_program.setFloat("darkCurrent", m_darkCurrent);
	m_program.setFloat("inverseOfOverallSystemGain", m_inverseOfOverallSystemGain); 
	m_program.setFloat("expositionTime", m_expositionTime);
	m_program.setFloat("fDNtoI", m_fDNtoI);
	m_program.setInt  ("saturationCapacity", m_saturationCapacity);

	drawQuad();
	glUseProgram(0);

	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	return texTarget;
}

EffectResizeImages::EffectResizeImages(Ptr<SharedObjectsFactory> sof):
	EffectGL(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture())
{
}

EffectResizeImages::EffectResizeImages(Ptr<GLTexturesRGBFactory> factoryRGB,  Ptr<GLTexturesRedFactory> factoryRed):
	EffectGL(factoryRGB, factoryRed)
{
}

Ptr<GLTexture2D> EffectResizeImages::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRGB->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	tex->bind();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	drawQuad();

	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	return texTarget;
}

Ptr<GLTexture2D> EffectResizeImages::processDepth(Ptr<GLTexture2D> depth)
{
	Ptr<GLTexture2D> texTarget = m_factoryTexRed->createProduct();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	depth->bind();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	drawQuad();

	framebufferTarget->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	return texTarget;
}

Ptr<GLTexture2D> EffectDOFDrawQuad::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap )
{
	Ptr<GLTexture2D> texTarget = createTexTarget ();
	Ptr<GLFramebuffer> framebufferTarget = m_factoryFramebuffers->createProduct();
	GLSaveViewport savedViewport (texTarget->getResolution());
	framebufferTarget->setColor(texTarget);
	framebufferTarget->bind();

	predraw(tex,depth);
	drawQuad();
	postdraw();

	framebufferTarget->unbind();

	return texTarget;
}

void EffectDOFDrawQuad::bindTexture( GLenum nTex, Ptr<GLTexture2D> tex )
{
	glActiveTexture(nTex);
	glEnable(GL_TEXTURE_2D);
	tex->bind();
}

void EffectDOFDrawQuad::unbindTexture( GLenum nTex )
{
	glActiveTexture(nTex);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}

EffectDOFCircleOfConfusionInternal::EffectDOFCircleOfConfusionInternal(Ptr<SharedObjectsFactory> sof, string fragmentShaderFilename ) : 
EffectDOFDrawQuad(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture()),
	m_func (1.77f, 3, 48, 42, 0.01f),
	m_program("", fragmentShaderFilename)
{

}

EffectDOFCircleOfConfusionInternal::EffectDOFCircleOfConfusionInternal(
	Ptr<GLTexturesRGBFactory> factoryRGB,  Ptr<GLTexturesRedFactory> factoryRed, string fragmentShaderFilename ) : 
	EffectDOFDrawQuad(factoryRGB, factoryRed),
	m_func (1.77f, 3, 48, 42, 0.01f),
	m_program("", fragmentShaderFilename)
{

}

void EffectDOFCircleOfConfusionInternal::predraw( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth )
{
	bindTexture(GL_TEXTURE0, tex);
	bindTexture(GL_TEXTURE1, depth);

	m_program.use();
	m_program.setInt("texColor", 0);
	m_program.setInt("texDepth", 1);
	m_program.setFloat("b1m", m_func.getB1()*m_func.getM());
	m_program.setFloat("s", m_func.getS());
	m_program.setFloat("inM", m_func.getInMult());
	setProgramVariables (m_program);
}

void EffectDOFCircleOfConfusionInternal::postdraw()
{
	glUseProgram(0);
	unbindTexture(GL_TEXTURE1);
	unbindTexture(GL_TEXTURE0);
}


class EffectDOFCore: public EffectDOFCircleOfConfusionInternal
{
public:
	EffectDOFCore (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed): 
		EffectDOFCircleOfConfusionInternal (factoryRGB, factoryRed, "shaders\\dofCore.fs")
	{
	}
protected:
	Ptr<GLTexture2D> createTexTarget ()
	{
		return m_factoryTexRGB->createProduct();
	}
	void setProgramVariables(GLProgram& program)
	{
		Vector<float2> offsets = getOffsets();
		program.setInt("sizeArr", offsets.getSize());
		program.setArrFloat2("arrCoordOffsets", offsets);
	}
	virtual Vector<float2>& getOffsets () = 0;
};

class EffectDOFRegularHor: public EffectDOFCore
{
	Vector<float2> m_arrOffsets;
public:
	EffectDOFRegularHor (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, int countSamples, float mult):
		EffectDOFCore(factoryRGB, factoryRed)
	{
		mult *= sqrtf(3.14159265358f / 4.0f);
		float divider = (float)countSamples*2 - 1;
		m_arrOffsets.setSize(countSamples);
		for (int i = 0; i < countSamples; i++)
		{
			float offset = (i*2+1) / divider;
			offset = offset*2 - 1;
			m_arrOffsets[i] = float2(offset, 0) * mult;
		}
	}
protected:
	Vector<float2>& getOffsets ()
	{
		return m_arrOffsets;
	}
};

class EffectDOFRegularVert: public EffectDOFCore
{
	Vector<float2> m_arrOffsets;
public:
	EffectDOFRegularVert (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, int countSamples, float mult):
		EffectDOFCore(factoryRGB, factoryRed)
	{
		mult *= sqrtf(3.14159265358f / 4.0f);
		float divider = (float)countSamples*2 - 1;
		m_arrOffsets.setSize(countSamples);
		for (int i = 0; i < countSamples; i++)
		{
			float offset = (i*2+1) / divider;
			offset = offset*2 - 1;
			m_arrOffsets[i] = float2(0, offset) * mult;
		}
	}
protected:
	Vector<float2>& getOffsets ()
	{
		return m_arrOffsets;
	}
};

EffectDOFRegular::EffectDOFRegular(Ptr<SharedObjectsFactory> sof, int countSamples, float mult, float aspectRatio ) : 
	m_dofHor (new EffectDOFRegularHor (sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), countSamples, mult)), 
	m_dofVert (new EffectDOFRegularVert (sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), countSamples, mult * aspectRatio)),
	EffectDOF(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture())
{

}

EffectDOFRegular::EffectDOFRegular(Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, 
		int countSamples, float mult, float aspectRatio ) : 
	m_dofHor (new EffectDOFRegularHor (factoryRGB, factoryRed, countSamples, mult)), 
	m_dofVert (new EffectDOFRegularVert (factoryRGB, factoryRed, countSamples, mult * aspectRatio)),
	EffectDOF(factoryRGB, factoryRed)
{

}

Ptr<GLTexture2D> EffectDOFRegular::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	tex = m_dofHor->process(tex, depth, envMap);
	tex = m_dofVert->process(tex, depth, envMap);
	return tex;
}

void EffectDOFRegular::setFuncCoC( FuncFtoFCompCoC& func )
{
	m_dofHor->setFuncCoC(func);
	m_dofVert->setFuncCoC(func);
}

class EffectDOFImportanceHor: public EffectDOFCore
{
	Vector<float2> m_arrOffsets;
public:
	EffectDOFImportanceHor (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, int countSamples, float mult):
		EffectDOFCore(factoryRGB, factoryRed)
	{
		float defaultOrgOffset = 10.0f / 100.0f;
		float multStd = getQuantilNormalDistro (defaultOrgOffset, 0, 1);
		mult *= multStd / 2;
		m_arrOffsets.setSize(countSamples);
		for (int i = 0; i < countSamples; i++)
		{
			float normOffset = (i*2+1) / (countSamples*2.0f);
			float offset = normOffset*(1-defaultOrgOffset*2) + defaultOrgOffset;
			float x = getQuantilNormalDistro(offset, 0, 1);
			m_arrOffsets[i] = float2(x, 0) * mult;
		}
	}
protected:
	Vector<float2>& getOffsets ()
	{
		return m_arrOffsets;
	}
};

class EffectDOFImportanceVert: public EffectDOFCore
{
	Vector<float2> m_arrOffsets;
public:
	EffectDOFImportanceVert (Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, int countSamples, float mult):
		EffectDOFCore(factoryRGB, factoryRed)
	{
		modifyOffsets(countSamples, mult);
	}
	void modifyOffsets(int countSamples, float mult)
	{
		float defaultOrgOffset = 10.0f / 100.0f;
		float multStd = getQuantilNormalDistro (defaultOrgOffset, 0, 1);
		mult *= multStd / 2;
		m_arrOffsets.setSize(countSamples);
		for (int i = 0; i < countSamples; i++)
		{
			float normOffset = (i*2+1) / (countSamples*2.0f);
			float offset = normOffset*(1-defaultOrgOffset*2) + defaultOrgOffset;
			float y = getQuantilNormalDistro(offset, 0, 1);
			m_arrOffsets[i] = float2(0, y) * mult;
		}
	}
protected:
	Vector<float2>& getOffsets ()
	{
		return m_arrOffsets;
	}
};

EffectDOFImportance::EffectDOFImportance(
	Ptr<SharedObjectsFactory> sof, int countSamples, float mult, float aspectRatio ) : 

m_dofHor (new EffectDOFImportanceHor(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), countSamples, mult)), 
	m_dofVert(new EffectDOFImportanceVert(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture(), countSamples, mult * aspectRatio)),
	EffectDOF(sof->getFactoryRGBTexture(), sof->getFactoryRedTexture())
{
	m_actualAspectRatio = aspectRatio;
	m_mult = mult;
	m_countSamples = countSamples;
}

EffectDOFImportance::EffectDOFImportance(
	Ptr<GLTexturesRGBFactory> factoryRGB, Ptr<GLTexturesRedFactory> factoryRed, 
	int countSamples, float mult, float aspectRatio ) : 

	m_dofHor (new EffectDOFImportanceHor(factoryRGB, factoryRed, countSamples, mult)), 
	m_dofVert(new EffectDOFImportanceVert(factoryRGB, factoryRed, countSamples, mult * aspectRatio)),
	EffectDOF(factoryRGB, factoryRed)
{
	m_actualAspectRatio = aspectRatio;
	m_mult = mult;
	m_countSamples = countSamples;
}

Ptr<GLTexture2D> EffectDOFImportance::process( Ptr<GLTexture2D> tex, Ptr<GLTexture2D> depth, Ptr<GLTextureEnvMap> envMap)
{
	float previousAspectRatio = m_actualAspectRatio;
	m_actualAspectRatio = tex->getResolution().y / (float)tex->getResolution().x;
	if(m_actualAspectRatio != previousAspectRatio)
		setAspectRatio(m_actualAspectRatio);
	tex = m_dofHor->process(tex, depth, envMap);
	tex = m_dofVert->process(tex, depth, envMap);
	return tex;
}

void EffectDOFImportance::setFuncCoC( FuncFtoFCompCoC& func )
{
	m_dofHor->setFuncCoC(func);
	m_dofVert->setFuncCoC(func);
}

void EffectDOFImportance::setAspectRatio( float aspectRatio )
{
	m_dofVert->modifyOffsets(m_countSamples, m_mult*aspectRatio);
}

bool EffectDOFImportance::requireDepth()
{
	return true;
}
