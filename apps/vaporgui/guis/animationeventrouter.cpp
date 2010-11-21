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


AnimationEventRouter::AnimationEventRouter(QWidget* parent, const char* name) : QWidget(parent,0), Ui_AnimationTab(), EventRouter() {
	setupUi(this);	

	myParamsBaseType = VizWinMgr::RegisterEventRouter(Params::_animationParamsTag, this);
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
	dontUpdate = false;

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
	MainForm::getInstance()->setCurrentTimestep(currentFrame);

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
	if (maxFrameRate> 1000.f) maxFrameRate = 1000.f;
	if (maxFrameRate< 0.001f) maxFrameRate = 0.001f;
	if (maxWait < 0.001f) maxWait = 0.001f;
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
		sliderVal = 0.5f+ 1000.f*(float)(currentFrame-startFrame)/(float)(endFrame-startFrame);
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
		//pauseButton->setChecked(true);
		playForwardButton->setChecked(false);
		playReverseButton->setChecked(false);
		stepForwardButton->setEnabled(true);
		stepReverseButton->setEnabled(true);
	} else if (playDirection==1){//forward play
		//pauseButton->setChecked(false);
		playForwardButton->setChecked(true);
		playReverseButton->setChecked(false);
		if (!aParams->isRepeating() && currentFrame >= endFrame){
			//kludge to make the buttons update at the end of
			//the animation sequence.  The problem is that
			//the update can't be called from the shared controller thread,
			//so we have to catch the animation end by other means.
			stepForwardButton->setEnabled(true);
			stepReverseButton->setEnabled(true);
		}
	} else {//reverse play
		//pauseButton->setChecked(false);
		playForwardButton->setChecked(false);
		playReverseButton->setChecked(true);
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
	dontUpdate=false;
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
	guiSetPlay(0);
}
void AnimationEventRouter::
animationPlayReverseClick(){
	if (!playReverseButton->isDown()){
		playForwardButton->setChecked(false);
		playReverseButton->setChecked(true);
		guiSetPlay(-1);
	}
}
void AnimationEventRouter::
animationPlayForwardClick(){
	
	playForwardButton->setChecked(true);
	playReverseButton->setChecked(false);
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
	MainForm::getInstance()->setCurrentTimestep(startFrame);
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
	MainForm::getInstance()->setCurrentTimestep(endFrame);
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
	int newFrameNum = startFrame + (int)((float)(endFrame - startFrame)*(float)position/1000.f + 0.5f);
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
	MainForm::getInstance()->setCurrentTimestep(currentFrame);
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
	MainForm::getInstance()->setCurrentTimestep(nextFrame);
	aParams->setCurrentFrameNumber(nextFrame);
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
	int sliderFrame = startFrame + (int)(0.5f+(float)(endFrame - startFrame)*(float)sliderPosition/1000.f);
	if (sliderFrame != currentFrame && (endFrame != startFrame)){
		animationSlider->setValue((int)(1000.f*((float)(currentFrame - startFrame)/(float)(endFrame-startFrame))));
	}
	sliderPosition = frameStepSlider->value();
	int stepsize = 1+(int)(0.5f+(float)(endFrame - startFrame)*(float)sliderPosition/1000.f);
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
void AnimationEventRouter::refreshFrontTab(){
	TabManager* tmgr = MainForm::getInstance()->getTabManager();
	EventRouter* eRouter = tmgr->getFrontEventRouter();
	eRouter->updateTab();
}
