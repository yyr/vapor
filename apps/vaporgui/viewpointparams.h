//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		viewpointparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2004
//
//	Description:	Defines the ViewpointParams class
//		This class contains the parameters associated with viewpoint and lights
//
#ifndef VIEWPOINTPARAMS_H
#define VIEWPOINTPARAMS_H

#include <qwidget.h>
#include "params.h"
#include "vizwinmgr.h"
#include "viewpoint.h"

class VizTab;

namespace VAPoR {

class MainForm;
class RegionParams;
class PanelCommand;

class ViewpointParams : public Params {
	
public: 
	ViewpointParams(int winnum);
	
	virtual ~ViewpointParams();
	void setTab(VizTab* tab) {myVizTab = tab;}
	virtual Params* deepCopy();
	virtual void makeCurrent(Params* p, bool newWin);
	
	void setNumLights(int n);
	float* getCameraPos() {return currentViewpoint->getCameraPos();}
	float getCameraPos(int coord) {return currentViewpoint->getCameraPos()[coord];}
	void setCameraPos(int i, float val) { currentViewpoint->setCameraPos(i, val);}
	void setCameraPos(float* val) {currentViewpoint->setCameraPos(val);}
	float* getViewDir() {return currentViewpoint->getViewDir();}
	void setViewDir(int i, float val) { currentViewpoint->setViewDir(i,val);}
	void setViewDir(float* val) {currentViewpoint->setViewDir(val);}
	float* getUpVec() {return currentViewpoint->getUpVec();}
	void setUpVec(int i, float val) { currentViewpoint->setUpVec(i,val);}
	void setUpVec(float* val) {currentViewpoint->setUpVec(val);}
	bool hasPerspective(){return currentViewpoint->hasPerspective();}
	void setPerspective(bool on) {currentViewpoint->setPerspective(on);}
	Viewpoint* getCurrentViewpoint() { return currentViewpoint;}

	float* getLightPos(int i) {return lightPositions + 3*i;}
	void setLightPos(int i, float* posn) {
		for (int j= 0; j< 3; j++)
			lightPositions[3*i+j] = posn[j];
	}
	//Update the dialog with values from this:
	//
	virtual void updateDialog();
	//And vice-versa:
	//
	virtual void updatePanelState();
	
	
	//Respond to mouse motion, changing viewer frame
	//
	void navigate(float* posn, float* viewDir, float* upVec);


	//Send params to the active renderer(s) that depend on these
	//panel settings, tell renderers that the values must be updated
	//Arguments are not used by viewpointparams
	//
	virtual void updateRenderer(bool, bool, bool);
	

	//Following methods are for undo/redo support:
	//Methods to capture state at start and end of mouse moves:
	//
	void captureMouseDown();
	void captureMouseUp();
	void guiSetPerspective(bool on);
	
	void guiCenterFullRegion(RegionParams* rParams);
	void guiCenterSubRegion(RegionParams* rParams);
	
	//Methods to handle home viewpoint
	void setHomeViewpoint();
	void useHomeViewpoint();
	
	//Reset viewpoint when new session is started:
	virtual void reinit();
	
	//Transformations to convert world coords to (unit)render cube and back
	//
	static void worldToCube(float fromCoords[3], float toCoords[3]);

	static void worldFromCube(float fromCoords[3], float toCoords[3]);
	static void setCoordTrans();

	float* getRotationCenter(){ return rotationCenter;}
	
protected:
	//Holder for saving state during mouse move:
	//
	PanelCommand* savedCommand;
	Viewpoint* currentViewpoint;
	Viewpoint* homeViewpoint;
	float rotationCenter[3];
	int numLights;
	float* lightPositions;
	VizTab* myVizTab;

	//Static coeffs for affine coord conversion:
	static float minCubeCoord[3];
	static float maxCubeSide;

};
};
#endif //VIEWPOINTPARAMS_H 

