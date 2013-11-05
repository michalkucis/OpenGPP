#include "Debug.h"
#include "Base.h"
#include "ApplicationPP.h"
#include <iostream>

void runDebug(string filename, string command)
{
	std::cout << "Debugging '" << command << "' in '" << filename << "'... ";
	{
		ApplicationPP app(filename, command);
		app.run();
	}
	std::cout << "OK" << std::endl;
}

void debug()
{	
	std::cout << "  Testing inputs:" << std::endl; 
	runDebug("camsim.xml", "sequenceEnv");
	runDebug("camsim.xml", "sequenceEnvResized");
	runDebug("camsim.xml", "sequence");
	runDebug("camsim.xml", "sequenceResized");
	runDebug("camsim.xml", "videoDevice");
	runDebug("camsim.xml", "videoDeviceResized");
	runDebug("camsim.xml", "videoFile");
	runDebug("camsim.xml", "videoFileResized");
	runDebug("camsim.xml", "singleEXRResizedEnvMap");
	runDebug("camsim.xml", "singleEXREnvMap");
	runDebug("camsim.xml", "singleEXRResized");
	runDebug("camsim.xml", "singleEXR");
	runDebug("camsim.xml", "singleCVResized");
	runDebug("camsim.xml", "singleCV");


	std::cout << "  Testing outputs:" << std::endl; 
	runDebug("camsim.xml", "outColorToScreen");
	runDebug("camsim.xml", "outDepthToScreen");
	runDebug("camsim.xml", "outImageToJPG");
	runDebug("camsim.xml", "outImageToEXR");
	runDebug("camsim.xml", "outSequenceToJPG");
	runDebug("camsim.xml", "outSequenceToEXR");
	runDebug("camsim.xml", "outVideoToAVI");


	std::cout << "  Testing effects:" << std::endl; 
	runDebug("camsim.xml", "effectNoiseEmva");
	runDebug("camsim.xml", "effectNoiseUniform");
	runDebug("camsim.xml", "effectInnerLensReflectionsEnvMap");
	runDebug("camsim.xml", "effectInnerLensReflectionsSimple");
	runDebug("camsim.xml", "effectMotionBlur2Phases");
	runDebug("camsim.xml", "effectMotionBlurSimple");
	runDebug("camsim.xml", "effectExposureAdaptation");
	runDebug("camsim.xml", "effectDOFDiffusion");
	runDebug("camsim.xml", "effectDOFRegular");
	runDebug("camsim.xml", "effectDOFImportance");
	runDebug("camsim.xml", "effectDOFShowCoC");
	runDebug("camsim.xml", "effectDistortionGrid");
	runDebug("camsim.xml", "effectDistortionFuncPolynomial");
	runDebug("camsim.xml", "effectChromaticAberration");
	runDebug("camsim.xml", "effectVignettingImage");
	 



	std::cout << "All tests have been passed successfully\n";
	std::cout << "Press any key to continue...";
	std::cin.ignore(1);
}