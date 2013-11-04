#pragma once

#include "Base.h"
#include <exception>

enum error_t 
{
	ERR_NO_ERROR,
	ERR_OPENGL = 1,
	ERR_STRUCT,
	ERR_SDL,
	ERR_OPENCV,
	ERR_OPENCL,
	ERR_XML,
	ERR_NOT_FOUND,
	ERR_NOT_IMPLEMENTED,
	ERR_NOT_DEFINED = -1
};

class Error: public std::exception
{
public:
	string getErrorDesc (error_t err)
	{
		switch (err)
		{
		case ERR_OPENGL:
			return "OpenGL error";
		case ERR_STRUCT:
			return "Invalid access to structure";
		default:
			return "No description";
		}
	}
	error_t m_error;
	string m_desc;
	string m_filename;
	string m_fileline;

	Error (error_t err, string desc, string file, string line);

	Error() 
	{
		m_error = ERR_NO_ERROR;
		m_desc = "No error";
		m_filename = "Unknown - it's used no standard way to raise error";
		m_fileline = "Unknown - it's used no standard way to raise error";
	}

	Error& operator=(Error& e);

	string getDesc ();
};
#pragma warning (disable: 4996)
#define error(ERR) {throw Error (ERR, "", __FILE__, int2str(__LINE__));}
#define error0(ERR, DESC) {throw Error (ERR, (string) DESC, __FILE__, int2str(__LINE__));}
#define error1(ERR, DESC, PARAM1) {char* buffer = new char[1024*1024]; sprintf(buffer, DESC,PARAM1); string str = buffer; delete [] buffer; throw Error (ERR, str, __FILE__, int2str(__LINE__));}
#define error2(ERR, DESC, PARAM1, PARAM2) {char* buffer = new char[1024*1024]; sprintf(buffer, DESC,PARAM1,PARAM2); string str = buffer; delete [] buffer; throw Error (ERR, str, __FILE__, int2str(__LINE__));}
#define error3(ERR, DESC, PARAM1, PARAM2, PARAM3) {char* buffer = new char[1024*1024]; sprintf(buffer, DESC,PARAM1,PARAM2,PARAM3); string str = buffer; delete []  buffer; throw Error (ERR, str, __FILE__, int2str(__LINE__));}
