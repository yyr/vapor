//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		animationparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		January 2005
//
//	Description:	Implements the AnimationParams class
//		This is derived from the Params class
//		It contains all the parameters required for animation

//

#include <qwidget.h>
#include <qslider.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qcombobox.h>
#include "params.h"
#include "animationparams.h"
#include "mainform.h"
#include "session.h"
#include "animationtab.h"
#include "panelcommand.h"
#include "animationcontroller.h"
#include "session.h"
#include "vizwinmgr.h"

using namespace VAPoR;

AnimationParams::AnimationParams(MainForm* mf, int winnum): Params(mf, winnum){
	thisParamType = AnimationParamsType;
	myAnimationTab = mf->getAnimationTab();
	myAnimationController = 0;
	// set everything to default state:
	playDirection = 0;
	repeatPlay = false;
	maxFrameRate = 10; 
	frameStepSize = 1;
	startFrame = 1;
	endFrame = 100;
	maxFrame = 100; 
	minFrame = 1;
	currentFrame = 1;
	
}
AnimationParams::~AnimationParams(){}


//Update the dialog with values from this:
void AnimationParams::updateDialog(){
	float sliderVal;
	QString strn;
	mainWin->getSession()->blockRecording();
	sliderVal = 0.f;
	if (endFrame > startFrame)
		sliderVal = 1000.f*(float)(currentFrame-startFrame)/(float)(endFrame-startFrame);
	myAnimationTab->animationSlider->setValue((int)sliderVal);
	sliderVal = 0.f;
	
	myAnimationTab->startFrameEdit->setText(strn.setNum(startFrame));
	myAnimationTab->currentFrameEdit->setText(strn.setNum(currentFrame));
	myAnimationTab->endFrameEdit->setText(strn.setNum(endFrame));
	myAnimationTab->minFrameLabel->setText(strn.setNum(minFrame));
	myAnimationTab->maxFrameLabel->setText(strn.setNum(maxFrame));
	myAnimationTab->maxFrameRateEdit->setText(strn.setNum(maxFrameRate,'g',3));
	myAnimationTab->frameStepEdit->setText(strn.setNum(frameStepSize));
	
	if (playDirection==0) {
		myAnimationTab->pauseButton->setDown(true);
		myAnimationTab->playForwardButton->setDown(false);
		myAnimationTab->playReverseButton->setDown(false);
	} else if (playDirection==1){
		myAnimationTab->pauseButton->setDown(false);
		myAnimationTab->playForwardButton->setDown(true);
		myAnimationTab->playReverseButton->setDown(false);
	} else {
		myAnimationTab->pauseButton->setDown(false);
		myAnimationTab->playForwardButton->setDown(false);
		myAnimationTab->playReverseButton->setDown(true);
	}
	if (repeatPlay){
		myAnimationTab->replayButton->setDown(true);
	} else {
		myAnimationTab->replayButton->setDown(false);
	}
	if (isLocal())
		myAnimationTab->LocalGlobal->setCurrentItem(1);
	else 
		myAnimationTab->LocalGlobal->setCurrentItem(0);

	guiSetTextChanged(false);
	mainWin->getSession()->unblockRecording();
		
}
//Update the params, in response to one or more text boxes changing
//
void AnimationParams::updatePanelState(){
	QString strn;
	startFrame = myAnimationTab->startFrameEdit->text().toInt();
	if (startFrame < minFrame || startFrame > maxFrame) {
		startFrame = minFrame;
		myAnimationTab->startFrameEdit->setText(strn.setNum(startFrame));
	}
	endFrame = myAnimationTab->endFrameEdit->text().toInt();
	if (endFrame < minFrame || endFrame > maxFrame || endFrame < startFrame) {
		endFrame = maxFrame;
		myAnimationTab->endFrameEdit->setText(strn.setNum(endFrame));
	}
	currentFrame = myAnimationTab->currentFrameEdit->text().toInt();
	if (currentFrame < startFrame) currentFrame = startFrame;
	if (currentFrame > endFrame) currentFrame = endFrame;
	myAnimationTab->currentFrameEdit->setText(strn.setNum(currentFrame));
	
	frameStepSize = myAnimationTab->frameStepEdit->text().toInt();
	if (frameStepSize > (maxFrame - minFrame)) frameStepSize = maxFrame - minFrame;
	if (frameStepSize < 1) frameStepSize = 1;
	//Modify sliders if there's been a change:
	setSliders();

	maxFrameRate = myAnimationTab->maxFrameRateEdit->text().toFloat();
	
	guiSetTextChanged(false);
}

