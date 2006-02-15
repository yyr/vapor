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
class XmlNode;
class RegionParams : public Params {
	
public: 
	RegionParams(int winnum);
	
	~RegionParams();
	virtual Params* deepCopy();
	virtual void makeCurrent(Params* p, bool newWin);
	//Method to calculate the read-only region info that is displayed in the regionTab
	void refreshRegionInfo();	
	//Just get the values needed for a call to DataMgr::getRegion:
	void getRegionVoxelCoords(int numxforms, int min_dim[3], int max_dim[3], size_t min_bdim[3], size_t max_bdim[3]);

	void setTab(RegionTab* tab) {myRegionTab = tab;}
	
	float getRegionMin(int coord){ return regionMin[coord];}
	float getRegionMax(int coord){ return regionMax[coord];}
	float getRegionCenter(int indx) {
		return (0.5f*(regionMin[indx]+regionMax[indx]));
	}
	
	//Update the dialog with values from this:
	//
	virtual void updateDialog();
	//And vice-versa:
	//
	virtual void updatePanelState();

	//Following methods are set from gui, have undo/redo support:
	//
	void guiSetCenter(float * coords);
	void guiSetMaxSize();
	void guiSetNumRefinements(int n);
	void guiSetVarNum(int n);
	void guiSetTimeStep(int n);
	void guiSetXCenter(int n);
	void guiSetXSize(int n);
	void guiSetYCenter(int n);
	void guiSetYSize(int n);
	void guiSetZCenter(int n);
	void guiSetZSize(int n);
	void guiCopyRakeToRegion();
	void guiCopyProbeToRegion();
	

	
	//
	//Start to slide a region face.  Need to save direction vector
	//
	void captureMouseDown();
	//When the mouse goes up, save the face displacement into the region.
	virtual void captureMouseUp();
	
	// Reinitialize due to new Session:
	void reinit(bool doOverride);
	void restart();
	
	XmlNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	//See if the proposed number of transformations is OK.  Return a valid value
	int validateNumTrans(int n);

	virtual void setBox(const float boxmin[], const float boxmax[]){
		for(int i = 0; i<3; i++){
			setRegionMin(i, boxmin[i]);
			setRegionMax(i, boxmax[i]);
		}
	}
	virtual void getBox(float boxMin[], float boxMax[]){
		for (int i = 0; i< 3; i++){
			boxMin[i] = getRegionMin(i);
			boxMax[i] = getRegionMax(i);
		}
	}
	

protected:
	static const string _regionMinTag;
	static const string _regionMaxTag;
	static const string _regionCenterTag;
	static const string _regionSizeTag;
	static const string _maxSizeAttr;
	static const string _numTransAttr;
	
	//Holder for saving state during mouse move:
	//
	PanelCommand* savedCommand;
	
	//Methods to make sliders and text valid and consistent for region:
	void textToSlider(int coord, float center, float size);
	void sliderToText(int coord, int center, int size);
	
	
	void setXCenter(int sliderVal);
	void setXSize(int sliderVal);
	void setYCenter(int sliderVal);
	void setYSize(int sliderVal);
	void setZCenter(int sliderVal);
	void setZSize(int sliderVal);
	//Methods to set the region max and min from a float value.
	//Called at end of region bounds drag
	//
	void setRegionMin(int coord, float minval);
	void setRegionMax(int coord, float maxval);
	//Region dirty bit is kept in vizWin
	//
	void setDirty();  
	
	
	int infoNumRefinements, infoVarNum, infoTimeStep;

	//New values (analog)
	float regionMax[3],regionMin[3];
	
	RegionTab* myRegionTab;
	
};

};
#endif //REGIONPARAMS_H 
