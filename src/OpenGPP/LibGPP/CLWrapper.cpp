#include "CLWrapper.h"

#include <iostream>
#include <fstream>
#include "Ptr.h"
#include "GLClasses.h"

const char* getCLErrorString(cl_int _err) 
{
	switch (_err) {
	case CL_SUCCESS:                          return "Success!";
	case CL_DEVICE_NOT_FOUND:                 return "Device not found.";
	case CL_DEVICE_NOT_AVAILABLE:             return "Device not available";
	case CL_COMPILER_NOT_AVAILABLE:           return "Compiler not available";
	case CL_MEM_OBJECT_ALLOCATION_FAILURE:    return "Memory object allocation failure";
	case CL_OUT_OF_RESOURCES:                 return "Out of resources";
	case CL_OUT_OF_HOST_MEMORY:               return "Out of host memory";
	case CL_PROFILING_INFO_NOT_AVAILABLE:     return "Profiling information not available";
	case CL_MEM_COPY_OVERLAP:                 return "Memory copy overlap";
	case CL_IMAGE_FORMAT_MISMATCH:            return "Image format mismatch";
	case CL_IMAGE_FORMAT_NOT_SUPPORTED:       return "Image format not supported";
	case CL_BUILD_PROGRAM_FAILURE:            return "Program build failure";
	case CL_MAP_FAILURE:                      return "Map failure";
	case CL_INVALID_VALUE:                    return "Invalid value";
	case CL_INVALID_DEVICE_TYPE:              return "Invalid device type";
	case CL_INVALID_PLATFORM:                 return "Invalid platform";
	case CL_INVALID_DEVICE:                   return "Invalid device";
	case CL_INVALID_CONTEXT:                  return "Invalid context";
	case CL_INVALID_QUEUE_PROPERTIES:         return "Invalid queue properties";
	case CL_INVALID_COMMAND_QUEUE:            return "Invalid command queue";
	case CL_INVALID_HOST_PTR:                 return "Invalid host pointer";
	case CL_INVALID_MEM_OBJECT:               return "Invalid memory object";
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:  return "Invalid image format descriptor";
	case CL_INVALID_IMAGE_SIZE:               return "Invalid image size";
	case CL_INVALID_SAMPLER:                  return "Invalid sampler";
	case CL_INVALID_BINARY:                   return "Invalid binary";
	case CL_INVALID_BUILD_OPTIONS:            return "Invalid build options";
	case CL_INVALID_PROGRAM:                  return "Invalid program";
	case CL_INVALID_PROGRAM_EXECUTABLE:       return "Invalid program executable";
	case CL_INVALID_KERNEL_NAME:              return "Invalid kernel name";
	case CL_INVALID_KERNEL_DEFINITION:        return "Invalid kernel definition";
	case CL_INVALID_KERNEL:                   return "Invalid kernel";
	case CL_INVALID_ARG_INDEX:                return "Invalid argument index";
	case CL_INVALID_ARG_VALUE:                return "Invalid argument value";
	case CL_INVALID_ARG_SIZE:                 return "Invalid argument size";
	case CL_INVALID_KERNEL_ARGS:              return "Invalid kernel arguments";
	case CL_INVALID_WORK_DIMENSION:           return "Invalid work dimension";
	case CL_INVALID_WORK_GROUP_SIZE:          return "Invalid work group size";
	case CL_INVALID_WORK_ITEM_SIZE:           return "Invalid work item size";
	case CL_INVALID_GLOBAL_OFFSET:            return "Invalid global offset";
	case CL_INVALID_EVENT_WAIT_LIST:          return "Invalid event wait list";
	case CL_INVALID_EVENT:                    return "Invalid event";
	case CL_INVALID_OPERATION:                return "Invalid operation";
	case CL_INVALID_GL_OBJECT:                return "Invalid OpenGL object";
	case CL_INVALID_BUFFER_SIZE:              return "Invalid buffer size";
	case CL_INVALID_MIP_LEVEL:                return "Invalid mip-map level";
	default:                                  return "Unknown";
	}
} 

#define catchCLError catch (cl::Error error) {error3 (ERR_OPENCL, "Error in (%s): %d(%s)", error.what(),error.err(),getCLErrorString(error.err()));}

