//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		session.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements the Session class 
//
#include <cstdlib>
#include <cstdio>
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include "GL/glew.h"
#include "session.h"
#include "dvrparams.h"
#include <vapor/DataMgr.h>
#include "pythonpipeline.h"
#include <vapor/DataMgrWB.h>
#include <vapor/LayeredIO.h>
#include <vapor/DataMgrFactory.h>
#include <vapor/LayeredIO.h>
#include <vapor/ParamNode.h>

#include "vizwinmgr.h"
#include "mainform.h"
#include "command.h"
#include "messagereporter.h"
#include <QMdiArea>
#include <qapplication.h>
#include <qcursor.h>
#include <qaction.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include "histo.h"
#include <vapor/ImpExp.h>
#include <vapor/Version.h>
#include "animationcontroller.h"
#include "transferfunction.h"
#include "floweventrouter.h"
#include "userpreferences.h"
#include <qstring.h>
#include "tabmanager.h"


#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

using namespace VAPoR;
Session* Session::theSession = 0;
const string Session::_interactiveRefLevelAttr = "InteractiveRefinementLevel";
const string Session::_specifyTextureSizeAttr = "SpecifyTextureSize";
const string Session::_textureSizeAttr = "SpecifiedTextureSize";
const string Session::_stretchFactorsAttr = "StretchFactors";
const string Session::_cacheSizeAttr = "CacheSize";
const string Session::_jpegQualityAttr = "JpegQuality";
const string Session::_metadataPathAttr = "MetadataPath";
const string Session::_transferFunctionPathAttr = "TransferFunctionPath";
const string Session::_imageCapturePathAttr = "JpegPath";
const string Session::_flowDirectoryPathAttr = "FlowPath";
const string Session::_pythonDirectoryPathAttr = "PythonPath";
const string Session::_autoSaveIntervalAttr = "AutoSaveInterval";

const string Session::_logFileNameAttr = "LogFileName";
const string Session::_exportFileNameAttr = "ExportFileName";
const string Session::_maxPopupAttr = "MaxPopups";
const string Session::_maxLogAttr = "MaxLogMsgs";
const string Session::_dataExtentsAttr = "DataExtents";
const string Session::_sessionTag = "Session";
const string Session::_globalParameterPanelsTag = "GlobalParameterPanels";
const string Session::_globalTransferFunctionsTag = "GlobalTransferFunctions";
const string Session::_sessionVariableTag = "SessionVariable";
const string Session::_variableNameAttr = "VariableName";
const string Session::_aboveGridAttr = "AboveGridValue";
const string Session::_belowGridAttr = "BelowGridValue";
const string Session::_extendUpAttr = "ExtendAboveGrid";
const string Session::_extendDownAttr = "ExtendBelowGrid";
const string Session::_VAPORVersionAttr = "VaporVersion";

const string Session::_pythonScriptsTag = "PythonScripts";
const string Session::_setupScriptTag = "PythonSetupScript";
const string Session::_scriptNameAttr = "ScriptName";
const string Session::_pythonDerivedScriptTag = "DerivedVariableDefinition";
const string Session::_pythonProgramTag = "PythonProgram";
const string Session::_python2DInputsTag = "Python2DInputs";
const string Session::_python3DInputsTag = "Python3DInputs";
const string Session::_python2DOutputsTag = "Python2DOutputs";
const string Session::_python3DOutputsTag = "Python3DOutputs";

