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
using namespace VAPoR;
Histo::Histo(int numberBins, float mnData, float mxData){
	numBins = numberBins;
	minData = mnData;
	maxData = mxData;
	binArray = new int[numBins];
	reset();
}
Histo::Histo(unsigned char* data, int dataSize){
	binArray = new int[256];
	minData = 0;
	maxData = 255;
	numBins = 256;
	reset();
	for (int i = 0; i<dataSize; i++){
		int val = (int)data[i];
		binArray[val]++;
		if (binArray[val] > maxBinSize) {
			maxBinSize = binArray[val];
			largestBin = val;
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
