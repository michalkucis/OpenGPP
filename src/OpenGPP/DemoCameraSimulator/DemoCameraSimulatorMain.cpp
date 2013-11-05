#include <iostream>
#include "Debug.h"
#include "ApplicationPP.h"



int main( int argc, char* argv[] )
{
	try
	{	
		if (argc == 1)
		{
			std::cout << "camsim.exe $XML_FILE $COMMAND" << std::endl << std::endl;
			std::cout << "example:" << std::endl;
			std::cout << "  camsim.exr debug" << std::endl;
			std::cout << "  camsim.exe \"camsim.xml\" video_with_dof" << std::endl;
		}
		else if (argc == 2 && (!strcmp(argv[1], "debug")))
		{
			debug();
		}
		else if (argc == 3)
		{
			ApplicationPP pp(argv[1], argv[2]); 
			pp.run();
		}
		else
			error0(ERR_STRUCT, "The command arguments are invalid");
	} catch (Error& ex) {
		std::string desc = ex.getDesc();
		printf ("%s\n", desc.c_str());
		std::cout << "Wait until any button to be pressed...";
		std::cin.ignore(1);	
	}

	return 0;
}
