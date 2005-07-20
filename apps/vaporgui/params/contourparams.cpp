//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		contourparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:	Implementation of the ContourParams class
//
#include "contourparams.h"
#include "contourplanetab.h"
#include "panelcommand.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include "mainform.h"
#include "params.h"
#include "vizwinmgr.h"
#include "session.h"
#include "tabmanager.h"
using namespace VAPoR;
ContourParams::ContourParams(int winnum) : Params(winnum){
	thisParamType = ContourParamsType;
	myContourTab = MainForm::getInstance()->getContourTab();
	enabled = false;
	varNum = 0;
	lightingOn = false;
	diffuseCoeff = 0.8f;
	ambientCoeff = 0.5f;
	specularCoeff = 0.3f;
	specularExponent = 20;
	normal[0] = 0.f;
	normal[1] = 0.8f;
	normal[2] = 0.6f;
	pointInPlane[0] = 4.f;
	pointInPlane[1] = 2.5f;
	pointInPlane[2] = .6f;
	
}

ContourParams::~ContourParams(){
}

Params* ContourParams::
deepCopy(){
	//Just make a shallow copy, since there's nothing (yet) extra
	return (Params*)(new ContourParams(*this));
}

void ContourParams::
makeCurrent(Params*, bool ) {
	
	VizWinMgr::getInstance()->setContourParams(vizNum, this);
	//Also update current Tab.  It's probably visible.
	updateDialog();
}

void ContourParams::updateDialog(){
	
	QString strn;
	
	Session::getInstance()->blockRecording();
	myContourTab->EnableDisable->setCurrentItem((enabled) ? 1 : 0);
	myContourTab->variableCombo1->setCurrentItem(varNum);
	myContourTab->lightingCheckbox->setChecked(lightingOn);
	
	myContourTab->diffuseShading->setText(strn.setNum(diffuseCoeff, 'g', 2));
	myContourTab->ambientShading->setText(strn.setNum(ambientCoeff, 'g', 2));
	myContourTab->specularShading->setText(strn.setNum(specularCoeff, 'g', 2));
	myContourTab->exponentShading->setText(strn.setNum(specularExponent));
	myContourTab->normalx->setText(strn.setNum(normal[0], 'g', 3));
	myContourTab->pointx->setText(strn.setNum(pointInPlane[0], 'g', 3));
	myContourTab->normaly->setText(strn.setNum(normal[1], 'g', 3));
	myContourTab->pointy->setText(strn.setNum(pointInPlane[1], 'g', 3));
	myContourTab->normalz->setText(strn.setNum(normal[2], 'g', 3));
	myContourTab->pointz->setText(strn.setNum(pointInPlane[2], 'g', 3));
	
	if (isLocal())
		myContourTab->LocalGlobal->setCurrentItem(1);
	else 
		myContourTab->LocalGlobal->setCurrentItem(0);
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
		
}
//Update all the panel state associated with textboxes.
void ContourParams::
updatePanelState(){
	
	diffuseCoeff = myContourTab->diffuseShading->text().toFloat();
	
	ambientCoeff = myContourTab->ambientShading->text().toFloat();
	specularCoeff = myContourTab->specularShading->text().toFloat();
	
	specularExponent = myContourTab->exponentShading->text().toInt();
	
	normal[0] = myContourTab->normalx->text().toFloat();
	normal[1] = myContourTab->normaly->text().toFloat();
	normal[2] = myContourTab->normalz->text().toFloat();

	pointInPlane[0] = myContourTab->pointx->text().toFloat();
	pointInPlane[1] = myContourTab->pointy->text().toFloat();
	pointInPlane[2] = myContourTab->pointz->text().toFloat();
	guiSetTextChanged(false);
	
}

void ContourParams::
guiSetEnabled(bool value){
	if (textChangedFlag) confirmText(false);
	//Capture previous state:
	PanelCommand* cmd = PanelCommand::captureStart(this, "toggle contour enabled");
	
	setEnabled(value);
	//Save new state
	PanelCommand::captureEnd(cmd, this);
	//updateRenderer();
}
void ContourParams::
guiSetLighting(bool val){
	if (textChangedFlag) confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "toggle contour lighting");
	updatePanelState();
	setLighting(val);
	PanelCommand::captureEnd(cmd, this);
	//updateRenderer();
}
void ContourParams::
guiSetVarNum(int val){
	if (textChangedFlag) confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,"contour set variable");
	setVarNum(val);
	//Save new state
	PanelCommand::captureEnd(cmd, this);
	//updateRenderer();
}
