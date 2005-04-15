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
#include "session.h"
#include "vapor/DataMgr.h"
#include "vizwinmgr.h"
#include "mainform.h"
#include "command.h"
#include "messagereporter.h"
#include <qaction.h>
#include <cassert>
#include <cstring>
#include "histo.h"
#include "vapor/Metadata.h"
#include "vapor/ImpExp.h"
#include "animationcontroller.h"
#include "transferfunction.h"
#include <qstring.h>

using namespace VAPoR;
Session* Session::theSession = 0;
Session::Session() {
	MyBase::SetErrMsgCB(errorCallbackFcn);
	MyBase::SetDiagMsgCB(infoCallbackFcn);
	recordingCount = 0;
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
	currentHistograms = 0;
	currentDataStatus = 0;
#ifdef IRIX
	cacheMB = 1024;
#else
	cacheMB = 512;
#endif
	renderOK = false;
	numTFs = 0;
	tfNames = 0;
	keptTFs = 0;
	tfListSize = 0;
	leftBounds = 0;
	rightBounds = 0;
	tfFilePath = new QString(".");
	currentMetadataPath = 0;
	jpegQuality = 75;
	dataExists = false;
}
Session::~Session(){
	int i;
	
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
	if (currentHistograms){
		int numVars = currentDataStatus->getNumVariables();
		for (int j = 0; j<numVars; j++){
			if (currentHistograms[j]) delete currentHistograms[j];
		}
		delete currentHistograms;
	}
	if(currentDataStatus) delete currentDataStatus;

	//Delete all the saved transfer functions:
	for (i = 0; i<numTFs; i++){
		delete keptTFs[i];
		delete tfNames[i];
	}
	if (tfListSize>0){
		delete keptTFs;
		delete tfNames;
		delete rightBounds;
		delete leftBounds;
	}
	if(currentMetadataPath) delete currentMetadataPath;
}

void Session::
save(char* ){
}

void Session::
restore(char* ){
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
		MessageReporter::errorMsg("Export data error;\nExporting data requires loaded data and active visualizer");
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
	
	int rc = exporter.Export(*currentMetadataPath,
		currentFrame,
		d->getStdVariableName(),
		minCoords,
		maxCoords,
		frameInterval);
	if (rc < 0){
		QString strng("Export data error: \n%s");
		strng += exporter.GetErrMsg();
		MessageReporter::errorMsg(strng.ascii());
	}
	return;
}
/**
 * create a new datamgr.  Also perform related functions such as
 * constructing histograms 
 */
