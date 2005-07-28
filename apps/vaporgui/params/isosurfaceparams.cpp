//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isosurfaceparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Implementation of isosurfaceparams class 
//		Currently this is a stand-in, intended to eventually support
//		all the parameters needed for an isosurface renderer
//		initially this is just rendering a glbox.
//
#include "isosurfaceparams.h"
#include "isotab.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "glbox.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include "mainform.h"
#include "session.h"
#include "params.h"
#include "panelcommand.h"
#include "tabmanager.h"
using namespace VAPoR;
IsosurfaceParams::IsosurfaceParams(int winnum) : Params(winnum) {
	thisParamType = IsoParamsType;
	myIsoTab = MainForm::getInstance()->getIsoTab();
	enabled = false;
	//Set default values
	diffuseCoeff = 0.8f; 
	ambientCoeff = 0.8f;
	specularCoeff = 0.8f;;
	specularExponent = 30;
	color1 = new QColor(200, 128, 128);
	value1 = 130.f;
	opac1 = 1.f;
	variable1 = 1;
	numSurfaces = 3;
	
}

//Make a copy of  parameters:
Params* IsosurfaceParams::
deepCopy(){
	IsosurfaceParams* newIsoParams = new IsosurfaceParams(*this);
	newIsoParams->color1 = new QColor(*(color1));
	return (Params*)newIsoParams;
}

IsosurfaceParams::~IsosurfaceParams(){
	delete color1;
}
void IsosurfaceParams::
makeCurrent(Params* prev, bool newWin) {
	
	VizWinMgr::getInstance()->setIsoParams(vizNum, this);
	updateDialog();
	//Need to create/destroy renderer if there's a change in local/global or enable/disable
	//or if the window is new
	//
	IsosurfaceParams* formerParams = (IsosurfaceParams*)prev;
	if (formerParams->enabled != enabled || formerParams->local != local || newWin){
		updateRenderer(formerParams->enabled, formerParams->local, newWin);
	}
}

void IsosurfaceParams::updateDialog(){
	
	Session::getInstance()->blockRecording();
	myIsoTab->EnableDisable->setCurrentItem((enabled) ? 1 : 0);
	QString strn;
	myIsoTab->variableCombo1->setCurrentItem(variable1);
	myIsoTab->diffuseShading->setText(strn.setNum(diffuseCoeff, 'g', 3));
	myIsoTab->ambientShading->setText(strn.setNum(ambientCoeff, 'g', 3));
	myIsoTab->specularShading->setText(strn.setNum(specularCoeff, 'g', 3));
	myIsoTab->exponentShading->setText(strn.setNum(specularExponent));

	myIsoTab->colorButton1->setPaletteBackgroundColor(*color1);
	myIsoTab->valueEdit1->setText(QString::number(value1,'g', 4));
	
	myIsoTab->valueSlider1->setValue((int)(0.5+ value1/0.375));
	myIsoTab->opacEdit1->setText(QString::number(opac1,'g', 3));
	
	myIsoTab->opacSlider1->setValue((int)(opac1*255. + 0.5));
	myIsoTab->variableCombo1->setCurrentItem( variable1);
	myIsoTab->numLevelSets->setValue(numSurfaces);
	
	
	
	if (isLocal())
		myIsoTab->LocalGlobal->setCurrentItem(1);
	else 
		myIsoTab->LocalGlobal->setCurrentItem(0);
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
}

