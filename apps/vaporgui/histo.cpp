//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		histo.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:  Implementation of Histo class 
//
#include "histo.h"
#include "session.h"
#include "regionparams.h"
#include "dvrparams.h"
#include "animationparams.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "vapor/Metadata.h"
#include "messagereporter.h"
using namespace VAPoR;
int Histo::histoArraySize = 0;
Histo** Histo::histoArray = 0;

Histo::Histo(int numberBins, float mnData, float mxData){
	numBins = numberBins;
	minData = mnData;
	maxData = mxData;
	binArray = new int[numBins];
	reset();
}
Histo::Histo(unsigned char* data, int min_dim[3], int max_dim[3], 
			 size_t min_bdim[3],size_t max_bdim[3],
			 float mnData, float mxData){
	binArray = new int[256];
	minData = mnData;
	maxData = mxData;
	numBins = 256;
	reset();
	int bs = Session::getInstance()->getCurrentMetadata()->GetBlockSize();
	// make subregion origin (0,0,0)
	// Note that this doesn't affect the calc of nx,ny,nz.
	//
	for(int i=0; i<3; i++) {
		while(min_bdim[i] > 0) {
			min_dim[i] -= bs; max_dim[i] -= bs;
			min_bdim[i] -= 1; max_bdim[i] -= 1;
		}
	}
		
	int nx = (max_bdim[0] - min_bdim[0] + 1) * bs;
	int ny = (max_bdim[1] - min_bdim[1] + 1) * bs;
	//int nz = (max_bdim[2] - min_bdim[2] + 1) * bs;
	int ix, iy, iz;
	for (ix = min_dim[0]; ix <= max_dim[0]; ix++){
		for (iy = min_dim[1]; iy <= max_dim[1]; iy++) {
			for (iz = min_dim[2]; iz <= max_dim[2]; iz++) {
				int val = (int)data[ix+nx*(iy+ny*iz)];
				binArray[val]++;
				if (val > 0 && val < 255 && binArray[val] > maxBinSize) {
					maxBinSize = binArray[val];
					largestBin = val;
				}
			}
		}
	}
}
	
Histo::~Histo(){
	delete [] binArray;
}
void Histo::reset(int newNumBins) {
	if (newNumBins != -1) {
		numBins = newNumBins;
		delete [] binArray;
		binArray = new int[numBins];
	}
	for (int i = 0; i< numBins; i++) binArray[i] = 0;
	numBelow = 0;
	numAbove = 0;
	maxBinSize = 0;
	largestBin = -1;
}
void Histo::addToBin(float val) {
	if (val < minData) numBelow++; 
	else if (val > maxData) numAbove++;
	else {
		int intVal = (int)((val - minData)*(float)numBins/(maxData - minData));
		if (intVal == numBins) intVal--;
		binArray[intVal]++;
		if (binArray[intVal] > maxBinSize) {
			maxBinSize = binArray[intVal];
			largestBin = intVal;
		}
	}
}
//Static stuff to maintain "cache" of histograms:
void Histo::releaseHistograms(){
	if (!histoArray) return;
	for (int i = 0; i<histoArraySize; i++) {
		if (histoArray[i]){
			delete histoArray[i];
			histoArray[i] = 0;
		}
	}
	delete histoArray;
	histoArray = 0;
	histoArraySize = 0;
}
	
Histo* Histo::
getHistogram(int varNum, int vizNum){
	assert (vizNum >=0 && varNum >= 0);
	//Check if there exists a histogram array:
	if (!histoArray || histoArraySize <= 0){
		int numVars = Session::getInstance()->getNumVariables();
		if (numVars <= 0) return 0;
		histoArray = new Histo*[numVars*MAXVIZWINS];
		histoArraySize = numVars*MAXVIZWINS;
		for (int j = 0; j<histoArraySize; j++) histoArray[j] = 0;
	}
	
	if (!histoArray[varNum*MAXVIZWINS + vizNum]) {
		//Need to construct one:
		refreshHistogram(vizNum);
	}
	return histoArray[varNum*MAXVIZWINS + vizNum];
}
// Force the construction of a new histogram, valid for a particular visualizer
// It will be saved in the cache
void Histo::
refreshHistogram(int vizNum)
{
	float extents[6], minFull[3], maxFull[3];
	int min_dim[3],max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	assert (vizNum >= 0);
	assert (histoArraySize > 0);
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	DvrParams* dParams = vizWinMgr->getDvrParams(vizNum);
	int varNum = dParams->getVarNum();
	if (histoArray[varNum*MAXVIZWINS + vizNum]){
		delete histoArray[varNum*MAXVIZWINS + vizNum];
		histoArray[varNum*MAXVIZWINS + vizNum] = 0;
	}
	RegionParams* rParams = vizWinMgr->getRegionParams(vizNum);
	int numTrans = rParams->getNumTrans();
	int timeStep = vizWinMgr->getAnimationParams(vizNum)->getCurrentFrameNumber();
	float dataMin = dParams->getMinMapBound();
	float dataMax = dParams->getMaxMapBound();
	
	rParams->calcRegionExtents(min_dim, max_dim, min_bdim, max_bdim, numTrans, minFull, maxFull, extents);
	DataMgr* dataMgr = Session::getInstance()->getDataMgr();
	assert (dataMgr);
	const Metadata* metaData = Session::getInstance()->getCurrentMetadata();
	
	vizWinMgr->getVizWin(vizNum)->setDataRangeDirty(false);
	
	unsigned char* data = (unsigned char*) dataMgr->GetRegionUInt8(
					timeStep, (const char*) metaData->GetVariableNames()[varNum].c_str(),
					numTrans,
					min_bdim, max_bdim,
					dParams->getCurrentDatarange(),
					0 //Don't lock!
				);
	//Make sure we can build a histogram
	if (!data) {
		MessageReporter::errorMsg("Invalid/nonexistent data cannot be histogrammed");
		histoArray[varNum*MAXVIZWINS + vizNum] = 0;
		return;
	}

	histoArray[varNum*MAXVIZWINS + vizNum] = new Histo(data,
			min_dim, max_dim, min_bdim, max_bdim, dataMin, dataMax);

}
	