void Session::
resetMetadata(const char* fileBase)
{
	int i;
	renderOK = false;
	
	//The metadata is created by (and obtained from) the datamgr
	currentMetadataPath = new string(fileBase);
	
	if (dataMgr) delete dataMgr;
	dataMgr = new DataMgr(currentMetadataPath->c_str(), cacheMB, 1);
	if (dataMgr->GetErrCode() != 0) {
		QString strng("Data Loading error, creating Data Manager:\n");
		strng += dataMgr->GetErrMsg();
		MessageReporter::errorMsg(strng.ascii());
		delete dataMgr;
		dataMgr = 0;
		return;
	}
	currentMetadata = dataMgr->GetMetadata();
	if (currentMetadata->GetErrCode() != 0) {
		QString strng("Data Loading error, creating Metadata:\n");
		strng += currentMetadata->GetErrMsg();
		MessageReporter::errorMsg(strng.ascii());
		delete dataMgr;
		dataMgr = 0;
		return;
	}

	myReader = (WaveletBlock3DRegionReader*)dataMgr->GetRegionReader();

	//If histograms exist, delete them:
	if (currentHistograms && currentDataStatus){
		for (i = 0; i< currentDataStatus->getNumVariables(); i++){
			if (currentHistograms[i]){
				delete currentHistograms[i];
				currentHistograms[i] = 0;
			}
		}
		delete currentHistograms;
		currentHistograms = 0;
	}
	
	if (currentDataStatus) delete currentDataStatus;
	currentDataStatus = setupDataStatus();

	//Is there any data here?
	if(!dataExists) {
		MessageReporter::errorMsg("No data in specified dataset");
		delete dataMgr;
		dataMgr = 0;
		return;
	}
	//Now create histograms for all the variables present.
	
	
	//Initially just use the first (valid)time step.
	int numVars = currentDataStatus->getNumVariables();
	currentHistograms = new Histo*[numVars];
	
	for (i = 0; i<numVars; i++){
		currentHistograms[i] = 0;
		//Tell the datamanager to use the overall max/min range
		//In doing the quantization.  Note that this will change
		//when the range is changed in the TFE
		//
		setMappingRange(i, currentDataStatus->getDataRange(i));
		float dataMin = currentDataStatus->getDataRange(i)[0];
		float dataMax = currentDataStatus->getDataRange(i)[1];
		//Obtain data dimensions for getting histogram:
		size_t min_bdim[3];
		size_t max_bdim[3];
		int min_dim[3], max_dim[3];
		
		size_t subDataSize[3];
		myReader->GetDim(currentDataStatus->getNumTransforms(),subDataSize);
		myReader->GetDimBlk(currentDataStatus->getNumTransforms(),max_bdim);
		for (int k = 0; k<3; k++){
			min_bdim[k] = 0;
			max_bdim[k]--;
			//Setup for full subarray:
			min_dim[k] = 0;
			max_dim[k] = subDataSize[k];
		}
		
		//Find the first timestep for which there is data,
		//Build a histogram , for this variable, on that data
		
		for (int ts = 0; ts< currentDataStatus->getNumTimesteps(); ts++){
			if (currentDataStatus->dataIsPresent(i,ts)){
				currentHistograms[i] = new Histo(
					(unsigned char*) dataMgr->GetRegionUInt8(
						ts, (const char*) currentMetadata->GetVariableNames()[i].c_str(),
						currentDataStatus->getNumTransforms(),
						min_bdim,
						max_bdim,
						0 //Don't lock!
					), 
				min_dim, max_dim, min_bdim, max_bdim,
				dataMin, dataMax
				);
				break;//stop after first successful construction
			}
		}
	}
	VizWinMgr* myVizWinMgr = VizWinMgr::getInstance();
	//Notify all params that there is new data:
	myVizWinMgr->reinitializeParams();
	//Delete all visualizers, then create one new one.
	
	for (i = 0; i< MAXVIZWINS; i++){
		if (myVizWinMgr->getVizWin(i)){
			myVizWinMgr->killViz(i);
		}
	}
	
	
	myVizWinMgr->launchVisualizer();
	myVizWinMgr->updateActiveParams();

	//Restart the animation controller:
	
	AnimationController::getInstance()->restart();

	//Reset the undo/redo queue
	resetCommandQueue();
	renderOK = true;
	return;
}
//Rebuild a specific histogram
//
void Session::
refreshHistogram(int varNum, RegionParams* rParams, int timestep, float dataMin, float dataMax)
{
	float extents[6], minFull[3], maxFull[3];
	int min_dim[3],max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	//size_t subDataSize[3];
	int numTrans = rParams->getNumTrans();
	//myReader->GetDim(numTrans,subDataSize);
	if (currentHistograms[varNum]) delete currentHistograms[varNum];
	rParams->calcRegionExtents(min_dim, max_dim, min_bdim, max_bdim, numTrans, minFull, maxFull, extents);
	
	currentHistograms[varNum] = new Histo((unsigned char*) dataMgr->GetRegionUInt8(
					timestep, (const char*) currentMetadata->GetVariableNames()[varNum].c_str(),
					numTrans,
					min_bdim, max_bdim,
					0 //Don't lock!
				), 
			min_dim, max_dim, min_bdim, max_bdim, dataMin, dataMax
		);

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
			//Find the minimum number of transforms available on disk
			//Start at the max (lowest res) and move down to min
			int xf;
			for (xf = numXForms; xf>= 0; xf--){
				//find the highest transform level that doesn't exist
				
				
				if (!myReader->VariableExists(ts, 
					currentMetadata->GetVariableNames()[var].c_str(),
					xf)) break;
			}
			//If xf == numXForms, there's no data
			
			
			//xf is the first one that is *not* present
			xf++;
			if (xf > numXForms)
				ds->setDataAbsent(var, ts);
			else {
				if (ts > maxts) maxts = ts;
				if (ts < mints) mints = ts;
				ds->setMinXFormPresent(var, ts, xf);
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
			if (ds->dataIsPresent(var, ts)){
				//Turn off error callback, we can handle missing datarange:
				MyBase::SetErrMsgCB(0);
				const vector<double>& mnmx = currentMetadata->GetVDataRange(ts, 
						currentMetadata->GetVariableNames()[var]);
				//Turn it back on:
				MyBase::SetErrMsgCB(errorCallbackFcn);
				
				if(mnmx.size()!= 2){
					MessageReporter::warningMsg("Missing DataRange in dataset; [0,1] assumed");
					minMax.push_back(0.);
					minMax.push_back(1.);
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
	for (size_t ts = minTimeStep; ts<maxTimeStep; ts++){
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
//with the session:
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
		float* tempLeftBounds = new float[tfListSize];
		float* tempRightBounds = new float[tfListSize];
		for (int i = 0; i<numTFs; i++){
			tempTFHolder[i] = keptTFs[i];
			tempTFNames[i] = tfNames[i];
			tempLeftBounds[i] = leftBounds[i];
			tempRightBounds[i] = rightBounds[i];
			delete tfNames[i];
			delete keptTFs[i];
		}
		keptTFs = tempTFHolder;
		tfNames = tempTFNames;
		leftBounds = tempLeftBounds;
		rightBounds = tempRightBounds;
	}
	//copy the tf, its name, and domain bounds
	keptTFs[numTFs] = new TransferFunction(*(dvrParams->getTransferFunction()));
	tfNames[numTFs] = new std::string(tfName);
	leftBounds[numTFs] = dvrParams->getMinMapBound();
	rightBounds[numTFs] = dvrParams->getMaxMapBound();
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
		leftBounds[j] = leftBounds[j+1];
		rightBounds[j] = rightBounds[j+1];
	}
	numTFs--;
	return true;

}
//Method to get a transfer function from session.  Clones the one
//that is saved in the session.
TransferFunction* Session::
getTF(const std::string* name, float *leftLim, float* rightLim){
	//See if the string is in the list:
	int i;
	for (i = 0; i< numTFs; i++){
		if (!tfNames[i]->compare(*name)) break;
	}
	if (i >= numTFs) return 0;
	TransferFunction* tf = new TransferFunction(*(keptTFs[i]));
	if (leftLim) *leftLim = leftBounds[i];
	if (rightLim) *rightLim = rightBounds[i];
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
	*tfFilePath = s->left(pos+1);
}
//Error callback:
void Session::
errorCallbackFcn(const char* msg, int err_code){
	QString strng("Error Code: ");
	strng += QString::number(err_code);
	strng += "\n Message: ";
	strng += msg;
	MessageReporter::warningMsg(strng.ascii());
}
//Diagnostic message callback:
void Session::
infoCallbackFcn(const char* msg){
	MessageReporter::infoMsg(msg);
}


//Here is the implementation of the DataStatus.
//This class is a repository for information about the current data...
//Whether or not it exists on disk, what's its max and min
//What resolutions are available.
//
DataStatus::
DataStatus(int numvariables, int numtimesteps)
{
	numVariables = numvariables;
	numTimesteps = numtimesteps;
	minData = new double[numvariables*numTimesteps];
	maxData = new double[numvariables*numTimesteps];
	dataPresent = new int[numvariables*numTimesteps];
	for (int i = 0; i< numVariables*numTimesteps; i++){
		minData[i] = 1.e30;
		maxData[i] = -1.e30;
		dataPresent[i] = -1;
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
	if (dataRange) delete [] dataRange;
}

		

