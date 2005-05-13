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
#include "vizwin.h"
#include "tabmanager.h"

using namespace VAPoR;

AnimationParams::AnimationParams(int winnum): Params( winnum){
	thisParamType = AnimationParamsType;
	myAnimationTab = MainForm::getInstance()->getAnimationTab();
	restart();
	
	
}
AnimationParams::~AnimationParams(){}


//Update the tab panel with values from this:
void AnimationParams::updateDialog(){
	float sliderVal;
	QString strn;
	Session::getInstance()->blockRecording();
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
		//myAnimationTab->pauseButton->setDown(true);
		myAnimationTab->playForwardButton->setDown(false);
		myAnimationTab->playReverseButton->setDown(false);
		
	} else if (playDirection==1){//forward play
		//myAnimationTab->pauseButton->setDown(false);
		myAnimationTab->playForwardButton->setDown(true);
		myAnimationTab->playReverseButton->setDown(false);
		
	} else {//reverse play
		//myAnimationTab->pauseButton->setDown(false);
		myAnimationTab->playForwardButton->setDown(false);
		myAnimationTab->playReverseButton->setDown(true);
	}
	if (repeatPlay){
		myAnimationTab->replayButton->setOn(true);
	} else {
		myAnimationTab->replayButton->setOn(false);
	}
	if (isLocal())
		myAnimationTab->LocalGlobal->setCurrentItem(1);
	else 
		myAnimationTab->LocalGlobal->setCurrentItem(0);

	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
		
}
//Update the params, in response to one or more text boxes changing
//
void AnimationParams::updatePanelState(){
	int savedFrameNum = currentFrame;
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
	if (frameStepSize > (endFrame - startFrame)) frameStepSize = endFrame - startFrame;
	if (frameStepSize < 1) frameStepSize = 1;
	myAnimationTab->frameStepEdit->setText(strn.setNum(frameStepSize));
	//Modify sliders if there's been a change:
	setSliders();

	maxFrameRate = myAnimationTab->maxFrameRateEdit->text().toFloat();
	//Constrain to a "reasonable" range:
	if (maxFrameRate>30.f) maxFrameRate = 30.f;
	if (maxFrameRate< 0.001f) maxFrameRate = 0.001f;
	VizWinMgr::getInstance()->animationParamsChanged(this);
	if (currentFrame != savedFrameNum){
		setDirty();
	}
	guiSetTextChanged(false);
	myAnimationTab->update();
}

//Currently nothing "deep" to copy:
Params* AnimationParams::deepCopy(){
	return (Params*)new AnimationParams(*this);
}
//Method to call when a new window installed via undo/redo
void AnimationParams::makeCurrent(Params* /* prev params p*/, bool/* newWin?*/){
	VizWinMgr* vwm = VizWinMgr::getInstance();
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
			cmd = PanelCommand::captureStart(this, "play reverse");
			
			myAnimationTab->stepForwardButton->setEnabled(false);
			myAnimationTab->stepReverseButton->setEnabled(false);
			
			break;
		case 0:
			cmd = PanelCommand::captureStart(this, "pause animation");
			
			myAnimationTab->stepForwardButton->setEnabled(true);
			myAnimationTab->stepReverseButton->setEnabled(true);
			break;
		case 1:
			cmd = PanelCommand::captureStart(this, "play forward");
			
			myAnimationTab->stepForwardButton->setEnabled(false);
			myAnimationTab->stepReverseButton->setEnabled(false);
			
			break;
		default:
			assert(0);
	}
	int previousDirection = playDirection;
	playDirection = direction;
	PanelCommand::captureEnd(cmd, this);
	if (direction && !previousDirection)VizWinMgr::getInstance()->startPlay(this);
	else if (!direction) VizWinMgr::getInstance()->animationParamsChanged(this);
	myAnimationTab->update();
	setDirty();
	//May need to wakeup the controller if starting to play.
	//No problem:  At most there's a one-second delay!
	//if( direction && !previousDirection) {
		//AnimationController::getInstance()->wakeup();
	//}
}
void AnimationParams::
guiJumpToBegin(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "Jump to animation start");
	currentFrame = startFrame;
	myAnimationTab->currentFrameEdit->setText(QString::number(currentFrame));
	setSliders();
	PanelCommand::captureEnd(cmd, this);
	guiSetTextChanged(false);
	myAnimationTab->update();
	VizWinMgr::getInstance()->animationParamsChanged(this);
	setDirty();
	
}
void AnimationParams::
guiJumpToEnd(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "Jump to animation end");
	currentFrame = endFrame;
	myAnimationTab->currentFrameEdit->setText(QString::number(currentFrame));
	setSliders();
	PanelCommand::captureEnd(cmd, this);
	guiSetTextChanged(false);
	myAnimationTab->update();
	VizWinMgr::getInstance()->animationParamsChanged(this);
	setDirty();
}