string Session::prefFile = "";
Session::Session() {

	//Initialize version to current version
	sessionVersionString = Version::GetVersionString();
	
	MyBase::SetDiagMsgCB(infoCallbackFcn);
	previousClass = 0;
	dataMgr = 0;
	
	stretchFactors[0] = stretchFactors[1] = stretchFactors[2] = 1.f;
	visualizeSpherically = false;
	
	sessionFilename = "VaporSaved.vss";
	
	
	//Note that the session will create the vizwinmgr!
	VizWinMgr::getInstance();
	
	//Setup command queue:
	//
	currentQueuePos = 0;
	startQueuePos = 0;
	endQueuePos = 0;
	for (int i = 0; i<MAX_HISTORY; i++){
		commandQueue[i] = 0;
	}
	
	DataStatus::clearMetadataVars();
	numTFs = 0;
	tfNames = 0;
	keptTFs = 0;
	tfListSize = 0;
	newSession = true;
	
	init();
}
Session::~Session(){
	int i;
	tempParsedTF = 0;
	tempParsedPanel = 0;
	//Before deleting the animation controller, stop the animation:
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	for (int i = 0; i< MAXVIZWINS; i++){
		if (vizMgr->getVizWin(i)){
			AnimationController::getInstance()->finishVisualizer(i);
			vizMgr->getVizWin(i)->setEnabled(false);
		}
	}
	delete AnimationController::getInstance();
	delete vizMgr;
	//Note: metadata is deleted by Datamgr
	if (dataMgr) delete dataMgr;
	DataStatus::clearActiveVars();
	
	for (i = startQueuePos; i<= endQueuePos; i++){
		if (commandQueue[i%MAX_HISTORY]) {
			delete commandQueue[i%MAX_HISTORY];
			commandQueue[i%MAX_HISTORY] = 0;
		}
	}
	//Must delete the histograms before the currentDataStatus:
	//Histo::releaseHistograms();
	if(DataStatus::getInstance()) delete DataStatus::getInstance();

	//Delete all the saved transfer functions:
	for (i = 0; i<numTFs; i++){
		delete keptTFs[i];
		delete tfNames[i];
	}
	if (tfListSize>0){
		delete [] keptTFs;
		delete [] tfNames;
		
	}
	
}
//Set session state to base values:
//
void Session::init() {
	int i;
	recordingCount = 0;
	metadataSaved = false;
	citationRemind = true;
	citationRemindDefault = true;

	
	stretchFactors[0] = stretchFactors[1] = stretchFactors[2] = 1.f;
	visualizeSpherically = false;
	currentExportFile = ImpExp::GetPath();
	//Delete all the saved transfer functions:
	for (i = 0; i<numTFs; i++){
		delete keptTFs[i];
		delete tfNames[i];
	}
	if (tfListSize>0){
		delete [] keptTFs;
		delete [] tfNames;
	}
	
	
	numTFs = 0;
	tfNames = 0;
	keptTFs = 0;
	tfListSize = 0;
	
	
	dataExists = false;
	
	newSession = true;
	extents[0] = extents[1] = extents[2] = 0.f;
	extents[3] = extents[4] = extents[5] = 1.f;
	stretchedExtents[0] = stretchedExtents[1] = stretchedExtents[2] = 0.f;
	stretchedExtents[3] = stretchedExtents[4] = stretchedExtents[5] = 1.f;
	
	DataStatus::removeMetadataVars();
	//Set current paths to default preferences
	setMetadataDirectory(preferenceMetadataDir.c_str());
	setTFFilePath(preferenceTFPath.c_str());
	setJpegDirectory(preferenceJpegDirectory.c_str());
	setFlowDirectory(preferenceFlowDirectory.c_str());
	setPythonDirectory(preferencePythonDirectory.c_str());
	setSessionDirectory(preferenceSessionDirectory.c_str());
	MainForm::getInstance()->setInteractiveRefinementSpin(0);
	
}
void Session::setDefaultPrefs(){
	cacheMB = 1024;
	textureSizeSpecified = false;
	textureSize = 0;
	//Set up default paths for log file and autosave file
	string str;
	string str1;
	string formerLogfileName = currentLogfileName;
#ifdef WIN32
	//Use the user name in the log file name
	char* tempDir = getenv("TEMP");
	if (!tempDir) tempDir = getenv("HOMEDRIVE");
	if (!tempDir) tempDir = "C:";
	char* defDir = getenv("HOME");
	if (!defDir) defDir = getenv("HOMEDRIVE");
	if (!defDir) defDir = "C:";
	const char* defaultDir = defDir;
	char buf2[50];
	
	DWORD size = 50;
	//Don't Use QT to convert from unicode back to toAscii
	//WNetGetUserA(0,(LPWSTR)buf2,&size);
	WNetGetUserA(0,(LPSTR)buf2,&size);
	str1 = string(tempDir)+"\\VaporAutosave."+string(buf2)+".vss";
	str = string(tempDir)+"\\vaporlog."+string(buf2)+".txt";

#else
	char buf[50];
	char buf1[50];
	
#ifdef Darwin
	char* defDir = getenv("HOME");
	if (!defDir) defDir = (char*)".";
	const char* defaultDir = defDir;
	sprintf (buf, "%s/vaporlog.txt", defaultDir);
	sprintf (buf1, "%s/VaporAutosave.vss", defaultDir);
	
#else
	uid_t	uid = getuid();
	const char* defaultDir = ".";
	sprintf (buf, "/tmp/vaporlog.%6.6d.txt",uid);
	sprintf (buf1, "/tmp/VaporAutosave.%6.6d.vss",uid);
#endif
	
	str = buf;
	str1 = buf1;
#endif
	currentLogfileName = str;
	if (currentLogfileName != formerLogfileName)
		MessageReporter::getInstance()->reset(currentLogfileName.c_str());
	autoSaveSessionFilename = str1;
	
	preferenceMetadataDir = defaultDir;
	preferenceJpegDirectory = defaultDir;	
	preferenceFlowDirectory = defaultDir;
	preferencePythonDirectory = defaultDir;
	preferenceSessionDirectory = defaultDir;
	preferenceTFPath = defaultDir;
	autoSaveInterval = 10;
	citationRemind = true;
	citationRemindDefault = true;
}

bool Session::
saveToFile(ofstream& ofs ){
	ParamNode* const rootNode = buildNode();
	ofs << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
	XmlNode::streamOut(ofs,(*rootNode));
	if (MyBase::GetErrCode() != 0) {
		MessageReporter::errorMsg("Session Save Error %d, creating Data Manager:\n%s",
			MyBase::GetErrCode(),MyBase::GetErrMsg());
		MyBase::SetErrCode(0);
		delete rootNode;
		return false;
	}
	delete rootNode;
	return true;
}

