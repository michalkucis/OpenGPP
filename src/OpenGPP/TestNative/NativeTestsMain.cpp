#include "TestingFramework.h"
#include <windows.h>

void main (int, char** argv)
{
	performTests ();
	MessageBoxA (NULL, "All tests passed", "Result", MB_OK|MB_ICONINFORMATION);
}