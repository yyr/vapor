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
//	Description:	Implements the Session class and the DataStatus class
//
#include <cstdlib>
#include <cstdio>
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include "session.h"
#include "vapor/DataMgr.h"
#include "vizwinmgr.h"
#include "mainform.h"
#include "command.h"
#include "messagereporter.h"
#include <qaction.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include "histo.h"
#include "vapor/Metadata.h"
#include "vapor/ImpExp.h"
#include "animationcontroller.h"
#include "transferfunction.h"
#include <qstring.h>

using namespace VAPoR;
Session* Session::theSession = 0;
const string Session::_cacheSizeAttr = "CacheSize";
const string Session::_jpegQualityAttr = "JpegQuality";
const string Session::_metadataPathAttr = "MetadataPath";
const string Session::_transferFunctionPathAttr = "TransferFunctionPath";
const string Session::_imageCapturePathAttr = "JpegPath";
const string Session::_logFileNameAttr = "LogFileName";
const string Session::_exportFileNameAttr = "ExportFileName";
const string Session::_maxPopupAttr = "MaxPopups";
const string Session::_maxLogAttr = "MaxLogMsgs";
const string Session::_dataExtentsAttr = "DataExtents";
const string Session::_sessionTag = "Session";
const string Session::_globalParameterPanelsTag = "GlobalParameterPanels";
const string Session::_globalTransferFunctionsTag = "GlobalTransferFunctions";
Session::Session() {
	
	MyBase::SetErrMsgCB(errorCallbackFcn);
	MyBase::SetDiagMsgCB(infoCallbackFcn);
	previousClass = 0;
	dataMgr = 0;
	currentMetadata = 0;
	myReader = 0;
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
	currentDataStatus = 0;
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
	delete AnimationController::getInstance();
	delete VizWinMgr::getInstance();
	//Note: metadata is deleted by Datamgr
	if (dataMgr) delete dataMgr;
	for (i = startQueuePos; i<= endQueuePos; i++){
		if (commandQueue[i%MAX_HISTORY]) {
			delete commandQueue[i%MAX_HISTORY];
			commandQueue[i%MAX_HISTORY] = 0;
		}
	}
	//Must delete the histograms before the currentDataStatus:
	Histo::releaseHistograms();
	if(currentDataStatus) delete currentDataStatus;

	//Delete all the saved transfer functions:
	for (i = 0; i<numTFs; i++){
		delete keptTFs[i];
		delete tfNames[i];
	}
	if (tfListSize>0){
		delete keptTFs;
		delete tfNames;
		
	}
	
}
//Set session state to base values:
//
void Session::init() {
	int i;
	recordingCount = 0;
	
#ifdef WIN32
	cacheMB = 500;
	currentMetadataFile = "F:\\run4\\RUN4.vdf";
	currentJpegDirectory = "C:\\temp";
#else
	cacheMB = 1024;
	currentMetadataFile = "*.vdf";
	currentJpegDirectory = "/tmp";	
#endif
	currentExportFile = ImpExp::GetPath();
	//Delete all the saved transfer functions:
	for (i = 0; i<numTFs; i++){
		delete keptTFs[i];
		delete tfNames[i];
	}
	if (tfListSize>0){
		delete keptTFs;
		delete tfNames;
	}
	currentTFPath = "/tmp";
	
	numTFs = 0;
	tfNames = 0;
	keptTFs = 0;
	tfListSize = 0;
	
	jpegQuality = 75;
	dataExists = false;
	renderOK = false;
	newSession = true;
	extents[0] = extents[1] = extents[2] = 0.f;
	extents[3] = extents[4] = extents[5] = 1.f;
	
}
bool Session::
saveToFile(ofstream& ofs ){
	XmlNode* const rootNode = buildNode();
	ofs << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
	XmlNode::streamOut(ofs,(*rootNode));
	if (MyBase::GetErrCode() != 0) {
		MessageReporter::errorMsg("Session Save Error %d, creating Data Manager:\n %s",
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
XmlNode* Session::
buildNode() {
	//Construct the main node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	ostringstream oss;

	oss.str(empty);
	oss << (long)cacheMB;
	attrs[_cacheSizeAttr] = oss.str();
	oss.str(empty);
	oss << (long)jpegQuality;
	attrs[_jpegQualityAttr] = oss.str();
	attrs[_metadataPathAttr] = currentMetadataFile;
	attrs[_transferFunctionPathAttr] = currentTFPath;
	attrs[_imageCapturePathAttr] = currentJpegDirectory;
	attrs[_exportFileNameAttr] = currentExportFile;
	MessageReporter* msgRpt = MessageReporter::getInstance();
	attrs[_logFileNameAttr] = getLogfileName();
	oss.str(empty);
	oss << (long)msgRpt->getMaxPopup(MessageReporter::Info) << " " <<
		(long)msgRpt->getMaxPopup(MessageReporter::Warning) << " " <<
		(long)msgRpt->getMaxPopup(MessageReporter::Error);
	attrs[_maxPopupAttr] = oss.str();
	oss.str(empty);
	oss << extents[0]<<" "<<extents[1]<<" "<<extents[2]<<" "<<extents[3]<<" "<<extents[4]<<" "<<extents[5];
	attrs[_dataExtentsAttr] = oss.str();
	
	oss.str(empty);
	oss << (long)msgRpt->getMaxLog(MessageReporter::Info) << " "
		<< (long)msgRpt->getMaxLog(MessageReporter::Warning) << " "
		<< (long)msgRpt->getMaxLog(MessageReporter::Error);
	attrs[_maxLogAttr] = oss.str();

	XmlNode* mainNode = new XmlNode(_sessionTag, attrs, numTFs+2);

	//Now add children:  One for all the saved transfer functions,
	//one for the global params, and one for the visualizers
	
	//Create a global transfer function node
	if (numTFs > 0){
		attrs.clear();
		XmlNode* globalTFs = mainNode->NewChild(_globalTransferFunctionsTag, attrs, numTFs);
		for (int i = 0; i< numTFs; i++){
			//Save all the state-saved tf's with their names
			XmlNode* tfNode = keptTFs[i]->buildNode(*tfNames[i]);
			globalTFs->AddChild(tfNode);
		}
	}
	//Create a global params node, and populate it.
	attrs.clear();
	XmlNode* globalPanels = mainNode->NewChild(_globalParameterPanelsTag, attrs, 5);
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//Have the global parameters populate this:
	XmlNode* dvrNode = vizMgr->getGlobalParams(Params::DvrParamsType)->buildNode();
	if(dvrNode) globalPanels->AddChild(dvrNode);
	XmlNode* rgNode = vizMgr->getGlobalParams(Params::RegionParamsType)->buildNode();
	if(rgNode) globalPanels->AddChild(rgNode);
	XmlNode* animNode = vizMgr->getGlobalParams(Params::AnimationParamsType)->buildNode();
	if(animNode) globalPanels->AddChild(animNode);
	XmlNode* vpNode = vizMgr->getGlobalParams(Params::ViewpointParamsType)->buildNode();
	if (vpNode) globalPanels->AddChild(vpNode);
	XmlNode* flowNode = vizMgr->getGlobalParams(Params::FlowParamsType)->buildNode();
	if (flowNode) globalPanels->AddChild(flowNode);
	//have vizwinmgr populate the vizwin nodes
	mainNode->AddChild(vizMgr->buildNode());
	
	return mainNode;
}

bool Session::
loadFromFile(ifstream& ifs){
	//Call resetMetadata to clean stuff out
	resetMetadata(0,true);
	//Then set values from file.
	ExpatParseMgr* parseMgr = new ExpatParseMgr(this);
	tempParsedTF = 0;
	parseMgr->parse(ifs);
	delete parseMgr;

	//Reopen the logfile
	MessageReporter::getInstance()->reset(currentLogfileName.c_str());
	//We should return pointer to 0 when done!
	
	//Don't activate anything until user opens a new metadata file.
	return true;
}
bool Session::
elementStartHandler(ExpatParseMgr* pm, int  depth, std::string& tag, const char **attrs){
	switch (depth){
	//Parse the global session parameters, depth = 0
		case(0): 
		{
			if (StrCmpNoCase(tag, _sessionTag) != 0) return false;
			MessageReporter* msgRpt = MessageReporter::getInstance();
			while (*attrs) {
				string attr = *attrs;
				attrs++;
				string value = *attrs;
				attrs++;

				istringstream ist(value);
				int int1, int2, int3;
				
				if (StrCmpNoCase(attr, _cacheSizeAttr) == 0) {
					ist >> cacheMB;
				}
				else if (StrCmpNoCase(attr, _jpegQualityAttr) == 0) {
					ist >> jpegQuality;
				}
				else if (StrCmpNoCase(attr, _transferFunctionPathAttr) == 0) {
					ist >> currentTFPath;
				}
				else if (StrCmpNoCase(attr, _imageCapturePathAttr) == 0) {
					ist >> currentJpegDirectory;
				}
				else if (StrCmpNoCase(attr, _metadataPathAttr) == 0) {
					ist >> currentMetadataFile;
				}
				else if (StrCmpNoCase(attr, _exportFileNameAttr) == 0) {
					ist >> currentExportFile;
				}
				else if (StrCmpNoCase(attr, _logFileNameAttr) == 0) {
					ist >> currentLogfileName;
				}
				else if (StrCmpNoCase(attr, _maxPopupAttr) == 0) {
					ist >> int1; ist>>int2; ist>>int3;
					msgRpt->setMaxPopup(MessageReporter::Info, int1);
					msgRpt->setMaxPopup(MessageReporter::Warning, int2);
					msgRpt->setMaxPopup(MessageReporter::Error, int3);
				}
				else if (StrCmpNoCase(attr, _dataExtentsAttr) == 0) {
					ist >> extents[0]; ist>>extents[1]; ist>>extents[2];
					ist >> extents[3]; ist>>extents[4]; ist>>extents[5];
				}
				else if (StrCmpNoCase(attr, _maxLogAttr) == 0) {
					ist >> int1; ist>>int2; ist>>int3;
					msgRpt->setMaxLog(MessageReporter::Info, int1);
					msgRpt->setMaxLog(MessageReporter::Warning, int2);
					msgRpt->setMaxLog(MessageReporter::Error, int3);
				}
				else {
					pm->parseError("Invalid session tag attribute : \"%s\"", attr.c_str());
				}
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
			} else return false;
		case(2):
			if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0){
				//Need to "push" to transfer function parser.
				//That parser will "pop" back to session when done.
				tempParsedTF = new TransferFunction();
				pm->pushClassStack(tempParsedTF);
				tempParsedTF->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else if (StrCmpNoCase(tag, Params::_dvrParamsTag) == 0){
				//Need to "push" to dvr parser.
				//That parser will "pop" back to session when done.
				tempParsedPanel = new DvrParams(-1);
				pm->pushClassStack(tempParsedPanel);
				tempParsedPanel->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else if (StrCmpNoCase(tag, Params::_regionParamsTag) == 0){
				//Need to "push" to dvr parser.
				//That parser will "pop" back to session when done.
				tempParsedPanel = new RegionParams(-1);
				pm->pushClassStack(tempParsedPanel);
				tempParsedPanel->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else if (StrCmpNoCase(tag, Params::_viewpointParamsTag) == 0){
				
				tempParsedPanel = new ViewpointParams(-1);
				pm->pushClassStack(tempParsedPanel);
				tempParsedPanel->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else if (StrCmpNoCase(tag, Params::_animationParamsTag) == 0){
				
				tempParsedPanel = new AnimationParams(-1);
				pm->pushClassStack(tempParsedPanel);
				tempParsedPanel->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else if (StrCmpNoCase(tag, Params::_flowParamsTag) == 0){
				//Need to "push" to flow parser.
				//That parser will "pop" back to session when done.
				tempParsedPanel = new FlowParams(-1);
				pm->pushClassStack(tempParsedPanel);
				tempParsedPanel->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else return false;
		default: return false;
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
			return false;
		case (2): //process transfer functions and global parameter panels
			if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0){
				//At completion of parsing a transfer function, save it.
				assert(tempParsedTF);
				//save the tf, its name
				addTF(tempParsedTF->getName(), tempParsedTF);
				tempParsedTF = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_dvrParamsTag) == 0){
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,Params::DvrParamsType);
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_regionParamsTag) == 0){
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,Params::RegionParamsType);
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_animationParamsTag) == 0){
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,Params::AnimationParamsType);
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_viewpointParamsTag) == 0){
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,Params::ViewpointParamsType);
				tempParsedPanel = 0;
				return true;	
			} else if (StrCmpNoCase(tag, Params::_flowParamsTag) == 0){
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,Params::FlowParamsType);
				tempParsedPanel = 0;
				return true;	
			} else return false;
		default: return false;
	}
}

