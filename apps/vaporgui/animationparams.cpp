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
	
	// set everything to default state:
	playDirection = 0;
	repeatPlay = false;
	maxFrameRate = 1; 
	frameStepSize = 1;
	startFrame = 1;
	endFrame = 100;
	maxFrame = 100; 
	minFrame = 1;
	currentFrame = 1;
	
}
AnimationParams::~AnimationParams(){}


//Update the tab panel with values from this:
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
	
	if (playDirection==0) {//pause
		myAnimationTab->pauseButton->setDown(true);
		myAnimationTab->playForwardButton->setDown(false);
		myAnimationTab->playReverseButton->setDown(false);
		//myAnimationTab->playReverseButton->setOn(false);
		//myAnimationTab->playForwardButton->setOn(false);
		//myAnimationTab->pauseButton->setOn(true);
	} else if (playDirection==1){//forward play
		myAnimationTab->pauseButton->setDown(false);
		myAnimationTab->playForwardButton->setDown(true);
		myAnimationTab->playReverseButton->setDown(false);
		//myAnimationTab->playReverseButton->setOn(false);
		//myAnimationTab->playForwardButton->setOn(true);
		//myAnimationTab->pauseButton->setOn(false);
	} else {//reverse play
		myAnimationTab->pauseButton->setDown(false);
		myAnimationTab->playForwardButton->setDown(false);
		myAnimationTab->playReverseButton->setDown(true);
		//myAnimationTab->playReverseButton->setOn(true);
		//myAnimationTab->playForwardButton->setOn(false);
		//myAnimationTab->pauseButton->setOn(false);
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
	//Constrain to a "reasonable" range:
	if (maxFrameRate>30.f) maxFrameRate = 30.f;
	if (maxFrameRate< 0.001f) maxFrameRate = 0.001f;
	
	guiSetTextChanged(false);
}

//Currently nothing "deep" to copy:
Params* AnimationParams::deepCopy(){
	return (Params*)new AnimationParams(*this);
}
//Method to call when a new window comes to front
void AnimationParams::makeCurrent(Params* /* prev params p*/, bool/* newWin?*/){
	VizWinMgr* vwm = mainWin->getVizWinMgr();
	vwm->setAnimationParams(vizNum, this);
	updateDialog();
}


//Following are set by gui, result in save history state.
//Whenever play is pressed, it wakes up the animation controller.

void AnimationParams::guiSetPlay(int direction){
	confirmText(false);
	PanelCommand* cmd;
	switch (direction) {
		case -1:
			cmd = PanelCommand::captureStart(this, mainWin->getSession(),"play reverse");
			//myAnimationTab->pauseButton->setOn(false);
			//myAnimationTab->playForwardButton->setOn(false);
			//myAnimationTab->pauseButton->setDown(false);
			//myAnimationTab->playForwardButton->setDown(false);
			myAnimationTab->stepForwardButton->setEnabled(false);
			myAnimationTab->stepReverseButton->setEnabled(false);
			
			break;
		case 0:
			cmd = PanelCommand::captureStart(this, mainWin->getSession(),"pause animation");
			//myAnimationTab->playReverseButton->setOn(false);
			//myAnimationTab->playForwardButton->setOn(false);
			//myAnimationTab->pauseButton->setDown(true);
			//myAnimationTab->playReverseButton->setDown(false);
			//myAnimationTab->playForwardButton->setDown(false);
			myAnimationTab->stepForwardButton->setEnabled(true);
			myAnimationTab->stepReverseButton->setEnabled(true);
			break;
		case 1:
			cmd = PanelCommand::captureStart(this, mainWin->getSession(),"play forward");
			//myAnimationTab->pauseButton->setOn(false);
			//myAnimationTab->playReverseButton->setOn(false);
			//myAnimationTab->pauseButton->setDown(false);
			//myAnimationTab->playReverseButton->setDown(false);
			myAnimationTab->stepForwardButton->setEnabled(false);
			myAnimationTab->stepReverseButton->setEnabled(false);
			
			break;
		default:
			assert(0);
	}
	int previousDirection = playDirection;
	playDirection = direction;
	PanelCommand::captureEnd(cmd, this);
	if (direction && !previousDirection)mainWin->getVizWinMgr()->startPlay(this);
	else if (!direction) mainWin->getVizWinMgr()->animationParamsChanged(this);
	setDirty();
	//May need to wakeup the controller if starting to play.
	if( direction && !previousDirection) {
		AnimationController::getInstance()->wakeup();
	}
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
	mainWin->getVizWinMgr()->animationParamsChanged(this);
	setDirty();
	
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
	mainWin->getVizWinMgr()->animationParamsChanged(this);
	setDirty();
}