//Construct an XML node from the session parameters
//
ParamNode* Session::
buildNode() {
	//Construct the main node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	ostringstream oss;
	attrs[_VAPORVersionAttr] = Version::GetVersionString();
	
	oss.str(empty);
	oss << (double)stretchFactors[0] << " " << (double)stretchFactors[1] << " " << (double)stretchFactors[2];
	attrs[_stretchFactorsAttr] = oss.str();
	oss.str(empty);
	oss << extents[0]<<" "<<extents[1]<<" "<<extents[2]<<" "<<extents[3]<<" "<<extents[4]<<" "<<extents[5];
	attrs[_dataExtentsAttr] = oss.str();
	attrs[_transferFunctionPathAttr] = currentTFPath;
	attrs[_imageCapturePathAttr] = currentJpegDirectory;
	attrs[_flowDirectoryPathAttr] = currentFlowDirectory;
	attrs[_pythonDirectoryPathAttr] = currentPythonDirectory;
	attrs[_metadataPathAttr] = currentMetadataFile;
	oss.str(empty);
	oss << DataStatus::getInteractiveRefinementLevel();
	attrs[_interactiveRefLevelAttr] = oss.str();
	
	int numSesVars = getNumSessionVariables();
	ParamNode* mainNode = new ParamNode(_sessionTag, attrs, numSesVars+numTFs+2);

	//Now add children:  One for all the saved transfer functions,
	//One for each of the session variables,
	//one for the global params, and one for the visualizers
	
	//Create a global transfer function node
	if (numTFs > 0){
		attrs.clear();
		ParamNode* globalTFs = new ParamNode(_globalTransferFunctionsTag, attrs, numTFs);
		mainNode->AddChild(globalTFs);
		for (int i = 0; i< numTFs; i++){
			//Save all the state-saved tf's with their names
			ParamNode* tfNode = keptTFs[i]->buildNode(*tfNames[i]);
			globalTFs->AddChild(tfNode);
		}
	}
	for (int i = 0; i< numSesVars; i++){
		std::map <string, string> attrs;
		attrs.clear();
		ostringstream oss;
		oss.str(empty);
		oss << getVariableName(i);
		attrs[_variableNameAttr] = oss.str();
		oss.str(empty);
		if (DataStatus::isExtendedUp(i)) oss << "true"; else oss << "false";
		attrs[_extendUpAttr] = oss.str();
		oss.str(empty);
		if (DataStatus::isExtendedDown(i)) oss << "true"; else oss << "false";
		attrs[_extendDownAttr] = oss.str();
		oss.str(empty);
		oss << getBelowValue(i);
		attrs[_belowGridAttr] = oss.str();
		oss.str(empty);
		oss << getAboveValue(i);
		attrs[_aboveGridAttr] = oss.str();
		mainNode->NewChild(_sessionVariableTag, attrs,0);
	}
	//Create a global params node, and populate it.
	attrs.clear();
	ParamNode* globalPanels = new ParamNode(_globalParameterPanelsTag, attrs, 5);
	mainNode->AddChild(globalPanels);
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//Have the global parameters populate this:
	ParamNode* rgNode = vizMgr->getGlobalParams(ParamsBase::GetTypeFromTag(Params::_regionParamsTag))->buildNode();
	if(rgNode) globalPanels->AddChild(rgNode);
	ParamNode* animNode = vizMgr->getGlobalParams(ParamsBase::GetTypeFromTag(Params::_animationParamsTag))->buildNode();
	if(animNode) globalPanels->AddChild(animNode);
	ParamNode* vpNode = vizMgr->getGlobalParams(ParamsBase::GetTypeFromTag(Params::_viewpointParamsTag))->buildNode();
	if (vpNode) globalPanels->AddChild(vpNode);

	//Create a pythonScript node and populate it
	
	ParamNode* pythonNode = new ParamNode(_pythonScriptsTag,1+DataStatus::getNumDerivedScripts());
	string prog = XmlNode::replaceAll(PythonPipeLine::getStartupScript(), "<","&lt;");
	prog = XmlNode::replaceAll(prog, ">","&gt;");
	pythonNode->SetElementString(_setupScriptTag,prog);
	//Iterate through the derived variable scripts
	
	if (DataStatus::getNumDerivedScripts() > 0){
		//Iterate through the scripts
		int maxID = DataStatus::getMaxDerivedScriptId();
		for (int indx = 1; indx <= maxID; indx++){
			string name = DataStatus::getDerivedScriptName(indx);
			if(name == "") continue;
			std::map <string, string> attrs;
			attrs.clear();
			ostringstream oss;
			oss.str(empty);
			oss << name;
			attrs[_scriptNameAttr] = oss.str();
			ParamNode* scriptNode = new ParamNode(_pythonDerivedScriptTag,attrs,3);
			string prog = XmlNode::replaceAll(DataStatus::getDerivedScript(indx), "<","&lt;");
			prog = XmlNode::replaceAll(prog, ">","&gt;");
			scriptNode->SetElementString(_pythonProgramTag,prog);
			if (DataStatus::getDerived2DInputVars(indx).size()>0)
				scriptNode->SetElementStringVec(_python2DInputsTag, DataStatus::getDerived2DInputVars(indx));
			if (DataStatus::getDerived3DInputVars(indx).size()>0)
				scriptNode->SetElementStringVec(_python3DInputsTag, DataStatus::getDerived3DInputVars(indx));
			if (DataStatus::getDerived2DOutputVars(indx).size()>0)
				scriptNode->SetElementStringVec(_python2DOutputsTag, DataStatus::getDerived2DOutputVars(indx));
			if (DataStatus::getDerived3DOutputVars(indx).size()>0)
				scriptNode->SetElementStringVec(_python3DOutputsTag, DataStatus::getDerived3DOutputVars(indx));
			pythonNode->AddChild(scriptNode);
		}
	}
	mainNode->AddChild(pythonNode);
	//have vizwinmgr populate the vizwin nodes
	mainNode->AddChild(vizMgr->buildNode());
	
	return mainNode;
}

