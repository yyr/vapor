//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		viewpointparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2004
//
//	Description:	Implements the ViewpointParams class
//		This class contains the parameters associated with viewpoint and lights
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "viewpointparams.h"
#include "viztab.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "params.h"
#include "panelcommand.h"
#include "session.h"
#include "glutil.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include "viewpoint.h"
#include "tabmanager.h"
#include "vapor/XmlNode.h"
float VAPoR::ViewpointParams::maxCubeSide = 1.f;
float VAPoR::ViewpointParams::minCubeCoord[3] = {0.f,0.f,0.f};

using namespace VAPoR;
const string ViewpointParams::_currentViewTag = "CurrentViewpoint";
const string ViewpointParams::_homeViewTag = "HomeViewpoint";

ViewpointParams::ViewpointParams(int winnum): Params(winnum){
	thisParamType = ViewpointParamsType;
	myVizTab = MainForm::getInstance()->getVizTab();
	homeViewpoint = 0;
	currentViewpoint = 0;
	restart();
	
}
Params* ViewpointParams::
deepCopy(){
	ViewpointParams* p = new ViewpointParams(*this);
	p->currentViewpoint = new Viewpoint(*currentViewpoint);
	p->homeViewpoint = new Viewpoint(*homeViewpoint);
	//never keep the SavedCommand:
	p->savedCommand = 0;
	return (Params*)(p);
}
ViewpointParams::~ViewpointParams(){
	if (lightPositions) delete lightPositions;
	if (savedCommand) delete savedCommand;
	delete currentViewpoint;
	delete homeViewpoint;
}
void ViewpointParams::
makeCurrent(Params* prev, bool) {
	
	VizWinMgr::getInstance()->setViewpointParams(vizNum, this);
	//If the local/global changes, need to tell the window, too.
	if (vizNum >=0 && prev->isLocal() != isLocal())
		VizWinMgr::getInstance()->getVizWin(vizNum)->setGlobalViewpoint(!isLocal());
	//Also update current Tab.  It's probably visible.
	updateDialog();
	updateRenderer(false, prev->isLocal(), false);
}

//Add or delete lights
void 
ViewpointParams::setNumLights(int nLights){
	int i;
	if (nLights <= numLights) {
		numLights = nLights;
		if (numLights < 0) numLights = 0;
		return;
	}
	float* newLights = new float[3*nLights];
	for (i = 0; i< numLights; i++){
		for (int j= 0; j<3; j++) newLights[j+3*i] = lightPositions[j+3*i];
	}
	for (i = numLights; i<nLights; i++){
		for (int j= 0; j<3; j++) newLights[j+3*i] = 0.f;
	}
	if (numLights > 0) delete lightPositions;
	numLights = nLights;
	lightPositions = newLights;
	
}
void ViewpointParams::updateDialog(){
	
	QString strng;
	Session* ses = Session::getInstance();
	ses->blockRecording();
	myVizTab->numLights->setText(strng.setNum(numLights));
	myVizTab->camPos0->setText(strng.setNum(currentViewpoint->getCameraPos(0), 'g', 3));
	myVizTab->camPos1->setText(strng.setNum(currentViewpoint->getCameraPos(1), 'g', 3));
	myVizTab->camPos2->setText(strng.setNum(currentViewpoint->getCameraPos(2), 'g', 3));
	myVizTab->viewDir0->setText(strng.setNum(currentViewpoint->getViewDir(0), 'g', 3));
	myVizTab->viewDir1->setText(strng.setNum(currentViewpoint->getViewDir(1), 'g', 3));
	myVizTab->viewDir2->setText(strng.setNum(currentViewpoint->getViewDir(2), 'g', 3));
	myVizTab->upVec0->setText(strng.setNum(currentViewpoint->getUpVec(0), 'g', 3));
	myVizTab->upVec1->setText(strng.setNum(currentViewpoint->getUpVec(1), 'g', 3));
	myVizTab->upVec2->setText(strng.setNum(currentViewpoint->getUpVec(2), 'g', 3));
	myVizTab->perspectiveCombo->setCurrentItem(currentViewpoint->hasPerspective());
	myVizTab->rotCenter0->setText(strng.setNum(getRotationCenter(0),'g',3));
	myVizTab->rotCenter1->setText(strng.setNum(getRotationCenter(1),'g',3));
	myVizTab->rotCenter2->setText(strng.setNum(getRotationCenter(2),'g',3));
	
	if (isLocal())
		myVizTab->LocalGlobal->setCurrentItem(1);
	else 
		myVizTab->LocalGlobal->setCurrentItem(0);
	guiSetTextChanged(false);
	ses->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
}
//Update all the panel state associated with textboxes.
void ViewpointParams::
updatePanelState(){
	
	numLights = myVizTab->numLights->text().toInt();
	setCameraPos(0,myVizTab->camPos0->text().toFloat());
	setCameraPos(1,myVizTab->camPos1->text().toFloat());
	setCameraPos(2, myVizTab->camPos2->text().toFloat());
	setViewDir(0, myVizTab->viewDir0->text().toFloat());
	setViewDir(1, myVizTab->viewDir1->text().toFloat());
	setViewDir(2, myVizTab->viewDir2->text().toFloat());
	setUpVec(0, myVizTab->upVec0->text().toFloat());
	setUpVec(1, myVizTab->upVec1->text().toFloat());
	setUpVec(2, myVizTab->upVec2->text().toFloat());
	setRotationCenter(0,myVizTab->rotCenter0->text().toFloat());
	setRotationCenter(1,myVizTab->rotCenter1->text().toFloat());
	setRotationCenter(2,myVizTab->rotCenter2->text().toFloat());
	updateRenderer(false, false, false);
	guiSetTextChanged(false);
}