void printCLInfo ()
{
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	for (uint i = 0; i < platforms.size(); i++)
	{
		cl::Platform platform = platforms[i];
		std::cout << "CL platform index:     " << int2str(i) << std::endl;
		std::cout << "CL platform profile:   " << platform.getInfo<CL_PLATFORM_PROFILE>() << std::endl;
		std::cout << "CL platform vendor:    " << platform.getInfo<CL_PLATFORM_VENDOR>() << std::endl;
		std::cout << "CL platform name:      " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
		std::cout << "CL platform version:   " << platform.getInfo<CL_PLATFORM_VERSION>() << std::endl;
		std::cout << "CL platform extensions:" << platform.getInfo<CL_PLATFORM_EXTENSIONS>() << std::endl;
		std::cout << std::endl;
		std::vector<cl::Device> devices;
		cl_int err = platform.getDevices (CL_DEVICE_TYPE_ALL, &devices);
		if (err != CL_SUCCESS)
			error0 (ERR_OPENCL, "cl::Platform::getDevices");
		for (uint j = 0; j < devices.size(); j++)
		{
			cl::Device device = devices[j];
			std::cout << "CL platform index:  " << int2str(i) << std::endl;
			std::cout << "CL device index:    " << int2str(j) << std::endl;
			std::cout << "CL device vendor:   " << device.getInfo<CL_DEVICE_VENDOR>() << std::endl;
			std::cout << "CL device name:     " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
			std::cout << "CL device version:  " << device.getInfo<CL_DEVICE_VERSION>() << std::endl;
			std::cout << "CL device profile:  " << device.getInfo<CL_DEVICE_PROFILE>() << std::endl;
			std::cout << "CL device extensions:" << device.getInfo<CL_DEVICE_EXTENSIONS>() << std::endl;
			std::cout << std::endl;
		}
	}
}

cl::CommandQueue getCLCommandQueue (cl::Context context)
{
	try {
		cl_int err = CL_SUCCESS;
		std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
		cl::CommandQueue queue(context, devices[0], 0, &err);			
		if (err != CL_SUCCESS)
			throw cl::Error(err, "cl::CommandQueue");
		return queue;
	}catchCLError;
}

cl::Context getCLGLContext ()
{
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	if (platforms.size() == 0) 
		error0 (ERR_OPENCL, "Platform size 0\n");

	for (uint i = 0; i < platforms.size(); i++)
	{
		cl::Platform platform = platforms[i];
		cl_context_properties properties[] = 
		{
			CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(), 
			CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(), 
			CL_CONTEXT_PLATFORM, (cl_context_properties) platform(), 
			0
		};

		try 
		{
			cl::Context context(CL_DEVICE_TYPE_GPU, properties); 
			std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

			return context;
		} catch (cl::Error) {}
	}
	error0(ERR_OPENCL, "Compatible OpenCL context cannot be found");
}

cl::Context getCLContext (int nPlatform, int deviceType)
{
	try 
	{
		cl_int err = CL_SUCCESS;
		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		if (platforms.size()) 
		{
			cl_context_properties properties[] = 
			{ CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[nPlatform])(), 0};
			cl::Context context(deviceType, properties);
			return context;
		}
		error0(ERR_OPENCL, "No OpenCL platform cannot be found");
	}catchCLError;
}

cl::Kernel createKernel (cl::Context context, string filename, string kernelName)
{
	try
	{
		string sSource;
		{
			std::ifstream fs(filename);
			if (! fs.is_open())
				error1(ERR_STRUCT, "File '%s' cannot be opened", filename.c_str());
			fs.seekg(0, std::ios::end);   
			sSource.reserve((uint)fs.tellg());
			fs.seekg(0, std::ios::beg);

			sSource.assign((std::istreambuf_iterator<char>(fs)),
				std::istreambuf_iterator<char>());
		}

		cl::Program::Sources source(1, std::make_pair(sSource.c_str(), sSource.length()));
		cl::Program program = cl::Program(context, source);
		std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

		string buildOptions = "-cl-mad-enable -Werror";
		cl_int err = clBuildProgram(program(), devices.size(), (cl_device_id*)&devices.front(),
			buildOptions.c_str(), NULL, NULL);
		if (err == CL_BUILD_PROGRAM_FAILURE)
		{	
			char* szLog;
			size_t logSize;
			clGetProgramBuildInfo(program(), devices[0](), CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
			szLog = new char[logSize+1];
			clGetProgramBuildInfo(program(), devices[0](), CL_PROGRAM_BUILD_LOG, logSize, szLog, NULL);
			szLog[logSize] = '\0';
			string log = szLog;
			delete[] (szLog);
			error2(ERR_OPENCL, "OpenCL program '%s' cannot be compiled. Desc.: %s", filename.c_str(),log.c_str());
		}
		else if(err != CL_SUCCESS)
			cl::detail::errHandler(err, "clBuildProgram");

		cl::Kernel kernel(program, kernelName.c_str());

		return kernel;
	}catchCLError;
}

cl::Image2DGL getImage2DGL (cl::Context context, cl_mem_flags flags, Ptr<GLTexture2D> tex)
{
	try
	{
		cl_int error = CL_SUCCESS;

		cl::Image2DGL image (context, flags, GL_TEXTURE_2D, 0, tex->getGLuint(), &error);
		if (error != CL_SUCCESS)
			error0 (ERR_OPENCL, "cl:Image2DGL");
		return image;
	}catchCLError;
}

#ifdef CL_VERSION_1_2
cl::ImageGL getImageGL (cl::Context context, cl_mem_flags flags, Ptr<GLTexture2D> tex)
{
	try {		
		cl_int error = CL_SUCCESS;

		cl::ImageGL imageGL (context, flags, GL_TEXTURE_2D, 0, *tex, &error);
		if (error != CL_SUCCESS)
			error0 (ERR_OPENCL, "cl::ImageGL");
		return imageGL;
	}catchCLError;
}
#endif