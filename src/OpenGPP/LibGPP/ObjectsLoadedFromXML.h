#pragma once

#include <sstream>

#include "Base.h"
#include "XML.h"
#include "Input.h"
#include "InputImpl.h"
#include "InputImplOpenCV.h"
#include "Effect.h"
#include "EffectImpl.h"
#include "EffectImplOpenCV.h"
#include "EffectGL.h"
#include "EffectGLImpl.h"
#include "EffectCL.h"
#include "EffectCLImpl.h"
#include "PostProcessor.h"
#include "FuncImpl.h"



class ObjectsLoadedFromXML
{
	Ptr<GLTexturesRGBFactory> m_factoryRGB;
	Ptr<GLTexturesRedFactory> m_factoryRed;

	void getParamsCommand(Ptr<XMLNode> nodeRoot, string commandName,
		string& out_strInput, string& out_strEffects, int& out_nCycles, uint2& out_ppResolution)
	{
		Ptr<XMLNode> nodeCommand = nodeRoot->getNode("command", "name", commandName);
		out_strInput = nodeCommand->getAttrString("input");
		out_strEffects = nodeCommand->getAttrString("effects");
		out_nCycles = nodeCommand->getAttrInt("cycles");
		{
			Ptr<XMLNode> nodeProcessing = nodeCommand->getNode("processing");
			out_ppResolution.x = nodeProcessing->getAttrInt("sizeX");
			out_ppResolution.y = nodeProcessing->getAttrInt("sizeY");
		}
		
		//Ptr<XMLNode> nodeWindow = nodeCommand->getNode("window");
		//out_nWindowSizeX = nodeWindow->getAttrInt("sizeX");
		//out_nWindowSizeY = nodeWindow->getAttrInt("sizeY");		
	}

	void getParamsMap(Ptr<XMLNode> nodeMap,	int2& out_resolution, InputOpenCV::interp_t& out_interp)
	{
		out_resolution = int2(0, 0);
		out_interp = InputOpenCV::NEAREST;
		if (nodeMap->existAttr("sizeX") 
			|| nodeMap->existAttr("sizeY")
			|| nodeMap->existAttr("interp"))
		{
			out_resolution.x = nodeMap->getAttrInt("sizeX");
			out_resolution.y = nodeMap->getAttrInt("sizeY");
			string interpMethod = nodeMap->getAttrString("interp");
			if (interpMethod == "nearest")
				out_interp = InputOpenCV::NEAREST;
			else if (interpMethod == "linear")
				out_interp = InputOpenCV::LINEAR;
			else if (interpMethod == "cubic")
				out_interp = InputOpenCV::CUBIC;
			else if (interpMethod == "area")
				out_interp = InputOpenCV::AREA;
			else if (interpMethod == "cubic")
				out_interp = InputOpenCV::CUBIC;
			else
				error0(ERR_XML, "Unkown interpolation method");
		}
	}

	void getParamsMap(Ptr<XMLNode> nodeMap, string& out_filepath,
		int2& out_resolution, InputOpenCV::interp_t& out_interp)
	{
		out_filepath = nodeMap->getAttrString("filepath");
		getParamsMap(nodeMap, out_resolution, out_interp);
	}

	Ptr<Input> getInputSingle(Ptr<XMLNode> nodeInput)
	{
		string colorMapFilepath;
		int2 colorMapRes;
		InputOpenCV::interp_t colorMapInterp;
		getParamsMap(nodeInput->getNode("colorMap"),
			colorMapFilepath, colorMapRes, colorMapInterp);

		Error errOpenEXREnvMap;
		try {
			string envMapFilepath;
			int2 envMapRes;
			InputOpenCV::interp_t envMapInterp;
			Ptr<XMLNode> nodeEnvMap = nodeInput->getNode("envMap");

			getParamsMap(nodeEnvMap, envMapFilepath, envMapRes, envMapInterp);

			return new InputLoadFromSingleFileWithEnvMapOpenEXR
				(colorMapFilepath, colorMapRes, colorMapInterp,
				envMapFilepath, envMapRes, envMapInterp);
		} catch (Error& err) {
			errOpenEXREnvMap = err;
		}

		Error errOpenCV;
		try {
			return new InputLoadFromSingleFileOpenCV(
				colorMapFilepath, colorMapRes, colorMapInterp); 
		} catch (Error& err) {
			errOpenCV = err;
		}
		Error errOpenEXR;
		try {
			return new InputLoadFromSingleFileOpenEXR(
				colorMapFilepath, colorMapRes, colorMapInterp);
		} catch (Error& err) {
			errOpenEXR = err;
		}
		error3(ERR_XML, "The input cannot be loaded: \nOpenEXR with environment: %s\nOpenEXR: %s\nOpenCV: %s",
			errOpenEXREnvMap.getDesc(), errOpenEXR.getDesc(), errOpenCV.getDesc());
	}

