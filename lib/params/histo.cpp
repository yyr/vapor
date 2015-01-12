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
#include "animationparams.h"

using namespace VAPoR;


Histo::Histo(int numberBins, float mnData, float mxData){
	numBins = numberBins;
	minData = mnData;
	maxData = mxData;
	binArray = new int[numBins];
	reset();
}
Histo::Histo(const RegularGrid *rg, const double exts[6], const float range[2]) {
	binArray = new int[256];
	minData = range[0];
	maxData = range[1];
	numBins = 256;
	reset();

	unsigned int qv;	// quantized value
	float v;
	RegularGrid *rg_const = (RegularGrid *) rg;   // kludge - no const_iterator
	RegularGrid::Iterator itr;
	double point[3];
	for (itr = rg_const->begin(); itr!=rg_const->end(); ++itr) {
		v = *itr;
		if (v == rg->GetMissingValue()) continue;
		itr.GetUserCoordinates(point, point+1, point+2);
		bool isIn = true;
		for (int j = 0; j<3; j++){
			if (point[j]>exts[j+3] || point[j] < exts[j]) isIn = false;
		}
		if (!isIn) continue;
		if (v<range[0]) qv=0;
		else if (v>range[1]) qv=255;
		else qv = (unsigned int) rint((v-range[0])/(range[1]-range[0]) * 255);

		binArray[qv]++;
		if (qv > 0 && qv < 255 && binArray[qv] > maxBinSize) {
			maxBinSize = binArray[qv];
			largestBin = qv;
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
		else intVal = (int)(((double) val - (double) minData)*(double)numBins/((double) maxData - (double) minData));
		if (intVal >= numBins) intVal = numBins-1;
		if (intVal <= 0) intVal = 0;
		binArray[intVal]++;
		if (binArray[intVal] > maxBinSize) {
			maxBinSize = binArray[intVal];
			largestBin = intVal;
		}
	}
}