bool Session::
loadFromFile(ifstream& ifs){
	//Call resetMetadata to clean stuff out
	vector<string> files;
	resetMetadata(files,true, false);
	//Reset message counts:
	MessageReporter::getInstance()->resetCounts();
	
	//Then set values from file.
	ExpatParseMgr* parseMgr = new ExpatParseMgr(this);
	tempParsedTF = 0;
	parseMgr->parse(ifs);
	delete parseMgr;

	//set the animation toolbar to agree with the animation panel
	MainForm::getInstance()->timestepEdit->setText(QString::number(VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber()));
	//Reopen the logfile
	MessageReporter::getInstance()->reset(currentLogfileName.c_str());

	//Reset the front tab to the DVR:
	MainForm::getTabManager()->moveToFront(Params::GetTypeFromTag(Params::_dvrParamsTag));
	
	//Don't activate anything until user opens a new metadata file.
	return true;
}
bool Session::
elementStartHandler(ExpatParseMgr* pm, int  depth, std::string& tag, const char **attrs){
	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	switch (depth){
	//Parse the global session parameters, depth = 0
		case(0): 
		{
			if (StrCmpNoCase(tag, _sessionTag) != 0) return false;
			//Initialize session string to 1.2.2, didn't have session strings before that
			sessionVersionString = "1.2.2";
			//Default setup script:
			PythonPipeLine::setStartupScript("");
			//Start with default stretch factors
			float stretchFac[3];
			stretchFac[0]=stretchFac[1]=stretchFac[2] = 1.f;
			while (*attrs) {
				string attr = *attrs;
				attrs++;
				string value = *attrs;
				attrs++;

				istringstream ist(value);
				
				if (StrCmpNoCase(attr, _specifyTextureSizeAttr) == 0) {
					//Ignore:  This is now in user preferences
				}
				else if (StrCmpNoCase(attr, _VAPORVersionAttr) == 0){
					ist >> sessionVersionString;
				}
				else if (StrCmpNoCase(attr, _textureSizeAttr) == 0){
					//ignore
				}
				else if (StrCmpNoCase(attr, _cacheSizeAttr) == 0) {
					//ignore
				}
				else if (StrCmpNoCase(attr, _interactiveRefLevelAttr) == 0){
					int val;
					ist >> val;
					MainForm::getInstance()->setInteractiveRefinementSpin(val);
				}
				else if (StrCmpNoCase(attr, _stretchFactorsAttr) == 0) {
					ist >> stretchFac[0];
					ist >> stretchFac[1];
					ist >> stretchFac[2];
				}
				else if (StrCmpNoCase(attr, _jpegQualityAttr) == 0) {
					//Ignore; now it's in preferences
				}
				else if (StrCmpNoCase(attr, _transferFunctionPathAttr) == 0) {
					if (value != "")
						currentTFPath = value;
				}
				else if (StrCmpNoCase(attr, _imageCapturePathAttr) == 0) {
					if (value != "")
						currentJpegDirectory = value;
				}
				else if (StrCmpNoCase(attr, _flowDirectoryPathAttr) == 0) {
					if (value != "")
						currentFlowDirectory = value;
				}
				else if (StrCmpNoCase(attr, _pythonDirectoryPathAttr) == 0) {
					if (value != "")
						currentPythonDirectory = value;
				}
				else if (StrCmpNoCase(attr, _metadataPathAttr) == 0) {
					currentMetadataFile = value;
					int lastpos = currentMetadataFile.find_last_of("/\\");
					if (lastpos >= 0)
						currentMetadataDir = currentMetadataFile.substr(0,lastpos);
					else currentMetadataDir = currentMetadataFile;
				}
				else if (StrCmpNoCase(attr, _exportFileNameAttr) == 0) {
					currentExportFile = value;
				}
				else if (StrCmpNoCase(attr, _logFileNameAttr) == 0) {
					//Ignore
				}
				else if (StrCmpNoCase(attr, _maxPopupAttr) == 0) {
					// ignore
				}
				else if (StrCmpNoCase(attr, _dataExtentsAttr) == 0) {
					ist >> extents[0]; ist>>extents[1]; ist>>extents[2];
					ist >> extents[3]; ist>>extents[4]; ist>>extents[5];
				}
				else if (StrCmpNoCase(attr, _maxLogAttr) == 0) {
					//ignore
				}
				else {
					pm->parseError("Invalid session tag attribute : \"%s\"", attr.c_str());
				}
			}
			//After parsing all session attr's, set the stretched Extents,
			//And tell the datastatus the current version number:
			DataStatus::getInstance()->setSessionVersion(sessionVersionString);
			for (int i = 0; i<3; i++){
				stretchFactors[i] = stretchFac[i];
				stretchedExtents[i] = extents[i]*stretchFactors[i];
				stretchedExtents[i+3] = extents[i+3]*stretchFactors[i];
			}
			return true;
		}
		case(1): 
			//Parse child tags

			if (StrCmpNoCase(tag, _globalParameterPanelsTag) == 0){
				return true;//The Params class will parse it at level 2 
			} else if (StrCmpNoCase(tag, VizWinMgr::_visualizersTag) == 0) {
				//The vizwinmgr will parse it
				//Need to "push" to vizwinmgr parser.
				//That parser will "pop" back to session when done.
				
				pm->pushClassStack(VizWinMgr::getInstance());
				VizWinMgr::getInstance()->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else if (StrCmpNoCase(tag, _globalTransferFunctionsTag) == 0){
				return true;
			} else if (StrCmpNoCase(tag, _sessionVariableTag) == 0){
				//start with default above/below values
				float belowVal = BELOW_GRID;
				float aboveVal = ABOVE_GRID;
				//Session files if not specifed do NOT extend, since
				//this option is specified in session files after 1.3
				bool extendUp = false;
				bool extendDown = false;
				std::string varName;
				
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);
					if (StrCmpNoCase(attr, _variableNameAttr) == 0) {
						ist >> varName;
					}
					else if (StrCmpNoCase(attr, _belowGridAttr) == 0){
						ist >> belowVal;
					}
					else if (StrCmpNoCase(attr, _aboveGridAttr) == 0){
						ist >> aboveVal;
					}
					else if (StrCmpNoCase(attr, _extendUpAttr) == 0){
						if (value == "true") extendUp = true; else extendUp = false;
					}
					else if (StrCmpNoCase(attr, _extendDownAttr) == 0){
						if (value == "true") extendDown = true; else extendDown = false;
					}
				}
				if (varName == "") return false;
				int varnum = mergeVariableName(varName);
				DataStatus::getInstance()->setOutsideValues(varnum, belowVal, aboveVal, extendDown, extendUp);
				return true;
			} else if (StrCmpNoCase(tag, _pythonScriptsTag) == 0){
				return true;
			}
			pm->skipElement(tag, depth);
			return true;

		case(2):
			//parse grandchild tags
			if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0){
				//Need to "push" to transfer function parser.
				//That parser will "pop" back to session when done.
				tempParsedTF = new TransferFunction();
				pm->pushClassStack(tempParsedTF);
				tempParsedTF->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else if (Params::IsParamsTag(tag)){
				tempParsedPanel = Params::CreateDefaultParams(Params::GetTypeFromTag(tag));
				pm->pushClassStack(tempParsedPanel);
				tempParsedPanel->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else if (StrCmpNoCase(tag, _setupScriptTag) == 0){
				state->has_data = 1;
				return true;
			} else if (StrCmpNoCase(tag, _pythonDerivedScriptTag) == 0){
				//Prepare to parse a program:
				parsedPythonProgram = "";
				parsed2DInputVars.clear();
				parsed3DInputVars.clear();
				parsed2DOutputVars.clear();
				parsed3DOutputVars.clear();
				return true;
			}
			else {
				pm->skipElement(tag, depth);
				return true;
			}
		case (3):
			if (StrCmpNoCase(tag, _pythonProgramTag) == 0){
				state->has_data = 1;
				return true;
			} else if (StrCmpNoCase(tag, _python2DInputsTag) == 0){
				state->has_data = 1;
				return true;
			} else if (StrCmpNoCase(tag, _python3DInputsTag) == 0){
				state->has_data = 1;
				return true;
			} else if (StrCmpNoCase(tag, _python2DOutputsTag) == 0){
				state->has_data = 1;
				return true;
			} else if (StrCmpNoCase(tag, _python3DOutputsTag) == 0){
				state->has_data = 1;
				return true;
			} else {
				pm->skipElement(tag, depth);
				return true;
			}
		default: 
				pm->skipElement(tag, depth);
				return true;
	}
}
//assemble transfer function and global params after they are parsed.
//Also check validity.  0-level is session tag.
//	1-level is either global TF, global Params, or visualizers
//  2-level need to actually save the parsed TF's or parsed params
bool Session::
elementEndHandler(ExpatParseMgr* pm, int depth, std::string& tag){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	switch (depth){
		case (0):
			if(StrCmpNoCase(tag, _sessionTag) != 0) return false;
			return true;
		case (1):
			if (StrCmpNoCase(tag, _globalTransferFunctionsTag) == 0) return true;
			if (StrCmpNoCase(tag, _globalParameterPanelsTag) == 0) return true;
			if (StrCmpNoCase(tag, VizWinMgr::_visualizersTag) == 0) return true;
			if (StrCmpNoCase(tag, _sessionVariableTag) == 0) return true;
			if (StrCmpNoCase(tag, _pythonScriptsTag) == 0) return true;
			return false;
		case (2): //process transfer functions and global parameter panels and python startup script
			if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0){
				//At completion of parsing a transfer function, save it.
				assert(tempParsedTF);
				//save the tf, its name
				addTF(tempParsedTF->getName(), tempParsedTF);
				tempParsedTF = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_dvrParamsTag) == 0){
				// just ignore it
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,ParamsBase::GetTypeFromTag(Params::_dvrParamsTag));
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_isoParamsTag) == 0){
				// just ignore it
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,ParamsBase::GetTypeFromTag(Params::_isoParamsTag));
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_probeParamsTag) == 0){
				// just ignore it
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,ParamsBase::GetTypeFromTag(Params::_probeParamsTag));
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_twoDImageParamsTag) == 0){
				// just ignore it
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,ParamsBase::GetTypeFromTag(Params::_twoDImageParamsTag));
				tempParsedPanel = 0;
				return true;
			} else if ((StrCmpNoCase(tag, Params::_twoDDataParamsTag) == 0) ||
						(StrCmpNoCase(tag, Params::_twoDParamsTag) == 0)){
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,ParamsBase::GetTypeFromTag(Params::_twoDDataParamsTag));
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_regionParamsTag) == 0){
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,ParamsBase::GetTypeFromTag(Params::_regionParamsTag));
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_animationParamsTag) == 0){
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,ParamsBase::GetTypeFromTag(Params::_animationParamsTag));
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_viewpointParamsTag) == 0){
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,ParamsBase::GetTypeFromTag(Params::_viewpointParamsTag));
				tempParsedPanel = 0;
				return true;	
			} else if (StrCmpNoCase(tag, Params::_flowParamsTag) == 0){
				// just ignore it
				assert(tempParsedPanel);
                vizWinMgr->replaceGlobalParams(tempParsedPanel,ParamsBase::GetTypeFromTag(Params::_flowParamsTag));
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, _setupScriptTag) == 0){
				const string &strdata = pm->getStringData();
				string prog = XmlNode::replaceAll(strdata, "&lt;","<");
				prog = XmlNode::replaceAll(prog, "&gt;",">");
				PythonPipeLine::setStartupScript(prog);
				return true;
			} else if (StrCmpNoCase(tag, _pythonDerivedScriptTag) == 0){
				//Completed parsing of a program:
				parsedPythonProgram = XmlNode::replaceAll(parsedPythonProgram,"&lt;","<");
				parsedPythonProgram = XmlNode::replaceAll(parsedPythonProgram,"&gt;",">");
				int id = DataStatus::getInstance()->addDerivedScript(
					parsed2DInputVars,parsed2DOutputVars,
					parsed3DInputVars,parsed3DOutputVars,
					parsedPythonProgram, false);
				if (id <= 0) 
					MessageReporter::errorMsg(" Invalid Python program in session file");
				return true;
			}
			else return false;
		case(3):
			if (StrCmpNoCase(tag, _pythonProgramTag) == 0){
				parsedPythonProgram = pm->getStringData();
				return true;
			} else if (StrCmpNoCase(tag, _python2DInputsTag) == 0){
				StrToWordVec(pm->getStringData(),parsed2DInputVars);
				return true;
			} else if (StrCmpNoCase(tag, _python3DInputsTag) == 0){
				StrToWordVec(pm->getStringData(),parsed3DInputVars);
				return true;
			} else if (StrCmpNoCase(tag, _python2DOutputsTag) == 0){
				StrToWordVec(pm->getStringData(),parsed2DOutputVars);
				return true;
			} else if (StrCmpNoCase(tag, _python3DOutputsTag) == 0){
				StrToWordVec(pm->getStringData(),parsed3DOutputVars);
				return true;
			}
			else return false;

		default: return false;
	}
}