	Ptr<Input> getInputVideo(Ptr<XMLNode> nodeInput)
	{
		string type = nodeInput->getAttrString("type");
		Ptr<XMLNode> nodeColorMap = nodeInput->getNode("colorMap");
		if (type == "videoFile")
		{
			string colorMapFilepath;
			int2 colorMapRes;
			InputOpenCV::interp_t colorMapInterp;
			getParamsMap(nodeColorMap, colorMapFilepath, colorMapRes, colorMapInterp);
			return new InputLoadFromVideoFileOpenCV(colorMapFilepath, colorMapRes, colorMapInterp);
		}
		else if (type == "videoDevice")
		{
			int deviceNum = nodeColorMap->getAttrInt("deviceNum");
			int2 colorMapRes;
			InputOpenCV::interp_t colorMapInterp;
			getParamsMap(nodeColorMap, colorMapRes, colorMapInterp);
			return new InputLoadFromVideoDeviceOpenCV(deviceNum, colorMapRes, colorMapInterp);
		}
		else
			error0(ERR_XML, "Unknown input type");
	}

	GeneratorOfInputSequenceFilenames getGeneratorOfInputSeqFilenames(Ptr<XMLNode> nodeSequence, string format)
	{
		int seqStart = nodeSequence->getAttrInt("start");
		int seqEnd = nodeSequence->getAttrInt("end");
		int seqStep = nodeSequence->getAttrInt("step");
		int seqRepeatStep = nodeSequence->getAttrInt("repeatStep");
		GeneratorOfInputSequenceFilenames generator(format,
			seqStart, seqEnd, seqStep, seqRepeatStep);
		return generator;
	}

	GeneratorOfOutputSequenceFilenames getGeneratorOfOutputSeqFilenames(Ptr<XMLNode> nodeSequence, string format)
	{
		int seqStart = nodeSequence->getAttrInt("start");
		int seqStep = nodeSequence->getAttrInt("step");
		GeneratorOfOutputSequenceFilenames generator(format,
			seqStart, seqStep);
		return generator;
	}

	Ptr<Input> getInputSequence(Ptr<XMLNode> nodeInput)
	{
		string colorMapFilepath;
		int2 colorMapRes;
		InputOpenCV::interp_t colorMapInterp;
		getParamsMap(nodeInput->getNode("colorMap"),
			colorMapFilepath, colorMapRes, colorMapInterp);

		Error errOpenEXREnvMap;
		try {
			string envMapFilepath;
			int2 envMapRes;
			InputOpenCV::interp_t envMapInterp;
			Ptr<XMLNode> nodeEnvMap = nodeInput->getNode("envMap");

			getParamsMap(nodeEnvMap, envMapFilepath, envMapRes, envMapInterp);

			Ptr<XMLNode> nodeSequence = nodeInput->getNode("sequence");
			GeneratorOfInputSequenceFilenames generatorColorMap = getGeneratorOfInputSeqFilenames
				(nodeSequence, colorMapFilepath); 
			GeneratorOfInputSequenceFilenames generatorEnvMap = getGeneratorOfInputSeqFilenames
				(nodeSequence, envMapFilepath);

			return new InputLoadFromSequenceWithEnvMap<InputLoadFromSingleFileWithEnvMapOpenEXR> (
				generatorColorMap, colorMapRes, colorMapInterp,
				generatorEnvMap, envMapRes, envMapInterp);
		} catch (Error& err) {
			errOpenEXREnvMap = err;
		}

		Error errOpenCV;
		try {
			Ptr<XMLNode> nodeSequence = nodeInput->getNode("sequence");
			GeneratorOfInputSequenceFilenames generatorColorMap = getGeneratorOfInputSeqFilenames
				(nodeSequence, colorMapFilepath); 
			return new InputLoadFromSequence<InputLoadFromSingleFileOpenCV>(
				generatorColorMap, colorMapRes, colorMapInterp); 
		} catch (Error& err) {
			errOpenCV = err;
		}
		Error errOpenEXR;
		try {
			Ptr<XMLNode> nodeSequence = nodeInput->getNode("sequence");
			GeneratorOfInputSequenceFilenames generatorColorMap = getGeneratorOfInputSeqFilenames
				(nodeSequence, colorMapFilepath); 
			return new InputLoadFromSequence<InputLoadFromSingleFileOpenEXR>(
				generatorColorMap, colorMapRes, colorMapInterp);
		} catch (Error& err) {
			errOpenEXR = err;
		}
		error3(ERR_XML, "The input cannot be loaded: \nOpenEXR with environment: %s\nOpenEXR: %s\nOpenCV: %s",
			errOpenEXREnvMap.getDesc(), errOpenEXR.getDesc(), errOpenCV.getDesc());

	}

