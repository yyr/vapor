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
//	Description:	Implements the Session class.
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
using namespace VAPoR;
Session::Session(MainForm* mainWindow) {
	recordingCount = 0;
	dataMgr = 0;
	currentMetadata = 0;
	vizWinMgr = new VizWinMgr(mainWindow);
	mainWin = mainWindow;
	//Setup command queue:
	//
	currentQueuePos = 0;
	startQueuePos = 0;
	endQueuePos = 0;
	for (int i = 0; i<MAX_HISTORY; i++){
		commandQueue[i] = 0;
	}
	currentHistogram = 0;
}
Session::~Session(){
	delete vizWinMgr;
	if (currentMetadata) delete currentMetadata;
	if (dataMgr) delete dataMgr;
	for (int i = startQueuePos; i<= endQueuePos; i++){
		if (commandQueue[i%MAX_HISTORY]) {
			delete commandQueue[i%MAX_HISTORY];
			commandQueue[i%MAX_HISTORY] = 0;
		}
	}
}

void Session::
save(char* ){
}

void Session::
restore(char* ){
}

/**
 * create a new datamgr.  
 */
void Session::
resetMetadata(const char* fileBase)
{
	string path(fileBase);
	if (currentMetadata) delete currentMetadata;
	currentMetadata = new Metadata(path);
	if (currentMetadata->GetErrCode() != 0) {
		qWarning( "Error creating Metadata %s\n", currentMetadata->GetErrMsg());
		return;
	}
	if (dataMgr) delete dataMgr;
	//Note:  currently hardwired 512 MB cache size.  Should be made
	//A configuration option
	//
	dataMgr = new DataMgr(currentMetadata, 512, 1);

	if (dataMgr->GetErrCode() != 0) {
		qWarning( "Error creating DataMgr %s\n", dataMgr->GetErrMsg());
		return;
	}
	//Notify all params that there is new data:
	vizWinMgr->reinitializeParams();
	/* do this in reinit:
	//Setup the global region parameters based on bounds in Metadata
	unsigned int nlevels;
	int nx, ny, nz;

	//Set default data range!
	
	dm->SetScale(0.f, 1.f);

	nlevels = dataMgr->GetFLevels();
	dataMgr->GetDimensionInVoxels(nlevels, &nx, &ny, &nz);
	//get the global region params:
	RegionParams* rParams = vizWinMgr->getRegionParams(-1);
	rParams->setMaxSize(MAX(MAX(nx, ny), nz));
	rParams->setFullSize(0, nx);
	rParams->setFullSize(1, ny);
	rParams->setFullSize(2, nz);

	rParams->setRegionSize(0, nx);
	rParams->setRegionSize(1, ny);
	rParams->setRegionSize(2, nz);
	
	rParams->setCenterPosition(0, nx/2);
	rParams->setCenterPosition(1, ny/2);
	rParams->setCenterPosition(2, nz/2);
	
	rParams->setMaxStride(nlevels+1);
	rParams->setStride(nlevels+1);
	//This will force the dialog to be reset to values
	//consistent with the data.
	//
	rParams->updateDialog();
	int nbx, nby, nbz;
	dataMgr->GetDimensionInBlocks(0, &nbx, &nby, &nbz);
	
	
		
    */
	//currentHistogram = new Histo((unsigned char*)dataMgr->GetRegion(
	//	0, 0, 0, 0, nbx>>1, nby>>1, nbz>>1, 0),
	//	(nbx*nby*nbz)<<15);
	resetCommandQueue();
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

	mainWin->editUndoAction->setEnabled(true);
	mainWin->editRedoAction->setEnabled(false);
}
void Session::backupQueue(){
	//Make sure we can do it!
	//
	assert(currentQueuePos > startQueuePos);
	commandQueue[currentQueuePos%MAX_HISTORY]->unDo();
	currentQueuePos--;
	mainWin->editRedoAction->setEnabled(true);
	if (currentQueuePos == startQueuePos) 
		mainWin->editUndoAction->setEnabled(false);
}
void Session::advanceQueue(){
	//Make sure we can do it:
	//
	assert(currentQueuePos < endQueuePos);
	//perform the next command
	//
	commandQueue[(++currentQueuePos)%MAX_HISTORY]->reDo();
	mainWin->editUndoAction->setEnabled(true);
	if (currentQueuePos == endQueuePos) 
		mainWin->editRedoAction->setEnabled(false);
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
}

