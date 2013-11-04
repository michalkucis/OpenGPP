#include "EffectGL.h"
#include "InputImplOpenCV.h"

Ptr<GLTexture2D> EffectGL::loadTextureFromFile (string filename)
{
	try {
		InputLoadFromSingleFileOpenCV input (filename);
		return input.getProcessed();
	} catch (Error&) {}
	try {
		InputLoadFromSingleFileOpenEXR input (filename);
		return input.getProcessed();
	} catch (Error&) {}
	error1 (ERR_NOT_FOUND, "Image file '%s' cannot be loaded", filename.c_str());
}