#include "MemoryLeakDetector.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>


MemoryLeakDetector::MemoryLeakDetector()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
}

void MemoryLeakDetector::setBreakpoint( int nIndex )
{
	_crtBreakAlloc = nIndex;
}