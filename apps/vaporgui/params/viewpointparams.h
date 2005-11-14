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
class ExpatParseMgr;
class MainForm;
class RegionParams;
class PanelCommand;
class XmlNode;

class ViewpointParams : public Params {
	
public: 
	ViewpointParams(int winnum);
	
	virtual ~ViewpointParams();
	void setTab(VizTab* tab) {myVizTab = tab;}
	virtual Params* deepCopy();
	virtual void makeCurrent(Params* p, bool newWin);
	
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
	int getNumLights() { return numLights;}
	const float* getLightDirection(int lightNum){return lightDirection[lightNum];}
	
	Viewpoint* getCurrentViewpoint() { return currentViewpoint;}

	//Update the dialog with values from this:
	//
	virtual void updateDialog();
	//And vice-versa:
	//
	virtual void updatePanelState();
	
	float* getRotationCenter(){return currentViewpoint->getRotationCenter();}
	float getRotationCenter(int i){ return currentViewpoint->getRotationCenter(i);}
	void setRotationCenter(int i, float val){currentViewpoint->setRotationCenter(i,val);}
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
	//Following are only accessible from main menu
	void guiCenterFullRegion(RegionParams* rParams);
	void guiCenterSubRegion(RegionParams* rParams);
	
	//Methods to handle home viewpoint
	void setHomeViewpoint();
	void useHomeViewpoint();
	
	//Reset viewpoint when new session is started:
	virtual void reinit(bool doOverride);
	virtual void restart();
	
	//Transformations to convert world coords to (unit)render cube and back
	//
	static void worldToCube(float fromCoords[3], float toCoords[3]);

	static void worldFromCube(float fromCoords[3], float toCoords[3]);
	static void setCoordTrans();
	static float* getMinCubeCoords() {return minCubeCoord;}
	static float* getMaxCubeCoords() {return maxCubeCoord;}

	//Following determines scale factor in coord transformation:
	//
	static float getMaxCubeSide() {return maxCubeSide;}

	//Maintain the OpenGL Model Matrices, since they can be shared between visualizers
	
	double* getModelViewMatrix() {return modelViewMatrix;}
	
	void setModelViewMatrix(double* mtx){
		for (int i = 0; i<16; i++) modelViewMatrix[i] = mtx[i];
	}
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	XmlNode* buildNode();

protected:
	static const string _currentViewTag;
	static const string _homeViewTag;
	static const string _lightTag;
	static const string _lightDirectionAttr;
	static const string _lightNumAttr;
	//Set to default viewpoint for specified region
	void centerFullRegion(RegionParams*);
	//Holder for saving state during mouse move:
	//
	PanelCommand* savedCommand;
	Viewpoint* currentViewpoint;
	Viewpoint* homeViewpoint;
	
	int numLights;
	int parsingLightNum;
	float lightDirection[3][4];
	VizTab* myVizTab;

	//Static coeffs for affine coord conversion:
	//
	static float minCubeCoord[3];
	static float maxCubeSide;
	//Max sides
	static float maxCubeCoord[3];
	//GL state saved here since it may be shared...
	//
	double modelViewMatrix[16];
};
};
#endif //VIEWPOINTPARAMS_H 

