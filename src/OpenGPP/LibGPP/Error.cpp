#include "Error.h"
#include <assert.h>
#include <sstream>
#include <iostream>
#include <signal.h>


Error::Error( error_t err, string desc, string file, string line ) :
m_error(err), m_desc(desc), m_filename (file), m_fileline (line)
{
	std::cerr << "";
}


string Error::getDesc()
{
	std::stringstream ss;
	ss << m_filename << "(" << m_fileline << "): " 
		<< "[" << m_error << ": " << getErrorDesc (m_error) << "] " << m_desc;
	return ss.str();
}

Error& Error::operator=( Error& e )
{
	m_error = e.m_error;
	m_desc = e.m_desc;
	m_filename = e.m_filename;
	m_fileline = e.m_fileline;
	return *this;
}
