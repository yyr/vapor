#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif
#include <QScrollArea>
#include <QScrollBar>
#include <qapplication.h>
#include <qcursor.h>
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
#include <qtimer.h>
#include <qtooltip.h>

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
#include "instancetable.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "params.h"
#include <vapor/XmlNode.h>
#include "tabmanager.h"
#include "glutil.h"
#include "arrowparams.h"
#include "arrowrenderer.h"
#include "arroweventrouter.h"
#include "eventrouter.h"


using namespace VAPoR;


ArrowEventRouter::ArrowEventRouter(QWidget* parent): QWidget(parent), Ui_Arrow(), EventRouter(){
        setupUi(this);
	myParamsBaseType = Params::GetTypeFromTag(ArrowParams::_arrowParamsTag);
	savedCommand = 0;
}


ArrowEventRouter::~ArrowEventRouter(){
	if (savedCommand) delete savedCommand;
}
/**********************************************************
 * Whenever a new Arrowtab is created it must be hooked up here
 ************************************************************/
void
ArrowEventRouter::hookUpTab()
{
	//following are needed for any renderer eventrouter:
	
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (lodCombo,SIGNAL(activated(int)), this, SLOT(guiSetCompRatio(int)));
	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setArrowEnabled(bool,int)));
	
	//Unique connections for ArrowTab:
	//Connect all line edits to textChanged and return pressed: 
	connect (thicknessEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (thicknessEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (scaleEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (scaleEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (rakeMinX, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (rakeMinX, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (rakeMinY, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (rakeMinY, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (rakeMinZ, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (rakeMinZ, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (rakeMaxX, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (rakeMaxX, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (rakeMaxY, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (rakeMaxY, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (rakeMaxZ, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (rakeMaxZ, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (rakeGridX, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (rakeGridX, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (rakeGridY, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (rakeGridY, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (rakeGridZ, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (rakeGridZ, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	
	//Connect variable combo boxes to their own slots:
	connect (xVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetXVarNum(int)));
	connect (yVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetYVarNum(int)));
	connect (zVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetZVarNum(int)));
	//checkboxes
	connect(terrainAlignCheckbox,SIGNAL(toggled(bool)), this, SLOT(guiToggleTerrainAlign(bool)));
	//buttons:
	connect (colorSelectButton, SIGNAL(pressed()), this, SLOT(guiSelectColor()));

}

/*********************************************************************************
 * Slots associated with ArrowTab:
 *********************************************************************************/
void ArrowEventRouter::guiChangeInstance(int newCurrent){
	performGuiChangeInstance(newCurrent);
}

	
void ArrowEventRouter::guiNewInstance(){
	performGuiNewInstance();
}
void ArrowEventRouter::guiDeleteInstance(){
	performGuiDeleteInstance();
}

void ArrowEventRouter::guiCopyInstanceTo(int toViz){
	if (toViz == 0) return; 
	if (toViz == 1) {performGuiCopyInstance(); return;}
	int viznum = copyCount[toViz];
	copyCombo->setCurrentIndex(0);
	performGuiCopyInstanceToViz(viznum);
}

void ArrowEventRouter::
setArrowTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void ArrowEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "edit Arrow text");
	bool changed = false;
	QString strn;
	//Get all text values from gui, apply to params
	int gridsize[3];
	gridsize[0] = rakeGridX->text().toInt();
	gridsize[1] = rakeGridY->text().toInt();
	gridsize[2] = rakeGridZ->text().toInt();
	for (int i = 0; i<3; i++){
		if (gridsize[i] < 1) { changed = true; gridsize[i] = 1;}
		if (gridsize[i] > 2000) {changed = true; gridsize[i] = 2000;}
	}
	aParams->SetRakeGrid(gridsize);
	vector<double> newExtents;
	newExtents.push_back(rakeMinX->text().toDouble());
	newExtents.push_back(rakeMinY->text().toDouble());
	newExtents.push_back(rakeMinZ->text().toDouble());
	newExtents.push_back(rakeMaxX->text().toDouble());
	newExtents.push_back(rakeMaxY->text().toDouble());
	newExtents.push_back(rakeMaxZ->text().toDouble());
	const float* exts = DataStatus::getInstance()->getExtents();
	for (int i = 0; i<3; i++){
		if (newExtents[i]<exts[i]) {changed = true; newExtents[i] = exts[i];}
		if (newExtents[i]>exts[i+3]) {changed = true; newExtents[i] = exts[i+3];}
		if (newExtents[i+3]<exts[i]) {changed = true; newExtents[i+3] = exts[i];}
		if (newExtents[i+3]>exts[i+3]) {changed = true; newExtents[i+3] = exts[i+3];}
		if (newExtents[i+3]<newExtents[i]) {changed = true; newExtents[i+3] = newExtents[i];}
	}
	aParams->SetRakeExtents(newExtents);

	float thickness = thicknessEdit->text().toFloat();
	if (thickness < 0.f) {changed = true; thickness = 0.f;}
	if (thickness > 1000.f) {changed = true; thickness = 1000.f;}
	aParams->SetLineThickness((double)thickness);

	float scale = scaleEdit->text().toFloat();
	aParams->SetVectorScale(scale);
	
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, aParams);
	if (changed) updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
	
}
//Save undo/redo state when user grabs a rake handle
//
void ArrowEventRouter::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	savedCommand = PanelCommand::captureStart(aParams,  "slide rake handle");
	
}
void ArrowEventRouter::
captureMouseUp(){
	VizWinMgr* vwm = VizWinMgr::getInstance();
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	//Update the tab if it's in front:
	if(MainForm::getTabManager()->isFrontTab(this)) {
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (aParams == vwm->getParams(viznum, ParamsBase::GetTypeFromTag(ArrowParams::_arrowParamsTag))))
			updateTab();
	}
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, aParams);
	
	//force render:
	VizWinMgr::getInstance()->forceRender(aParams);
	savedCommand = 0;
}
void ArrowEventRouter::
arrowReturnPressed(void){
	confirmText(true);
}
void ArrowEventRouter::
guiSetXVarNum(int vnum){
	confirmText(true);
	
	int sesvarnum=0;
	if(vnum > 0) {
		//Make sure its a valid variable..
		sesvarnum = DataStatus::getInstance()->mapActiveToSessionVarNum3D(vnum-1);
		if (sesvarnum < 0) return; 
	}
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "set arrow x variable");
	if (vnum > 0){
		aParams->SetFieldVariableName(0,DataStatus::getInstance()->getVariableName3D(sesvarnum));
	} else aParams->SetFieldVariableName(0,"0");
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiSetYVarNum(int vnum){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "set arrow y variable");
	if(vnum > 0) {
		int sesvarnum = DataStatus::getInstance()->mapActiveToSessionVarNum3D(vnum-1);
		aParams->SetFieldVariableName(1,DataStatus::getInstance()->getVariableName3D(sesvarnum));
	} else aParams->SetFieldVariableName(1,"0");
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiSetZVarNum(int vnum){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "set arrow z variable");
	if(vnum > 0) {
		int sesvarnum = DataStatus::getInstance()->mapActiveToSessionVarNum3D(vnum-1);
		aParams->SetFieldVariableName(2,DataStatus::getInstance()->getVariableName3D(sesvarnum));
	} else aParams->SetFieldVariableName(2,"0");
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiToggleTerrainAlign(bool isOn){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "toggle terrain alignment");
	aParams->SetTerrainMapped(isOn);
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiSelectColor(){
	confirmText(true);
	QPalette pal(colorBox->palette());
	QColor newColor = QColorDialog::getColor(pal.color(QPalette::Base), this);
	if (!newColor.isValid()) return;
	pal.setColor(QPalette::Base, newColor);
	colorBox->setPalette(pal);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "change arrow color");
	qreal rgb[3];
	newColor.getRgbF(rgb,rgb+1,rgb+2);
	float rgbf[3];
	for (int i = 0; i<3; i++) rgbf[i] = (float)rgb[i];
	aParams->SetConstantColor(rgbf);
	PanelCommand::captureEnd(cmd, aParams);
}
	
void ArrowEventRouter::
setArrowEnabled(bool val, int instance){

	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	ArrowParams* dParams = (ArrowParams*)(Params::GetParamsInstance(ArrowParams::_arrowParamsTag, activeViz, instance));
	//Make sure this is a change:
	if (dParams->isEnabled() == val ) return;
	//If we are enabling, also make this the current instance:
	if (val) {
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val,instance);
	
}




//Insert values from params into tab panel
//
void ArrowEventRouter::updateTab(){
	if(!MainForm::getInstance()->getTabManager()->isFrontTab(this)) return;
	MessageReporter::infoMsg("ArrowEventRouter::updateTab()");
	if (!isEnabled()) return;
	Session *session = Session::getInstance();
	session->blockRecording();

	//Set up the instance table:
	DataStatus* ds = DataStatus::getInstance();
	if (ds->getDataMgr()) instanceTable->setEnabled(true);
	else instanceTable->setEnabled(false);
	instanceTable->rebuild(this);
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	ArrowParams* arrowParams = (ArrowParams*) VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	int winnum = vizMgr->getActiveViz();
	int numViz = vizMgr->getNumVisualizers();
	copyCombo->clear();
	copyCombo->addItem("Duplicate In:");
	copyCombo->addItem("This visualizer");
	if (numViz > 1) {
		int copyNum = 2;
		for (int i = 0; i<MAXVIZWINS; i++){
			if (vizMgr->getVizWin(i) && winnum != i){
				copyCombo->addItem(vizMgr->getVizWinName(i));
				//Remember the viznum corresponding to a combo item:
				copyCount[copyNum++] = i;
			}
		}
	}
	deleteInstanceButton->setEnabled(vizMgr->getNumInstances(winnum, Params::GetTypeFromTag(ArrowParams::_arrowParamsTag)) > 1);

	//Set up refinements and LOD combos:
	int numRefs = arrowParams->GetRefinementLevel();
	if(numRefs <= refinementCombo->count())
		refinementCombo->setCurrentIndex(numRefs);
	lodCombo->setCurrentIndex(arrowParams->GetCompressionLevel());
	
	//Set the combo based on the (3D) field variables
	int comboIndex[3] = { 0,0,0};
	for (int i = 0; i<3; i++) {
		string vname = arrowParams->GetFieldVariableName(i);
		if(vname != "0") comboIndex[i] = 1+ds->getSessionVariableNum3D(vname);
	}
	xVarCombo->setCurrentIndex(comboIndex[0]);
	yVarCombo->setCurrentIndex(comboIndex[1]);
	zVarCombo->setCurrentIndex(comboIndex[2]);

	//Set the constant color box
	const float* clr = arrowParams->GetConstantColor();
	QColor newColor;
	newColor.setRgbF((qreal)clr[0],(qreal)clr[1],(qreal)clr[2]);
	QPalette pal(colorBox->palette());
	pal.setColor(QPalette::Base, newColor);
	
	colorBox->setPalette(pal);
	
	//Set the rake extents
	double exts[6];
	arrowParams->GetRakeExtents(exts);
	rakeMinX->setText(QString::number(exts[0]));
	rakeMinY->setText(QString::number(exts[1]));
	rakeMinZ->setText(QString::number(exts[2]));
	rakeMaxX->setText(QString::number(exts[3]));
	rakeMaxY->setText(QString::number(exts[4]));
	rakeMaxZ->setText(QString::number(exts[5]));

	const vector<long>rakeGrid = arrowParams->GetRakeGrid();
	rakeGridX->setText(QString::number(rakeGrid[0]));
	rakeGridY->setText(QString::number(rakeGrid[1]));
	rakeGridZ->setText(QString::number(rakeGrid[2]));
	
	thicknessEdit->setText(QString::number(arrowParams->GetLineThickness()));
	scaleEdit->setText(QString::number(arrowParams->GetVectorScale()));
	//Only allow terrainAlignCheckbox if there is a HGT variable
	int varnum = ds->getSessionVariableNum2D("HGT");
	if (ds->variableIsPresent2D(varnum)){
		terrainAlignCheckbox->setEnabled(true);
		terrainAlignCheckbox->setChecked(arrowParams->IsTerrainMapped());
	} else {
		terrainAlignCheckbox->setEnabled(false);
		terrainAlignCheckbox->setChecked(false);
		if(arrowParams->IsTerrainMapped())
			arrowParams->SetTerrainMapped(false);
	}

	update();
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	vizMgr->getTabManager()->update();
}

//Reinitialize Arrow tab settings, session has changed.
//Note that this is called after the globalArrowParams are set up, but before
//any of the localArrowParams are setup.
void ArrowEventRouter::
reinitTab(bool doOverride){
	
	DataStatus* ds = DataStatus::getInstance();
	if (ds->dataIsPresent3D()) {
		setEnabled(true);
		instanceTable->setEnabled(true);
		instanceTable->rebuild(this);
	}
	else setEnabled(false);
	int i;
	

	//Set up the refinement combo:
	const DataMgr *dataMgr = ds->getDataMgr();
	
	int numRefinements = dataMgr->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (i = 0; i<= numRefinements; i++){
		refinementCombo->addItem(QString::number(i));
	}
	
	if (dataMgr){
		vector<size_t> cRatios = dataMgr->GetCRatios();
		lodCombo->clear();
		lodCombo->setMaxCount(cRatios.size());
		for (int i = 0; i<cRatios.size(); i++){
			QString s = QString::number(cRatios[i]);
			s += ":1";
			lodCombo->addItem(s);
		}
	}
	//Set up the variable combos.  
	//The first entry is "0"
	xVarCombo->clear();
	xVarCombo->setMaxCount(ds->getNumActiveVariables3D()+1);
	xVarCombo->addItem(QString("0"));
	for (int i = 0; i< ds->getNumActiveVariables3D(); i++){
		const std::string& s = ds->getActiveVarName3D(i);
		xVarCombo->addItem(QString::fromStdString(s));
	}
	yVarCombo->clear();
	yVarCombo->setMaxCount(ds->getNumActiveVariables3D()+1);
	yVarCombo->addItem(QString("0"));
	for (int i = 0; i< ds->getNumActiveVariables3D(); i++){
		const std::string& s = ds->getActiveVarName3D(i);
		yVarCombo->addItem(QString::fromStdString(s));
	}
	zVarCombo->clear();
	zVarCombo->setMaxCount(ds->getNumActiveVariables3D()+1);
	zVarCombo->addItem(QString("0"));
	for (int i = 0; i< ds->getNumActiveVariables3D(); i++){
		const std::string& s = ds->getActiveVarName3D(i);
		zVarCombo->addItem(QString::fromStdString(s));
	}
	updateTab();
}

void ArrowEventRouter::
guiSetCompRatio(int num){
	confirmText(false);
	//make sure we are changing it
	ArrowParams* dParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	if (num == dParams->GetCompressionLevel()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set compression level");
	
	dParams->SetCompressionLevel(num);
	lodCombo->setCurrentIndex(num);
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->forceRender(dParams);
}

void ArrowEventRouter::
guiSetNumRefinements(int num){
	confirmText(false);
	//make sure we are changing it
	ArrowParams* dParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	if (num == dParams->GetRefinementLevel()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set number of refinements");
		
	dParams->SetRefinementLevel(num);
	refinementCombo->setCurrentIndex(num);
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->forceRender(dParams);
}
void ArrowEventRouter::
guiSetEnabled(bool value, int instance){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = vizWinMgr->getActiveViz();
	//Ignore spurious clicks.
	ArrowParams* dParams = (ArrowParams*)(Params::GetParamsInstance(ArrowParams::_arrowParamsTag, winnum, instance));
	
	if (value == dParams->isEnabled()) return;
	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "toggle arrow enabled", instance);
	dParams->setEnabled(value);
	PanelCommand::captureEnd(cmd, dParams);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(dParams,!value, false);
	
	updateTab();
	vizWinMgr->forceRender(dParams);
	
}



/* Handle the change of status associated with change of enablement 
 
 * If the window is new, (i.e. we are just creating a new window, use 
 * prevEnabled = false
 
 */
void ArrowEventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled, bool newWindow){
	
	
	ArrowParams* dParams = (ArrowParams*)rParams;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = dParams->getVizNum();
	
	if (newWindow) {
		prevEnabled = false;
	}
	
	bool nowEnabled = rParams->isEnabled();
	
	if (prevEnabled == nowEnabled) return;

	
	VizWin* viz = 0;
	if(winnum >= 0){//Find the viz that this applies to:
		//Note that this is only for the cases below where one particular
		//visualizer is needed
		viz = vizWinMgr->getVizWin(winnum);
	} 
	
	//cases to consider:
	//1.  unchanged disabled renderer; do nothing.
	//  enabled renderer, just force refresh:
	
	if (prevEnabled == nowEnabled) {
		if (!prevEnabled) return;
		VizWinMgr::getInstance()->forceRender(dParams);
		return;
	}
	
	//2.  Change of disable->enable .  Create a new renderer in active window.
	
	
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	
	
	//For a new renderer

	
	if (nowEnabled && !prevEnabled ){//For case 2.:  create a renderer in the active window:

		Renderer* myArrow = new ArrowRenderer(viz->getGLWindow(), dParams);


		viz->getGLWindow()->insertSortedRenderer(dParams,myArrow);

		//force the renderer to refresh 
		
		VizWinMgr::getInstance()->forceRender(dParams);
	
		//Quit 
		return;
	}
	
	assert(prevEnabled && !nowEnabled); //case 6, disable 
	viz->getGLWindow()->removeRenderer(dParams);

	return;
}


//Method called when undo/redo changes params.  If prevParams is null, the
//vizwinmgr will create a new instance.
void ArrowEventRouter::
makeCurrent(Params* prevParams, Params* newParams, bool newWin, int instance, bool reEnable) {
	assert(instance >= 0);

	ArrowParams* dParams = (ArrowParams*)(newParams->deepCopy());
	int vizNum = dParams->getVizNum();
	
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumInstances(vizNum,Params::GetTypeFromTag(ArrowParams::_arrowParamsTag)) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, dParams, Params::GetTypeFromTag(ArrowParams::_arrowParamsTag), instance);
	
	
	ArrowParams* formerParams = (ArrowParams*)prevParams;
	if (reEnable){
		//Need to disable the current instance
		assert(dParams->isEnabled() && formerParams->isEnabled());
		assert(!newWin);
		dParams->setEnabled(false);
		//Install dParams disabled:
		updateRenderer(dParams, true, false);
		//Then enable it
		dParams->setEnabled(true);
		updateRenderer(dParams, false, false);

	} else {
		bool wasEnabled = false;
		if (formerParams) wasEnabled = formerParams->isEnabled();
		//Check if the enabled and/or Local settings changed:
		if (newWin || (formerParams->isEnabled() != dParams->isEnabled())){
			updateRenderer(dParams, wasEnabled,  newWin);
		}
	}

	updateTab();
}

