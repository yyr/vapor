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
	playDirection = 0;
	repeatPlay = true;
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
//And vice-versa, respond to a text box changing:
//
void AnimationParams::updatePanelState(){
	startFrame = myAnimationTab->startFrameEdit->text().toInt();
	endFrame = myAnimationTab->endFrameEdit->text().toInt();
	currentFrame = myAnimationTab->currentFrameEdit->text().toInt();
	frameStepSize = myAnimationTab->frameStepEdit->text().toInt();
	maxFrameRate = myAnimationTab->maxFrameRateEdit->text().toFloat();

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

void AnimationParams::guiSetPlay(int direction){
	confirmText(false);
	PanelCommand* cmd;
	switch (direction) {
		case -1:
			cmd = PanelCommand::captureStart(this, mainWin->getSession(),"play reverse");
			break;
		case 0:
			cmd = PanelCommand::captureStart(this, mainWin->getSession(),"pause animation");
			break;
		case 1:
			cmd = PanelCommand::captureStart(this, mainWin->getSession(),"play forward");
			break;
		default:
			assert(0);
	}
	playDirection = direction;
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



void AnimationParams::guiReleasePositionSlider(int position){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Change current frame number");
	currentFrame = position;
	PanelCommand::captureEnd(cmd, this);
}

void AnimationParams::guiReleaseStepsizeSlider(int position){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Change current step size");
	frameStepSize = position;
	PanelCommand::captureEnd(cmd, this);
}