/*
 * respond to change in viewer frame
 * Don't record position except when mouse up/down
 */
void ViewpointParams::
navigate (float* posn, float* viewDir, float* upVec){
	
	setCameraPos(posn);
	setUpVec(upVec);
	setViewDir(viewDir);
	updateDialog();
}
/*
 * Update appropriate visualizer(s) with the latest viewer coords, update the dirty bit
 * This should only be called when these parameters are associated with the visualizer(s)
 */
void ViewpointParams::
updateRenderer(bool, bool , bool ) {
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
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
		if (local) viz->setValuesFromGui(this);
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
void ViewpointParams::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(this,  "drag viewpoint");
		
}

void ViewpointParams::
captureMouseUp(){
	//If no command saved, then don't capture the event!
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, this);
	savedCommand = 0;
}
void ViewpointParams::
guiSetPerspective(bool on){
	//capture text changes
	
	confirmText(false);
	
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	PanelCommand* cmd = PanelCommand::captureStart(this, "toggle perspective");
	setPerspective(on);
	PanelCommand::captureEnd(cmd,this);
}
void ViewpointParams::
guiCenterSubRegion(RegionParams* rParams){
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	PanelCommand* cmd = PanelCommand::captureStart(this, "center sub-region view");
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
		setRotationCenter(i,rParams->getRegionCenter(i));
	}
	updateDialog();
	updateRenderer(false, false, false);
	PanelCommand::captureEnd(cmd,this);
	
}
void ViewpointParams::
guiCenterFullRegion(RegionParams* rParams){
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	PanelCommand* cmd = PanelCommand::captureStart(this, "center full region view");
	centerFullRegion(rParams);
	
	updateDialog();
	updateRenderer(false, false, false);
	PanelCommand::captureEnd(cmd,this);
	
}
void ViewpointParams::
centerFullRegion(RegionParams* rParams){
	//Find the largest of the dimensions of the current region:
	float maxSide = Max(rParams->getFullDataExtent(5)-rParams->getFullDataExtent(2), 
		Max(rParams->getFullDataExtent(3)-rParams->getFullDataExtent(0),
		rParams->getFullDataExtent(4)-rParams->getFullDataExtent(1)));
	//calculate the camera position: center - 2.5*viewDir*maxSide;
	//Position the camera 2.5*maxSide units away from the center, aimed
	//at the center
	//Make sure the viewDir is normalized:
	vnormal(currentViewpoint->getViewDir());
	for (int i = 0; i<3; i++){
		float dataCenter = 0.5f*(rParams->getFullDataExtent(i+3)+rParams->getFullDataExtent(i));
		float camPosCrd = dataCenter -2.5*maxSide*currentViewpoint->getViewDir(i);
		currentViewpoint->setCameraPos(i, camPosCrd);
		setRotationCenter(i,rParams->getFullCenter(i));
	}
}
void ViewpointParams::
setHomeViewpoint(){
	PanelCommand* cmd = PanelCommand::captureStart(this, "Set Home Viewpoint");
	delete homeViewpoint;
	homeViewpoint = new Viewpoint(*currentViewpoint);
	updateDialog();
	updateRenderer(false, false, false);
	PanelCommand::captureEnd(cmd, this);
}
void ViewpointParams::
useHomeViewpoint(){
	PanelCommand* cmd = PanelCommand::captureStart(this, "Use Home Viewpoint");
	delete currentViewpoint;
	currentViewpoint = new Viewpoint(*homeViewpoint);
	updateDialog();
	updateRenderer(false, false, false);
	PanelCommand::captureEnd(cmd, this);
}
//Reinitialize viewpoint settings, to center view on the center of full region.
//(this is starting state)
void ViewpointParams::
restart(){
	savedCommand = 0;
	numLights = 0;
	lightPositions = 0;
	if (currentViewpoint) delete currentViewpoint;
	currentViewpoint = new Viewpoint();
	//Set default values in viewpoint:
	currentViewpoint->setPerspective(true);
	for (int i = 0; i< 3; i++){
		setCameraPos(i,0.5f);
		setViewDir(i,0.f);
		setUpVec(i, 0.f);
		setRotationCenter(i,0.5f);
	}
	setUpVec(1,1.f);
	setViewDir(2,-1.f); 
	setCameraPos(2,2.5f);
	if (homeViewpoint) delete homeViewpoint;
	homeViewpoint = new Viewpoint(*currentViewpoint);
	
	setCoordTrans();
	
	updateRenderer(false, false, false);
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myVizTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getViewpointParams(viznum)))
			updateDialog();
	}
	
}
//Reinitialize viewpoint settings, when metadata changes.
//Really not much to do!
//If we can override, set to default for current region.
//Note that this should be called after the region is init'ed
//
void ViewpointParams::
reinit(bool doOverride){
	VizWinMgr* vwm = VizWinMgr::getInstance();
	setCoordTrans();
	if (doOverride){
		setViewDir(0,0.f);
		setViewDir(1,0.f);
		setUpVec(0,0.f);
		setUpVec(1,0.f);
		setUpVec(2,1.f);
		setViewDir(2,-1.f);
		centerFullRegion(vwm->getRegionParams(-1));
	}
	updateRenderer(false, false, false);
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myVizTab)) {
		
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getViewpointParams(viznum)))
			updateDialog();
	}
}
//Static methods for converting between world coords and unit cube coords:
//
void ViewpointParams::
worldToCube(float fromCoords[3], float toCoords[3]){
	for (int i = 0; i<3; i++){
		toCoords[i] = (fromCoords[i]-minCubeCoord[i])/maxCubeSide;
	}
	return;
}

