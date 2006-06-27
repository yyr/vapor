//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		viewpointeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Implements the ViewpointsEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the viewpoint tab
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif
#include <qdesktopwidget.h>
#include <qrect.h>
#include <qlineedit.h>

#include <qcombobox.h>
#include <qlabel.h>
#include "viewpointeventrouter.h"
#include "viewpointparams.h"
#include "viztab.h"
#include "tabmanager.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"

using namespace VAPoR;


ViewpointEventRouter::ViewpointEventRouter(QWidget* parent,const char* name): VizTab(parent, name), EventRouter(){
	myParamsType = Params::ViewpointParamsType;
	savedCommand = 0;
}


ViewpointEventRouter::~ViewpointEventRouter(){
	if (savedCommand) delete savedCommand;
}
/**********************************************************
 * Whenever a new viztab is created it must be hooked up here
 ************************************************************/
void
ViewpointEventRouter::hookUpTab()
{
	
	//connect (perspectiveCombo, SIGNAL (activated(int)), this, SLOT (setVPPerspective(int)));
	connect (numLights, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos00, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos01, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos02, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos10, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos11, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos12, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos20, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos21, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos22, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (camPos0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (camPos1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (camPos2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (viewDir0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (viewDir1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (viewDir2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (upVec0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (upVec1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (upVec2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (rotCenter0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (rotCenter1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (rotCenter2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	
	//Connect all the returnPressed signals, these will update the visualizer.
	connect (lightPos00, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos01, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos02, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos10, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos11, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos12, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos20, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos21, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos22, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (camPos0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (camPos1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (camPos2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (viewDir0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (viewDir1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (viewDir2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (upVec0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (upVec1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (upVec2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (rotCenter0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (rotCenter1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (rotCenter2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (numLights, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
}

/*********************************************************************************
 * Slots associated with ViewpointTab:
 *********************************************************************************/

void ViewpointEventRouter::
setVtabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
//Put all text changes into the params
void ViewpointEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	ViewpointParams* vParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ViewpointParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(vParams, "edit Viewpoint text");

	//Did numLights change?
	int nLights = numLights->text().toInt();
	
	//make sure it's a valid number:
	bool changed = false;
	if (nLights < 0 ){
		nLights = 0; changed = true;
	}
	if (nLights > 3){
		nLights = 3; changed = true;
	}
	if (changed) {
		numLights->setText(QString::number(nLights));
	}
	if (nLights != vParams->getNumLights()){
		changed = true;
		vParams->setNumLights(nLights);
		bool lightOn;
		lightOn = (nLights > 0);
		lightPos00->setEnabled(lightOn);
		lightPos01->setEnabled(lightOn);
		lightPos02->setEnabled(lightOn);
		lightOn = (nLights > 1);
		lightPos10->setEnabled(lightOn);
		lightPos11->setEnabled(lightOn);
		lightPos12->setEnabled(lightOn);
		lightOn = (nLights > 2);
		lightPos20->setEnabled(lightOn);
		lightPos21->setEnabled(lightOn);
		lightPos22->setEnabled(lightOn);
	}
	//Get the light directions from the gui:
	vParams->setLightDirection(0,0,lightPos00->text().toFloat());
	vParams->setLightDirection(0,1,lightPos01->text().toFloat());
	vParams->setLightDirection(0,2,lightPos02->text().toFloat());
	vParams->setLightDirection(1,0,lightPos10->text().toFloat());
	vParams->setLightDirection(1,1,lightPos11->text().toFloat());
	vParams->setLightDirection(1,2,lightPos12->text().toFloat());
	vParams->setLightDirection(2,0,lightPos20->text().toFloat());
	vParams->setLightDirection(2,1,lightPos21->text().toFloat());
	vParams->setLightDirection(2,2,lightPos22->text().toFloat());
	//final component is 0 (for gl directional light)
	vParams->setLightDirection(0,3,0.f);
	vParams->setLightDirection(1,3,0.f);
	vParams->setLightDirection(2,3,0.f);
	

	vParams->setCameraPos(0, camPos0->text().toFloat());
	vParams->setCameraPos(1, camPos1->text().toFloat());
	vParams->setCameraPos(2, camPos2->text().toFloat());
	vParams->setViewDir(0, viewDir0->text().toFloat());
	vParams->setViewDir(1, viewDir1->text().toFloat());
	vParams->setViewDir(2, viewDir2->text().toFloat());
	vParams->setUpVec(0, upVec0->text().toFloat());
	vParams->setUpVec(1, upVec1->text().toFloat());
	vParams->setUpVec(2, upVec2->text().toFloat());
	vParams->setRotationCenter(0,rotCenter0->text().toFloat());
	vParams->setRotationCenter(1,rotCenter1->text().toFloat());
	vParams->setRotationCenter(2,rotCenter2->text().toFloat());
	
	PanelCommand::captureEnd(cmd, vParams);
	updateRenderer(vParams,false, false, false);
	guiSetTextChanged(false);

	if (changed) updateTab(vParams);
	
}
void ViewpointEventRouter::
viewpointReturnPressed(void){



	confirmText(true);
}


//Insert values from params into tab panel
//
void ViewpointEventRouter::updateTab(Params* params){
	ViewpointParams* vpParams = (ViewpointParams*) params;
	
	QString strng;
	Session::getInstance()->blockRecording();
	
	if (vpParams->isLocal())
		LocalGlobal->setCurrentItem(1);
	else 
		LocalGlobal->setCurrentItem(0);
	
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
	int nLights = vpParams->getNumLights();
	numLights->setText(strng.setNum(nLights));
	camPos0->setText(strng.setNum(currentViewpoint->getCameraPos(0), 'g', 3));
	camPos1->setText(strng.setNum(currentViewpoint->getCameraPos(1), 'g', 3));
	camPos2->setText(strng.setNum(currentViewpoint->getCameraPos(2), 'g', 3));
	viewDir0->setText(strng.setNum(currentViewpoint->getViewDir(0), 'g', 3));
	viewDir1->setText(strng.setNum(currentViewpoint->getViewDir(1), 'g', 3));
	viewDir2->setText(strng.setNum(currentViewpoint->getViewDir(2), 'g', 3));
	upVec0->setText(strng.setNum(currentViewpoint->getUpVec(0), 'g', 3));
	upVec1->setText(strng.setNum(currentViewpoint->getUpVec(1), 'g', 3));
	upVec2->setText(strng.setNum(currentViewpoint->getUpVec(2), 'g', 3));
	//perspectiveCombo->setCurrentItem(currentViewpoint->hasPerspective());
	rotCenter0->setText(strng.setNum(vpParams->getRotationCenter(0),'g',3));
	rotCenter1->setText(strng.setNum(vpParams->getRotationCenter(1),'g',3));
	rotCenter2->setText(strng.setNum(vpParams->getRotationCenter(2),'g',3));
	const float* lightDir = vpParams->getLightDirection(0);
	lightPos00->setText(QString::number(lightDir[0]));
	lightPos01->setText(QString::number(lightDir[1]));
	lightPos02->setText(QString::number(lightDir[2]));
	lightDir = vpParams->getLightDirection(1);
	lightPos10->setText(QString::number(lightDir[0]));
	lightPos11->setText(QString::number(lightDir[1]));
	lightPos12->setText(QString::number(lightDir[2]));
	lightDir = vpParams->getLightDirection(2);
	lightPos20->setText(QString::number(lightDir[0]));
	lightPos21->setText(QString::number(lightDir[1]));
	lightPos22->setText(QString::number(lightDir[2]));
	

	//Enable light direction text boxes as needed:
	bool lightOn;
	lightOn = (nLights > 0);
	lightPos00->setEnabled(lightOn);
	lightPos01->setEnabled(lightOn);
	lightPos02->setEnabled(lightOn);
	lightOn = (nLights > 1);
	lightPos10->setEnabled(lightOn);
	lightPos11->setEnabled(lightOn);
	lightPos12->setEnabled(lightOn);
	lightOn = (nLights > 2);
	lightPos20->setEnabled(lightOn);
	lightPos21->setEnabled(lightOn);
	lightPos22->setEnabled(lightOn);

	
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
	update();
}
void ViewpointEventRouter::
guiCenterSubRegion(RegionParams* rParams){
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ViewpointParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "center sub-region view");
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
	//Find the largest of the dimensions of the current region:
	float maxSide = Max(rParams->getRegionMax(2)-rParams->getRegionMin(2), 
		Max(rParams->getRegionMax(0)-rParams->getRegionMin(0),
		rParams->getRegionMax(1)-rParams->getRegionMin(1)));
	//calculate the camera position: center - 2*viewDir*maxSide;
	//Position the camera 2.5*maxSide units away from the center, aimed
	//at the center
	//Make sure the viewDir is normalized:
	vnormal(currentViewpoint->getViewDir());
	for (int i = 0; i<3; i++){
		float camPosCrd = rParams->getRegionCenter(i) -2.5*maxSide*currentViewpoint->getViewDir(i);
		currentViewpoint->setCameraPos(i, camPosCrd);
		vpParams->setRotationCenter(i,rParams->getRegionCenter(i));
	}
	//modify near/far distance as needed:
	VizWinMgr::getInstance()->resetViews(rParams,vpParams);
	updateTab(vpParams);
	updateRenderer(vpParams,false, false, false);
	PanelCommand::captureEnd(cmd,vpParams);
	
}
void ViewpointEventRouter::
guiCenterFullRegion(RegionParams* rParams){
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ViewpointParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "center full region view");
	vpParams->centerFullRegion();
	//modify near/far distance as needed:
	VizWinMgr::getInstance()->resetViews(rParams,vpParams);
	updateTab(vpParams);
	updateRenderer(vpParams,false, false, false);
	PanelCommand::captureEnd(cmd,vpParams);
	
}
//Align the view direction to one of the axes.
//axis is 1,2,3 for +X,Y,Z,  and -1,-2,-3 for -X,-Y,-Z
//
void ViewpointEventRouter::
guiAlignView(int axis){
	float vdir[3] = {0.f, 0.f, 0.f};
	float up[3] = {0.f, 1.f, 0.f};
	float vpos[3];
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ViewpointParamsType);
	//establish view direction, up vector:
	switch (axis){
		case(1): vdir[0] = 1.f; break;
		case(2): vdir[1] = 1.f; up[1]=0.f; up[0] = 1.f; break;
		case(3): vdir[2] = 1.f; break;
		case(4): vdir[0] = -1.f; break;
		case(5): vdir[1] = -1.f; up[1]=0.f; up[0] = 1.f; break;
		case(6): vdir[2] = -1.f; break;
		default: assert(0);
	}
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "align viewpoint to axis");
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
	
	//Determine distance from center to camera:
	vsub(currentViewpoint->getCameraPos(),currentViewpoint->getRotationCenter(),vpos);
	float viewDist = vlength(vpos);
	//Keep the view center as is, make view direction vdir 
	currentViewpoint->setViewDir(vdir);
	//Make the up vector +Y, or +X
	currentViewpoint->setUpVec(up);
	//Position the camera the same distance from the center but down the -X direction
	vmult(vdir, viewDist, vdir);
	vsub(currentViewpoint->getRotationCenter(), vdir, vpos);
	currentViewpoint->setCameraPos(vpos);
	updateTab(vpParams);
	updateRenderer(vpParams,false,false,false);
	PanelCommand::captureEnd(cmd,vpParams);
}

//Reset the center of view.  Leave the camera where it is
void ViewpointEventRouter::
guiSetCenter(const float* coords){
	float vdir[3];
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ViewpointParamsType);
	
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "set view center");
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
	//Determine the new viewDir:
	vcopy(coords, vdir);
	vsub(vdir,currentViewpoint->getCameraPos(), vdir);
	//Make sure the viewDir is normalized:
	vnormal(vdir);
	currentViewpoint->setViewDir(vdir);
	
	for (int i = 0; i<3; i++){
		vpParams->setRotationCenter(i,coords[i]);
	}
	updateTab(vpParams);
	updateRenderer(vpParams,false, false, false);
	PanelCommand::captureEnd(cmd,vpParams);
	
}

void ViewpointEventRouter::
setHomeViewpoint(){
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ViewpointParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "Set Home Viewpoint");
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
	vpParams->setHomeViewpoint(new Viewpoint(*currentViewpoint));
	updateTab(vpParams);
	updateRenderer(vpParams,false, false, false);
	PanelCommand::captureEnd(cmd, vpParams);
}
void ViewpointEventRouter::
useHomeViewpoint(){
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ViewpointParamsType);

	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "Use Home Viewpoint");
	Viewpoint* homeViewpoint = vpParams->getHomeViewpoint();
	vpParams->setCurrentViewpoint(new Viewpoint(*homeViewpoint));
	updateTab(vpParams);
	updateRenderer(vpParams,false, false, false);
	PanelCommand::captureEnd(cmd, vpParams);
}
void ViewpointEventRouter::
captureMouseUp(){
	//Update the tab:
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ViewpointParamsType);
	if (!savedCommand) return;
	updateTab(vpParams);
	PanelCommand::captureEnd(savedCommand, vpParams);
	//Set region  dirty
	VizWinMgr::getInstance()->setVizDirty(vpParams, NavigatingBit, true);
	savedCommand = 0;
	//Force rerender??
}


/*
 * respond to change in viewer frame
 * Don't record position except when mouse up/down
 */
void ViewpointEventRouter::
navigate (ViewpointParams* vpParams, float* posn, float* viewDir, float* upVec){
	
	vpParams->setCameraPos(posn);
	vpParams->setUpVec(upVec);
	vpParams->setViewDir(viewDir);
	updateTab(vpParams);
}
//Reinitialize Viewpoint tab settings, session has changed.
//Note that this is called after the globalViewpointParams are set up, but before
//any of the localViewpointParams are setup.
void ViewpointEventRouter::
reinitTab(bool doOverride){
}

//Save undo/redo state when user grabs a rake handle
//
void ViewpointEventRouter::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ViewpointParamsType);
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(vpParams,  "viewpoint navigation");
}

void ViewpointEventRouter::
updateRenderer(ViewpointParams* vpParams, bool prevEnabled,  bool wasLocal, bool newWindow){
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	bool local = vpParams->isLocal();
	//Always set the values in the active viz.  This amounts to stuffing
	//the new values into the trackball.
	//If the settings are global, then
	//also make the other global visualizers redraw themselves
	//(All global visualizers will be sharing the same trackball)
	//
	VizWin* viz = myVizMgr->getActiveVisualizer();
	//If this panel is associated with the active visualizer, stuff the values
	//into that viz:
	if (viz) {
		if (local) viz->setValuesFromGui(vpParams);
		else viz->setValuesFromGui(myVizMgr->getViewpointParams(viz->getWindowNum()));
	}
	
	if (!local){
		//Find all the viz windows that are using global settings.
		//That is the case if the window is non-null, and if the corresponding
		//vpparms is either null or set to Global
		//
		for (int i = 0; i< MAXVIZWINS; i++){
			if (i == myVizMgr->getActiveViz()) continue;
			if( viz = myVizMgr->getVizWin(i)){
				viz->setViewerCoordsChanged(true);
				//Bypass normal access to vpParams!
				if(!(myVizMgr->getRealVPParams(i)) || !(myVizMgr->getRealVPParams(i)->isLocal())){
					viz->updateGL();
				}
			}
		}
	}
}

void ViewpointEventRouter::
makeCurrent(Params* prev, Params* next, bool) {
	ViewpointParams* vParams = (ViewpointParams*) next;
	int vizNum = vParams->getVizNum();
	VizWinMgr::getInstance()->setViewpointParams(vizNum, vParams);
	//If the local/global changes, need to tell the window, too.
	if (vizNum >=0 && prev->isLocal() != next->isLocal())
		VizWinMgr::getInstance()->getVizWin(vizNum)->setGlobalViewpoint(!next->isLocal());
	//Also update current Tab.  It's probably visible.
	updateTab(vParams);
	updateRenderer(vParams, false, prev->isLocal(), false);
}