//Update all the panel state associated with textboxes, after user presses return
//
void IsosurfaceParams::
updatePanelState(){
	
	diffuseCoeff = myIsoTab-> diffuseShading->text().toFloat();
	ambientCoeff = myIsoTab->ambientShading->text().toFloat();
	specularCoeff = myIsoTab->specularShading->text().toFloat();
	specularExponent = myIsoTab->exponentShading->text().toFloat();
	value1 = myIsoTab->valueEdit1->text().toFloat();
	if (value1 > 375.) value1 = 375.;
	if (value1 < 0.) value1 = 0.;
	myIsoTab->valueSlider1->setValue((int)(value1*1000./375.));
	opac1 = myIsoTab->opacEdit1->text().toFloat();
	if (opac1 > 1.0) opac1 = 1.0;
	if (opac1 < 0.0) opac1 = 0.0;
	myIsoTab->opacSlider1->setValue((int)(opac1*255.5));
	guiSetTextChanged(false);
}
//Methods that record changes in the history:
//
void IsosurfaceParams::
guiSetEnabled(bool on){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "enable/disable iso render");
	setEnabled(on);
	PanelCommand::captureEnd(cmd, this);
}
void IsosurfaceParams::
guiSetColor1(QColor* c){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "change iso color");
	setColor1(c);
	PanelCommand::captureEnd(cmd, this);
}
void IsosurfaceParams::
guiSetOpac1(int val){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "change iso opacity");
	setOpac1((float)val/255.);
	myIsoTab->opacEdit1->setText(QString::number(opac1, 'g', 2));
	textChangedFlag = false;
	PanelCommand::captureEnd(cmd, this);
}
void IsosurfaceParams::
guiSetValue1(int val){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "change iso value");
	setValue1(val*375./1000.);
	myIsoTab->valueEdit1->setText(QString::number(value1, 'g', 4));
	
	textChangedFlag = false;
	PanelCommand::captureEnd(cmd, this);
}
void IsosurfaceParams::
guiSetNumSurfaces(int n){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set num isosurfaces");
	setNumSurfaces(n);
	PanelCommand::captureEnd(cmd, this);
}
void IsosurfaceParams::
guiSetVariable1(int n){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "change iso variable");
	//Assume the max value is 375
	setVariable1((float)n/375.);
	PanelCommand::captureEnd(cmd, this);
}
	
/* Handle the change of status associated with change of enablement and change
 * of local/global.  If we are enabling global, a renderer must be created in every
 * global window, including active one.  If we are enabling local, only active one is created.
 * If we change from local to global, (no change in enablement) then new renderers are
 * created for every additional global window.  Similar for disable.
 * It can occur that both enablement and local/global change, if the local and global enablement
 * are different, during a local/global change
 * This assumes that the VizWinMgr already is set with the current (new) local/global
 * iso settings.  
 * If the window is new, (i.e. we are just creating a new window, use: 
 * prevEnabled = false, wasLocal = isLocal = true,
 * even if the renderer is really global, since we don't want to affect other global renderers.
 */
void IsosurfaceParams::
updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow){
	bool isLocal = local;
	if (newWindow) {
		prevEnabled = false;
		wasLocal = true;
		isLocal = true;
	}
	if (prevEnabled == enabled && wasLocal == local) return;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	VizWin* viz = vizWinMgr->getActiveVisualizer();
	if (!viz) return;
	int activeViz = vizWinMgr->getActiveViz();
	assert(activeViz >= 0);
	
	//Four cases to consider:
	//1.  change of local/global with unchanged disabled renderer; do nothing.
	// Same for change of local/global with unchanged, enabled renderer
	
	if (prevEnabled == enabled) return;
	
	//2.  Change of disable->enable with unchanged local renderer.  Create a new renderer in active window.
	// Also applies to double change: disable->enable and local->global 
	// Also applies to disable->enable with global->local
	//3.  change of disable->enable with unchanged global renderer.  Create new renderers in all global windows, 
	//    including active window.
	//5.  Change of enable->disable with unchanged global , disable all global renderers, provided the
	//   VizWinMgr already has the current local/global iso settings
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	//  change of enable->disable with global->local.  (Must disable the local renderer)
	//  change of enable->disable with local->global (Must disable the local renderer)
	
	//For a new renderer

	
	if (enabled && !prevEnabled){//For cases 2. or 3. :  create a renderer in the active window:
		GLBox* myBox = new GLBox (viz);
		viz->insertRenderer(myBox, IsoParamsType);
		//Quit if not case 3:
		if (wasLocal || isLocal) return;
	}
	
	if (!isLocal && enabled){ //case 3: create renderers in all *other* global windows, then return
		for (int i = 0; i<MAXVIZWINS; i++){
			if (i == activeViz) continue;
			viz = VizWinMgr::getInstance()->getVizWin(i);
			if (viz && !vizWinMgr->getIsoParams(i)->isLocal()){
				GLBox* myBox = new GLBox (viz);
				viz->insertRenderer(myBox, IsoParamsType);
			}
		}
		return;
	}
	if (!enabled && prevEnabled && !isLocal && !wasLocal) { //case 5., disable all global renderers
		for (int i = 0; i<MAXVIZWINS; i++){
			viz = VizWinMgr::getInstance()->getVizWin(i);
			if (viz && !vizWinMgr->getIsoParams(i)->isLocal()){
				viz->removeRenderer(IsoParamsType);
			}
		}
		return;
	}
	assert(prevEnabled && !enabled && (isLocal ||(isLocal != wasLocal))); //case 6, disable local only
	viz->removeRenderer(IsoParamsType);
	return;
}
