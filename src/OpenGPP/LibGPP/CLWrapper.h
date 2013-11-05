#pragma once

#include <gl\glew.h>
#include "Error.h"
#include "Base.h"
#define __CL_ENABLE_EXCEPTIONS
#include <CL\OpenCL.h>
#include "..\LibOpenCLWrapper\LibOpenCLWrapper.h"
#include "GLClasses.h"


const char* getCLErrorString(cl_int _err);

#define catchCLError catch (cl::Error error) {error3 (ERR_OPENCL, "Error in (%s): %d(%s)", error.what(),error.err(),getCLErrorString(error.err()));}

void printCLInfo ();
cl::CommandQueue getCLCommandQueue (cl::Context context);
cl::Context getCLGLContext ();
cl::Context getCLContext (int nPlatform, int deviceType = CL_CONTEXT_PLATFORM);
cl::Kernel createKernel (cl::Context context, string filename, string kernelName);
cl::Image2DGL getImage2DGL (cl::Context context, cl_mem_flags flags, Ptr<GLTexture2D> tex);

#ifdef CL_VERSION_1_2
cl::ImageGL getImageGL (cl::Context context, cl_mem_flags flags, Ptr<GLTexture2D> tex);
#endif