// Export the data specification in the current active visualizer:
//
void Session::
exportData(){
	//Make sure the current vdf is version 2.0 or greater:
	
	ImpExp exporter;
	//Note:  will export data associated with current active visualizer
	VizWinMgr* winMgr = VizWinMgr::getInstance();
	int winNum = winMgr->getActiveViz();
	if (winNum < 0 || (getDataMgr() == 0)){
		MessageReporter::errorMsg("%s","Export data error;\nExporting data requires loaded data\nand active visualizer");
		return;
	}
	RegionParams* r = winMgr->getRegionParams(winNum);
	
	//Set up arguments to Export():
	//
	AnimationParams*  p = winMgr->getAnimationParams(winNum);
	
	DvrParams* dParams = (DvrParams*)(winMgr->getDvrParams(winNum));
	//always go for max number of transforms:
	int numxforms = DataStatus::getInstance()->getNumTransforms();
	
	size_t currentFrame = (size_t)p->getCurrentFrameNumber();
	size_t frameInterval[2];
	size_t minCoords[3],maxCoords[3];
	size_t mncrds[3],mxcrds[3];
	size_t minbdim[3],maxbdim[3];
	frameInterval[0] = (size_t)p->getStartFrameNumber();
	frameInterval[1] = (size_t)p->getEndFrameNumber();
	//Note that we will export the current region, even if there's no
	//valid data in it...
	r->getRegionVoxelCoords(numxforms, mncrds, mxcrds, minbdim, maxbdim,currentFrame);
	for (int i = 0; i< 3; i++) {
		minCoords[i] = mncrds[i];
		maxCoords[i] = mxcrds[i];
	}
	if (DataStatus::getInstance()->dataIsLayered()) {
		//Make sure region is full in z dimension:
		
		size_t max_zdim = DataStatus::getInstance()->getFullSizeAtLevel(numxforms,2) - 1;
		if (max_zdim != maxCoords[2] || minCoords[2] != 0){
			MessageReporter::errorMsg("Export of a region on layered grids\nis only permitted when\nthe region is full in the\nvertical (z) dimension.");
			return;
		}
		//Determine the unlayered vertical grid size of the data:
		size_t dim[3];
		getDataMgr()->GetDim(dim, -1);
		maxCoords[2] = dim[2];
	}
	
	int rc = exporter.Export(currentMetadataFile,
		currentFrame,
		getVariableName(dParams->getSessionVarNum()),
		minCoords,
		maxCoords,
		frameInterval);
	if (rc < 0){
		MessageReporter::errorMsg("Export data error:\n%s", exporter.GetErrMsg());
		exporter.SetErrCode(0);
	} else {
		MessageReporter::warningMsg("Exported time step %d of region in %s .\nNote: recently imported variables \nmay not be exported",
			currentFrame, (const char*)VizWinMgr::getInstance()->getVizWinName(winNum).toAscii());
	}
	return;
}
/**
 * set up state for a new datamanager or, if argument is null, set to 
 * default state.  If newSession is true, the current settings are ignored, otherwise
 * the established session attempts to be compatible with previous settings.
 * (the newSession flag indicates that the user has clicked "new", or this is the first time
 * a datafile has been loaded, so that the current state is irrelevant)
 * Also perform related functions 
 * When a session is restored from file, all the session state is loaded up.  But the metadata is
 * not reloaded.  
 */