//Respond to release of frame position slider:
void AnimationParams::guiSetPosition(int position){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "Change current frame number");
	currentFrame = startFrame + (int)((float)(endFrame - startFrame)*(float)position/1000.f);
	myAnimationTab->currentFrameEdit->setText(QString::number(currentFrame));
	PanelCommand::captureEnd(cmd, this);
	guiSetTextChanged(false);
	myAnimationTab->update();
	VizWinMgr::getInstance()->animationParamsChanged(this);
	setDirty();
}
//Respond to release of stepsize slider.  Max range of slider is end - start
void AnimationParams::guiSetFrameStep(int stepsize){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "Change current step size");
	frameStepSize = (int)((float)(endFrame - startFrame)*(float)stepsize/1000.f);
	if (frameStepSize < 1) frameStepSize = 1;
	myAnimationTab->frameStepEdit->setText(QString::number(frameStepSize));
	PanelCommand::captureEnd(cmd, this);
	guiSetTextChanged(false);
	myAnimationTab->update();
	
	
}
void AnimationParams::guiToggleReplay(bool replay){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,"Toggle replay button");
	repeatPlay = replay;
	PanelCommand::captureEnd(cmd, this);
	myAnimationTab->update();
	
}
void AnimationParams::guiSingleStep(bool forward){
	confirmText(false);
	PanelCommand* cmd;
	if (forward){
		cmd = PanelCommand::captureStart(this,"Single-step forward");
		currentFrame += frameStepSize;
	} else {
		cmd = PanelCommand::captureStart(this,"Single-step reverse");
		currentFrame -= frameStepSize;
	}
	if (endFrame == startFrame) currentFrame = startFrame;
	else {
		if (!repeatPlay) {
			if (currentFrame > endFrame) currentFrame = endFrame;
			if (currentFrame < startFrame) currentFrame = startFrame;
		}
		else {// repeating.  Force to loop in region
			if (currentFrame > endFrame) {
				assert(forward);
				currentFrame = startFrame -1 + (currentFrame -endFrame);
				if (currentFrame > endFrame) currentFrame = endFrame;
			}
			if (currentFrame < startFrame) {
				assert (!forward);
				currentFrame = endFrame +1 - (startFrame-currentFrame);
				if (currentFrame < startFrame) currentFrame = startFrame;
			}
		}
	}
	myAnimationTab->currentFrameEdit->setText(QString::number(currentFrame));
	setSliders();
	guiSetTextChanged(false);
	assert(currentFrame >= startFrame && currentFrame <= endFrame);
	PanelCommand::captureEnd(cmd, this);
	VizWinMgr::getInstance()->animationParamsChanged(this);
	setDirty();
	myAnimationTab->update();
	

}
//Reset to initial state
//
void AnimationParams::
restart(){
	// set everything to default state:
	playDirection = 0;
	repeatPlay = false;
	maxFrameRate = 10; 
	frameStepSize = 1;
	startFrame = 1;
	endFrame = 100;
	maxFrame = 100; 
	minFrame = 1;
	currentFrame = 0;
	setDirty();
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myAnimationTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getAnimationParams(viznum)))
			updateDialog();
	}
}
//Respond to change in Metadata
//
void AnimationParams::
reinit(bool doOverride){
	Session* session = Session::getInstance();
	//Make min and max conform to new data:
	minFrame = (int)session->getMinTimestep();
	maxFrame = (int)session->getMaxTimestep();
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
	//force start & end to be consistent:
	if (doOverride){
		startFrame = minFrame;
		endFrame = maxFrame;
		currentFrame = startFrame;
	} else {
		if (startFrame > maxFrame) startFrame = maxFrame;
		if (startFrame < minFrame) startFrame = minFrame;
		if (endFrame < minFrame) endFrame = minFrame;
		if (endFrame > maxFrame) endFrame = maxFrame;
		if (currentFrame < startFrame) currentFrame = startFrame;
		if (currentFrame > endFrame) currentFrame = endFrame;
	}
	
	// set pause state
	playDirection = 0;

	setDirty();
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myAnimationTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (this == vwm->getAnimationParams(viznum)))
			updateDialog();
	}
}
//Set the position slider consistent with latest value of currentPosition, frameStep, and bounds
void AnimationParams::
setSliders(){
	int sliderPosition = myAnimationTab->animationSlider->value();
	int sliderFrame = startFrame + (int)(0.5f+(float)(endFrame - startFrame)*(float)sliderPosition/1000.f);
	if (sliderFrame != currentFrame && (endFrame != startFrame)){
		myAnimationTab->animationSlider->setValue((int)(1000.f*((float)(currentFrame - startFrame)/(float)(endFrame-startFrame))));
	}
	sliderPosition = myAnimationTab->frameStepSlider->value();
	int stepsize = (int)(0.5f+(float)(endFrame - startFrame)*(float)sliderPosition/1000.f);
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
//This returns true if the caller needs to set the change bit, because the
//we have changed to "pause" status
bool AnimationParams::
advanceFrame(){
	
	assert(playDirection);
	bool retCode = false;
	int lastFrameCompleted = currentFrame;
	currentFrame += playDirection*frameStepSize;
	if (currentFrame > endFrame) {
		if (repeatPlay) {
			currentFrame = startFrame - 1 + (currentFrame - endFrame);
			if (currentFrame > endFrame) currentFrame = endFrame;
		} else {
			//Stop if we are at the end:
			currentFrame = endFrame;
		}
	}
	if (currentFrame < startFrame){
		if (repeatPlay) {
			currentFrame = endFrame + 1 - (startFrame - currentFrame);
			if (currentFrame < startFrame) currentFrame = startFrame;
		} else {
			//Stop if we are at the end:
			currentFrame = startFrame;
		}
	}
	if (currentFrame == lastFrameCompleted) {
		//Change to "pause" status:
		playDirection = 0;
		retCode = true;//Need to set the change bit, for the next rendering
	}
	//Only update the dialog if this animation is in the active visualizer
	//int viznum = VizWinMgr::getInstance()->getActiveViz();
	//Don't call Update dialog here because this method can be called from
	//the controller thread (X11 doesn't like that!!!)
	//Instead the update is called from AnimationController::endRender()
	//if (viznum >= 0 && VizWinMgr::getInstance()->getAnimationParams(viznum) == this)
		//updateDialog();
	return retCode;
}
	
void AnimationParams::setDirty(){
	VizWinMgr::getInstance()->setAnimationDirty(this);
}
//Set change bits when global/local change occurs, so that the animation
//controller will change the local/global status at the end of the next rendering.
void AnimationParams::guiSetLocal(bool lg){
	Params::guiSetLocal(lg);
	int viznum = vizNum;
	if (viznum < 0){
		viznum = VizWinMgr::getInstance()->getActiveViz();
		assert(viznum >=0);
	}
	//Need to rerender the active viz, and set its change bit
	AnimationController::getInstance()->paramsChanged(viznum);
	VizWin* viz = VizWinMgr::getInstance()->getVizWin(viznum);
	viz->setRegionDirty(true);
	viz->updateGL();
}
bool AnimationParams::
elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/){
	return false;
}
bool AnimationParams::
elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/){
	return false;
}
