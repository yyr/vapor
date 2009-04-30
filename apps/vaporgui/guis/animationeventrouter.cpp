//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		animationeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Implements the animationEventRouter class.
//		This class supports routing messages from the gui to the params
//		This is derived from the animation tab
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif
#include <qdesktopwidget.h>
#include <qrect.h>
#include <qmessagebox.h>
#include <qworkspace.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qbuttongroup.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qtable.h>
#include "animationparams.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qlabel.h>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "vapor/Metadata.h"
#include "vapor/XmlNode.h"
#include "vapor/VDFIOBase.h"
#include "tabmanager.h"
#include "glutil.h"

#include "animationeventrouter.h"
#include "animationcontroller.h"
#include "mainform.h"
#include "eventrouter.h"

using namespace VAPoR;


AnimationEventRouter::AnimationEventRouter(QWidget* parent, const char* name) : AnimationTab(parent,name), EventRouter() {
	myParamsType = Params::AnimationParamsType;
}


AnimationEventRouter::~AnimationEventRouter(){
	if (savedCommand) delete savedCommand;
}
/**********************************************************
 * Whenever a new Animationtab is created it must be hooked up here
 ************************************************************/
void
AnimationEventRouter::hookUpTab()
{
	//Signals and slots:
	
 	
	connect (startFrameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (currentFrameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (endFrameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (frameStepEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (maxFrameRateEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (maxWaitEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	
	
	//Connect all the returnPressed signals, these will update the visualizer.
	connect (startFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (currentFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (endFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (frameStepEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (maxFrameRateEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (maxWaitEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));

	//Sliders only do anything when released
	connect (frameStepSlider, SIGNAL(sliderReleased()), this, SLOT (animationSetFrameStep()));
	connect (animationSlider, SIGNAL(sliderReleased()), this, SLOT (animationSetPosition()));

	connect (timestepSampleCheckbox, SIGNAL(toggled(bool)), this, SLOT(guiToggleTimestepSample(bool)));
	connect (timestepSampleTable, SIGNAL(valueChanged(int,int)), this, SLOT(timestepChanged(int,int)));
	connect (addSampleButton,SIGNAL(clicked()), this, SLOT(addSample()));
	connect (deleteSampleButton,SIGNAL(clicked()), this, SLOT(deleteSample()));
	connect (rebuildButton, SIGNAL(clicked()), this, SLOT(guiRebuildList()));
	
	//Button clicking for toggle buttons:
	connect(pauseButton, SIGNAL(clicked()), this, SLOT(animationPauseClick()));
	connect(playReverseButton, SIGNAL(clicked()), this, SLOT(animationPlayReverseClick()));
	connect(playForwardButton, SIGNAL(clicked()), this, SLOT(animationPlayForwardClick()));
	connect(replayButton, SIGNAL(clicked()), this, SLOT(animationReplayClick()));
	//and non-toggle buttons:
	connect(toBeginButton, SIGNAL(clicked()), this, SLOT(animationToBeginClick()));
	connect(toEndButton, SIGNAL(clicked()), this, SLOT(animationToEndClick()));
	connect(stepReverseButton, SIGNAL(clicked()), this, SLOT(animationStepReverseClick()));
	connect(stepForwardButton, SIGNAL(clicked()), this, SLOT(animationStepForwardClick()));

}
/*********************************************************************************
 * Slots associated with AnimationTab:
 *********************************************************************************/

void AnimationEventRouter::
setAtabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void AnimationEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Animation tab text change");
	
	int savedFrameNum = aParams->getCurrentFrameNumber();
	QString strn;
	int startFrame = startFrameEdit->text().toInt();
	int minFrame = aParams->getMinFrame();
	int maxFrame = aParams->getMaxFrame();
	

	if (startFrame < minFrame || startFrame > maxFrame) {
		startFrame = minFrame;
		startFrameEdit->setText(strn.setNum(startFrame));
		
	}
	aParams->setStartFrameNumber(startFrame);
	int endFrame = endFrameEdit->text().toInt();
	if (endFrame < minFrame || endFrame > maxFrame || endFrame < startFrame) {
		endFrame = maxFrame;
		endFrameEdit->setText(strn.setNum(endFrame));
	}
	aParams->setEndFrameNumber(endFrame);

	int currentFrame = currentFrameEdit->text().toInt();
	if (currentFrame < startFrame) currentFrame = startFrame;
	if (currentFrame > endFrame) currentFrame = endFrame;
	currentFrameEdit->setText(strn.setNum(currentFrame));
	aParams->setCurrentFrameNumber(currentFrame);
	
	int frameStepSize = frameStepEdit->text().toInt();
	if (frameStepSize > (endFrame - startFrame)) frameStepSize = endFrame - startFrame;
	if (frameStepSize < 1) frameStepSize = 1;
	frameStepEdit->setText(strn.setNum(frameStepSize));
	aParams->setFrameStepSize(frameStepSize);
	setSliders(aParams);

	float maxFrameRate = maxFrameRateEdit->text().toFloat();
	float maxWait = maxWaitEdit->text().toFloat();
	//Constrain to a "reasonable" range:
	if (maxFrameRate> 30.f) maxFrameRate = 30.f;
	if (maxFrameRate< 0.001f) maxFrameRate = 0.001f;
	if (maxWait < 0.05f) maxWait = 0.05f;
	aParams->setMaxFrameRate(maxFrameRate);
	aParams->setMaxWait(maxWait);
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	if (currentFrame != savedFrameNum){
		VizWinMgr::getInstance()->setAnimationDirty(aParams);
	}
	update();
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, aParams);
}

//Insert values from params into tab panel
//
void AnimationEventRouter::updateTab(){
	if(!MainForm::getInstance()->getTabManager()->isFrontTab(this)) return;
	AnimationParams* aParams = (AnimationParams*) VizWinMgr::getInstance()->getActiveAnimationParams();
	float sliderVal;
	QString strn;
	Session::getInstance()->blockRecording();
	sliderVal = 0.f;
	int startFrame = aParams->getStartFrameNumber();
	int endFrame = aParams->getEndFrameNumber();
	int currentFrame = aParams->getCurrentFrameNumber();
	
	if (endFrame > startFrame)
		sliderVal = 1000.f*(float)(currentFrame-startFrame)/(float)(endFrame-startFrame);
	animationSlider->setValue((int)sliderVal);
	
	
	startFrameEdit->setText(strn.setNum(startFrame));
	currentFrameEdit->setText(strn.setNum(currentFrame));
	endFrameEdit->setText(strn.setNum(endFrame));

	minFrameLabel->setText(strn.setNum(aParams->getMinFrame()));
	maxFrameLabel->setText(strn.setNum(aParams->getMaxFrame()));
	maxFrameRateEdit->setText(strn.setNum(aParams->getMaxFrameRate(),'g',3));
	frameStepEdit->setText(strn.setNum(aParams->getFrameStepSize()));
	maxWaitEdit->setText(strn.setNum(aParams->getMaxWait(),'g',3));
	int playDirection = aParams->getPlayDirection();

	if (playDirection==0) {//pause
		//pauseButton->setDown(true);
		playForwardButton->setDown(false);
		playReverseButton->setDown(false);
		stepForwardButton->setEnabled(true);
		stepReverseButton->setEnabled(true);
	} else if (playDirection==1){//forward play
		//pauseButton->setDown(false);
		playForwardButton->setDown(true);
		playReverseButton->setDown(false);
		if (!aParams->isRepeating() && currentFrame >= endFrame){
			//kludge to make the buttons update at the end of
			//the animation sequence.  The problem is that
			//the update can't be called from the shared controller thread,
			//so we have to catch the animation end by other means.
			stepForwardButton->setEnabled(true);
			stepReverseButton->setEnabled(true);
		}
	} else {//reverse play
		//pauseButton->setDown(false);
		playForwardButton->setDown(false);
		playReverseButton->setDown(true);
		if (!aParams->isRepeating() && currentFrame <= startFrame){
			stepForwardButton->setEnabled(true);
			stepReverseButton->setEnabled(true);
		}
	}
	if (aParams->isRepeating()){
		replayButton->setOn(true);
	} else {
		replayButton->setOn(false);
	}
	if (aParams->isLocal())
		LocalGlobal->setCurrentItem(1);
	else 
		LocalGlobal->setCurrentItem(0);

	//Set up the timestep sample table:
	timestepSampleTable->horizontalHeader()->hide();
	timestepSampleTable->setSelectionMode(QTable::SingleRow);
	timestepSampleTable->setTopMargin(0);
	timestepSampleTable->setColumnWidth(0,35);
	populateTimestepTable();

	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	update();
	VizWinMgr::getInstance()->getTabManager()->update();
}





//Reinitialize Animation tab settings, session has changed

void AnimationEventRouter::
reinitTab(bool doOverride){
	if (VizWinMgr::getInstance()->getNumVisualizers() > 1) LocalGlobal->setEnabled(true);
	else LocalGlobal->setEnabled(false);
}

/*************************************************************************************
 *  slots associated with AnimationTab
 *************************************************************************************/
void AnimationEventRouter::
animationReturnPressed(void){
	confirmText(true);
}

//Respond to release of frame-step slider
void AnimationEventRouter::
animationSetFrameStep(){
	guiSetFrameStep(frameStepSlider->value());
}
//Respond to release of animation position slider
void AnimationEventRouter::
animationSetPosition(){
	guiSetPosition(animationSlider->value());
}
//Respond to pause button press.

void AnimationEventRouter::
animationPauseClick(){
	playForwardButton->setDown(false);
	playReverseButton->setDown(false);
	guiSetPlay(0);
}
void AnimationEventRouter::
animationPlayReverseClick(){
	if (!playReverseButton->isDown()){
		playForwardButton->setDown(false);
		playReverseButton->setDown(true);
		guiSetPlay(-1);
	}
}
void AnimationEventRouter::
animationPlayForwardClick(){
	if (!playForwardButton->isDown()){
		playForwardButton->setDown(true);
		playReverseButton->setDown(false);
		guiSetPlay(1);
	}
}
void AnimationEventRouter::
animationReplayClick(){
	guiToggleReplay(replayButton->isOn());
}
void AnimationEventRouter::
animationToBeginClick(){
	guiJumpToBegin();
}
void AnimationEventRouter::
animationToEndClick(){
	guiJumpToEnd();
}
void AnimationEventRouter::
animationStepForwardClick(){
	guiSingleStep(true);
}
void AnimationEventRouter::
animationStepReverseClick(){
	guiSingleStep(false);
}

//Following are set by gui, result in save history state.
//Whenever play is pressed, it wakes up the animation controller.

void AnimationEventRouter::guiSetPlay(int direction){
	confirmText(false);
	PanelCommand* cmd = 0;
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	switch (direction) {
		case -1:
			cmd = PanelCommand::captureStart(aParams, "play reverse");
			
			stepForwardButton->setEnabled(false);
			stepReverseButton->setEnabled(false);
			
			break;
		case 0:
			cmd = PanelCommand::captureStart(aParams, "pause animation");
			
			stepForwardButton->setEnabled(true);
			stepReverseButton->setEnabled(true);
			break;
		case 1:
			cmd = PanelCommand::captureStart(aParams, "play forward");
			
			stepForwardButton->setEnabled(false);
			stepReverseButton->setEnabled(false);
			
			break;
		default:
			assert(0);
	}
	int previousDirection = aParams->getPlayDirection();
	aParams->setPlayDirection(direction);
	PanelCommand::captureEnd(cmd, aParams);
	if (direction && !previousDirection) VizWinMgr::getInstance()->startPlay(aParams);
	else if (!direction) VizWinMgr::getInstance()->animationParamsChanged(aParams);
	updateTab();
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
	
}
void AnimationEventRouter::
guiJumpToBegin(){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Jump to animation start");
	int startFrame = aParams->getStartFrameNumber();
	aParams->setCurrentFrameNumber(startFrame);
	currentFrameEdit->setText(QString::number(startFrame));
	setSliders(aParams);
	PanelCommand::captureEnd(cmd, aParams);
	guiSetTextChanged(false);
	update();
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
	
}
void AnimationEventRouter::
guiJumpToEnd(){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Jump to animation end");
	int endFrame = aParams->getEndFrameNumber();
	aParams->setCurrentFrameNumber(endFrame);
	currentFrameEdit->setText(QString::number(endFrame));
	setSliders(aParams);
	PanelCommand::captureEnd(cmd, aParams);
	guiSetTextChanged(false);
	update();
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
}


//Respond to release of frame position slider:
void AnimationEventRouter::guiSetPosition(int position){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Change current frame number");
	int startFrame = aParams->getStartFrameNumber();
	int endFrame = aParams->getEndFrameNumber();
	int newFrameNum = startFrame + (int)((float)(endFrame - startFrame)*(float)position/1000.f);
	//Find the nearest valid frame number:
	DataStatus* ds = DataStatus::getInstance();
	int maxdist = ds->getMaxTimestep()-ds->getMinTimestep();
	for (int i = 0; i< maxdist; i++){
		if (ds->dataIsPresent(newFrameNum+i)) {
			newFrameNum += i;
			break;
		}
		if (ds->dataIsPresent(newFrameNum-i)) {
			newFrameNum -= i;
			break;
		}
	}
	aParams->setCurrentFrameNumber(newFrameNum);
	int currentFrame = aParams->getCurrentFrameNumber();
	currentFrameEdit->setText(QString::number(currentFrame));
	PanelCommand::captureEnd(cmd, aParams);
	guiSetTextChanged(false);
	updateTab();
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
}
//Respond to release of stepsize slider.  Max range of slider is end - start
void AnimationEventRouter::guiSetFrameStep(int stepsize){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Change current step size");
	int endFrame = aParams->getEndFrameNumber();
	int startFrame = aParams->getStartFrameNumber();
	int frameStepSize = 1+ (int)((float)(endFrame - startFrame)*(float)stepsize/1000.f);
	if (frameStepSize < 1) frameStepSize = 1;
	aParams->setFrameStepSize(frameStepSize);
	
	frameStepEdit->setText(QString::number(frameStepSize));
	PanelCommand::captureEnd(cmd, aParams);
	guiSetTextChanged(false);
	update();
	
	
}
void AnimationEventRouter::guiToggleReplay(bool replay){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams,"Toggle replay button");
	aParams->setRepeating(replay);
	PanelCommand::captureEnd(cmd, aParams);
	update();
	
}
//Set change bits when global/local change occurs, so that the animation
//controller will change the local/global status at the end of the next rendering.
void AnimationEventRouter::guiSetLocal(Params* p, bool lg){
	
	EventRouter::guiSetLocal(p,lg);
	
	int viznum = VizWinMgr::getInstance()->getActiveViz();
		
	//Need to rerender the active viz, and set its change bit
	AnimationController::getInstance()->paramsChanged(viznum);
	VizWin* viz = VizWinMgr::getInstance()->getVizWin(viznum);
	viz->setAnimationDirty(true);
	viz->updateGL();
}

void AnimationEventRouter::guiSingleStep(bool forward){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	PanelCommand* cmd;
	int currentFrame = aParams->getCurrentFrameNumber();
	int dir = (forward ? 1 : -1);
	int nextFrame = aParams->getNextFrame(dir);
	if (nextFrame == currentFrame) return;
	if (forward){
		cmd = PanelCommand::captureStart(aParams,"Single-step forward");
	} else {
		cmd = PanelCommand::captureStart(aParams,"Single-step reverse");
	}
	currentFrameEdit->setText(QString::number(nextFrame));
	aParams->setCurrentFrameNumber(nextFrame);
	setSliders(aParams);
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
	update();
}
//Set the position slider consistent with latest value of currentPosition, frameStep, and bounds
void AnimationEventRouter::
setSliders(AnimationParams* aParams){
	int sliderPosition = animationSlider->value();
	int currentFrame = aParams->getCurrentFrameNumber();
	int startFrame = aParams->getStartFrameNumber();
	int endFrame = aParams->getEndFrameNumber();
	int frameStepSize = aParams->getFrameStepSize();
	int sliderFrame = startFrame + (int)(0.5f+(float)(endFrame - startFrame)*(float)sliderPosition/1000.f);
	if (sliderFrame != currentFrame && (endFrame != startFrame)){
		animationSlider->setValue((int)(1000.f*((float)(currentFrame - startFrame)/(float)(endFrame-startFrame))));
	}
	sliderPosition = frameStepSlider->value();
	int stepsize = 1+(int)(0.5f+(float)(endFrame - startFrame)*(float)sliderPosition/1000.f);
	if(stepsize < 1) stepsize = 1;
	if (stepsize != frameStepSize && endFrame != startFrame){
		frameStepSlider->setValue((int)(1000.f*((float)frameStepSize/(float)(endFrame-startFrame))));
	}
}
//Method to call when a new window installed via undo/redo
void AnimationEventRouter::makeCurrent(Params* /* prev params p*/, Params* newParams, bool, int,bool){
	AnimationParams* aParams = (AnimationParams*) newParams;
	int vizNum = aParams->getVizNum();
	VizWinMgr* vwm = VizWinMgr::getInstance();
	vwm->setAnimationParams(vizNum, aParams);
	updateTab();
}
void AnimationEventRouter::
guiToggleTimestepSample(bool on){
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(aParams,  "toggle use of timestep sample list");
	aParams->setTimestepSampleList(on);
	timestepSampleTable->setEnabled(on);
	frameStepSlider->setEnabled(!on);
	frameStepEdit->setEnabled(!on);
	deleteSampleButton->setEnabled(on);
	addSampleButton->setEnabled(on);
	PanelCommand::captureEnd(cmd, aParams);

	//This has no real effect until the next animation playing.
	updateTab();
	
}
//Respond to user has typed in a row of timestep table. Convert it to an int, swap it up or down
//until it's in ascending order.
void AnimationEventRouter::timestepChanged(int row, int col){
	int newVal = timestepSampleTable->text(row,col).toInt();
	int i;
	//First, find the first one above it that's larger:
	for (i = 0; i< row; i++){
		int rowInt = timestepSampleTable->text(i,col).toInt();
		if (rowInt > newVal) break;
	}
	if (i < row) { //found one to swap:  Swap from row to i
		for (int j = row; j>i; j--){
			timestepSampleTable->swapRows(j,j-1);
		}
		//It changed, update the flowparams:
		guiUpdateTimestepList(timestepSampleTable, "edit timestep list");
		return;
	} 
	//Now look below this one for the lowest one that is smaller
	for (i =  timestepSampleTable->numRows()-1; i>row; i--){
		int rowInt = timestepSampleTable->text(i,col).toInt();
		if (rowInt < newVal) break;
	}
	if (i > row){ //found one to swap:  Swap from row to i
		for (int j = row; j<i; j++){
			timestepSampleTable->swapRows(j,j+1);
		}
		//It changed, update the flowparams:
		guiUpdateTimestepList(timestepSampleTable, "edit timestep list");
		return;
	} 
	//No Change of order, just value:
	guiUpdateTimestepList(timestepSampleTable, "edit timestep list");
	return;
}
//Send the contents of the timestepTable to the params.
//Assumes that the timestepTable is sorted in ascending order.
void AnimationEventRouter::guiUpdateTimestepList(QTable* tbl, const char* descr){	
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, descr);
	std::vector<int>& timesteplist = aParams->getTimestepList();
	timesteplist.clear();
	int prevTime = -1;
	for (int i = 0; i< tbl->numRows(); i++){
		int newTime = tbl->text(i,0).toInt();
		if (newTime > prevTime) {
			timesteplist.push_back(newTime);
			prevTime = newTime;
		}
	}
	
	PanelCommand::captureEnd(cmd, aParams);
	updateTab();
}
//Add a new (blank) row to the table
void AnimationEventRouter::addSample(){
	timestepSampleTable->insertRows(timestepSampleTable->numRows());
}
//Delete the current selected row
void AnimationEventRouter::deleteSample(){
	timestepSampleTable->removeRow(timestepSampleTable->currentRow());
	guiUpdateTimestepList(timestepSampleTable, "remove timestep from list");
}
void AnimationEventRouter::populateTimestepTable(){
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	std::vector<int>& tSteps = aParams->getTimestepList();
	timestepSampleTable->setNumRows(tSteps.size());
	for (int i = 0; i< tSteps.size(); i++){
		timestepSampleTable->setText(i,0,QString::number(tSteps[i]));
	}
	timestepSampleCheckbox->setChecked(aParams->usingTimestepList());
}
void AnimationEventRouter::guiRebuildList(){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Rebuild timestep list");
	std::vector<int>& timesteplist = aParams->getTimestepList();
	timesteplist.clear();
	DataStatus* ds = DataStatus::getInstance();
	int minTime = ds->getMinTimestep();
	int maxTime = ds->getMaxTimestep();
	for (int i = minTime; i<= maxTime; i++){
		if (ds->dataIsPresent(i)) timesteplist.push_back(i);
	}
	populateTimestepTable();
	PanelCommand::captureEnd(cmd, aParams);
	updateTab();
}
void AnimationEventRouter::guiSetTimestep(int framenum){
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Change current time step");
	aParams->setCurrentFrameNumber(framenum);
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
}
 