	Ptr<Input> getInput(Ptr<XMLNode> nodeRoot, string inputName)
	{
		Ptr<XMLNode> nodeInput = nodeRoot->getNode("input", "name", inputName);
		string type = nodeInput->getAttrString("type");
		if (type == "single")
			return getInputSingle(nodeInput);
		else if (type == "videoFile" || type == "videoDevice")
			return getInputVideo(nodeInput);
		else if (type == "sequence")
			return getInputSequence(nodeInput);
		else
			error1(ERR_XML, "Uknown type '%s' of the input", type.c_str());
		return NULL;
	}

	Ptr<Effect> getEffectOutputScreen(Ptr<XMLNode> effect)
	{
		Ptr<XMLNode> nodeScreen = effect->getNode("screen");
		float x = nodeScreen->getAttrFloat("x");
		float y = nodeScreen->getAttrFloat("y");
		float sizeX = nodeScreen->getAttrFloat("sizeX");
		float sizeY = nodeScreen->getAttrFloat("sizeY");
		return new EffectRenderToScreen(x, y, sizeX, sizeY);
	}

	Ptr<Effect> getEffectDepthOutputScreen(Ptr<XMLNode> effect)
	{
		Ptr<XMLNode> nodeScreen = effect->getNode("screen");
		float x = nodeScreen->getAttrFloat("x");
		float y = nodeScreen->getAttrFloat("y");
		float sizeX = nodeScreen->getAttrFloat("sizeX");
		float sizeY = nodeScreen->getAttrFloat("sizeY");
		return new EffectRenderDepthToScreen(x, y, sizeX, sizeY);
	}

	Ptr<Effect> getEffectOutputFile(Ptr<XMLNode> effect)
	{
		string type = effect->getAttrString("type");
		if(type=="video")
		{
			Ptr<XMLNode> nodeVideo = effect->getNode("video");
			string filename = nodeVideo->getAttrString("filename");
			string fourcc = nodeVideo->getAttrString("fourcc");
			float fps = nodeVideo->getAttrFloat("fps");
			int repeatFrame = nodeVideo->getAttrInt("repeatFrame");
			string library = effect->getNode("library")->getAttrString("name");
			if (library == "opencv")
				return new EffectSaveToVideoFileOpenCV(filename, fps, repeatFrame, fourcc);
			else
				error0(ERR_XML, "It is set unknown saving library of video");
		}
		else if(type=="imageLDR")
		{	
			string filename = effect->getNode("image")->getAttrString("filename");
			return new EffectSaveToSingleFileOpenCV(filename);
		}
		else if(type=="imageHDR")
		{	
			string filename = effect->getNode("image")->getAttrString("filename");
			return new EffectSaveToSingleFileOpenEXR(filename);
		}
		else if(type=="sequenceLDR")
		{	
			string filename = effect->getNode("image")->getAttrString("filename");
			Ptr<XMLNode> nodeSeq = effect->getNode("sequence");
			GeneratorOfOutputSequenceFilenames generator =
				getGeneratorOfOutputSeqFilenames(nodeSeq, filename);
			return new EffectSaveToSequence<EffectSaveToSingleFileOpenCV>(generator);
		}
		else if(type=="sequenceHDR")
		{	
			string filename = effect->getNode("image")->getAttrString("filename");
			Ptr<XMLNode> nodeSeq = effect->getNode("sequence");
			GeneratorOfOutputSequenceFilenames generator =
				getGeneratorOfOutputSeqFilenames(nodeSeq, filename);
			return new EffectSaveToSequence<EffectSaveToSingleFileOpenEXR>(generator);
		}
		else
			error0(ERR_XML, "Unknown type of the outputFile");
	}