bool Session::
resetMetadata(vector<string>& files, bool restoredSession, bool importing, bool doMerge, int mergeOffset)
{
	int i;
	//This is a flag used by renderers to avoid rendering while state
	//is changing.
	if (DataStatus::getInstance())
		DataStatus::getInstance()->setRenderReady(false);
	VizWinMgr::getInstance()->disableAllRenderers();
	bool defaultSession = (files.size() == 0);
	//if (restoredSession) assert(defaultSession);
	//The metadata is created by (and obtained from) the datamgr
	//Don't update the currentMetadataFile if we are doing a merge
	if (!defaultSession && !doMerge) {
		currentMetadataFile = files[0].c_str();
		int lastpos = currentMetadataFile.find_last_of("/\\");
		if (lastpos >= 0)
			currentMetadataDir = currentMetadataFile.substr(0,lastpos);
		else currentMetadataDir = currentMetadataFile;
	}
	//If we don't already have a dataMgr, we can't really be merging:
	if (!dataMgr) {
		if (doMerge) {doMerge = false; }
	} else if(defaultSession || !doMerge) {
		delete dataMgr;
		dataMgr = 0;
	}
	if (doMerge) assert (!defaultSession);

	//Handle the various cases of loading the metadata
	if (defaultSession){
		DataStatus::clearVariableNames();
		//Clear out any dummy params classes:
		Params::clearDummyParamsInstances();
		ParamsBase::clearDummyParamsBaseInstances();
	} else {
		
		if (!doMerge) {
			if (!restoredSession) DataStatus::clearVariableNames();
			if (importing) {
				dataMgr = DataMgrFactory::New(files, cacheMB,"wrf");

				if (DataMgr::GetErrCode() != 0) {
					MessageReporter::errorMsg("WRF loading error %d, creating Data Manager:\n %s",
						DataMgr::GetErrCode(),DataMgr::GetErrMsg());
					DataMgr::SetErrCode(0);
					if (dataMgr) delete dataMgr;
					dataMgr = 0;
					return false;
				}
			} else {
				vector <string> metafiles;
				metafiles.push_back(currentMetadataFile);
				dataMgr = DataMgrFactory::New(metafiles, cacheMB);
				if (DataMgr::GetErrCode() != 0) {
					MessageReporter::errorMsg("Data Loading error %d, creating Data Manager:\n %s",
						DataMgr::GetErrCode(),DataMgr::GetErrMsg());
					DataMgr::SetErrCode(0);
					if (dataMgr) delete dataMgr;
					dataMgr = 0;
					return false;
				}
			}
		} else {//merge
			assert (dataMgr);
			MetadataVDC* md; 
			DataMgrWB *dataMgrWB = dynamic_cast<DataMgrWB *> (dataMgr);
			LayeredIO* dataMgrLayered = dynamic_cast<LayeredIO *> (dataMgr);
			
			if (dataMgrWB) {
				md = (MetadataVDC*) dataMgrWB;
			} else if (dataMgrLayered){
				md = (MetadataVDC*) dataMgrLayered;
			} else {
				DataStatus::getInstance()->setRenderReady(true);
				return false;
			}
				
			//Need a non-const pointer to the metadata, since we will modify it:
			size_t offset = (size_t) mergeOffset;
			//Use the fileBase, not the currentMetadataFile for the merge.
			int rc = md->Merge(files[0].c_str(),offset);
			if (rc < 0) {
				DataStatus::getInstance()->setRenderReady(true);
				return false;
			}
			

		}
		
	} 

	//Get the extents from the metadata, if it exists:
	if (dataMgr){
		std::vector<double> mdExtents = dataMgr->GetExtents();
		DataMgrWB *dataMgrWB = dynamic_cast<DataMgrWB*> (dataMgr);

        // Automatically calculate stretch factors for spherical data, 
        // so that, volume bounding box will be a unit cube. The spherical
        // rendering will be centered in this unit cube. This is a bit of
        // a hack. However, until vapor is re-designed to with a more 
        // general framework that can handle non-cartesian coordinate
        // systems, this provides a convenient, low-impact way to handle 
        // the volume exents vs. data extents issue for spherical rendering.
        if (dataMgrWB && dataMgrWB->GetCoordSystemType() == "spherical") {
          float maxExtent = 0;

          for (int i=0; i<3; i++)
          {
            maxExtent = MAX(mdExtents[i+3] - mdExtents[i], maxExtent);
          }

          for (int i=0; i<3; i++)
          {
            stretchFactors[i] = maxExtent / (mdExtents[i+3] - mdExtents[i]);
          }
        }

		for (i = 0; i< 6; i++){
			extents[i] = (float)mdExtents[i];
			stretchedExtents[i] = extents[i]*stretchFactors[i%3];
		}
	}
	//Histo::releaseHistograms();
	
	if (DataStatus::getInstance()) delete DataStatus::getInstance();
	
	
	if (!defaultSession) {
		//If we are not merging, clean out the variableNames.
		//We add the new ones back in setupDataStatus();
		if (!doMerge && !restoredSession) DataStatus::clearVariableNames();
		setupDataStatus();
		
		//Is there any data here?
		if(!dataExists) {
			MessageReporter::errorMsg(
				"Session: No data in specified dataset,\nor data in specified files cannot be read");
		}

		
	}
	VizWinMgr* myVizWinMgr = VizWinMgr::getInstance();
	
	//If we're restarting session: Delete all visualizers, then create one new one.
	//Do this before setting up params, since they will get deleted if their window
	//vanishes.
	//
	if (defaultSession){
		for (i = 0; i< MAXVIZWINS; i++){
			if (myVizWinMgr->getVizWin(i)){
				myVizWinMgr->killViz(i);
			}
		}
		MainForm::getInstance()->getMDIArea()->closeAllSubWindows();
		if(!restoredSession) { 
			//Create one new visualizer, set all params to default
			myVizWinMgr->launchVisualizer();
			myVizWinMgr->restartParams();
			init();
		// if we are restoring, the next time want the newSession flag off, so 
		// we will respect the values set in the params on loading the datamgr.
		} else newSession = false;
	}
	else {
		myVizWinMgr->reinitializeParams(newSession);
		
		//set the annotation to use current extents in all active visualizers
		if (newSession || !restoredSession) {
			for (int i = 0; i< MAXVIZWINS; i++){
				if (myVizWinMgr->getVizWin(i))
					myVizWinMgr->getVizWin(i)->setAxisExtents(extents);
			}
		}
		//Set the newSession flag, next time we'll use these settings.
		newSession = false;
	}

	//Make all the visualizers use their viewpoint params:
	myVizWinMgr->initViews();
	//Restart the animation controller:
	AnimationController::getInstance()->restart();

	//Reset the undo/redo queue
	resetCommandQueue();
	if (DataStatus::getInstance())
		DataStatus::getInstance()->setRenderReady(true);
	//Set the metadataSaved flag depending on whether or not we merged:
	//That way we know the next session save will also need to save metadata
	//If we failed to merge or load, the metadataSaved flag does not change.
	metadataSaved = !doMerge;
	MainForm::getInstance()->timestepEdit->setText(QString::number(VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber()));
	return true;
}

