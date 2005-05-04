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
class ViewpointParams;
class RegionParams : public Params {
	
public: 
	RegionParams(int winnum);
	
	~RegionParams();
	virtual Params* deepCopy();
	virtual void makeCurrent(Params* p, bool newWin);
	void calcRegionExtents(int min_dim[3], int max_dim[3], size_t min_bdim[3], size_t max_bdim[3], 
				  int numxforms, float minFull[3], float maxFull[3], float extents[6]);
		
	void setNumTrans(int n) {
		numTrans = Min(n, maxNumTrans); 
		numTrans = Max(numTrans, minNumTrans);
		setDirty();}
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
	void setMinNumTrans(int minNT) {minNumTrans = minNT;}
	int getMaxNumTrans() {return maxNumTrans;}
	int getMinNumTrans() {return minNumTrans;}
	//void setCurrentTimestep(int val) {currentTimestep = val; setDirty();}
	//int getCurrentTimestep() {return currentTimestep;}
	void setDataExtents(const std::vector<double> ext){
		fullDataExtents = ext;
	}
	float getFullDataExtent(int i) { return fullDataExtents[i];}
	float getRegionMin(int coord){ return (float)(fullDataExtents[coord] + (fullDataExtents[coord+3]-fullDataExtents[coord])*
		(centerPosition[coord] - regionSize[coord]*.5)/(double)(fullSize[coord]));
	}
	float getRegionMax(int coord){ return (float)(fullDataExtents[coord] + (fullDataExtents[coord+3]-fullDataExtents[coord])*
		(centerPosition[coord] + regionSize[coord]*.5)/(double)(fullSize[coord]));
	}
	float getRegionCenter(int indx) {
		return (0.5f*(getRegionMin(indx)+getRegionMax(indx)));
	}
	float getFullCenter(int indx) {
		return ((float)(0.5*(fullDataExtents[indx]+fullDataExtents[indx+3])));
	}
	//If a face is selected, this value is >= 0:
	//
	int getSelectedFaceNum() {return selectedFaceNum;}
	//While sliding face, the faceDisplacement indicates how far selected face is 
	//moved.
	float getFaceDisplacement() {return faceDisplacement;}
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

	//Set the rotation/viewpoint 
	void guiCenterFull(ViewpointParams* vpp);
	void guiCenterRegion(ViewpointParams* vpp);
	//
	//Start to slide a region face.  Need to save direction vector
	//
	void captureMouseDown(int faceNum, float camPos[3], float dirVec[]);
	//When the mouse goes up, save the face displacement into the region.
	void captureMouseUp();
	//Intersect the ray with the specified face, determining the intersection
	//in world coordinates.  Note that meaning of faceNum is specified in 
	//renderer.h
	// Faces of the cube are numbered 0..5 based on view from pos z axis:
	// back, front, bottom, top, left, right
	//
	bool rayCubeIntersect(float ray[3], float cameraPos[3], int faceNum, float intersect[3]);
	//Slide the face based on mouse move from previous capture.  
	//Requires new direction vector associated with current mouse position
	void slideCubeFace(float movedRay[3]);
	//Indicate we are currently dragging a cube face:
	bool draggingFace() {return (selectedFaceNum >= 0);}
	// Reinitialize due to new Session:
	void reinit(bool doOverride);
	void restart();
	void unSelectFace() { selectedFaceNum = -1;}
	
	
protected:
	//Holder for saving state during mouse move:
	//
	PanelCommand* savedCommand;
	//Method to force center, size, maxsize to be valid
	//Return true if anything changed.
	//
	bool enforceConsistency(int i);
	//See if the proposed number of transformations is OK.  Return a valid value
	int validateNumTrans(int n);
	//Recalc extents:
	void setCurrentExtents(int coord);
	//Methods to set the region max and min from a float value.
	//Called at end of region bounds drag
	//
	void setRegionMin(int coord, float minval);
	void setRegionMax(int coord, float maxval);
	//Region dirty bit is kept in vizWin
	//
	void setDirty();  
	int centerPosition[3];
	int regionSize[3];
	int fullSize[3];
	int maxSize, numTrans, maxNumTrans, minNumTrans;
	//int currentTimestep;
	RegionTab* myRegionTab;
	std::vector<double> fullDataExtents;
	int selectedFaceNum;
	float faceDisplacement;
	float initialSelectionRay[3];
};

};
#endif //REGIONPARAMS_H 