	Ptr<FuncFtoF> getFuncFtoF(Ptr<XMLNode> nodeFunc)
	{
		string type = nodeFunc->getAttrString("type");
		if(type == "polynomial")
			return new FuncFtoFPolynomial(nodeFunc);
		else
			error0(ERR_XML, "The type of the function is not known");
	}	

	Ptr<EffectCLObjectsFactory> m_factoryCLobjects;
	
	Ptr<EffectCLObjectsFactory> getCLObjectsFactory()
	{
		if (m_factoryCLobjects.isNull())
			m_factoryCLobjects = new EffectCLObjectsFactory;
		return m_factoryCLobjects;
	}

	Ptr<Effect> getEffectDistortion(Ptr<XMLNode> nodeDist)
	{
		string type = nodeDist->getAttrString("type");
		if(type=="grid")
		{
			Ptr<XMLNode> nodeGrid = nodeDist->getNode("grid");
			int2 count (nodeGrid->getAttrInt("countX"), nodeGrid->getAttrInt("countY"));
			int totalCount = count.x*count.y;
			Vector<Ptr<XMLNode>> vecNodes = nodeGrid->getVecNodes("point2");
			if (totalCount != vecNodes.getSize())
				error0(ERR_XML, "In the distortion, the countX*countY differs with the count of nodes 'point2'");
			
			Vector<float2> vecPoints;
			for(int i = 0; i < totalCount; i++)
			{
				Ptr<XMLNode> node = vecNodes[i];
				if (node->getTag()!="point2")
					error0(ERR_XML, "The tag in the grid is invalid");
				float2 point2 (node->getAttrFloat("x"), node->getAttrFloat("y"));
				vecPoints.pushBack(point2);
			}
			
			Matrix<float2> matrix (uint2(count.x, count.y));
			int i = 0;
			for(int y = 0; y < count.y; y++)
				for(int x  = 0; x < count.x; x++)
				{
					uint2 pos(x, y);
					matrix[pos] = vecPoints[i++] * 2 - 1;
				}
			return new EffectDistortionGrid(m_factoryRGB, m_factoryRed, matrix);
		}
		else if(type=="radialFunc")
		{
			Ptr<XMLNode> nodeFunc = nodeDist->getNode("func");
			Ptr<FuncFtoF> func = getFuncFtoF(nodeFunc);
			uint2 resolution (128, 128);
			return new EffectDistortionGrid(m_factoryRGB, m_factoryRed, resolution, func);
		}
		else
			error0(ERR_XML, "Unknown type of the distortion");
	}

	Ptr<Effect> getChromaticAberration(Ptr<XMLNode> nodeChromAber)
	{
		string type = nodeChromAber->getAttrString("type");
		if(type=="radialFunc")
		{
			Ptr<XMLNode> nodeRed = nodeChromAber->getNode("redToCyan");
			Ptr<XMLNode> nodeBlue = nodeChromAber->getNode("blueToYellow");
			Ptr<FuncFtoF> funcRed = new FuncFtoFPolynomial(nodeRed->getNode("func"));
			Ptr<FuncFtoF> funcBlue = new FuncFtoFPolynomial(nodeBlue->getNode("func"));

			uint2 resolution (128, 128);
			EffectChromaticAberration* effect = new EffectChromaticAberration(m_factoryRGB, m_factoryRed);
			effect->setGrid(0, resolution, funcRed);
			effect->setGrid(2, resolution, funcBlue);
			return effect;
		}
		else
			error0(ERR_XML, "Unknown type of the chromatic aberration");
	}