//Add a command to the circular command queue
//
void Session::
addToHistory(Command* cmd){
	if (recordingCount>0) {
		
		delete cmd;
		return;
	}
	assert(recordingCount == 0);  //better not be negative!
	//Check if we are invalidating the remainder of queue:
	//
	if (currentQueuePos > endQueuePos){
		for (int i = currentQueuePos+1; i<= endQueuePos; i++){
			Command* c =  commandQueue[i%MAX_HISTORY];
			if (c != 0) delete c;
			commandQueue[i%MAX_HISTORY] = 0;
		}
		endQueuePos = currentQueuePos;
	}

	//Check if we are at queue end:
	//
	if ((currentQueuePos - startQueuePos) == MAX_HISTORY +1 ){
		//release entry we will be overwriting:
		//
		delete commandQueue[startQueuePos%MAX_HISTORY];
		commandQueue[startQueuePos%MAX_HISTORY] = 0;
		startQueuePos++;
	}
	//Insert the command at the appropriate place:
	//Advance queue is not necessary to be called after this!
	//
	currentQueuePos++;
	endQueuePos = currentQueuePos;
	commandQueue[endQueuePos%MAX_HISTORY] = cmd;

	MainForm::getInstance()->editUndoAction->setEnabled(true);
	MainForm::getInstance()->editRedoAction->setEnabled(false);
	//Perform session auto-save if requested, and if the data mgr exists:
	if (autoSaveInterval > 0 && (endQueuePos%autoSaveInterval == 0 && getDataMgr())){
		ofstream fileout;
		fileout.open(getAutoSaveSessionFilename().c_str());
		if (! fileout) {
			MessageReporter::errorMsg( "Unable to auto-save session to file: \n %s\n%s", autoSaveSessionFilename.c_str(),
				"Choose another autosave location \nfrom user preferences");
			return;
		}
		
		if (!saveToFile(fileout)){//Report error if can't save to file
			MessageReporter::errorMsg("Failed to auto-save session to:\n %s\n%s", autoSaveSessionFilename.c_str(),
				"Choose another autosave location \nfrom user preferences");
			fileout.close();
			return;
		}
		fileout.close();
	}

}
void Session::backupQueue(){
	//Make sure we can do it!
	//
	assert(currentQueuePos > startQueuePos);
	commandQueue[currentQueuePos%MAX_HISTORY]->unDo();
	currentQueuePos--;
	MainForm::getInstance()->editRedoAction->setEnabled(true);
	if (currentQueuePos == startQueuePos) 
		MainForm::getInstance()->editUndoAction->setEnabled(false);
}
void Session::advanceQueue(){
	//Make sure we can do it:
	//
	assert(currentQueuePos < endQueuePos);
	//perform the next command
	//
	commandQueue[(++currentQueuePos)%MAX_HISTORY]->reDo();
	MainForm::getInstance()->editUndoAction->setEnabled(true);
	if (currentQueuePos == endQueuePos) 
		MainForm::getInstance()->editRedoAction->setEnabled(false);
}
//Destroy history (e.g. on restoring a session).
void Session::resetCommandQueue(){
	for (int i = 0; i<MAX_HISTORY; i++){
		if (commandQueue[i]) {
			delete commandQueue[i];
			commandQueue[i] = 0;
		}
	}
	currentQueuePos = 0;
	startQueuePos = 0;
	endQueuePos = 0;
	MainForm::getInstance()->disableUndoRedo();
}

