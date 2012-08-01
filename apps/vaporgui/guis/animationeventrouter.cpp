//************************************************************************
//									*
//		     Copyright (C)  2006				*
//     University Corporation for Atmospheric Research			*
//		     All Rights Reserved				*
//									*
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
#include <algorithm>
#include <qdesktopwidget.h>
#include <qrect.h>
#include <qmessagebox.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qlabel.h>
#include <QAbstractItemView>
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
#include <vapor/XmlNode.h>
#include "tabmanager.h"
#include "glutil.h"

#include "animationeventrouter.h"
#include "animationcontroller.h"
#include "mainform.h"
#include "eventrouter.h"
#include "ui/animationtab.h"
#include <qglobal.h>

#include "../images/playreverseA.xpm"
#include "../images/playforwardA.xpm" 
#include "../images/pauseA.xpm"
#include "../images/stepfwdA.xpm"
#include "../images/stepbackA.xpm"
#include "../images/toendA.xpm"
#include "../images/tobeginA.xpm"
#include "../images/replayA.xpm"

using namespace VAPoR;
QT_USE_NAMESPACE


AnimationEventRouter::AnimationEventRouter(QWidget* parent) : QWidget(parent,0), Ui_AnimationTab(), EventRouter() {
	setupUi(this);	
	currentKeyIndex = 0;
	myParamsBaseType = Params::GetTypeFromTag(Params::_animationParamsTag);
	MessageReporter::infoMsg("AnimationEventRouter::AnimationEventRouter()");
	dontUpdate = false;
	QPixmap* playForwardIcon = new QPixmap(playforward);
	playForwardButton->setIcon(QIcon(*playForwardIcon));
	playForwardButton->setIconSize(QSize(30,18));
	QPixmap* playReverseIcon = new QPixmap(playreverse);
	playReverseButton->setIcon(QIcon(*playReverseIcon));
	playReverseButton->setIconSize(QSize(30,18));
	QPixmap* pauseIcon = new QPixmap( pause_);
	pauseButton->setIcon(QIcon(*pauseIcon));
	pauseButton->setIconSize(QSize(30,18));
	QPixmap* toBeginIcon = new QPixmap(toBegin_);
	toBeginButton->setIcon(QIcon(*toBeginIcon));
	toBeginButton->setIconSize(QSize(30,18));
	QPixmap* toEndIcon = new QPixmap(toEnd_);
	toEndButton->setIcon(QIcon(*toEndIcon));
	toEndButton->setIconSize(QSize(30,18));
	QPixmap* replayIcon = new QPixmap( replay_);
	replayButton->setIcon(QIcon(*replayIcon));
	replayButton->setIconSize(QSize(30,18));
	QPixmap* stepfwdIcon = new QPixmap(stepfwd);
	stepForwardButton->setIcon(QIcon(*stepfwdIcon));
	stepForwardButton->setIconSize(QSize(30,18));
	QPixmap* stepbackIcon = new QPixmap(stepback);
	stepReverseButton->setIcon(QIcon(*stepbackIcon));
	stepReverseButton->setIconSize(QSize(30,18));
	MainForm* mf = MainForm::getInstance();
	mainPlayForwardAction = mf->playForwardAction;
	mainPlayBackwardAction = mf->playBackwardAction;
	mainPauseAction = mf->pauseAction;

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
	connect (endFrameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setEndFrameTextChanged(const QString&)));
	connect (frameStepEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (maxFrameRateEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (maxWaitEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (keyframeSpeedEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setKeyframeTextChanged(const QString&)));
	connect (keyTimestepEdit, SIGNAL( textChanged(const QString&) ) , this, SLOT( setKeyframeTextChanged(const QString&)));
	connect (endTimestepEdit,  SIGNAL( textChanged(const QString&) ) , this, SLOT( setKeyframeTextChanged(const QString&)));
	connect (durationEdit,  SIGNAL( textChanged(const QString&) ), this, SLOT( setKeyframeTextChanged(const QString&)));
	connect (speedEdit,  SIGNAL( textChanged(const QString&) ) , this,SLOT( setKeyframeTextChanged(const QString&)));

	
	//Connect all the returnPressed signals, these will update the visualizer.
	connect (startFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (currentFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (endFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (frameStepEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (maxFrameRateEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (maxWaitEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	
	

	connect (frameStepSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetFrameStep(int)));
	connect (animationSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetPosition(int)));
	connect (timestepSampleCheckbox, SIGNAL(toggled(bool)), this, SLOT(guiToggleTimestepSample(bool)));
	connect (timestepSampleTable, SIGNAL(cellChanged(int,int)), this, SLOT(timestepChanged(int,int)));
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

	//Animation control widgets that result in rebuilding animation
	connect (keyTimestepEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (endTimestepEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (durationEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (speedEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (keyframeSpeedEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));

	connect (keyIndexSpin, SIGNAL(valueChanged(int)),this, SLOT(guiChangeKeyIndex(int)));
	connect (enableKeyframeCheckBox, SIGNAL(toggled(bool)),this,SLOT(guiEnableKeyframing(bool)));
	connect (adjustKeyButton, SIGNAL(clicked()), this, SLOT(guiChangeKeyframe()));
	connect (deleteButton, SIGNAL(clicked()), this, SLOT(guiDeleteKeyframe()));
	connect (insertKeyframeButton, SIGNAL(clicked()), this, SLOT(guiInsertKeyframe()));
	connect (gotoButton, SIGNAL(clicked()), this, SLOT(guiGotoKeyframe()));

	connect (LocalGlobal, SIGNAL (activated (int)), VizWinMgr::getInstance(), SLOT (setAnimationLocalGlobal(int)));
	connect (VizWinMgr::getInstance(), SIGNAL(enableMultiViz(bool)), LocalGlobal, SLOT(setEnabled(bool)));

	currentKeyIndex = keyIndexSpin->value();
	dontUpdate = false;

}
/*********************************************************************************
 * Slots associated with AnimationTab:
 *********************************************************************************/

void AnimationEventRouter::
setAtabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void AnimationEventRouter::
setKeyframeTextChanged(const QString& ){
	keyframeTextChanged = true;
	textChangedFlag = true;
}
void AnimationEventRouter::
setEndFrameTextChanged(const QString& ){
	keyframeTextChanged = true;
	textChangedFlag = true;
	endFrameTextChanged = true;
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
	int endFrame = aParams->getEndFrameNumber();
	if (endFrameTextChanged){
		endFrame = endFrameEdit->text().toInt();
		//make sure endFrame is valid.  If keyframing is enabled, it needs to be no greater than the 
		if (endFrame > maxFrame || endFrame < minFrame) {
			endFrame = maxFrame;
			endFrameEdit->setText(strn.setNum(endFrame));
		}
		aParams->setEndFrameNumber(endFrame);
		endFrameTextChanged = false;
	}

	float cameraSpeed= keyframeSpeedEdit->text().toFloat();
	if (cameraSpeed >= 0.) aParams->setCurrentCameraSpeed(cameraSpeed);
	else keyframeSpeedEdit->setText(QString::number(aParams->getCurrentCameraSpeed()));
	
	int currentFrame = currentFrameEdit->text().toInt();
	if (currentFrame < startFrame) currentFrame = startFrame;
	if (currentFrame > endFrame) currentFrame = endFrame;
	currentFrameEdit->setText(strn.setNum(currentFrame));
	MainForm::getInstance()->setCurrentFrameNum(currentFrame);

	aParams->setCurrentFrameNumber(currentFrame);
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	
	int frameStepSize = frameStepEdit->text().toInt();
	if (frameStepSize > (endFrame - startFrame)) frameStepSize = endFrame - startFrame;
	if (frameStepSize < 1) frameStepSize = 1;
	frameStepEdit->setText(strn.setNum(frameStepSize));
	aParams->setFrameStepSize(frameStepSize);
	setSliders(aParams);

	float maxFrameRate = maxFrameRateEdit->text().toFloat();
	float maxWait = maxWaitEdit->text().toFloat();
	//Constrain to a "reasonable" range:
	if (maxFrameRate> 1000.f) maxFrameRate = 1000.f;
	if (maxFrameRate< 0.001f) maxFrameRate = 0.001f;
	if (maxWait < 0.001f) maxWait = 0.001f;
	aParams->setMaxFrameRate(maxFrameRate);
	aParams->setMaxWait(maxWait);
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	if (currentFrame != savedFrameNum){
		VizWinMgr::getInstance()->setAnimationDirty(aParams);
	}
	if (keyframeTextChanged){
		
		Keyframe* key = aParams->getKeyframe(currentKeyIndex);
		key->speed = abs(speedEdit->text().toFloat());
		int tstep = keyTimestepEdit->text().toInt();
		DataStatus* ds = DataStatus::getInstance();
		if (tstep < ds->getMinTimestep()) tstep = ds->getMinTimestep();
		if (tstep > ds->getMaxTimestep()) tstep = ds->getMaxTimestep();
		key->timeStep = tstep;
		int endstep = endTimestepEdit->text().toInt();
		if (endstep < ds->getMinTimestep()) endstep = ds->getMinTimestep();
		if (endstep > ds->getMaxTimestep()) endstep = ds->getMaxTimestep();
		
		if (durationEdit->isEnabled()){
			int newDuration = durationEdit->text().toInt();
			//Need to make sure that the new duration at least 0
			if (newDuration < 0) newDuration = 0;
			key->duration = newDuration;
			key->endTimestep = endstep;
		} else {
			key->duration = 0;
			key->endTimestep = tstep;
		}
		fixKeyframes();
			
		aParams->buildViewsAndTimes();
		keyframeTextChanged = false;
	}

	updateTab();
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, aParams);
}

//Insert values from params into tab panel
//
void AnimationEventRouter::updateTab(){
	if(!MainForm::getTabManager()->isFrontTab(this)) return;
	AnimationParams* aParams = (AnimationParams*) VizWinMgr::getInstance()->getActiveAnimationParams();
	float sliderVal;
	QString strn;
	Session::getInstance()->blockRecording();
	sliderVal = 0.f;
	int startFrame = aParams->getStartFrameNumber();
	int endFrame = aParams->getEndFrameNumber();
	int currentFrame = aParams->getCurrentFrameNumber();
	
	if (endFrame > startFrame)
		sliderVal = 0.5f+ 20000.f*(float)(currentFrame-startFrame)/(float)(endFrame-startFrame);
	animationSlider->setValue((int)sliderVal);
	
	
	startFrameEdit->setText(strn.setNum(startFrame));
	currentFrameEdit->setText(strn.setNum(currentFrame));
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	endFrameEdit->setText(strn.setNum(endFrame));
	endFrameTextChanged = false;
	minFrameLabel->setText(strn.setNum(aParams->getMinFrame()));
	maxFrameLabel->setText(strn.setNum(aParams->getMaxFrame()));
	maxFrameRateEdit->setText(strn.setNum(aParams->getMaxFrameRate(),'g',3));
	frameStepEdit->setText(strn.setNum(aParams->getFrameStepSize()));
	maxWaitEdit->setText(strn.setNum(aParams->getMaxWait(),'g',3));
	int playDirection = aParams->getPlayDirection();

	if (playDirection==0) {//pause
		pauseButton->setChecked(true);
		mainPauseAction->setChecked(true);
		playForwardButton->setChecked(false);
		mainPlayForwardAction->setChecked(false);
		playReverseButton->setChecked(false);
		mainPlayBackwardAction->setChecked(false);
		stepForwardButton->setEnabled(true);
		stepReverseButton->setEnabled(true);
	} else if (playDirection==1){//forward play
		pauseButton->setChecked(false);
		mainPauseAction->setChecked(false);
		playForwardButton->setChecked(true);
		mainPlayForwardAction->setChecked(true);
		playReverseButton->setChecked(false);
		mainPlayBackwardAction->setChecked(false);
		if (!aParams->isRepeating() && currentFrame >= endFrame){
			//kludge to make the buttons update at the end of
			//the animation sequence.  The problem is that
			//the update can't be called from the shared controller thread,
			//so we have to catch the animation end by other means.
			stepForwardButton->setEnabled(true);
			stepReverseButton->setEnabled(true);
		}
	} else {//reverse play
		pauseButton->setChecked(false);
		mainPauseAction->setChecked(false);
		playForwardButton->setChecked(false);
		mainPlayForwardAction->setChecked(false);
		playReverseButton->setChecked(true);
		mainPlayBackwardAction->setChecked(true);
		if (!aParams->isRepeating() && currentFrame <= startFrame){
			stepForwardButton->setEnabled(true);
			stepReverseButton->setEnabled(true);
		}
	}
	if (aParams->isRepeating()){
		replayButton->setChecked(true);
	} else {
		replayButton->setChecked(false);
	}
	if (aParams->isLocal())
		LocalGlobal->setCurrentIndex(1);
	else 
		LocalGlobal->setCurrentIndex(0);

	//Set up the timestep sample table:
	timestepSampleTable->horizontalHeader()->hide();
	timestepSampleTable->setSelectionMode(QAbstractItemView::SingleSelection);
	timestepSampleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	timestepSampleTable->setColumnWidth(0,80);
	populateTimestepTable();

	//setup keyframe parameters:
	keyframeSpeedEdit->setText(QString::number(aParams->getCurrentCameraSpeed()));
	keyIndexSpin->setMaximum(aParams->getNumKeyframes()-1);
	currentKeyIndex = keyIndexSpin->value();
	Keyframe* kf = aParams->getKeyframe(currentKeyIndex);
	keyTimestepEdit->setText(QString::number(kf->timeStep));
	endTimestepEdit->setText(QString::number(kf->endTimestep));
	durationEdit->setText(QString::number(kf->duration));
	//Count all previous frameCounts:
	vector<Keyframe*>& keyframes = aParams->getKeyframes();
	int totframes = 0;
	for (int i = 0; i<currentKeyIndex; i++) {
		totframes += keyframes[i]->frameNum + keyframes[i]->duration;
	}
	frameIndexEdit->setText(QString::number(totframes));
	speedEdit->setText(QString::number(kf->speed));
	enableKeyframeCheckBox->setChecked(aParams->keyframingEnabled());
	if (kf->speed > 0.f){
		durationEdit->setEnabled(false);
		endTimestepEdit->setEnabled(false);
	} else {
		durationEdit->setEnabled(true);
		endTimestepEdit->setEnabled(true);
	}

	guiSetTextChanged(false);
	keyframeTextChanged = false;
	Session::getInstance()->unblockRecording();
	update();
	VizWinMgr::getInstance()->getTabManager()->update();
}





//Reinitialize Animation tab settings, session has changed

void AnimationEventRouter::
reinitTab(bool doOverride){
	if (VizWinMgr::getInstance()->getNumVisualizers() > 1) LocalGlobal->setEnabled(true);
	else LocalGlobal->setEnabled(false);
	dontUpdate=false;
	endFrameTextChanged = false;
}

/*************************************************************************************
 *  slots associated with AnimationTab
 *************************************************************************************/
void AnimationEventRouter::
animationReturnPressed(void){
	confirmText(true);
}


//Respond to pause button press.

void AnimationEventRouter::
animationPauseClick(){
	playForwardButton->setChecked(false);
	playReverseButton->setChecked(false);
	mainPlayForwardAction->setChecked(false);
	mainPlayBackwardAction->setChecked(false);
	mainPauseAction->setChecked(true);
	pauseButton->setChecked(true);
	guiSetPlay(0);
}
void AnimationEventRouter::
animationPlayReverseClick(){
	if (!playReverseButton->isDown()){
		playForwardButton->setChecked(false);
		mainPlayForwardAction->setChecked(false);
		playReverseButton->setChecked(true);
		mainPlayBackwardAction->setChecked(true);
		pauseButton->setChecked(true);
		mainPauseAction->setChecked(false);
		guiSetPlay(-1);
	}
}
void AnimationEventRouter::
animationPlayForwardClick(){
	
	playForwardButton->setChecked(true);
	mainPlayForwardAction->setChecked(true);
	playReverseButton->setChecked(false);
	mainPlayBackwardAction->setChecked(false);
	mainPauseAction->setChecked(false);
	pauseButton->setChecked(false);
	guiSetPlay(1);
	
}
void AnimationEventRouter::
animationReplayClick(){
	guiToggleReplay(replayButton->isChecked());
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
	
	aParams->setPlayDirection(direction);
	PanelCommand::captureEnd(cmd, aParams);
	if (direction) VizWinMgr::getInstance()->startPlay(aParams);
	else VizWinMgr::getInstance()->animationParamsChanged(aParams);
	updateTab();
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
	if (direction == 0){ //refresh front tab image on stop play ...
		refreshFrontTab();
	}
	
}
void AnimationEventRouter::
guiJumpToBegin(){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Jump to animation start");
	int startFrame = aParams->getStartFrameNumber();
	aParams->setCurrentFrameNumber(startFrame);
	currentFrameEdit->setText(QString::number(startFrame));
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	MainForm::getInstance()->setCurrentFrameNum(startFrame);
	setSliders(aParams);
	PanelCommand::captureEnd(cmd, aParams);
	guiSetTextChanged(false);
	update();
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
	refreshFrontTab();
}
void AnimationEventRouter::
guiJumpToEnd(){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Jump to animation end");
	int endFrame = aParams->getEndFrameNumber();
	aParams->setCurrentFrameNumber(endFrame);
	currentFrameEdit->setText(QString::number(endFrame));
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	MainForm::getInstance()->setCurrentFrameNum(endFrame);
	setSliders(aParams);
	PanelCommand::captureEnd(cmd, aParams);
	guiSetTextChanged(false);
	update();
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
	refreshFrontTab();
}


//Respond to release of frame position slider:
void AnimationEventRouter::guiSetPosition(int position){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	
	int startFrame = aParams->getStartFrameNumber();
	int endFrame = aParams->getEndFrameNumber();
	int newFrameNum = startFrame + (int)((float)(endFrame - startFrame)*(float)position/20000.f + 0.5f);
	if(newFrameNum == aParams->getCurrentFrameNumber()) return;
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Change current frame number");
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
	MainForm::getInstance()->setCurrentFrameNum(currentFrame);
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	PanelCommand::captureEnd(cmd, aParams);
	guiSetTextChanged(false);
	updateTab();
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
	refreshFrontTab();
}
//Respond to release of stepsize slider.  Max range of slider is end - start
void AnimationEventRouter::guiSetFrameStep(int stepsize){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Change current step size");
	int endFrame = aParams->getEndFrameNumber();
	int startFrame = aParams->getStartFrameNumber();
	int frameStepSize = (int)(1.5+ ((float)(endFrame - startFrame)*(float)stepsize/1000.f));
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
	MainForm::getInstance()->setCurrentFrameNum(nextFrame);
	aParams->setCurrentFrameNumber(nextFrame);
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	setSliders(aParams);
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
	update();
	refreshFrontTab();
}
//Set the position slider consistent with latest value of currentPosition, frameStep, and bounds
void AnimationEventRouter::
setSliders(AnimationParams* aParams){
	guiSetTextChanged(false);//don't trigger text changed event when move slider
	int sliderPosition = animationSlider->value();
	int currentFrame = aParams->getCurrentFrameNumber();
	int startFrame = aParams->getStartFrameNumber();
	int endFrame = aParams->getEndFrameNumber();
	int frameStepSize = aParams->getFrameStepSize();
	int sliderFrame = startFrame + (int)(0.5f+(float)(endFrame - startFrame)*(float)sliderPosition/20000.f);
	if (sliderFrame != currentFrame && (endFrame != startFrame)){
		animationSlider->setValue((int)(20000.f*((float)(currentFrame - startFrame)/(float)(endFrame-startFrame))));
	}
	sliderPosition = frameStepSlider->value();
	int stepsize = 1+(int)(0.5f+(float)(endFrame - startFrame)*(float)sliderPosition/20000.f);
	if(stepsize < 1) stepsize = 1;
	if (stepsize != frameStepSize && endFrame != startFrame){
		frameStepSlider->setValue((int)(0.5+(1000.f*((float)(frameStepSize-1.)/(float)(endFrame-startFrame)))));
	}
}
//Method to call when a new window installed via undo/redo
void AnimationEventRouter::makeCurrent(Params* /* prev params p*/, Params* newParams, bool, int,bool){
	AnimationParams* aParams = (AnimationParams*) newParams;
	int vizNum = aParams->getVizNum();
	VizWinMgr* vwm = VizWinMgr::getInstance();
	vwm->setAnimationParams(vizNum, aParams);
	aParams->buildViewsAndTimes();
	updateTab();
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
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
//Respond to user has typed in a row of timestep table. 
//
void AnimationEventRouter::timestepChanged(int row, int col){
	if (dontUpdate) return;
	guiUpdateTimestepList("edit timestep list");
	return;
}
//Send the contents of the timestepTable to the params.
void AnimationEventRouter::guiUpdateTimestepList(const char* descr){	
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, descr);
	std::vector<int>& timesteplist = aParams->getTimestepList();
	timesteplist.clear();
	
	for (int i = 0; i< timestepSampleTable->rowCount(); i++){
		int newTime = timestepSampleTable->item(i,0)->text().toInt();
		timesteplist.push_back(newTime);	
	}
	//Sort the times
	std::sort(timesteplist.begin(), timesteplist.end());
	//Eliminate duplicates:
	for (int i = timesteplist.size()-1; i>0; i--)
		if (timesteplist[i] == timesteplist[i-1]) timesteplist.erase(timesteplist.begin()+i);

	PanelCommand::captureEnd(cmd, aParams);
	updateTab();
}
//Add a new (blank) row to the table
void AnimationEventRouter::addSample(){
	int numRows = timestepSampleTable->rowCount()+1;
	timestepSampleTable->setRowCount(numRows);
	QTableWidgetItem* tstepItem = new QTableWidgetItem("");
	dontUpdate=true;
	timestepSampleTable->setItem(numRows-1,0, tstepItem);
	dontUpdate=false;
	timestepSampleTable->setCurrentCell(numRows-1,0);
	
}
//Delete the current selected row
void AnimationEventRouter::deleteSample(){
	int thisRow = timestepSampleTable->currentRow();
	if (thisRow <0) return;
	timestepSampleTable->removeRow(thisRow);
	if(thisRow>0)timestepSampleTable->setCurrentCell(thisRow-1,0);
	else timestepSampleTable->setCurrentCell(0,0);
	guiUpdateTimestepList("remove timestep from list");
}
void AnimationEventRouter::populateTimestepTable(){
	dontUpdate = true;
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	std::vector<int>& tSteps = aParams->getTimestepList();
	timestepSampleTable->setRowCount(tSteps.size());
	timestepSampleTable->setColumnCount(1);
	for (int i = 0; i< tSteps.size(); i++){
		QTableWidgetItem* tstepItem = new QTableWidgetItem(QString::number(tSteps[i]));
		timestepSampleTable->setItem(i,0, tstepItem);
	}
	timestepSampleCheckbox->setChecked(aParams->usingTimestepList());
	dontUpdate = false;
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
	refreshFrontTab();
}

void AnimationEventRouter::guiChangeKeyIndex(int keyIndex){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	currentKeyIndex = keyIndex;
	Keyframe* kf = aParams->getKeyframe(currentKeyIndex);
	keyTimestepEdit->setText(QString::number(kf->timeStep));
	endTimestepEdit->setText(QString::number(kf->endTimestep));
	durationEdit->setText(QString::number(kf->duration));
	vector<Keyframe*> keyframes = aParams->getKeyframes();
	int totframes = 0;
	for (int i = 0; i<currentKeyIndex; i++) totframes += (keyframes[i]->frameNum+keyframes[i]->duration);
	frameIndexEdit->setText(QString::number(totframes));
	speedEdit->setText(QString::number(kf->speed));
	if (kf->speed > 0.){
		durationEdit->setEnabled(false);
		endTimestepEdit->setEnabled(false);
	} else {
		durationEdit->setEnabled(true);
		endTimestepEdit->setEnabled(true);
	}

	keyframeTextChanged = false;
}
void AnimationEventRouter::guiEnableKeyframing(bool enabled){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Toggle keyframing enabled");
	aParams->enableKeyframing(enabled);
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->animationParamsChanged(aParams);
	VizWinMgr::getInstance()->setAnimationDirty(aParams);
	updateTab();
}
void AnimationEventRouter::guiChangeKeyframe(){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Adjust Keyframe");
	currentKeyIndex = keyIndexSpin->value();
	Keyframe* kf = aParams->getKeyframe(currentKeyIndex);
	Viewpoint* vp = VizWinMgr::getInstance()->getActiveVPParams()->getCurrentViewpoint();
	delete kf->viewpoint;
	kf->viewpoint = new Viewpoint(*vp);
	kf->timeStep = aParams->getCurrentTimestep();
	
	//speed and framenum are unchanged;
	fixKeyframes();
	aParams->buildViewsAndTimes();
	PanelCommand::captureEnd(cmd, aParams);
	updateTab();
}
void AnimationEventRouter::guiDeleteKeyframe(){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	int numkeys = aParams->getNumKeyframes();
	if (numkeys <2) return;
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Delete Keyframe");
	currentKeyIndex = keyIndexSpin->value();
	aParams->deleteKeyframe(currentKeyIndex);
	fixKeyframes();
	if(currentKeyIndex == numkeys-1){
		keyIndexSpin->setValue(currentKeyIndex-1);
		currentKeyIndex--;
	}
	aParams->buildViewsAndTimes();
	PanelCommand::captureEnd(cmd, aParams);
	updateTab();
}
void AnimationEventRouter::guiInsertKeyframe(){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Insert Keyframe");
	//use settings from the app
	currentKeyIndex = keyIndexSpin->value();
	Viewpoint* newVP = new Viewpoint(*VizWinMgr::getInstance()->getActiveVPParams()->getCurrentViewpoint());
	int newTS = aParams->getCurrentTimestep();
	float newSpeed = aParams->getCurrentCameraSpeed();
	//If camera position hasn't changed, just ignore this insert.
	float camDist = vdist(newVP->getCameraPosLocal(),aParams->getKeyframe(currentKeyIndex)->viewpoint->getCameraPosLocal());
	DataStatus* ds = DataStatus::getInstance();
	float sceneSize = vlength(ds->getFullSizes());
	if (camDist < 1.e-4 * sceneSize){
		MessageReporter::warningMsg("Keyframe insertion ignored. \nViewpoint did not move.");
		delete newVP;
		delete cmd;
		return;
	}

	
	Keyframe* newKeyframe = new Keyframe(newVP,newSpeed,newTS,1);
	
	aParams->insertKeyframe(currentKeyIndex,newKeyframe);
	if (keyIndexSpin->maximum() < currentKeyIndex+1) keyIndexSpin->setMaximum(currentKeyIndex+1);
	keyIndexSpin->setValue(currentKeyIndex+1);
	currentKeyIndex++;
	fixKeyframes();
	aParams->buildViewsAndTimes();
	PanelCommand::captureEnd(cmd, aParams);
	updateTab();
}

void AnimationEventRouter::guiGotoKeyframe(){
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	currentKeyIndex = keyIndexSpin->value();
	Keyframe* key = aParams->getKeyframe(currentKeyIndex);
	//If keyframing is enabled, just need to go to that keyframe
	//Put the vpParams in the undo/redo queue.
	ViewpointParams* vpParams = VizWinMgr::getActiveVPParams();
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "Go To Keyframe");
	if (aParams->keyframingEnabled()){
		vector<Keyframe*> keyframes = aParams->getKeyframes();
		int posn = 0;
		for (int i = 0; i<currentKeyIndex; i++) posn+=(keyframes[i]->frameNum+keyframes[i]->duration);
		aParams->setCurrentFrameNumber(posn);
	} else {
		//otherwise, set the current timestep and viewpoint based on this keyframe:
		aParams->setCurrentFrameNumber(key->timeStep);
		vpParams->setCurrentViewpoint(new Viewpoint(*key->viewpoint));
	}
	updateTab();
	VizWinMgr::getInstance()->setViewerCoordsChanged(vpParams);
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	viz->setRegionNavigating(true);
	
	viz->updateGL();
	PanelCommand::captureEnd(cmd, vpParams);
}

void AnimationEventRouter::fixKeyframes(){
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	vector<Keyframe*> keyframes = aParams->getKeyframes();
	DataStatus* ds = DataStatus::getInstance();
	float sceneSize = vlength(ds->getFullSizes());
	//go through and remove if adjacent camera position coincide;
	for (int i = keyframes.size()-2; i>= 0; i--){
		float* campos1 = keyframes[i]->viewpoint->getCameraPosLocal();
		float* campos2 = keyframes[i+1]->viewpoint->getCameraPosLocal();
		float dist = vdist(campos1,campos2);
		if (dist < 1.e-4 * sceneSize){
			MessageReporter::warningMsg("Keyframe %d was deleted, \n%s",
					i+1, "because the camera position did not change between this and the preceding keyframe");
			aParams->deleteKeyframe(i+1);
		}
	}
	//Next, if adjacent speeds are zero, and camera is not fixed then insert a moving frame at speed 0.1.
	//
	bool allOK = false;
	while (!allOK){
		if (keyframes.size()<2) allOK = true;
		for (int i = 0; i<keyframes.size()-1; i++){
			float speed1 = keyframes[i]->speed;
			float speed2 = keyframes[i+1]->speed;
			if (speed1 < 1.e-5 && speed2 < 1.e-5){
				float* campos1 = keyframes[i]->viewpoint->getCameraPosLocal();
				float* campos2 = keyframes[i+1]->viewpoint->getCameraPosLocal();
				float dist = vdist(campos1,campos2);
				if (dist > 1.e-4 * sceneSize){
					//Create a viewpoint in between the two frames
					Viewpoint* newVP = Viewpoint::interpolate(keyframes[i]->viewpoint,keyframes[i+1]->viewpoint, 0.5f);
					int midTS = (int)(0.5f +0.5f*(keyframes[i]->timeStep + keyframes[i+1]->timeStep));
					Keyframe* newKey = new Keyframe(newVP, 0.1f,midTS, (int)(0.5f + 0.5f*keyframes[i]->frameNum));
					aParams->insertKeyframe(i,newKey);
					keyframes = aParams->getKeyframes(); 
					keyIndexSpin->setMaximum(aParams->getNumKeyframes());
					MessageReporter::warningMsg("An additional keyframe was inserted after keyframe %d, \n%s",
						i, "to enable camera to move between two stationary positions");
					currentKeyIndex = keyIndexSpin->value();
					if (currentKeyIndex > i && currentKeyIndex < aParams->getNumKeyframes()){
						keyIndexSpin->setValue(currentKeyIndex+1);
						currentKeyIndex++;
					}
					break;
				}
			}
			//If you ever get to the end, it's OK.
			if (i == keyframes.size()-2) allOK = true;
		}
	}
	return;
}
void AnimationEventRouter::refreshFrontTab(){
	TabManager* tmgr = MainForm::getTabManager();
	EventRouter* eRouter = tmgr->getFrontEventRouter();
	eRouter->updateTab();
}