	Ptr<Effect> getVignetting(Ptr<XMLNode> nodeVignetting)
	{
		string type = nodeVignetting->getAttrString("type");
		if(type == "func")
		{
			Ptr<XMLNode> nodeFunc = nodeVignetting->getNode("func");
			Ptr<FuncFtoF> func = getFuncFtoF(nodeFunc);
			return new EffectVignettingFunc(m_factoryRGB, m_factoryRed, func);
		}
		else if(type == "filemask")
		{
			Ptr<XMLNode> nodeFilemask = nodeVignetting->getNode("filemask");
			string filename = nodeFilemask->getAttrString("filepath");
			return new EffectVignettingImageFile(m_factoryRGB, m_factoryRed, filename, false);
		}
		else
			error0(ERR_XML, "Unknown type of the vignetting");
	}

	Ptr<Effect> getDepthOfField(Ptr<XMLNode> node)
	{
		string type = node->getAttrString("type");
		Ptr<XMLNode> params = node->getNode("dofParams");
		float focusDistance = params->getAttrFloat("focusDistance");
		float fnumber = params->getAttrFloat("fnumber");
		float focalLength = params->getAttrFloat("focalLength");
		float senzorSize = params->getAttrFloat("sensorSize");
		float inputDepthMult = params->getAttrFloat("inputDepthMult");
		if(type == "showCoC")
		{
			float outMult = node->getNode("showcocParams")->getAttrFloat("outMult");
			FuncFtoFCompCoC func(focusDistance, fnumber, 
				focalLength, senzorSize, inputDepthMult, outMult);
			EffectDOFShowCoC* effect = new EffectDOFShowCoC(m_factoryRGB, m_factoryRed);
			effect->setFuncCoC(func);
			return effect;
		}
		else if(type == "importance")
		{
			Ptr<XMLNode> params = node->getNode("importanceParams");
			int numSamples = params->getAttrInt("numSamples");
			float cocRadiusMult = params->getAttrFloat("cocRadiusMult");
			FuncFtoFCompCoC func(focusDistance, fnumber, focalLength, 
				senzorSize, inputDepthMult, cocRadiusMult);
			EffectDOFImportance* effect = new EffectDOFImportance(m_factoryRGB, m_factoryRed, numSamples, 1.0f, 1.0f);
			effect->setFuncCoC(func);
			return effect;
		}
		else if(type == "regular")
		{
			Ptr<XMLNode> params = node->getNode("regularParams");
			int numSamples = params->getAttrInt("numSamples");
			float cocRadiusMult = params->getAttrFloat("cocRadiusMult");
			FuncFtoFCompCoC func(focusDistance, fnumber, focalLength, 
				senzorSize, inputDepthMult, cocRadiusMult);
			EffectDOFRegular* effect = new EffectDOFRegular(m_factoryRGB, m_factoryRed, numSamples, 1.0f, 1.0f);
			effect->setFuncCoC(func);
			return effect;
		}
		else if(type == "diffusion")
		{
			FuncFtoFCompCoC func(focusDistance, fnumber, focalLength, 
				senzorSize, inputDepthMult, 1.0f);
			EffectCLDOFDistribution* effect = new EffectCLDOFDistribution(m_factoryRGB, m_factoryRed, 1.0f, getCLObjectsFactory());
			effect->setFuncCoC(func);
			return effect;
		}
		else
		{
			error0(ERR_XML, "Unknown type of depth of field effect");
		}
	}

	Ptr<Effect> getExposureAdaptation(Ptr<XMLNode> node)
	{
		string type = node->getAttrString("type");

		if(type == "pid")
		{
			Ptr<XMLNode> nodePID = node->getNode("pidParams");
			float targetValue = nodePID->getAttrFloat("targetValue");
			float valueP = nodePID->getAttrFloat("valueP");
			float valueI = nodePID->getAttrFloat("valueI");
			float valueD = nodePID->getAttrFloat("valueD");
			PIDController pid(targetValue, valueP, valueI, valueD);
			Ptr<Effect> effect = new EffectBrightnessAdapter(m_factoryRGB, m_factoryRed, pid);
			return effect;
		}
		else
			error0(ERR_XML, "Unknown type of depth of field effect");
	}