//Currently nothing "deep" to copy:
Params* AnimationParams::deepCopy(){
	return (Params*)new AnimationParams(*this);
}
//Method to call when a new window comes to front
void AnimationParams::makeCurrent(Params* p, bool newWin){
	VizWinMgr* vwm = mainWin->getVizWinMgr();
	vwm->setAnimationParams(vizNum, this);
	updateDialog();
}


//Following are set by gui, result in save history state:

void AnimationParams::guiSetPlay(int direction){
	confirmText(false);
	PanelCommand* cmd;
	switch (direction) {
		case -1:
			cmd = PanelCommand::captureStart(this, mainWin->getSession(),"play reverse");
			myAnimationTab->pauseButton->setOn(false);
			myAnimationTab->playForwardButton->setOn(false);
			myAnimationTab->pauseButton->setDown(false);
			myAnimationTab->playForwardButton->setDown(false);
			myAnimationTab->stepForwardButton->setEnabled(false);
			myAnimationTab->stepReverseButton->setEnabled(false);
			break;
		case 0:
			cmd = PanelCommand::captureStart(this, mainWin->getSession(),"pause animation");
			myAnimationTab->playReverseButton->setOn(false);
			myAnimationTab->playForwardButton->setOn(false);
			myAnimationTab->playReverseButton->setDown(false);
			myAnimationTab->playForwardButton->setDown(false);
			myAnimationTab->stepForwardButton->setEnabled(true);
			myAnimationTab->stepReverseButton->setEnabled(true);
			break;
		case 1:
			cmd = PanelCommand::captureStart(this, mainWin->getSession(),"play forward");
			myAnimationTab->pauseButton->setOn(false);
			myAnimationTab->playReverseButton->setOn(false);
			myAnimationTab->pauseButton->setDown(false);
			myAnimationTab->playReverseButton->setDown(false);
			myAnimationTab->stepForwardButton->setEnabled(false);
			myAnimationTab->stepReverseButton->setEnabled(false);
			break;
		default:
			assert(0);
	}
	playDirection = direction;
	PanelCommand::captureEnd(cmd, this);
	dirtyBit = true;
	if(myAnimationController)myAnimationController->wakeup();
}
void AnimationParams::
guiJumpToBegin(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Jump to animation start");
	currentFrame = startFrame;
	myAnimationTab->currentFrameEdit->setText(QString::number(currentFrame));
	setSliders();
	PanelCommand::captureEnd(cmd, this);
	guiSetTextChanged(false);
	dirtyBit = true;
	if(myAnimationController)myAnimationController->wakeup();
}
void AnimationParams::
guiJumpToEnd(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Jump to animation end");
	currentFrame = endFrame;
	myAnimationTab->currentFrameEdit->setText(QString::number(currentFrame));
	setSliders();
	PanelCommand::captureEnd(cmd, this);
	guiSetTextChanged(false);
	dirtyBit = true;
	if(myAnimationController)myAnimationController->wakeup();
}


