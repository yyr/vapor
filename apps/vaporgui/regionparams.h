//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		regionparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Defines the RegionParams class.
//		This class supports parameters associted with the
//		region panel, describing the rendering region
//
#ifndef REGIONPARAMS_H
#define REGIONPARAMS_H


#include <qwidget.h>
#include <vector>
#include "vaporinternal/common.h"
#include "params.h"
#include "vapor/MyBase.h"


using namespace VetsUtil;
class RegionTab;

namespace VAPoR {
class MainForm;

class RegionParams : public Params {
	
public: 
	RegionParams(MainForm*, int winnum);
	
	~RegionParams();
	virtual Params* deepCopy();
	virtual void makeCurrent(Params* p, bool newWin);
	
		
	void setNumTrans(int n) {numTrans = Min(n, maxNumTrans); setDirty();}
	int getNumTrans() {return numTrans;}
	int getMaxSize() {return maxSize;}
	//Note: setMaxSize is a hack to make it easy for user to
	//Manipulate all 3 sizes
	//
	void setMaxSize(int newMax) {
		maxSize = newMax;
		for (int i = 0; i<3; i++){
			if (regionSize[i]> maxSize)setRegionSize(i,maxSize);
			if (regionSize[i]< maxSize)
				setRegionSize(i, Min(maxSize, fullSize[i]));
		}
	}
	//Note:  Fullsize is not modified in the UI, it comes from data
	//
	int* getFullSize() {return fullSize;}
	void setFullSize(int i, int val){fullSize[i] = val; setDirty();}
	int getFullSize(int indx) {return fullSize[indx];}
	int* getCenterPos() {return centerPosition;}
	int getCenterPosition(int indx) {return centerPosition[indx];}
	int setCenterPosition(int i, int val) { 
		val = Min(val, fullSize[i]-regionSize[i]/2);
		centerPosition[i] = Max(val, regionSize[i]/2);
		enforceConsistency(i);
		setDirty();
		return centerPosition[i];
	}
	int* getRegionSize() {return regionSize;}
	int getRegionSize(int indx) {return regionSize[indx];}
	int setRegionSize(int i, int val) { 
		regionSize[i] = Min(val, fullSize[i]);
		enforceConsistency(i);
		setDirty();
		return regionSize[i];
	}
	void setTab(RegionTab* tab) {myRegionTab = tab;}
	void setMaxNumTrans(int maxNT) {maxNumTrans = maxNT;}
	int getMaxNumTrans() {return maxNumTrans;}
	void setCurrentTimestep(int val) {currentTimestep = val; setDirty();}
	int getCurrentTimestep() {return currentTimestep;}
	void setDataExtents(const std::vector<double> ext){
		dataExtents = ext;
	}

	//Update the dialog with values from this:
	//
	virtual void updateDialog();
	//And vice-versa:
	//
	virtual void updatePanelState();

	//Following methods are set from gui, have undo/redo support:
	//
	void guiSetMaxSize(int newMax);
	void guiSetNumTrans(int n);
	void guiSetXCenter(int n);
	void guiSetXSize(int n);
	void guiSetYCenter(int n);
	void guiSetYSize(int n);
	void guiSetZCenter(int n);
	void guiSetZSize(int n);
	void guiSetTimestep(int n);
	
	// respond to sliding the mouse for region selection
	//
	void slide (QPoint& p);

	// Reinitialize due to new Session:
	void reinit();
	
protected:
	//Method to force center, size, maxsize to be valid
	//Return true if anything changed.
	//
	bool enforceConsistency(int i);
	//Region dirty bit is kept in vizWin
	//
	void setDirty();  
	int centerPosition[3];
	int regionSize[3];
	int fullSize[3];
	int maxSize, numTrans, maxNumTrans;
	int currentTimestep;
	RegionTab* myRegionTab;
	std::vector<double> dataExtents;

};

};
#endif //REGIONPARAMS_H 