	Ptr<Effect> getMotionBlur(Ptr<XMLNode> node)
	{
		string type = node->getAttrString("type");

		Ptr<XMLNode> nodeDeltaPos(node->getNode("deltaPos"));
		float3 deltaPos(nodeDeltaPos->getAttrFloat("x"), 
			nodeDeltaPos->getAttrFloat("y"),
			nodeDeltaPos->getAttrFloat("z"));
		Ptr<XMLNode> nodeDeltaView(node->getNode("deltaView"));
		float2 deltaView(nodeDeltaView->getAttrFloat("x"), 
			nodeDeltaView->getAttrFloat("y"));
		if(type == "simple")
		{
			int samples = node->getNode("samples")->getAttrInt("count");
			return new EffectMotionBlurSimple(m_factoryRGB, m_factoryRed, deltaPos, deltaView, samples);
		}
		else if(type == "2phases")
		{
			Ptr<XMLNode> nodeSamples(node->getNode("samples"));
			int phase1st = nodeSamples->getAttrInt("phase1st");
			int phase2nd = nodeSamples->getAttrInt("phase2nd");
			return new EffectMotionBlur2Phases(m_factoryRGB, m_factoryRed, deltaPos, deltaView, phase1st, phase2nd);
		}
		else
			error0(ERR_XML, "Unknown type of depth of field effect");

		return NULL;
	}

	Ptr<Effect> getInnerLensReflection(Ptr<XMLNode> node)
	{
		string type = node->getAttrString("type");
		if(type == "simple")
		{
			Ptr<XMLNode> nodeILR = node->getNode("ilrMap");
			int width = nodeILR->getAttrInt("width");
			int height = nodeILR->getAttrInt("height");
			bool dynamic = nodeILR->getAttr("dynamic");

			EffectLensFlareStarFromSimple* effect = 
					new EffectLensFlareStarFromSimple(m_factoryRGB, m_factoryRed, width, height, dynamic, getCLObjectsFactory());

			float attenuation = node->getNode("attenuation")->getAttrFloat("value");
			effect->setLensFlareMult(1/attenuation);
			return effect;
		}
		else if(type == "simpleEnvMap")
		{
			Ptr<XMLNode> nodeILR = node->getNode("ilrMap");
			int width = nodeILR->getAttrInt("width");
			int height = nodeILR->getAttrInt("height");
			bool dynamic = nodeILR->getAttrBool("dynamic");

			EffectLensFlareStarFromEnvMap* effect = new EffectLensFlareStarFromEnvMap
					(m_factoryRGB, m_factoryRed, width, height, dynamic, getCLObjectsFactory());

			float attenuation = node->getNode("attenuation")->getAttrFloat("value");
			effect->setLensFlareMult(1/attenuation);
			return effect;
		}
		else
			error0(ERR_XML, "Unknown type of depth of field effect");

		return NULL;
	}

	Ptr<Effect> getNoise(Ptr<XMLNode> node)
	{
		string type = node->getAttrString("type");
		if(type == "uniform")
		{
			Ptr<XMLNode> nodeUniform = node->getNode("uniform");
			float begin = nodeUniform->getAttrFloat("begin");
			float end = nodeUniform->getAttrFloat("end");

			return new EffectNoiseUniform(m_factoryRGB, m_factoryRed, begin, end);
		}
		else if(type == "emva1288")
		{
			Ptr<XMLNode> nodeItoP = node->getNode("intensityToPhotons");
			float fItoP = nodeItoP->getAttrFloat("value");

			Ptr<XMLNode> nodeDNtoI = node->getNode("DNtoIntensity");
			float fDNtoI = nodeDNtoI->getAttrFloat("value");

			Ptr<XMLNode> nodeEmva = node->getNode("emva1288");
			float totalQuantumEfficiently = nodeEmva->getAttrFloat("totalQuantumEfficiently") / 100;
			float readNoise = nodeEmva->getAttrFloat("readNoise");
			float darkCurrent = nodeEmva->getAttrFloat("darkCurrent");
			float inverseOfOverallSystemGain = nodeEmva->getAttrFloat("inverseOfOverallSystemGain");
			int saturationCapacity = nodeEmva->getAttrInt("saturationCapacity");
			float expositionTime = node->getNode("exposition")->getAttrFloat("time");
			return new EffectNoiseEmva(m_factoryRGB, m_factoryRed, fItoP, totalQuantumEfficiently,
				readNoise, darkCurrent, inverseOfOverallSystemGain, saturationCapacity,
				expositionTime, fDNtoI);
		}
		else
			error0(ERR_XML, "Unknown type of depth of field effect");

		return NULL;
	}