void ViewpointParams::
worldFromCube(float fromCoords[3], float toCoords[3]){
	for (int i = 0; i<3; i++){
		toCoords[i] = (fromCoords[i]*maxCubeSide) + minCubeCoord[i];
	}
	return;
}
void ViewpointParams::
setCoordTrans(){
	const Metadata* md = Session::getInstance()->getCurrentMetadata();
	std::vector<double> extents;
	if (md) extents = md->GetExtents();
	else { //default values
		extents = vector<double>(6);
		extents[3] = 1.;
		extents[4] = 1.;
		extents[5] = 1.;
	}
	maxCubeSide = -1.f;
	int i;
	//find largest cube side, it will map to 1.0
	for (i = 0; i<3; i++) {
		if ((float)(extents[i+3]-extents[i]) > maxCubeSide) maxCubeSide = (float)(extents[i+3]-extents[i]);
		minCubeCoord[i] = (float)extents[i];
	}
}
bool ViewpointParams::
elementStartHandler(ExpatParseMgr* pm, int  depth , std::string& tagString, const char ** attrs){
	//Get the attributes, and then make the viewpoints parse the children
	if (StrCmpNoCase(tagString, _viewpointParamsTag) == 0) {
		//If it's a Dvr tag, save 5 attributes (2 are from Params class)
		//Do this by repeatedly pulling off the attribute name and value
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _vizNumAttr) == 0) {
				ist >> vizNum;
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				if (value == "true") setLocal(true); else setLocal(false);
			}
			else return false;
		}
		return true;
	}
	//Parse current and home viewpoints
	else if (StrCmpNoCase(tagString, _currentViewTag) == 0) {
		//Need to "push" to viewpoint parser.
		//That parser will "pop" back to viewpointparams when done.
		pm->pushClassStack(currentViewpoint);
		currentViewpoint->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	}
	else if (StrCmpNoCase(tagString, _homeViewTag) == 0) {
		//Need to "push" to viewpoint parser.
		//That parser will "pop" back to viewpointparams when done.
		pm->pushClassStack(homeViewpoint);
		homeViewpoint->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	}
	else return false;
}
bool ViewpointParams::
elementEndHandler(ExpatParseMgr* pm, int depth, std::string& tag){
	if (StrCmpNoCase(tag, _viewpointParamsTag) == 0) {
		//If this is a viewpointparams, need to
		//pop the parse stack.  
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} else if (StrCmpNoCase(tag, _homeViewTag) == 0){
		return true;
	} else if (StrCmpNoCase(tag, _currentViewTag) == 0){
		return true;
	} else {
		pm->parseError("Unrecognized end tag in ViewpointParams %s",tag.c_str());
		return false;  //Could there be other end tags that we ignore??
	}
}
XmlNode* ViewpointParams::
buildNode(){
	//Construct the viewpoint node
	string empty;
	std::map <const string, string> attrs;
	attrs.clear();
	
	ostringstream oss;

	oss.str(empty);
	oss << (long)vizNum;
	attrs[_vizNumAttr] = oss.str();

	oss.str(empty);
	if (local)
		oss << "true";
	else 
		oss << "false";
	attrs[_localAttr] = oss.str();
	
	XmlNode* vpParamsNode = new XmlNode(_viewpointParamsTag, attrs, 2);

	//Now add children: home and current viewpoints  
	attrs.clear();
	XmlNode* currVP = vpParamsNode->NewChild(_currentViewTag, attrs, 1);
	currVP->AddChild(currentViewpoint->buildNode());
	XmlNode* homeVP = vpParamsNode->NewChild(_homeViewTag, attrs, 1);
	homeVP->AddChild(homeViewpoint->buildNode());
	return vpParamsNode;
}
