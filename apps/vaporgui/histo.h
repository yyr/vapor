//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		histo.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:  Definition of Histo class: 
//		it contains a histogram derived from volume data.
//		Used by TFEditor to draw histogram behind transfer function opacity
//
#ifndef HISTO_H
#define HISTO_H
namespace VAPoR {
class Histo{
public:
	Histo(int numberBins, float mnData, float mxData);
	//Special constructor for unsigned char data:
	//
	Histo(unsigned char* data, int dataSize);
	~Histo();
	void reset(int newNumBins = -1);
	void addToBin(float val);	
	int getBinSize(int posn) {return binArray[posn];}
	int getMaxBinSize() {return maxBinSize;}
private:
	int* binArray;
	int numBelow, numAbove;
	int numBins;
	float minData, maxData;
	int maxBinSize;
	int largestBin;
};
};

#endif //HISTO_H

