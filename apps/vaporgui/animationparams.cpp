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

using namespace VAPoR;

AnimationParams::AnimationParams(MainForm* mf, int winnum): Params(mf, winnum){
	thisParamType = AnimationParamsType;
	myAnimationTab = mf->getAnimationTab();
	// set everything to default state:
	isPaused = true;
	repeatPlay = true;
	minFrameSeconds = 0.1f;
	frameStepSize = 1;
	startFrame = 1;
	endFrame = 100;
	maxFrame = 1; 
	minFrame = 100;
	currentFrame = 1;
}
AnimationParams::~AnimationParams(){}


//Update the dialog with values from this:
void AnimationParams::updateDialog(){
	QString strn;
	mainWin->getSession()->blockRecording();
	float sliderVal = 0.f;
	if (endFrame > startFrame)
		sliderVal = 1000.f*(float)(currentFrame-startFrame)/(float)(endFrame-startFrame);
	myAnimationTab->animationSlider->setValue((int)sliderVal);
	myAnimationTab->startFrameEdit->setText(strn.setNum(startFrame));
	myAnimationTab->currentFrameEdit->setText(strn.setNum(currentFrame));
	myAnimationTab->endFrameEdit->setText(strn.setNum(endFrame));
	myAnimationTab->minFrameLabel->setText(strn.setNum(minFrame));
	myAnimationTab->maxFrameLabel->setText(strn.setNum(maxFrame));
	myAnimationTab->minFrameTimeEdit->setText(strn.setNum(minFrameSeconds,'g',3));
	myAnimationTab->frameStepEdit->setText(strn.setNum(frameStepSize));
	
	if (isPaused) {
		myAnimationTab->pauseButton->setDown(true);
		myAnimationTab->playButton->setDown(false);
	} else {
		myAnimationTab->pauseButton->setDown(false);
		myAnimationTab->playButton->setDown(true);
	}
	if (repeatPlay){
		myAnimationTab->replayButton->setDown(true);
		myAnimationTab->onewayButton->setDown(false);
	} else {
		myAnimationTab->replayButton->setDown(false);
		myAnimationTab->onewayButton->setDown(true);
	}
	if (isLocal())
		myAnimationTab->LocalGlobal->setCurrentItem(1);
	else 
		myAnimationTab->LocalGlobal->setCurrentItem(0);

	guiSetTextChanged(false);
	mainWin->getSession()->unblockRecording();
		
}
//And vice-versa, respond to a text box changing:
//
void AnimationParams::updatePanelState(){
	startFrame = myAnimationTab->startFrameEdit->text().toInt();
	endFrame = myAnimationTab->endFrameEdit->text().toInt();
	currentFrame = myAnimationTab->currentFrameEdit->text().toInt();
	frameStepSize = myAnimationTab->frameStepEdit->text().toInt();
	minFrameSeconds = myAnimationTab->minFrameTimeEdit->text().toFloat();

	//Update the slider
	float sliderVal = 0.f;
	if (endFrame > startFrame)
		sliderVal = 1000.f*(float)(currentFrame-startFrame)/(float)(endFrame-startFrame);
	
	guiSetTextChanged(false);
}

//Currently nothing "deep" to copy:
Params* AnimationParams::deepCopy(){
	return (Params*)new AnimationParams(*this);
}

void AnimationParams::makeCurrent(Params* p, bool newWin){
}


//Following are set by gui, result in save history state:
void AnimationParams::guiSetPause(bool val){
	confirmText(false);
	PanelCommand* cmd;
	if (val)
		cmd = PanelCommand::captureStart(this, mainWin->getSession(),"pause animation");
	else 
		cmd = PanelCommand::captureStart(this, mainWin->getSession(),"play animation");
	isPaused = val;
	PanelCommand::captureEnd(cmd, this);
}
void AnimationParams::guiGoToStart(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Jump to animation start");
	currentFrame = startFrame;
	PanelCommand::captureEnd(cmd, this);
}
void AnimationParams::guiGoToEnd(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Jump to animation end");
	currentFrame = endFrame;
	PanelCommand::captureEnd(cmd, this);
}
void AnimationParams::guiFastPlay(bool forward){
	confirmText(false);
	PanelCommand* cmd;
	if (isPaused){ // single-step forward or reverse
		cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Frame single step");
		if (forward) currentFrame += frameStepSize;
		else currentFrame -= frameStepSize;
		if (!repeatPlay){
			if (currentFrame > endFrame)currentFrame = endFrame;
			if (currentFrame < startFrame) currentFrame = startFrame;
		} else {
			if (currentFrame > endFrame) {
				currentFrame = startFrame + (currentFrame - endFrame);
				if (currentFrame > endFrame) currentFrame = startFrame;
			}
			if (currentFrame < startFrame){
				currentFrame = endFrame - (startFrame - currentFrame);
				if (currentFrame < startFrame)
					currentFrame = endFrame;
			}
		}
	}
	else { // increase or decrease playback rate
		cmd = PanelCommand::captureStart(this, mainWin->getSession(),"modify forward/reverse speed");
		if (forward) {
			frameStepSize++;
			if (frameStepSize == 0) frameStepSize = 1;
			if (frameStepSize >= (endFrame - startFrame)) frameStepSize = 1;
		} else {
			frameStepSize--;
			if (frameStepSize == 0) frameStepSize = -1;
			if (frameStepSize <= (startFrame - endFrame)) frameStepSize = -1;
		}
	}
	PanelCommand::captureEnd(cmd, this);
}


