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
	showAppearance = false;
	showLayout = false;
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
	
	connect (xDimEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (xDimEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (yDimEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (yDimEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (zDimEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (zDimEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));

	connect (xStrideEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (xStrideEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (yStrideEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (yStrideEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	
	//Connect variable combo boxes to their own slots:
	connect (xVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetXVarNum(int)));
	connect (yVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetYVarNum(int)));
	connect (zVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetZVarNum(int)));
	connect (variableDimCombo, SIGNAL(activated(int)), this, SLOT(guiSetVariableDims(int)));
	//checkboxes
	connect(terrainAlignCheckbox,SIGNAL(toggled(bool)), this, SLOT(guiToggleTerrainAlign(bool)));
	connect(alignDataCheckbox,SIGNAL(toggled(bool)),this, SLOT(guiAlignToData(bool)));
	//buttons:
	connect (colorSelectButton, SIGNAL(pressed()), this, SLOT(guiSelectColor()));
	connect (boxSliderFrame, SIGNAL(extentsChanged()), this, SLOT(guiChangeExtents()));
	connect (showHideLayoutButton, SIGNAL(pressed()), this, SLOT(showHideLayout()));
	connect (showHideAppearanceButton, SIGNAL(pressed()), this, SLOT(showHideAppearance()));
	connect (fitDataButton, SIGNAL(pressed()), this, SLOT(guiFitToData()));
	//slider:
	connect (barbLengthSlider, SIGNAL(sliderMoved(int)),this,SLOT(guiMoveScaleSlider(int)));

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
	gridsize[0] = xDimEdit->text().toInt();
	gridsize[1] = yDimEdit->text().toInt();
	gridsize[2] = zDimEdit->text().toInt();
	for (int i = 0; i<3; i++){
		if (gridsize[i] < 1) { changed = true; gridsize[i] = 1;}
		if (gridsize[i] > 2000) {changed = true; gridsize[i] = 2000;}
	}
	aParams->SetRakeGrid(gridsize);
	
	//Check if the strides have changed; if so will need to updateTab.
	if (aParams->IsAlignedToData()){
		const vector<long> currStrides = aParams->GetGridAlignStrides();
		long xstride = xStrideEdit->text().toInt();
		long ystride = yStrideEdit->text().toInt();
		if (currStrides[0] != xstride || currStrides[1] != ystride){
			vector<long> strides;
			strides.push_back( xstride);
			strides.push_back( ystride);
			aParams->SetGridAlignStrides(strides);
			changed = true;
		}
	}

	float thickness = thicknessEdit->text().toFloat();
	if (thickness < 0.f) {changed = true; thickness = 0.f;}
	if (thickness > 1000.f) {changed = true; thickness = 1000.f;}
	aParams->SetLineThickness((double)thickness);

	double scale = scaleEdit->text().toDouble();
	if (scale != aParams->GetVectorScale()){
		aParams->SetVectorScale(scale);
		double defaultScale = aParams->calcDefaultScale();
		int sliderPos = (int)(0.5+log10(scale/defaultScale));
		if (sliderPos < -100) sliderPos = -100;
		if (sliderPos > 100) sliderPos = 100;
		barbLengthSlider->setValue(sliderPos);
	}
	
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
guiSetVariableDims(int is3D){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	if (aParams->VariablesAre3D() == (is3D == 1)) return;
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "set barb variable dimensions");
	aParams->SetVariables3D(is3D == 1);
	xVarCombo->setCurrentIndex(0);
	yVarCombo->setCurrentIndex(0);
	zVarCombo->setCurrentIndex(0);
	aParams->SetFieldVariableName(0,"0");
	aParams->SetFieldVariableName(1,"0");
	aParams->SetFieldVariableName(2,"0");
	//Set up variable combos:
	populateVariableCombos(is3D);
	aParams->SetVectorScale(aParams->calcDefaultScale());
	PanelCommand::captureEnd(cmd,aParams);
	updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
}


void ArrowEventRouter::
populateVariableCombos(bool is3D){
	DataStatus* ds;
	ds = DataStatus::getInstance();
	if (is3D){
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
	} else { //2D vars:
		//The first entry is "0"
		xVarCombo->clear();
		xVarCombo->setMaxCount(ds->getNumActiveVariables2D()+1);
		xVarCombo->addItem(QString("0"));
		for (int i = 0; i< ds->getNumActiveVariables2D(); i++){
			const std::string& s = ds->getActiveVarName2D(i);
			xVarCombo->addItem(QString::fromStdString(s));
		}
		yVarCombo->clear();
		yVarCombo->setMaxCount(ds->getNumActiveVariables2D()+1);
		yVarCombo->addItem(QString("0"));
		for (int i = 0; i< ds->getNumActiveVariables2D(); i++){
			const std::string& s = ds->getActiveVarName2D(i);
			yVarCombo->addItem(QString::fromStdString(s));
		}
		zVarCombo->clear();
		zVarCombo->setMaxCount(ds->getNumActiveVariables2D()+1);
		zVarCombo->addItem(QString("0"));
		for (int i = 0; i< ds->getNumActiveVariables2D(); i++){
			const std::string& s = ds->getActiveVarName2D(i);
			zVarCombo->addItem(QString::fromStdString(s));
		}
	}
}


void ArrowEventRouter::
guiSetXVarNum(int vnum){
	confirmText(true);
	
	int sesvarnum=0;
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	bool is3D = aParams->VariablesAre3D();
	if(vnum > 0) {
		//Make sure its a valid variable..
		if (is3D)
			sesvarnum = DataStatus::getInstance()->mapActiveToSessionVarNum3D(vnum-1);
		else 
			sesvarnum = DataStatus::getInstance()->mapActiveToSessionVarNum2D(vnum-1);
		if (sesvarnum < 0) return; 
	}
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "set barb x variable");
	if (vnum > 0){
		if (is3D)
			aParams->SetFieldVariableName(0,DataStatus::getInstance()->getVariableName3D(sesvarnum));
		else 
			aParams->SetFieldVariableName(0,DataStatus::getInstance()->getVariableName2D(sesvarnum));
	} else aParams->SetFieldVariableName(0,"0");
	aParams->SetVectorScale(aParams->calcDefaultScale());
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiSetYVarNum(int vnum){
	int sesvarnum=0;
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	bool is3D = aParams->VariablesAre3D();
	if(vnum > 0) {
		//Make sure its a valid variable..
		if (is3D)
			sesvarnum = DataStatus::getInstance()->mapActiveToSessionVarNum3D(vnum-1);
		else 
			sesvarnum = DataStatus::getInstance()->mapActiveToSessionVarNum2D(vnum-1);
		if (sesvarnum < 0) return; 
	}
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "set barb y variable");
	if (vnum > 0){
		if (is3D)
			aParams->SetFieldVariableName(1,DataStatus::getInstance()->getVariableName3D(sesvarnum));
		else 
			aParams->SetFieldVariableName(1,DataStatus::getInstance()->getVariableName2D(sesvarnum));
	} else aParams->SetFieldVariableName(1,"0");
	aParams->SetVectorScale(aParams->calcDefaultScale());
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiSetZVarNum(int vnum){
	int sesvarnum=0;
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	bool is3D = aParams->VariablesAre3D();
	if(vnum > 0) {
		//Make sure its a valid variable..
		if (is3D)
			sesvarnum = DataStatus::getInstance()->mapActiveToSessionVarNum3D(vnum-1);
		else 
			sesvarnum = DataStatus::getInstance()->mapActiveToSessionVarNum2D(vnum-1);
		if (sesvarnum < 0) return; 
	}
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "set barb z variable");
	if (vnum > 0){
		if (is3D)
			aParams->SetFieldVariableName(2,DataStatus::getInstance()->getVariableName3D(sesvarnum));
		else 
			aParams->SetFieldVariableName(2,DataStatus::getInstance()->getVariableName2D(sesvarnum));
	} else aParams->SetFieldVariableName(2,"0");
	aParams->SetVectorScale(aParams->calcDefaultScale());
	updateTab();
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
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "change barb color");
	qreal rgb[3];
	newColor.getRgbF(rgb,rgb+1,rgb+2);
	float rgbf[3];
	for (int i = 0; i<3; i++) rgbf[i] = (float)rgb[i];
	aParams->SetConstantColor(rgbf);
	PanelCommand::captureEnd(cmd, aParams);
}
void ArrowEventRouter::
guiChangeExtents(){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "change barb extents");
	double newExts[6];
	boxSliderFrame->getBoxExtents(newExts);
	Box* bx = aParams->GetBox();
	bx->SetExtents(newExts);
	PanelCommand::captureEnd(cmd,aParams);
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiAlignToData(bool doAlign){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "toggle align to data ");
	aParams->AlignGridToData(doAlign);
	xStrideEdit->setEnabled(doAlign);
	yStrideEdit->setEnabled(doAlign);
	PanelCommand::captureEnd(cmd,aParams);
	updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
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
	int currentTimeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
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
	
	//Set the combo based on the current field variables
	int comboIndex[3] = { 0,0,0};
	bool is3D = arrowParams->VariablesAre3D();
	int dimComIndex = variableDimCombo->currentIndex();
	if (is3D != (dimComIndex == 1)){
		variableDimCombo->setCurrentIndex(1-dimComIndex);
		populateVariableCombos(is3D);
	}
	if (is3D){
		for (int i = 0; i<3; i++) {
			string vname = arrowParams->GetFieldVariableName(i);
			if(vname != "0") comboIndex[i] = 1+ds->getSessionVariableNum3D(vname);
			else comboIndex[i]=0;
		}
	} else {
		for (int i = 0; i<3; i++) {
			string vname = arrowParams->GetFieldVariableName(i);
			if(vname != "0") comboIndex[i] = 1+ds->getSessionVariableNum2D(vname);
			else comboIndex[i]=0;
		}
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
	const float* fullExtents = ds->getExtents();
	double dbExts[6];
	for (int i = 0; i<6; i++) dbExts[i] = fullExtents[i];
	boxSliderFrame->setFullDomain(dbExts);

	const vector<double>exts = arrowParams->GetRakeExtents();
	boxSliderFrame->setBoxExtents(exts);

	//Provide latlon corner coords if available:
	if (DataStatus::getProjectionString().size() == 0){
		latLonFrame->hide();
	} else {
		double cornerLatLon[2];
		const vector<double> exts = arrowParams->GetBox()->GetExtents();
		cornerLatLon[0] = exts[0];
		cornerLatLon[1] = exts[1];
		if (DataStatus::convertToLatLon(currentTimeStep,cornerLatLon)){
			LLLonEdit->setText(QString::number(cornerLatLon[0]));
			LLLatEdit->setText(QString::number(cornerLatLon[1]));
			cornerLatLon[0] = exts[3];
			cornerLatLon[1] = exts[4];
			DataStatus::convertToLatLon(currentTimeStep,cornerLatLon);
			URLonEdit->setText(QString::number(cornerLatLon[0]));
			URLatEdit->setText(QString::number(cornerLatLon[1]));
			latLonFrame->show();
		} else {
			latLonFrame->hide();
		}
	}

	const vector<long>rakeGrid = arrowParams->GetRakeGrid();
	xDimEdit->setText(QString::number(rakeGrid[0]));
	yDimEdit->setText(QString::number(rakeGrid[1]));
	zDimEdit->setText(QString::number(rakeGrid[2]));

	vector<long> strides = arrowParams->GetGridAlignStrides();
	xStrideEdit->setText(QString::number(strides[0]));
	yStrideEdit->setText(QString::number(strides[1]));
	
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
	bool isAligned = arrowParams->IsAlignedToData();
	alignDataCheckbox->setChecked(isAligned);
	xStrideEdit->setEnabled(isAligned);
	yStrideEdit->setEnabled(isAligned);
	xDimEdit->setEnabled(!isAligned);
	yDimEdit->setEnabled(!isAligned);
	
	if (isAligned){
		double exts[6];
		int grdExts[3];
		arrowParams->calcDataAlignment(exts,grdExts);
		//Display the new aligned grid extents
		xDimEdit->setText(QString::number(grdExts[0]));
		yDimEdit->setText(QString::number(grdExts[1]));
	}
	if (showLayout) layoutFrame->show();
	else layoutFrame->hide();
	if (showAppearance) appearanceFrame->show();
	else appearanceFrame->hide();
	adjustSize();
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
	//Set up the variable combos with default 3D variables  
	populateVariableCombos(true);
	
	
	//set the combo to 3D
	variableDimCombo->setCurrentIndex(1);
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
guiMoveScaleSlider(int sliderval){
	confirmText(false);
	
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "move barb scale slider");
	double defaultScale = aParams->calcDefaultScale();
	// find the new length as 10**(sliderVal)*defaultLength
	// note that the slider goes from -100 to +100
	double newVal = defaultScale*pow(10.,(double)sliderval/100.);
	scaleEdit->setText(QString::number(newVal));
	guiSetTextChanged(false); //Don't respond to text-change event
	aParams->SetVectorScale(newVal);
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->forceRender(aParams);
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
guiSetEnabled(bool value, int instance, bool undoredo){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = vizWinMgr->getActiveViz();
	//Ignore spurious clicks.
	ArrowParams* dParams = (ArrowParams*)(Params::GetParamsInstance(ArrowParams::_arrowParamsTag, winnum, instance));
	
	if (value == dParams->isEnabled()) return;
	confirmText(false);
	PanelCommand* cmd;
	if(undoredo) cmd = PanelCommand::captureStart(dParams, "toggle arrow enabled", instance);
	dParams->setEnabled(value);
	if(undoredo) PanelCommand::captureEnd(cmd, dParams);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(dParams,!value, false);
	
	updateTab();
	vizWinMgr->forceRender(dParams);
	
}
void ArrowEventRouter::
guiFitToData(){
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(aParams, "Fit to full data extents");
		
	const float* exts = DataStatus::getInstance()->getExtents();
	Box* box = aParams->GetBox();
	const vector<double> boxExts = box->GetExtents();
	vector<double> newExtents;
	newExtents.push_back(exts[0]);
	newExtents.push_back(exts[1]);
	newExtents.push_back(boxExts[2]);
	newExtents.push_back(exts[3]);
	newExtents.push_back(exts[4]);
	newExtents.push_back(boxExts[5]);
	boxSliderFrame->setBoxExtents(newExtents);
	box->SetExtents(newExtents);
	PanelCommand::captureEnd(cmd, aParams);
	VizWinMgr::getInstance()->forceRender(aParams);
}

void ArrowEventRouter::
showHideAppearance(){
	if (showAppearance) {
		showAppearance = false;
		showHideAppearanceButton->setText("Show Appearance Options");
	} else {
		showAppearance = true;
		showHideAppearanceButton->setText("Hide Appearance Options");
	}
	//Following HACK is needed to convince Qt to remove the extra space in the tab:
	updateTab();
	VizWinMgr::getInstance()->getTabManager()->toggleFrontTabs(Params::GetTypeFromTag(ArrowParams::_arrowParamsTag));
	updateTab();

}

void ArrowEventRouter::
showHideLayout(){
	if (showLayout) {
		showLayout = false;
		showHideLayoutButton->setText("Show Barb Layout Options");
	} else {
		showLayout = true;
		showHideLayoutButton->setText("Hide Barb Layout Options");
	}
	//Following HACK is needed to convince Qt to remove the extra space in the tab:
	updateTab();
	VizWinMgr::getInstance()->getTabManager()->toggleFrontTabs(Params::GetTypeFromTag(ArrowParams::_arrowParamsTag));
	updateTab();
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

QSize ArrowEventRouter::sizeHint() const {
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	if (!aParams) return QSize(460,1500);
	int vertsize = 265;//basic panel plus instance panel 
	//add showAppearance button, showLayout button, frames
	vertsize += 100;
	if (showLayout) {
		vertsize += 358;
		if (DataStatus::getProjectionString().size() == 0) vertsize -= 66; //no lat/lon coordinates
	}
	if (showAppearance) vertsize += 153;  //Add in appearance panel 
	
	//Mac and Linux have gui elements fatter than windows by about 10%
#ifndef WIN32
	vertsize = (int)(1.1*vertsize);
#endif

	return QSize(460,vertsize);
}