// Read the Metadata to determine exactly what data is present
// This data is inserted into currentDatastatus;


void Session::
setupDataStatus(){
	DataStatus* currentDataStatus = DataStatus::getInstance();
	QApplication* app = MainForm::getInstance()->getApp();
	if(currentDataStatus->reset(dataMgr, cacheMB, app)) {
		dataExists = true;
		currentDataStatus->stretchExtents(stretchFactors);
		currentDataStatus->specifyTextureSize(textureSizeSpecified);
		currentDataStatus->setTextureSize(textureSize);
		currentDataStatus->setSessionVersion(sessionVersionString);
	}
	else dataExists = false;
}

//Methods to keep or remove a transfer function 
//with the session.  The transFunc is always the current one from the dvrParams
//
void Session::addTF(const char* tfName, RenderParams* params){

	//Check first if this name is already in the list.  If so, remove it.
	const std::string tfname(tfName);
	removeTF(&tfname);

	if (numTFs >= tfListSize){
		tfListSize += 10;
		//Not enough space, need to allocate more room:
		TransferFunction** tempTFHolder = new TransferFunction*[tfListSize];
		std::string** tempTFNames = new std::string*[tfListSize];
		
		for (int i = 0; i<numTFs; i++){
			tempTFHolder[i] = keptTFs[i];
			tempTFNames[i] = tfNames[i];
			
			delete tfNames[i];
			delete keptTFs[i];
		}
		keptTFs = tempTFHolder;
		tfNames = tempTFNames;
		
	}
	//copy the tf, its name
	keptTFs[numTFs] = new TransferFunction(*((TransferFunction*)params->GetMapperFunc()));
	tfNames[numTFs] = new std::string(tfName);
	
	//Don't retain the pointers to dvrParams and TFE:
	keptTFs[numTFs]->setParams(0);
	numTFs++;
	return;
}
//Add a transfer function not associated with a Params
//Don't clone it, add it directly
void Session::addTF(const std::string tfName, TransferFunction* tf){

	//Check first if this name is already in the list.  If so, remove it.
	removeTF(&tfName);

	if (numTFs >= tfListSize){
		tfListSize += 10;
		//Not enough space, need to allocate more room:
		TransferFunction** tempTFHolder = new TransferFunction*[tfListSize];
		std::string** tempTFNames = new std::string*[tfListSize];
		
		for (int i = 0; i<numTFs; i++){
			tempTFHolder[i] = keptTFs[i];
			tempTFNames[i] = tfNames[i];
			
			delete tfNames[i];
			delete keptTFs[i];
		}
		keptTFs = tempTFHolder;
		tfNames = tempTFNames;
		
	}
	//copy the tf, its name
	keptTFs[numTFs] = tf;
	tfNames[numTFs] = new string(tfName);
	
	//Don't retain the pointers to dvrParams and TFE:
	keptTFs[numTFs]->setParams(0);
	numTFs++;
	return;
}
bool Session::
removeTF(const std::string* name){
	//See if the string is in the list:
	int i;
	for (i = 0; i< numTFs; i++){
		if (!tfNames[i]->compare(*name)) break;
	}
	if (i >= numTFs) return false;
	delete tfNames[i];
	delete keptTFs[i];
	//move the others up:
	for (int j = i; j<numTFs-1; j++){
		tfNames[j] = tfNames[j+1];
		keptTFs[j] = keptTFs[j+1];
		
	}
	numTFs--;
	return true;

}
//Method to get a transfer function from session.  Clones the one
//that is saved in the session.
TransferFunction* Session::
getTF(const std::string* name){
	//See if the string is in the list:
	int i;
	for (i = 0; i< numTFs; i++){
		if (!tfNames[i]->compare(*name)) break;
	}
	if (i >= numTFs) return 0;
	TransferFunction* tf = new TransferFunction(*(keptTFs[i]));
	
	return tf;
}
//Method to see if a transfer function is in list
bool Session::
isValidTFName(const std::string* name){
	//See if the string is in the list:
	int i;
	for (i = 0; i< numTFs; i++){
		if (!tfNames[i]->compare(*name)) break;
	}
	if (i >= numTFs) return false;
	return true;
}
//Save the most recent file path used for save/restore of transfer functions:
void Session::
updateTFFilePath(QString* s){
	//First find the last / or \.  Strip everything to the right:
	int pos = s->lastIndexOf('\\');
	if (pos < 0) pos = s->lastIndexOf('/');
	assert (pos>= 0);
	if (pos < 0) return;
	currentTFPath = s->left(pos+1).toStdString();
}

//Diagnostic message callback:
void Session::
infoCallbackFcn(const char* msg){
	MessageReporter::infoMsg("%s",msg);
}

const string& Session::getPreferencesFile(){
	const char* prefPath = getenv("VAPOR_PREFS_DIR");
	if (!prefPath)
		prefPath = getenv("HOME");
	if (!prefPath){
		char *tmp = getenv("VAPOR_SHARE");
		if (tmp) {
			prefFile = string(tmp)+"/examples/.vapor_prefs";
			return prefFile;
		}
	}
	prefFile = string(prefPath)+"/.vapor_prefs";
	return prefFile;
}
void Session::makeSessionFilepath(std::string& path){
	string s = currentSessionDirectory;
	if (s == "") {
		path = sessionFilename;
		return;
	}
	char c = s[s.length()-1];
	if (c == '/' || c == '\\' ) {
		path = (s + sessionFilename);
		return;
	}
	path =  (s + '/' + sessionFilename);
	return;
}
void Session::setSessionFilepath(const char* path){
	
	string spath = path;
	//extract filename; find last '/'
	int posn = spath.find_last_of("/");
	if (posn == -1) {
		posn = spath.find_last_of("\\");
	}
	if (posn == -1){
		currentSessionDirectory = "";
		sessionFilename = path;
		return;
	}
	currentSessionDirectory = spath.substr(0, posn);
	sessionFilename = spath.substr(posn+1);
}



