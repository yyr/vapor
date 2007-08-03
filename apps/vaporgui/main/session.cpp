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
#include "session.h"
#include "dvrparams.h"
#include "ParamsIso.h"
#include "vapor/DataMgr.h"
#include "vizwinmgr.h"
#include "mainform.h"
#include "command.h"
#include "messagereporter.h"
#include <qapplication.h>
#include <qcursor.h>
#include <qaction.h>
#include <qcheckbox.h>
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
#include "floweventrouter.h"
#include <qstring.h>
#include "tabmanager.h"

using namespace VAPoR;
Session* Session::theSession = 0;
const string Session::_stretchFactorsAttr = "StretchFactors";
const string Session::_cacheSizeAttr = "CacheSize";
const string Session::_jpegQualityAttr = "JpegQuality";
const string Session::_metadataPathAttr = "MetadataPath";
const string Session::_transferFunctionPathAttr = "TransferFunctionPath";
const string Session::_imageCapturePathAttr = "JpegPath";
const string Session::_flowDirectoryPathAttr = "FlowPath";

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
Session::Session() {
	
	MyBase::SetErrMsgCB(errorCallbackFcn);
	MyBase::SetDiagMsgCB(infoCallbackFcn);
	previousClass = 0;
	dataMgr = 0;
	currentMetadata = 0;
	cacheMB = 1024;
	stretchFactors[0] = stretchFactors[1] = stretchFactors[2] = 1.f;
	visualizeSpherically = false;
	
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
	delete AnimationController::getInstance();
	delete VizWinMgr::getInstance();
	//Note: metadata is deleted by Datamgr
	if (dataMgr) delete dataMgr;
	DataStatus::removeMetadataVars();
	
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
		delete keptTFs;
		delete tfNames;
		
	}
	
}
//Set session state to base values:
//
void Session::init() {
	int i;
	recordingCount = 0;
	metadataSaved = false;

	currentMetadataFile = "";
	currentJpegDirectory = "";	
	currentFlowDirectory = "";
	currentTFPath = "";
	stretchFactors[0] = stretchFactors[1] = stretchFactors[2] = 1.f;
	visualizeSpherically = false;
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
	
	
	numTFs = 0;
	tfNames = 0;
	keptTFs = 0;
	tfListSize = 0;
	
	GLWindow::setJpegQuality(100);
	dataExists = false;
	
	newSession = true;
	extents[0] = extents[1] = extents[2] = 0.f;
	extents[3] = extents[4] = extents[5] = 1.f;
	stretchedExtents[0] = stretchedExtents[1] = stretchedExtents[2] = 0.f;
	stretchedExtents[3] = stretchedExtents[4] = stretchedExtents[5] = 1.f;
	DataStatus::removeMetadataVars();
	
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
	oss << (long)GLWindow::getJpegQuality();
	attrs[_jpegQualityAttr] = oss.str();
	

	attrs[_metadataPathAttr] = currentMetadataFile;
	attrs[_transferFunctionPathAttr] = currentTFPath;
	attrs[_imageCapturePathAttr] = currentJpegDirectory;
	attrs[_flowDirectoryPathAttr] = currentFlowDirectory;
	attrs[_exportFileNameAttr] = currentExportFile;
	MessageReporter* msgRpt = MessageReporter::getInstance();
	attrs[_logFileNameAttr] = getLogfileName();
	oss.str(empty);
	oss << (long)msgRpt->getMaxPopup(MessageReporter::Info) << " " <<
		(long)msgRpt->getMaxPopup(MessageReporter::Warning) << " " <<
		(long)msgRpt->getMaxPopup(MessageReporter::Error);
	attrs[_maxPopupAttr] = oss.str();
	oss.str(empty);
	oss << (double)stretchFactors[0] << " " << (double)stretchFactors[1] << " " << (double)stretchFactors[2];
	attrs[_stretchFactorsAttr] = oss.str();
	oss.str(empty);
	oss << extents[0]<<" "<<extents[1]<<" "<<extents[2]<<" "<<extents[3]<<" "<<extents[4]<<" "<<extents[5];
	attrs[_dataExtentsAttr] = oss.str();
	
	oss.str(empty);
	oss << (long)msgRpt->getMaxLog(MessageReporter::Info) << " "
		<< (long)msgRpt->getMaxLog(MessageReporter::Warning) << " "
		<< (long)msgRpt->getMaxLog(MessageReporter::Error);
	attrs[_maxLogAttr] = oss.str();

	int numSesVars = getNumSessionVariables();
	XmlNode* mainNode = new XmlNode(_sessionTag, attrs, numSesVars+numTFs+2);

	//Now add children:  One for all the saved transfer functions,
	//One for each of the session variables,
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
	for (int i = 0; i< numSesVars; i++){
		std::map <string, string> attrs;
		attrs.clear();
		ostringstream oss;
		oss.str(empty);
		oss << getVariableName(i);
		attrs[_variableNameAttr] = oss.str();
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
	XmlNode* globalPanels = mainNode->NewChild(_globalParameterPanelsTag, attrs, 5);
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//Have the global parameters populate this:
	XmlNode* rgNode = vizMgr->getGlobalParams(Params::RegionParamsType)->buildNode();
	if(rgNode) globalPanels->AddChild(rgNode);
	XmlNode* animNode = vizMgr->getGlobalParams(Params::AnimationParamsType)->buildNode();
	if(animNode) globalPanels->AddChild(animNode);
	XmlNode* vpNode = vizMgr->getGlobalParams(Params::ViewpointParamsType)->buildNode();
	if (vpNode) globalPanels->AddChild(vpNode);
	//have vizwinmgr populate the vizwin nodes
	mainNode->AddChild(vizMgr->buildNode());
	
	return mainNode;
}

bool Session::
loadFromFile(ifstream& ifs){
	//Call resetMetadata to clean stuff out
	resetMetadata(0,true);
	//Reset message counts:
	MessageReporter::getInstance()->resetCounts();
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
				else if (StrCmpNoCase(attr, _stretchFactorsAttr) == 0) {
					ist >> stretchFactors[0];
					ist >> stretchFactors[1];
					ist >> stretchFactors[2];
				}
				else if (StrCmpNoCase(attr, _jpegQualityAttr) == 0) {
					int qual;
					ist >> qual;
					GLWindow::setJpegQuality(qual);
				}
				else if (StrCmpNoCase(attr, _transferFunctionPathAttr) == 0) {
					ist >> currentTFPath;
				}
				else if (StrCmpNoCase(attr, _imageCapturePathAttr) == 0) {
					ist >> currentJpegDirectory;
				}
				else if (StrCmpNoCase(attr, _flowDirectoryPathAttr) == 0) {
					ist >> currentFlowDirectory;
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
			//After parsing all session attr's, set the stretched Extents:
			for (int i = 0; i<3; i++){
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
					else return false;
				}
				if (varName == "") return false;
				int varnum = mergeVariableName(varName);
				DataStatus::getInstance()->setOutsideValues(varnum, belowVal, aboveVal);
				return true;
			}
			return false;

		case(2):
			//parse grandchild tags
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
			} else if (StrCmpNoCase(tag, Params::_isoParamsTag) == 0){
				//Need to "push" to iso parser.
				//That parser will "pop" back to session when done.
				tempParsedPanel = new ParamsIso(0, -1);
				pm->pushClassStack(tempParsedPanel);
				tempParsedPanel->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else if (StrCmpNoCase(tag, Params::_probeParamsTag) == 0){
				//Need to "push" to probe parser.
				//That parser will "pop" back to session when done.
				tempParsedPanel = new ProbeParams(-1);
				pm->pushClassStack(tempParsedPanel);
				tempParsedPanel->elementStartHandler(pm, depth, tag, attrs);
				return true;
			} else if (StrCmpNoCase(tag, Params::_regionParamsTag) == 0){
				//Need to "push" to region parser.
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
			} 
			else return false;
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
			if (StrCmpNoCase(tag, _sessionVariableTag) == 0) return true;
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
				// just ignore it
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,Params::DvrParamsType);
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_isoParamsTag) == 0){
				// just ignore it
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,Params::IsoParamsType);
				tempParsedPanel = 0;
				return true;
			} else if (StrCmpNoCase(tag, Params::_probeParamsTag) == 0){
				// just ignore it
				assert(tempParsedPanel);
				vizWinMgr->replaceGlobalParams(tempParsedPanel,Params::ProbeParamsType);
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
				// just ignore it
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
	//Make sure the current vdf is version 2.0 or greater:
	
	ImpExp exporter;
	//Note:  will export data associated with current active visualizer
	VizWinMgr* winMgr = VizWinMgr::getInstance();
	int winNum = winMgr->getActiveViz();
	if (winNum < 0 || (currentMetadata == 0)){
		MessageReporter::errorMsg("%s","Export data error;\nExporting data requires loaded data and active visualizer");
		return;
	}
	int ver = currentMetadata->GetVDFVersion();
	if (ver < 2) {
		MessageReporter::errorMsg("Export of pre-version-2 metadata not supported");
		return;
	}
	//Set up arguments to Export():
	//
	AnimationParams*  p = winMgr->getAnimationParams(winNum);
	RegionParams* r = winMgr->getRegionParams(winNum);
	DvrParams* dParams = winMgr->getDvrParams(winNum);
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
	r->getRegionVoxelCoords(numxforms, mncrds, mxcrds, minbdim, maxbdim);
	for (int i = 0; i< 3; i++) {
		minCoords[i] = mncrds[i];
		maxCoords[i] = mxcrds[i];
	}

	
	int rc = exporter.Export(currentMetadataFile,
		currentFrame,
		getVariableName(dParams->getSessionVarNum()),
		minCoords,
		maxCoords,
		frameInterval);
	if (rc < 0){
		MessageReporter::errorMsg("Export data error: \n%s", exporter.GetErrMsg());
		exporter.SetErrCode(0);
	} else {
		MessageReporter::warningMsg("Exported time step %d of region in %s .\nNote: recently imported variables may not be exported",
			currentFrame, VizWinMgr::getInstance()->getVizWinName(winNum).ascii());
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
resetMetadata(const char* fileBase, bool restoredSession, bool doMerge, int mergeOffset)
{
	int i;
	//This is a flag used by renderers to avoid rendering while state
	//is changing.
	if (DataStatus::getInstance())
		DataStatus::getInstance()->setRenderReady(false);
	
	bool defaultSession = (fileBase == 0);
	//if (restoredSession) assert(defaultSession);
	//The metadata is created by (and obtained from) the datamgr
	//Don't update the currentMetadataFile if we are doing a merge
	if (!defaultSession && !doMerge) currentMetadataFile = fileBase;
	
	//If we don't already have a dataMgr, we can't really be merging:
	if (!dataMgr) {
		if (doMerge) {doMerge = false; }
	}
	else { delete dataMgr; dataMgr = 0;}
	if (doMerge) assert (!defaultSession);
	
	//Handle the various cases of loading the metadata
	if (defaultSession){
		currentMetadata = 0;
		DataStatus::clearVariableNames();
	} else {
		if (!doMerge) {
			if (!restoredSession) DataStatus::clearVariableNames();
			dataMgr = new DataMgr(currentMetadataFile.c_str(), cacheMB, 1);
			if (dataMgr->GetErrCode() != 0) {
				MessageReporter::errorMsg("Data Loading error %d, creating Data Manager:\n %s",
					dataMgr->GetErrCode(),dataMgr->GetErrMsg());
				dataMgr->SetErrCode(0);
				delete dataMgr;
				dataMgr = 0;
				return false;
			}
		} else {//merge
			//keep dataMgr, do a merge:
			//Need a non-const pointer to the metadata, since we will modify it:
			Metadata* md= (Metadata*)currentMetadata;
			size_t offset = (size_t) mergeOffset;
			//Use the fileBase, not the currentMetadataFile for the merge.
			int rc = md->Merge(fileBase,offset);
			if (rc < 0) return false;
			dataMgr = new DataMgr(currentMetadata, cacheMB, 1);
		}
		
		currentMetadata = dataMgr->GetMetadata();
		if (currentMetadata->GetErrCode() != 0) {
			MessageReporter::errorMsg("Data Loading error %d, creating Metadata:\n %s",
				currentMetadata->GetErrCode(),currentMetadata->GetErrMsg());
			currentMetadata->SetErrCode(0);
			delete dataMgr;
			dataMgr = 0;
			return false;
		}

		
	} 

	//Get the extents from the metadata, if it exists:
	if (currentMetadata){
		std::vector<double> mdExtents = currentMetadata->GetExtents();
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
			MessageReporter::errorMsg("%s",
				"Session: No data in specified dataset, or data in specified files cannot be read");
			delete dataMgr;
			dataMgr = 0;
			return false;
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
		//currentDataStatus->fillMetadataVars();
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
	keptTFs[numTFs] = new TransferFunction(*((TransferFunction*)params->getMapperFunc()));
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
	int pos = s->findRev('\\');
	if (pos < 0) pos = s->findRev('/');
	assert (pos>= 0);
	if (pos < 0) return;
	currentTFPath = s->left(pos+1).ascii();
}
//Error callback:
void Session::
errorCallbackFcn(const char* msg, int err_code){
	QString strng("Code: ");
	strng += QString::number(err_code,16);
	strng += "\n Message: ";
	strng += msg;
	if (err_code & 0x2000){
		MessageReporter::warningMsg(strng.ascii());
	} else if (err_code & 0x4000){
		MessageReporter::errorMsg(strng.ascii());
	} else if (err_code & 0x8000){
		MessageReporter::fatalMsg(strng.ascii());
	} else {
		MessageReporter::errorMsg((QString("Unclassified error: ")+strng).ascii());
	}
	
	MainForm::getInstance()->getTabManager()->getFrontEventRouter()->updateUrgentTabState();
	//Turn off error:
	MyBase::SetErrCode(0);
}
//Diagnostic message callback:
void Session::
infoCallbackFcn(const char* msg){
	MessageReporter::infoMsg("%s",msg);
}



