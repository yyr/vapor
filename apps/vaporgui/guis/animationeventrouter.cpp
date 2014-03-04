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
#include "glutil.h"	// Must be included first!!!
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
#include "vapor/ControlExecutive.h"

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

#include "animationeventrouter.h"

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
	connect (currentTimestepEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (endFrameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setEndFrameTextChanged(const QString&)));
	connect (frameStepEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (maxFrameRateEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	
	
	//Connect all the returnPressed signals, these will update the visualizer.
	connect (startFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (currentFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (currentTimestepEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (endFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (frameStepEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (maxFrameRateEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	
	//Animation control widgets that result in rebuilding animation
	connect (keyTimestepEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (numFramesEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (speedEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (timestepRateEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	

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

	

	connect (keyIndexSpin, SIGNAL(valueChanged(int)),this, SLOT(guiChangeKeyIndex(int)));
	
	

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
setEndFrameTextChanged(const QString& ){
	
	textChangedFlag = true;
	endFrameTextChanged = true;
}
void AnimationEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	guiSetTextChanged(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	
	Command* cmd = Command::CaptureStart(aParams,"animation text edit");
	bool frameChanged = false;
	
	QString strn;
	int startFrame = startFrameEdit->text().toInt();
	int minFrame = aParams->getMinTimestep();
	int maxFrame = aParams->getMaxTimestep();
	
	if (startFrame < minFrame || startFrame > maxFrame) {
		startFrame = minFrame;
		startFrameEdit->setText(strn.setNum(startFrame));
	}

	aParams->setStartTimestep(startFrame);
	int endFrame = aParams->getEndTimestep();
	if (endFrameTextChanged){
		endFrame = endFrameEdit->text().toInt();
		//make sure endFrame is valid.  If keyframing is enabled, it needs to be no greater than the 
		if (endFrame > maxFrame || endFrame < minFrame) {
			endFrame = maxFrame;
			endFrameEdit->setText(strn.setNum(endFrame));
		}
		aParams->setEndTimestep(endFrame);
		endFrameTextChanged = false;
	}
	int savedFrameNum = aParams->getCurrentTimestep();
	int currentFrame = currentFrameEdit->text().toInt();
	int currentFrame2 = currentTimestepEdit->text().toInt();
	if (currentFrame2 != aParams->getCurrentTimestep()) currentFrame = currentFrame2;
	if (currentFrame < startFrame) currentFrame = startFrame;
	if (currentFrame > endFrame) currentFrame = endFrame;
	currentFrameEdit->setText(strn.setNum(currentFrame));
	currentTimestepEdit->setText(strn.setNum(currentFrame));


	aParams->setCurrentTimestep(currentFrame);
	MainForm::getInstance()->setCurrentTimestep(aParams->getCurrentTimestep());
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));

	int frameStepSize = frameStepEdit->text().toInt();
	if (frameStepSize > (endFrame - startFrame)) frameStepSize = endFrame - startFrame;
	if (frameStepSize < 1) frameStepSize = 1;
	frameStepEdit->setText(strn.setNum(frameStepSize));
	aParams->setFrameStepSize(frameStepSize);
	setSliders(aParams);
	if (savedFrameNum != currentFrame) frameChanged=true;


	float maxFrameRate = maxFrameRateEdit->text().toFloat();
	
	//Constrain to a "reasonable" range:
	if (maxFrameRate> 1000.f) maxFrameRate = 1000.f;
	if (maxFrameRate< 0.001f) maxFrameRate = 0.001f;
	
	aParams->setMaxFrameRate(maxFrameRate);
	
	aParams->Validate(false);
	Command::CaptureEnd(cmd,aParams);
	
	updateTab();
	guiSetTextChanged(false);
	
}

//Insert values from params into tab panel
//
void AnimationEventRouter::updateTab(){
	if(!MainForm::getTabManager()->isFrontTab(this)) return;
	if (!DataStatus::getInstance()->getDataMgr()) return;
	AnimationParams* aParams = (AnimationParams*) VizWinMgr::getInstance()->getActiveAnimationParams();
	float sliderVal;
	QString strn;
	
	
	sliderVal = 0.f;
	int startFrame = aParams->getStartTimestep();
	int endFrame = aParams->getEndTimestep();
	int currentFrame = aParams->getCurrentTimestep();
	
	if (endFrame > startFrame)
		sliderVal = 0.5f+ 20000.f*(float)(currentFrame-startFrame)/(float)(endFrame-startFrame);
	animationSlider->setValue((int)sliderVal);
	
	
	startFrameEdit->setText(strn.setNum(startFrame));
	currentFrameEdit->setText(strn.setNum(currentFrame));
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	MainForm::getInstance()->setCurrentTimestep(aParams->getCurrentTimestep());
	endFrameEdit->setText(strn.setNum(endFrame));
	endFrameTextChanged = false;
	minFrameLabel->setText(strn.setNum(aParams->getMinTimestep()));
	maxFrameLabel->setText(strn.setNum(aParams->getMaxTimestep()));
	maxFrameRateEdit->setText(strn.setNum(aParams->getMaxFrameRate(),'g',3));
	frameStepEdit->setText(strn.setNum(aParams->getFrameStepSize()));
	
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
	if (aParams->IsLocal())
		LocalGlobal->setCurrentIndex(1);
	else 
		LocalGlobal->setCurrentIndex(0);

	

	
	minframeLabel->setText("Min Time Step:");
	maxframeLabel->setText("Max Time Step:");
	
	

	guiSetTextChanged(false);
	
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
	currentKeyIndex=0;
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
	
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	switch (direction) {
		case -1:
			stepForwardButton->setEnabled(false);
			stepReverseButton->setEnabled(false);
			
			break;
		case 0:
			
			
			stepForwardButton->setEnabled(true);
			stepReverseButton->setEnabled(true);
			break;
		case 1:
		
			
			stepForwardButton->setEnabled(false);
			stepReverseButton->setEnabled(false);
			
			break;
		default:
			assert(0);
	}
	
	aParams->setPlayDirection(direction);
	updateTab();
	
	if (direction == 0){ //refresh front tab image on stop play ...
		refreshFrontTab();
	}
	
}
void AnimationEventRouter::
guiJumpToBegin(){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	
	int startFrame = aParams->getStartTimestep();
	aParams->setCurrentTimestep(startFrame);
	currentFrameEdit->setText(QString::number(startFrame));
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	MainForm::getInstance()->setCurrentTimestep(aParams->getCurrentTimestep());
	setSliders(aParams);
	guiSetTextChanged(false);
	update();
	
	refreshFrontTab();
}
void AnimationEventRouter::
guiJumpToEnd(){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	
	int endFrame = aParams->getEndTimestep();
	aParams->setCurrentTimestep(endFrame);
	currentFrameEdit->setText(QString::number(endFrame));
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	MainForm::getInstance()->setCurrentTimestep(aParams->getCurrentTimestep());
	setSliders(aParams);
	
	guiSetTextChanged(false);
	update();
	
	refreshFrontTab();
}


//Respond to release of frame position slider:
void AnimationEventRouter::guiSetPosition(int position){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	
	int startFrame = aParams->getStartTimestep();
	int endFrame = aParams->getEndTimestep();
	int newFrameNum = startFrame + (int)((float)(endFrame - startFrame)*(float)position/20000.f + 0.5f);
	if(newFrameNum == aParams->getCurrentTimestep()) return;
	
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

	aParams->setCurrentTimestep(newFrameNum);
	int currentFrame = aParams->getCurrentTimestep();
	currentFrameEdit->setText(QString::number(currentFrame));
	MainForm::getInstance()->setCurrentTimestep(aParams->getCurrentTimestep());
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	
	guiSetTextChanged(false);
	updateTab();
	
	refreshFrontTab();
}
//Respond to release of stepsize slider.  Max range of slider is end - start
void AnimationEventRouter::guiSetFrameStep(int stepsize){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	
	int endFrame = aParams->getEndTimestep();
	int startFrame = aParams->getStartTimestep();
	int frameStepSize = (int)(1.5+ ((float)(endFrame - startFrame)*(float)stepsize/1000.f));
	if (frameStepSize < 1) frameStepSize = 1;
	aParams->setFrameStepSize(frameStepSize);
	
	frameStepEdit->setText(QString::number(frameStepSize));
	
	guiSetTextChanged(false);
	update();
	
	
}
void AnimationEventRouter::guiToggleReplay(bool replay){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	
	aParams->setRepeating(replay);
	
	update();
	
}
//Set change bits when global/local change occurs, so that the animation
//controller will change the local/global status at the end of the next rendering.
void AnimationEventRouter::guiSetLocal(Params* p, bool lg){
	
	EventRouter::guiSetLocal(p,lg);
	
	int viznum = VizWinMgr::getInstance()->getActiveViz();
		
	
	VizWin* viz = VizWinMgr::getInstance()->getVizWin(viznum);
	viz->updateGL();
}

void AnimationEventRouter::guiSingleStep(bool forward){
	confirmText(false);
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
	
	int currentFrame = aParams->getCurrentTimestep();
	int dir = (forward ? 1 : -1);
	int nextFrame = aParams->getNextFrame(dir);
	if (nextFrame == currentFrame) return;
	
	currentFrameEdit->setText(QString::number(nextFrame));
	
	aParams->setCurrentTimestep(nextFrame);
	currentTimestepEdit->setText(QString::number(aParams->getCurrentTimestep()));
	MainForm::getInstance()->setCurrentTimestep(aParams->getCurrentTimestep());
	setSliders(aParams);
	guiSetTextChanged(false);
	

	update();
	refreshFrontTab();
}
//Set the position slider consistent with latest value of currentPosition, frameStep, and bounds
void AnimationEventRouter::
setSliders(AnimationParams* aParams){
	guiSetTextChanged(false);//don't trigger text changed event when move slider
	int sliderPosition = animationSlider->value();
	int currentFrame = aParams->getCurrentTimestep();
	int startFrame = aParams->getStartTimestep();
	int endFrame = aParams->getEndTimestep();
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



void AnimationEventRouter::guiSetTimestep(int framenum){
	AnimationParams* aParams = VizWinMgr::getInstance()->getActiveAnimationParams();
	
	aParams->setCurrentTimestep(framenum);
	
	refreshFrontTab();
}


void AnimationEventRouter::refreshFrontTab(){
	TabManager* tmgr = MainForm::getTabManager();
	EventRouter* eRouter = tmgr->getFrontEventRouter();
	eRouter->updateTab();
}