// Export the data specification in the current active visualizer:
//
void Session::
exportData(){
	ImpExp exporter;
	//Note:  will export data associated with current active visualizer
	VizWinMgr* winMgr = VizWinMgr::getInstance();
	int winNum = winMgr->getActiveViz();
	if (winNum < 0 || (currentMetadata == 0)){
		MessageReporter::errorMsg("%s","Export data error;\nExporting data requires loaded data and active visualizer");
		return;
	}
	//Set up arguments to Export():
	//
	AnimationParams*  p = winMgr->getAnimationParams(winNum);
	RegionParams* r = winMgr->getRegionParams(winNum);
	DvrParams* d = winMgr->getDvrParams(winNum);
	size_t currentFrame = (size_t)p->getCurrentFrameNumber();
	size_t frameInterval[2];
	size_t minCoords[3];
	size_t maxCoords[3];
	frameInterval[0] = (size_t)p->getStartFrameNumber();
	frameInterval[1] = (size_t)p->getEndFrameNumber();
	for (int i = 0; i<3; i++){
		int halfsize = r->getRegionSize(i)/2;
		minCoords[i] = (size_t)(r->getCenterPosition(i) - halfsize); 
		maxCoords[i] = (size_t)(r->getCenterPosition(i) + halfsize - 1);
		assert(minCoords[i] < 100000000);
		assert(maxCoords[i] <= (size_t)(r->getFullSize(i) -1));
	}
	
	int rc = exporter.Export(currentMetadataFile,
		currentFrame,
		d->getStdVariableName(),
		minCoords,
		maxCoords,
		frameInterval);
	if (rc < 0){
		MessageReporter::errorMsg("Export data error: \n%s", exporter.GetErrMsg());
		exporter.SetErrCode(0);
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
void Session::
resetMetadata(const char* fileBase, bool restoredSession)
{
	int i;
	//This is a flag used by renderers to avoid rendering while state
	//is changing.
	renderOK = false;
	
	bool defaultSession = (fileBase == 0);
	if (restoredSession) assert(defaultSession);
	//The metadata is created by (and obtained from) the datamgr
	if (!defaultSession) currentMetadataFile = fileBase;
	
	if (dataMgr) delete dataMgr;
	if (defaultSession){
		dataMgr = 0;
		currentMetadata = 0;
		myReader = 0;
	} else {
		dataMgr = new DataMgr(currentMetadataFile.c_str(), cacheMB, 1);
		if (dataMgr->GetErrCode() != 0) {
			MessageReporter::errorMsg("Data Loading error %d, creating Data Manager:\n %s",
				dataMgr->GetErrCode(),dataMgr->GetErrMsg());
			dataMgr->SetErrCode(0);
			delete dataMgr;
			dataMgr = 0;
			return;
		}
		currentMetadata = dataMgr->GetMetadata();
		if (currentMetadata->GetErrCode() != 0) {
			MessageReporter::errorMsg("Data Loading error %d, creating Metadata:\n %s",
				currentMetadata->GetErrCode(),currentMetadata->GetErrMsg());
			currentMetadata->SetErrCode(0);
			delete dataMgr;
			dataMgr = 0;
			return;
		}

		myReader = (WaveletBlock3DRegionReader*)dataMgr->GetRegionReader();
	} 

	//Get the extents from the metadata, if it exists:
	if (currentMetadata){
		std::vector<double> mdExtents = currentMetadata->GetExtents();
		for (i = 0; i< 6; i++)
			extents[i] = (float)mdExtents[i];
	}
	Histo::releaseHistograms();
	
	if (currentDataStatus) delete currentDataStatus;
	currentDataStatus = 0;
	
	if (!defaultSession) {
		currentDataStatus = setupDataStatus();
		
		//Is there any data here?
		if(!dataExists) {
			MessageReporter::errorMsg("%s",
				"Session: No data in specified dataset");
			delete dataMgr;
			dataMgr = 0;
			return;
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
		//Set the newSession flag, next time we'll use these settings.
		newSession = false;
	}

	//Restart the animation controller:
	AnimationController::getInstance()->restart();

	//Reset the undo/redo queue
	resetCommandQueue();
	renderOK = true;
	return;
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
DataStatus* Session::
setupDataStatus(){
	unsigned int numTimeSteps = (unsigned int)currentMetadata->GetNumTimeSteps();
	int numVariables = currentMetadata->GetVariableNames().size();
	DataStatus* ds = new DataStatus(numVariables, numTimeSteps);
	int numXForms = currentMetadata->GetNumTransforms();
	ds->setNumTransforms(numXForms);
	for (int k = 0; k< 3; k++){
		ds->setFullDataSize(k, currentMetadata->GetDimension()[k]);
	}
	//As we go through the variables and timesteps, keepTrack of min and max times
	unsigned int mints = 1000000000;
	unsigned int maxts = 0;
	dataExists = false;
	for (unsigned int ts = 0; ts< numTimeSteps; ts++){
		for (int var = 0; var< numVariables; var++){
			//Find the minimum and maximum number of transforms available on disk
			//Start at the max (lowest res) and move down to min
			int xf;
			//Find the highest transform level  (lowest resolution) that exists,
			//then the first one lower than that that doesn't exist:
			int maxXLevel = -1;
			for (xf = numXForms; xf>= 0; xf--){
				if (myReader->VariableExists(ts, 
					currentMetadata->GetVariableNames()[var].c_str(),
					xf)) {
						if (maxXLevel < 0) maxXLevel = xf;
					}
				else { //data is missing
					if (maxXLevel >= 0) break; //we have found an interval of data
				}
			}
			
			
			
			//xf is the first one that is *not* present
			xf++;
			if (maxXLevel == -1)//is there nothing at all?
				ds->setDataAbsent(var, ts);
			else {
				if (ts > maxts) maxts = ts;
				if (ts < mints) mints = ts;
				ds->setMinXFormPresent(var, ts, xf);
				ds->setMaxXFormPresent(var, ts, maxXLevel);
				dataExists = true;
			}
			//Now fill in the max and min.
			//This can be independent of whether or not the data is
			//present
			//
			// For right now, only deal with data present.
			// Absent data will get default min/max values, and will
			// not affect overall maxima/minima
			vector<double>minMax;
			if (ds->minXFormPresent(var,ts) >= 0){
				//Turn off error callback, we can handle missing datarange:
				MyBase::SetErrMsgCB(0);
				const vector<double>& mnmx = currentMetadata->GetVDataRange(ts, 
						currentMetadata->GetVariableNames()[var]);
				//Turn it back on:
				MyBase::SetErrMsgCB(errorCallbackFcn);
				
				if(mnmx.size()!= 2){
					MessageReporter::warningMsg("Missing DataRange in variable %s, at timestep %d \n Interval [0,1] assumed",
						currentMetadata->GetVariableNames()[var].c_str(), ts);
					minMax.push_back(0.);
					minMax.push_back(1.);
					MyBase::SetErrCode(0);
				}
				else{
					minMax.push_back(mnmx[0]);
					minMax.push_back(mnmx[1]);
				}
				
				if (minMax.size()>1){
					//Don't set the min or max if it's not valid.
					//It will remain at the initial value +-1.e30
					//
					ds->setDataMax(var,ts,minMax[1]);
					ds->setDataMin(var,ts,minMax[0]);
				
					if (ds->getDataMin(var,ts) < ds->getDataMinOverTime(var))
						ds->setMinimum(var,ds->getDataMin(var,ts));
					if (ds->getDataMax(var,ts) > ds->getDataMaxOverTime(var))
						ds->setMaximum(var,ds->getDataMax(var,ts));
				}
			}
		}
	}
	ds->setMinTimestep((size_t)mints);
	ds->setMaxTimestep((size_t)maxts);
	return ds;
}
//Determine min transform for *any* timestep or variable
//Needed for setting region params
//Returns -1 if no data present.
//
int DataStatus::minXFormPresent(){
	int minXForm = 10;
	for (size_t ts = minTimeStep; ts<=maxTimeStep; ts++){
		for (int varnum = 0; varnum<numVariables; varnum++){
			int minXF = minXFormPresent(varnum, (int)ts);
			if (minXF >=0 && minXF < minXForm) minXForm = minXF;
		}
	}
	if (minXForm < 10) return minXForm;
	else return -1;
}
//Determine if variable is present for *any* timestep 
//Needed for setting DVR panel
//
bool DataStatus::variableIsPresent(int varnum){
	for (size_t ts = minTimeStep; ts<=maxTimeStep; ts++){
		if (dataIsPresent(varnum, ts)) return true;
	}
	return false;
}
//Methods to keep or remove a transfer function 
//with the session.  The transFunc is always the current one from the dvrParams
//
void Session::addTF(const char* tfName, DvrParams* dvrParams){

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
	keptTFs[numTFs] = new TransferFunction(*((TransferFunction*)dvrParams->getMapperFunc()));
	tfNames[numTFs] = new std::string(tfName);
	
	//Don't retain the pointers to dvrParams and TFE:
	keptTFs[numTFs]->setParams(0);
	keptTFs[numTFs]->setEditor(0);
	numTFs++;
	return;
}
//Add a transfer function not associated with a dvrParams
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
	keptTFs[numTFs]->setEditor(0);
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
	int pos = s->findRev('\\');
	if (pos < 0) pos = s->findRev('/');
	assert (pos>= 0);
	if (pos < 0) return;
	currentTFPath = s->left(pos+1).ascii();
}
//Error callback:
void Session::
errorCallbackFcn(const char* msg, int err_code){
	QString strng("Error Code: ");
	strng += QString::number(err_code);
	strng += "\n Message: ";
	strng += msg;
	MessageReporter::warningMsg(strng.ascii());
	//Turn off error:
	MyBase::SetErrCode(0);
}
//Diagnostic message callback:
void Session::
infoCallbackFcn(const char* msg){
	MessageReporter::infoMsg("%s",msg);
}


//Here is the implementation of the DataStatus.
//This class is a repository for information about the current data...
//Whether or not it exists on disk, what's its max and min
//What resolutions are available.
//
DataStatus::
DataStatus(int numvariables, int numtimesteps)
{
	//Initially, assume all timesteps are valid:
	minTimeStep = 0;
	maxTimeStep = numtimesteps-1;
	numVariables = numvariables;
	numTimesteps = numtimesteps;
	minData = new double[numvariables*numTimesteps];
	maxData = new double[numvariables*numTimesteps];
	dataPresent = new int[numvariables*numTimesteps];
	maxXPresent = new int[numvariables*numTimesteps];
	for (int i = 0; i< numVariables*numTimesteps; i++){
		minData[i] = 1.e30;
		maxData[i] = -1.e30;
		dataPresent[i] = -1;
		maxXPresent[i] = -1;
	}
	
	
	dataRange = new float*[numVariables];
	for (int i = 0; i<numVariables; i++){
		dataRange[i] = new float[2];
		dataRange[i][0] = 1.e30f;
		dataRange[i][1] = -1.e30f;
	}
}
DataStatus::
~DataStatus(){
	if (minData) delete minData;
	if (maxData) delete maxData;
	if (dataPresent) delete dataPresent;
	if (maxXPresent) delete maxXPresent;
	if (dataRange) delete [] dataRange;
}
int DataStatus::
getFirstTimestep(int varnum){
	for (int i = 0; i< numTimesteps; i++){
		if(dataIsPresent(varnum,i)) return i;
	}
	return -1;
}
		

