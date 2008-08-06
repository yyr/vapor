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
#include "regionparams.h"
#include "dvrparams.h"
#include "animationparams.h"
#include "vapor/Metadata.h"
#include <qapplication.h>
#include <qcursor.h>
using namespace VAPoR;


Histo::Histo(int numberBins, float mnData, float mxData){
	numBins = numberBins;
	minData = mnData;
	maxData = mxData;
	binArray = new int[numBins];
	reset();
}
Histo::Histo(unsigned char* data, size_t min_dim[3], size_t max_dim[3], 
			 size_t min_bdim[3],size_t max_bdim[3],
			 float mnData, float mxData){
	binArray = new int[256];
	minData = mnData;
	maxData = mxData;
	numBins = 256;
	reset();
	const size_t *bs = DataStatus::getInstance()->getCurrentMetadata()->GetBlockSize();
	// make subregion origin (0,0,0)
	// Note that this doesn't affect the calc of nx,ny,nz.
	//
	for(int i=0; i<3; i++) {
		while(min_bdim[i] > 0) {
			min_dim[i] -= bs[i]; max_dim[i] -= bs[i];
			min_bdim[i] -= 1; max_bdim[i] -= 1;
		}
	}
		
	int nx = (max_bdim[0] - min_bdim[0] + 1) * bs[0];
	int ny = (max_bdim[1] - min_bdim[1] + 1) * bs[1];
	//int nz = (max_bdim[2] - min_bdim[2] + 1) * bs[2];
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
	int intVal;
	if (val < minData) numBelow++; 
	else if (val > maxData) numAbove++;
	else {
		if (maxData == minData) intVal = 0;
		else intVal = (int)((val - minData)*(float)numBins/(maxData - minData));
		if (intVal == numBins) intVal--;
		binArray[intVal]++;
		if (binArray[intVal] > maxBinSize) {
			maxBinSize = binArray[intVal];
			largestBin = intVal;
		}
	}
}