	Ptr<Effect> getResizeEffect()
	{
		return new EffectResizeImages(m_factoryRGB, m_factoryRed);
	}

	Vector<Ptr<Effect>> getEffects(Ptr<XMLNode> nodeRoot, string effectTag, string effectName)
	{
		Vector<Ptr<Effect>> vecEffects;
		Ptr<XMLNode> effect = nodeRoot->getNode(effectTag, "name", effectName);
		if(effectTag == "outputScreen")
		{
			vecEffects.pushBack(getEffectOutputScreen(effect));
		}
		else if(effectTag == "outputDepthScreen")
		{
			vecEffects.pushBack(getEffectDepthOutputScreen(effect));
		}
		else if(effectTag == "outputFile")
		{
			vecEffects.pushBack(getResizeEffect());
			vecEffects.pushBack(getEffectOutputFile(effect));
		}
		else if(effectTag == "distortion")
		{
			vecEffects.pushBack(getEffectDistortion(effect));
		}
		else if(effectTag == "chromaticAberration")
		{
			vecEffects.pushBack(getChromaticAberration(effect));
		}
		else if(effectTag == "vignetting")
		{
			vecEffects.pushBack(getVignetting(effect));
		}
		else if(effectTag == "depthOfField")
		{
			if(effect->getAttrString("type") == "diffusion")
				vecEffects.pushBack(getResizeEffect());

			vecEffects.pushBack(getDepthOfField(effect));
		}
		else if(effectTag == "exposureAdaptation")
		{
			vecEffects.pushBack(getExposureAdaptation(effect));
		}
		else if(effectTag == "motionBlur")
		{
			vecEffects.pushBack(getMotionBlur(effect));
		}
		else if(effectTag == "innerLensReflections")
		{
			vecEffects.pushBack(getInnerLensReflection(effect));
		}
		else if(effectTag == "noise")
		{	
			vecEffects.pushBack(getNoise(effect));
		}
		else
			error0(ERR_XML, "Unknown effect tag");
		//TODO: parse effects
		//TODO: a processing size processes
		//
		return vecEffects;
	}

	Vector<Ptr<Effect>> getVecEffects(Ptr<XMLNode> nodeRoot, string effectsName)
	{
		bool resizedInput = false;
		Vector<Ptr<Effect>> vecEffects;

		Ptr<XMLNode> nodeEffects = nodeRoot->getNode("effects", "name", effectsName);
		Vector<Ptr<XMLNode>> effects = nodeEffects->getVecNodes();

		for(uint i = 0; i < effects.getSize(); i++)
		{
			Ptr<XMLNode> node = effects[i];

			string tag = node->getTag();
			string name = node->getAttrString("name");

			Vector<Ptr<Effect>> effects = getEffects(nodeRoot, tag, name);
			for(uint i = 0; i < effects.getSize(); i++)
				vecEffects.pushBack(effects[i]);
		}
		return vecEffects;
	}

	Ptr<PostProcessor> m_pp;
	int m_nCycles;
public:
	ObjectsLoadedFromXML(string filename, string commandName)
	{
		Ptr<XMLNode> nodeRoot = new XMLNode;
		nodeRoot->parseFile(filename);

		string strInput, strEffects;
		uint2 ppResolution;
		getParamsCommand(nodeRoot, commandName, strInput, strEffects, m_nCycles, ppResolution);

		m_factoryRGB = new GLTexturesRGBFactory(ppResolution);
		m_factoryRed = new GLTexturesRedFactory(ppResolution);

		m_pp = new PostProcessor(ppResolution);
		m_pp->m_input = getInput(nodeRoot, strInput);
		m_pp->m_vecEffects = getVecEffects(nodeRoot, strEffects);
	}
	Ptr<PostProcessor> getPostProcessor ()
	{
		return m_pp;
	}
	int getNumberCycles()
	{
		return m_nCycles;
	}
};


