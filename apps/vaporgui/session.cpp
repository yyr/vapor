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
#include <qaction.h>
#include <cassert>
#include <cstring>
#include "histo.h"
#include "vapor/Metadata.h"
#include "animationcontroller.h"
using namespace VAPoR;
Session* Session::theSession = 0;
Session::Session() {
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
	cacheMB = 512;
	//AnimationController::getInstance()->run();
}
Session::~Session(){
	delete VizWinMgr::getInstance();
	delete AnimationController::getInstance();
	//Note: metadata is deleted by Datamgr
	if (dataMgr) delete dataMgr;
	for (int i = startQueuePos; i<= endQueuePos; i++){
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
}

void Session::
save(char* ){
}

void Session::
restore(char* ){
}

/**
 * create a new datamgr.  Also perform related functions such as
 * constructing histograms and 
 */
void Session::
resetMetadata(const char* fileBase)
{
	//Reinitialize the animation controller:
	AnimationController::getInstance()->restart();
	//The metadata is created by (and obtained from) the datamgr
	string path(fileBase);
	if (dataMgr) delete dataMgr;
	dataMgr = new DataMgr(fileBase, cacheMB, 1);
	
	currentMetadata = dataMgr->GetMetadata();
	if (currentMetadata->GetErrCode() != 0) {
		qWarning( "Error creating Metadata %s\n", currentMetadata->GetErrMsg());
		return;
	}
	
	if (dataMgr->GetErrCode() != 0) {
		qWarning( "Error creating DataMgr %s\n", dataMgr->GetErrMsg());
		return;
	}

	myReader = (WaveletBlock3DRegionReader*)dataMgr->GetRegionReader();

	resetCommandQueue();
	if (currentDataStatus) delete currentDataStatus;
	currentDataStatus = setupDataStatus();

	//Now create histograms for all the variables present.
	//Initially just use the first (valid)time step.
	int numVars = currentDataStatus->getNumVariables();
	currentHistograms = new Histo*[numVars];
	for (int i = 0; i<numVars; i++){
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
		size_t bs = currentMetadata->GetBlockSize();
		int fullDataSize = 1;
		for (int k = 0; k<3; k++){
			fullDataSize *= currentDataStatus->getFullDataSize(k);
			min_bdim[k] = 0;
			max_bdim[k] = (currentDataStatus->getFullDataSize(k)>>currentDataStatus->getNumTransforms())/bs -1;
		}
		//Initialize the histograms array to null.
		currentHistograms[i] = 0;
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
				fullDataSize>>(3*currentDataStatus->getNumTransforms()),
				dataMin, dataMax
				);
				break;//stop after first successful construction
			}
		}
	}

	//Notify all params that there is new data:
	VizWinMgr::getInstance()->reinitializeParams();
	//Then make the tab panels refresh:
	VizWinMgr::getInstance()->updateActiveParams();
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
	int numTimeSteps = currentMetadata->GetNumTimeSteps()[0];
	int numVariables = currentMetadata->GetVariableNames().size();
	DataStatus* ds = new DataStatus(numVariables, numTimeSteps);
	int numXForms = currentMetadata->GetNumTransforms();
	ds->setNumTransforms(numXForms);
	for (int k = 0; k< 3; k++){
		ds->setFullDataSize(k, currentMetadata->GetDimension()[k]);
	}
	//As we go through the variables and timesteps, keepTrack of min and max times
	minTimeStep = 1000000000;
	maxTimeStep = 0;
	for (size_t ts = 0; ts<(size_t)numTimeSteps; ts++){
		for (int var = 0; var< numVariables; var++){
			//Find the minimum number of transforms available on disk
			//Start at the max (lowest res) and move down to min
			int xf;
			for (xf = numXForms; xf>= 0; xf--){
				//If it's not present, we can quit
				
				
				if (!myReader->VariableExists(ts, 
					currentMetadata->GetVariableNames()[var].c_str(),
					xf)) break;
			}
			//Aha, there is some data for some variable at this timestep.
			if (ts > maxTimeStep) maxTimeStep = ts;
			if (ts < minTimeStep) minTimeStep = ts;
			//xf is the first one that is *not* present
			xf++;
			if (xf > numXForms)
				ds->setDataAbsent(var, ts);
			else
				ds->setMinXFormPresent(var, ts, xf);
			//Now fill in the max and min.
			//This can be independent of whether or not the data is
			//present
			//
			// For right now, only deal with data present.
			// Absent data will get default min/max values, and will
			// not affect overall maxima/minima
			if (ds->dataIsPresent(var, ts)){
				const vector<double>& minMax = currentMetadata->GetVDataRange(ts, 
						currentMetadata->GetVariableNames()[var]);
				
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

	return ds;
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

		