//Respond to release of frame position slider:
void AnimationParams::guiSetPosition(int position){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Change current frame number");
	currentFrame = startFrame + (int)((float)(endFrame - startFrame)*(float)position/1000.f);
	myAnimationTab->currentFrameEdit->setText(QString::number(currentFrame));
	PanelCommand::captureEnd(cmd, this);
	guiSetTextChanged(false);
	dirtyBit = true;
	if(myAnimationController)myAnimationController->wakeup();
}
//Respond to release of stepsize slider.  Max range of slider is full animation length
void AnimationParams::guiSetFrameStep(int stepsize){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Change current step size");
	frameStepSize = (int)((float)(maxFrame - minFrame)*(float)stepsize/1000.f);
	if (frameStepSize < 1) frameStepSize = 1;
	myAnimationTab->frameStepEdit->setText(QString::number(frameStepSize));
	setSliders();
	PanelCommand::captureEnd(cmd, this);
	guiSetTextChanged(false);
	dirtyBit = true;
	if(myAnimationController)myAnimationController->wakeup();
}
void AnimationParams::guiToggleReplay(bool replay){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Toggle replay button");
	repeatPlay = replay;
	PanelCommand::captureEnd(cmd, this);
	dirtyBit = true;
	if(myAnimationController)myAnimationController->wakeup();
}
void AnimationParams::guiSingleStep(bool forward){
	confirmText(false);
	PanelCommand* cmd;
	if (forward){
		cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Single-step forward");
		currentFrame += frameStepSize;
	} else {
		cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Single-step reverse");
		currentFrame -= frameStepSize;
	}
	if (endFrame == startFrame) currentFrame = startFrame;
	else {
		if (!repeatPlay) {
			if (currentFrame > endFrame) currentFrame = endFrame;
			if (currentFrame < startFrame) currentFrame = startFrame;
		}
		else {// repeating.  Force to lie in region
			if (currentFrame > endFrame) currentFrame = startFrame + (currentFrame -endFrame)%(endFrame-startFrame+1);
			if (currentFrame < startFrame) currentFrame = endFrame - (startFrame-currentFrame)%(endFrame-startFrame+1);
		}
	}
	myAnimationTab->currentFrameEdit->setText(QString::number(currentFrame));
	setSliders();
	guiSetTextChanged(false);
	assert(currentFrame >= startFrame && currentFrame <= endFrame);
	PanelCommand::captureEnd(cmd, this);
	dirtyBit = true;
	if(myAnimationController)myAnimationController->wakeup();
}
//Respond to change in Metadata
//
void AnimationParams::
reinit(){
	Session* session = mainWin->getSession();
	//Get the max, min time ranges:
	minFrame = session->getMinTimestep();
	maxFrame = session->getMaxTimestep();
	//force animation range to be inside limits
	if (startFrame < minFrame) startFrame = minFrame;
	if (startFrame > maxFrame) startFrame = maxFrame;
	if (endFrame < minFrame) endFrame = minFrame;
	if (endFrame > maxFrame) endFrame = maxFrame;
	if (currentFrame < minFrame) currentFrame = minFrame;
	if (currentFrame > maxFrame) currentFrame = maxFrame;
	myAnimationController = session->getAnimationController();
	dirtyBit = true;
	updateDialog();
}
//Set the position slider consistent with latest value of currentPosition, frameStep, and bounds
void AnimationParams::
setSliders(){
	int sliderPosition = myAnimationTab->animationSlider->value();
	int sliderFrame = startFrame + (int)((float)(endFrame - startFrame)*(float)sliderPosition/1000.f);
	if (sliderFrame != currentFrame && (endFrame != startFrame)){
		myAnimationTab->animationSlider->setValue((int)(1000.f*((float)(currentFrame - startFrame)/(float)(endFrame-startFrame))));
	}
	sliderPosition = myAnimationTab->frameStepSlider->value();
	int stepsize = (int)((float)(endFrame - startFrame)*(float)sliderPosition/1000.f);
	if(stepsize < 1) stepsize = 1;
	if (stepsize != frameStepSize && endFrame != startFrame){
		myAnimationTab->frameStepSlider->setValue((int)(1000.f*((float)stepsize/(float)(endFrame-startFrame))));
	}
}