//Respond to release of frame position slider:
void AnimationParams::guiSetPosition(int position){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Change current frame number");
	currentFrame = startFrame + (int)((float)(endFrame - startFrame)*(float)position/1000.f);
	myAnimationTab->currentFrameEdit->setText(QString::number(currentFrame));
	PanelCommand::captureEnd(cmd, this);
	guiSetTextChanged(false);
	mainWin->getVizWinMgr()->animationParamsChanged(this);
	setDirty();
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
}
void AnimationParams::guiToggleReplay(bool replay){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"Toggle replay button");
	repeatPlay = replay;
	PanelCommand::captureEnd(cmd, this);
	
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
	mainWin->getVizWinMgr()->animationParamsChanged(this);
	setDirty();
	

}
//Respond to change in Metadata
//
void AnimationParams::
reinit(){
	Session* session = mainWin->getSession();
	//Get the max, min time ranges:
	minFrame = session->getMinTimestep();
	maxFrame = session->getMaxTimestep();
	//Narrow the range to the actual data limits:
	int numvars = session->getNumVariables();
	//Find the first framenum with data:
	int i;
	for (i = minFrame; i<= maxFrame; i++){
		int varnum;
		for (varnum = 0; varnum<numvars; varnum++){
			if(session->dataIsPresent(varnum, i)) break;
		}
		if (varnum < numvars) break;
	}
	minFrame = i;
	//Find the last framenum with data:
	for (i = maxFrame; i>= minFrame; i--){
		int varnum;
		for (varnum = 0; varnum<numvars; varnum++){
			if(session->dataIsPresent(varnum, i)) break;
		}
		if (varnum < numvars) break;
	}
	maxFrame = i;

	//force animation range to be inside limits
	if (startFrame < minFrame) startFrame = minFrame;
	if (startFrame > maxFrame) startFrame = maxFrame;
	if (endFrame < minFrame) endFrame = minFrame;
	if (endFrame > maxFrame) endFrame = maxFrame;
	if (currentFrame < minFrame) currentFrame = minFrame;
	if (currentFrame > maxFrame) currentFrame = maxFrame;
	setDirty();
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
//Following method called when a frame has been rendered, to update the currentFrame, in
//preparation for next rendering.
//It should not be called if the UI has changed the frame number; in that case
//setDirty should be called, and AnimationController::paramsChanged() should be
//called, to force any ongoing animation to not increment the frame number
void AnimationParams::
advanceFrame(){
	assert(playDirection);
	int lastFrameCompleted = currentFrame;
	currentFrame += playDirection*frameStepSize;
	if (currentFrame > endFrame) {
		if (repeatPlay) {
			currentFrame = startFrame + (currentFrame - endFrame);
			if (currentFrame > endFrame) currentFrame = endFrame;
		} else {
			//Stop if we are at the end:
			currentFrame = endFrame;
		}
	}
	if (currentFrame < startFrame){
		if (repeatPlay) {
			currentFrame = endFrame - (startFrame - currentFrame);
			if (currentFrame < startFrame) currentFrame = startFrame;
		} else {
			//Stop if we are at the end:
			currentFrame = startFrame;
		}
	}
	if (currentFrame == lastFrameCompleted) {
		//Change to "pause" status:
		playDirection = 0;
		mainWin->getVizWinMgr()->animationParamsChanged(this);
	}
	//Only update the dialog if this animation is in the active visualizer
	int viznum = mainWin->getVizWinMgr()->getActiveViz();
	if (viznum >= 0 && mainWin->getVizWinMgr()->getAnimationParams(viznum) == this)
		updateDialog();
}
	
void AnimationParams::setDirty(){
	mainWin->getVizWinMgr()->setAnimationDirty(this);
}
//Set change bits when global/local change occurs, so that the animation
//controller will change the local/global status at the end of the next rendering.
void AnimationParams::guiSetLocal(bool lg){
	Params::guiSetLocal(lg);
	int viznum = vizNum;
	if (viznum < 0){
		viznum = mainWin->getVizWinMgr()->getActiveViz();
		assert(viznum >=0);
	}
	AnimationController::getInstance()->paramsChanged(viznum);
}

