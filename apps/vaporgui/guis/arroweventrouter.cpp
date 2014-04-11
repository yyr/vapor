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

#include "params.h"
#include <vapor/XmlNode.h>
#include "tabmanager.h"
#include "arrowparams.h"
#include "mousemodeparams.h"
#include "arrowrenderer.h"
#include "arroweventrouter.h"
#include "eventrouter.h"
#include "vapor/ControlExecutive.h"


using namespace VAPoR;


ArrowEventRouter::ArrowEventRouter(QWidget* parent): QWidget(parent), Ui_Arrow(), EventRouter(){
        setupUi(this);
	myParamsBaseType = Params::GetTypeFromTag(ArrowParams::_arrowParamsTag);

	showAppearance = false;
	showLayout = false;
}


ArrowEventRouter::~ArrowEventRouter(){
	
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
	connect (heightCombo, SIGNAL(activated(int)),this,SLOT(guiSetHeightVarNum(int)));
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
	connect (barbLengthSlider, SIGNAL(sliderReleased()),this,SLOT(guiReleaseScaleSlider()));

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
	
	Command* cmd = Command::CaptureStart(aParams,"barbs text edit");
	
	
	QString strn;
	//Get all text values from gui, apply to params
	int gridsize[3];
	gridsize[0] = xDimEdit->text().toInt();
	gridsize[1] = yDimEdit->text().toInt();
	gridsize[2] = zDimEdit->text().toInt();
	
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
		}
	}

	float thickness = thicknessEdit->text().toFloat();
	aParams->SetLineThickness((double)thickness);

	double scale = scaleEdit->text().toDouble();
	
	aParams->SetVectorScale(scale);
	
	guiSetTextChanged(false);
	aParams->Validate(false);
	Command::CaptureEnd(cmd,aParams);
	
	updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
	
}
//Save undo/redo state when user grabs a rake handle
//
void ArrowEventRouter::
captureMouseDown(int){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	
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
	
	
	//force render:
	VizWinMgr::getInstance()->forceRender(aParams,true);
	
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
	
	updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
}


void ArrowEventRouter::
populateVariableCombos(bool is3D){
	DataStatus* ds;
	ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	const vector<string>& vars = (is3D ? dataMgr->GetVariables3D() : dataMgr->GetVariables2DXY());
	if (is3D){
		//The first entry is "0"
		xVarCombo->clear();
		xVarCombo->setMaxCount(ds->getNumActiveVariables3D()+1);
		xVarCombo->addItem(QString("0"));
		for (int i = 0; i< ds->getNumActiveVariables3D(); i++){
			const std::string& s = vars[i];
			xVarCombo->addItem(QString::fromStdString(s));
		}
		yVarCombo->clear();
		yVarCombo->setMaxCount(ds->getNumActiveVariables3D()+1);
		yVarCombo->addItem(QString("0"));
		for (int i = 0; i< ds->getNumActiveVariables3D(); i++){
			const std::string& s = vars[i];
			yVarCombo->addItem(QString::fromStdString(s));
		}
		zVarCombo->clear();
		zVarCombo->setMaxCount(ds->getNumActiveVariables3D()+1);
		zVarCombo->addItem(QString("0"));
		for (int i = 0; i< ds->getNumActiveVariables3D(); i++){
			const std::string& s = vars[i];
			zVarCombo->addItem(QString::fromStdString(s));
		}
	} else { //2D vars:
		//The first entry is "0"
		xVarCombo->clear();
		xVarCombo->setMaxCount(ds->getNumActiveVariables2D()+1);
		xVarCombo->addItem(QString("0"));
		for (int i = 0; i< ds->getNumActiveVariables2D(); i++){
			const std::string& s = vars[i];
			xVarCombo->addItem(QString::fromStdString(s));
		}
		yVarCombo->clear();
		yVarCombo->setMaxCount(ds->getNumActiveVariables2D()+1);
		yVarCombo->addItem(QString("0"));
		for (int i = 0; i< ds->getNumActiveVariables2D(); i++){
			const std::string& s = vars[i];
			yVarCombo->addItem(QString::fromStdString(s));
		}
		zVarCombo->clear();
		zVarCombo->setMaxCount(ds->getNumActiveVariables2D()+1);
		zVarCombo->addItem(QString("0"));
		for (int i = 0; i< ds->getNumActiveVariables2D(); i++){
			const std::string& s = vars[i];
			zVarCombo->addItem(QString::fromStdString(s));
		}
	}
	//Populate the height variable combo with all 2d Vars:
	heightCombo->clear();
	heightCombo->setMaxCount(ds->getNumActiveVariables2D());
	for (int i = 0; i< ds->getNumActiveVariables2D(); i++){
		const std::string& s = dataMgr->GetVariables2DXY()[i];
		heightCombo->addItem(QString::fromStdString(s));
	}
}


void ArrowEventRouter::
guiSetXVarNum(int vnum){
	confirmText(true);
	
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	bool is3D = aParams->VariablesAre3D();
	
	if (vnum > 0){
		if (is3D)
			aParams->SetFieldVariableName(0,DataStatus::getInstance()->getDataMgr()->GetVariables3D()[vnum-1]);
		else 
			aParams->SetFieldVariableName(0,DataStatus::getInstance()->getDataMgr()->GetVariables2DXY()[vnum-1]);
	} else aParams->SetFieldVariableName(0,"0");
	updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiSetYVarNum(int vnum){
	
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	bool is3D = aParams->VariablesAre3D();
	
	
	if (vnum > 0){
		if (is3D)
			aParams->SetFieldVariableName(1,DataStatus::getInstance()->getDataMgr()->GetVariables3D()[vnum-1]);
		else 
			aParams->SetFieldVariableName(1,DataStatus::getInstance()->getDataMgr()->GetVariables2DXY()[vnum-1]);
	} else aParams->SetFieldVariableName(1,"0");
	updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiSetZVarNum(int vnum){
	
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	bool is3D = aParams->VariablesAre3D();
	if (vnum > 0){
		if (is3D)
			aParams->SetFieldVariableName(2,DataStatus::getInstance()->getDataMgr()->GetVariables3D()[vnum-1]);
		else 
			aParams->SetFieldVariableName(2,DataStatus::getInstance()->getDataMgr()->GetVariables2DXY()[vnum-1]);
	} else aParams->SetFieldVariableName(2,"0");
	updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiSetHeightVarNum(int vnum){
	
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	
	aParams->SetHeightVariableName(DataStatus::getInstance()->getDataMgr()->GetVariables2DXY()[vnum-1]);
	updateTab();
	
	if (!aParams->IsTerrainMapped()) return;
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiToggleTerrainAlign(bool isOn){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	
	aParams->SetTerrainMapped(isOn);
	
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
	
	qreal rgb[3];
	newColor.getRgbF(rgb,rgb+1,rgb+2);
	double rgbd[3];
	for (int i = 0; i<3; i++) rgbd[i] = rgb[i];
	aParams->SetConstantColor(rgbd);
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiChangeExtents(){
	confirmText(true);
	if (!DataStatus::getInstance()->getDataMgr()) return;
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);
	
	double newExts[6];
	boxSliderFrame->getBoxExtents(newExts);
	Box* bx = aParams->GetBox();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	//convert newExts (in user coords) to local extents, by subtracting time-varying extents origin 
	const vector<double>& tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	for (int i = 0; i<6; i++) newExts[i] -= tvExts[i%3];
	bx->SetLocalExtents(newExts,aParams);
	
	updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
guiAlignToData(bool doAlign){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getInstance()->getApplicableParams(ArrowParams::_arrowParamsTag);

	aParams->AlignGridToData(doAlign);
	xStrideEdit->setEnabled(doAlign);
	yStrideEdit->setEnabled(doAlign);
	
	updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
setArrowEnabled(bool val, int instance){

	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	ArrowParams* dParams = (ArrowParams*)(Params::GetParamsInstance(ArrowParams::_arrowParamsTag, activeViz, instance));
	//Make sure this is a change:
	if (dParams->IsEnabled() == val ) return;
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
	
	//Set up the instance table:
	DataStatus* ds = DataStatus::getInstance();
	if (ds->getDataMgr()) instanceTable->setEnabled(true);
	else instanceTable->setEnabled(false);
	instanceTable->rebuild(this);
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	ArrowParams* arrowParams = (ArrowParams*) VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	int currentTimeStep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	int winnum = vizMgr->getActiveViz();
	int numViz = vizMgr->getNumVisualizers();
	copyCombo->clear();
	copyCombo->addItem("Duplicate In:");
	copyCombo->addItem("This visualizer");
	if (numViz > 1) {
		copyCount.clear();
		for (int i = 0; i<vizMgr->getNumVisualizers(); i++){
			if (vizMgr->getVizWin(i) && winnum != i){
				copyCombo->addItem(vizMgr->getVizWinName(i));
				//Remember the viznum corresponding to a combo item:
				copyCount.push_back(i);
			}
		}
	}
	deleteInstanceButton->setEnabled(vizMgr->getNumInstances(winnum, Params::GetTypeFromTag(ArrowParams::_arrowParamsTag)) > 1);
	if (!DataStatus::getInstance()->getDataMgr()) return;
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
			if(vname != "0") comboIndex[i] = 1+ds->getActiveVarNum3D(vname);
			else comboIndex[i]=0;
		}
	} else {
		for (int i = 0; i<3; i++) {
			string vname = arrowParams->GetFieldVariableName(i);
			if(vname != "0") comboIndex[i] = 1+ds->getActiveVarNum2D(vname);
			else comboIndex[i]=0;
		}
	}
	xVarCombo->setCurrentIndex(comboIndex[0]);
	yVarCombo->setCurrentIndex(comboIndex[1]);
	zVarCombo->setCurrentIndex(comboIndex[2]);

	const string& hname = arrowParams->GetHeightVariableName();
	int hNum = ds->getActiveVarNum2D(hname);
	if (hNum <0) hNum = 0;
	heightCombo->setCurrentIndex(hNum);

	//Set the constant color box
	const vector<double>& clr = arrowParams->GetConstantColor();
	QColor newColor;
	newColor.setRgbF((qreal)clr[0],(qreal)clr[1],(qreal)clr[2]);
	QPalette pal(colorBox->palette());
	pal.setColor(QPalette::Base, newColor);
	
	colorBox->setPalette(pal);
	
	//Set the rake extents
	const float* fullSizes = ds->getFullSizes();
	double fullUsrExts[6];
	for (int i = 0; i<3; i++) {
		fullUsrExts[i] = 0.;
		fullUsrExts[i+3] = fullSizes[i];
	}
	//To get the rake extents in user coordinates, need to get the rake extents and add the user coord domain displacement
	vector<double> usrRakeExts;
	const vector<double>& rakeexts = arrowParams->GetRakeLocalExtents();
	//Now adjust for moving extents
	if (!DataStatus::getInstance()->getDataMgr()) {
		return;
	}
		
	const vector<double>& usrExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)currentTimeStep);
	for (int i = 0; i<6; i++) {
		fullUsrExts[i]+= usrExts[i%3];
		usrRakeExts.push_back(rakeexts[i]+usrExts[i%3]);
	}
	boxSliderFrame->setFullDomain(fullUsrExts);
	boxSliderFrame->setBoxExtents(usrRakeExts);
	boxSliderFrame->setNumRefinements(numRefs);

	

	const vector<long>rakeGrid = arrowParams->GetRakeGrid();
	xDimEdit->setText(QString::number(rakeGrid[0]));
	yDimEdit->setText(QString::number(rakeGrid[1]));
	zDimEdit->setText(QString::number(rakeGrid[2]));

	vector<long> strides = arrowParams->GetGridAlignStrides();
	xStrideEdit->setText(QString::number(strides[0]));
	yStrideEdit->setText(QString::number(strides[1]));
	
	thicknessEdit->setText(QString::number(arrowParams->GetLineThickness()));
	scaleEdit->setText(QString::number(arrowParams->GetVectorScale()));
	//Only allow terrainAlignCheckbox if height variable exists
	
	if (ds->getDataMgr()->VariableExists(currentTimeStep,arrowParams->GetHeightVariableName().c_str())){
		terrainAlignCheckbox->setEnabled(true);
		terrainAlignCheckbox->setChecked(arrowParams->IsTerrainMapped());
	} else {
		terrainAlignCheckbox->setEnabled(false);
		terrainAlignCheckbox->setChecked(false);
		if(arrowParams->IsTerrainMapped())
			arrowParams->SetTerrainMapped(false);
	}
	heightCombo->setEnabled(ds->getNumActiveVariables2D() > 0);
	bool isAligned = arrowParams->IsAlignedToData();
	alignDataCheckbox->setChecked(isAligned);
	xStrideEdit->setEnabled(isAligned);
	yStrideEdit->setEnabled(isAligned);
	xDimEdit->setEnabled(!isAligned);
	yDimEdit->setEnabled(!isAligned);
	
	if (isAligned){
		double exts[6];
		int grdExts[3];
		arrowParams->calcDataAlignment(exts,grdExts,(size_t)currentTimeStep);
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
	
	vizMgr->getTabManager()->update();
}

//Reinitialize Arrow tab settings, session has changed.
//Note that this is called after the globalArrowParams are set up, but before
//any of the localArrowParams are setup.
void ArrowEventRouter::
reinitTab(bool doOverride){
	
	DataStatus* ds = DataStatus::getInstance();
	if (ds->getDataMgr()) {
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
	
	dParams->SetCompressionLevel(num);
	lodCombo->setCurrentIndex(num);
	
	VizWinMgr::getInstance()->forceRender(dParams);
}
void ArrowEventRouter::
guiMoveScaleSlider(int sliderval){
	//just update the 
	confirmText(false);
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	double defaultScale = aParams->calcDefaultScale();
	// find the new length as 10**(sliderVal)*defaultLength
	// note that the slider goes from -100 to +100
	double newVal = defaultScale*pow(10.,(double)sliderval/100.);
	scaleEdit->setText(QString::number(newVal));
	guiSetTextChanged(false); //Don't respond to text-change event
	
	
}
void ArrowEventRouter::
guiReleaseScaleSlider(){
	confirmText(false);
	int sliderval = barbLengthSlider->value();
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	
	
	double defaultScale = aParams->calcDefaultScale();
	// find the new length as 10**(sliderVal)*defaultLength
	// note that the slider goes from -100 to +100
	double newVal = defaultScale*pow(10.,(double)sliderval/100.);
	scaleEdit->setText(QString::number(newVal));
	guiSetTextChanged(false); //Don't respond to text-change event
	aParams->SetVectorScale(newVal);
	
	VizWinMgr::getInstance()->forceRender(aParams);
}

void ArrowEventRouter::
guiSetNumRefinements(int num){
	confirmText(false);
	//make sure we are changing it
	ArrowParams* dParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	if (num == dParams->GetRefinementLevel()) return;

	dParams->SetRefinementLevel(num);
	refinementCombo->setCurrentIndex(num);
	boxSliderFrame->setNumRefinements(num);
	
	VizWinMgr::getInstance()->forceRender(dParams);
}
void ArrowEventRouter::
guiSetEnabled(bool value, int instance, bool undoredo){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = vizWinMgr->getActiveViz();
	//Ignore spurious clicks.
	ArrowParams* dParams = (ArrowParams*)(Params::GetParamsInstance(ArrowParams::_arrowParamsTag, winnum, instance));
	
	if (value == dParams->IsEnabled()) return;
	confirmText(false);
	
	dParams->SetEnabled(value);
	
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(dParams,!value,instance, false);
	
	updateTab();
	vizWinMgr->forceRender(dParams);
	
}
void ArrowEventRouter::
guiFitToData(){
	ArrowParams* aParams = (ArrowParams*)VizWinMgr::getActiveParams(ArrowParams::_arrowParamsTag);
	if (!DataStatus::getInstance()->getDataMgr()) return;
	
		
	const float* fullSizes = DataStatus::getInstance()->getFullSizes();
	Box* box = aParams->GetBox();
	const vector<double> boxExts = box->GetLocalExtents();
	vector<double> newExtents;
	newExtents.push_back(0.);
	newExtents.push_back(0.);
	newExtents.push_back(0.);
	newExtents.push_back(fullSizes[0]);
	newExtents.push_back(fullSizes[1]);
	newExtents.push_back(fullSizes[2]);
	
	box->SetLocalExtents(newExtents,aParams);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>& currExts =DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	boxSliderFrame->setBoxExtents(currExts);
	MouseModeParams::mouseModeType t = MouseModeParams::getCurrentMouseMode();
	VizWinMgr::getInstance()->forceRender(aParams,t == MouseModeParams::barbMode);
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
updateRenderer(RenderParams* rParams, bool prevEnabled,int instance, bool newWindow){
	
	
	ArrowParams* dParams = (ArrowParams*)rParams;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = dParams->GetVizNum();
	
	if (newWindow) {
		prevEnabled = false;
	}
	
	bool nowEnabled = rParams->IsEnabled();
	if (prevEnabled == nowEnabled) return;

	VizWin* viz = 0;
	if(winnum >= 0){//Find the viz that this applies to:
		viz = vizWinMgr->getVizWin(winnum);
	} 
	
	//cases to consider:
	//1.  unchanged disabled renderer; do nothing.
	//  enabled renderer, just refresh:
	ControlExecutive* ce = ControlExecutive::getInstance();
	if (prevEnabled == nowEnabled) {
		if (!prevEnabled) return;
		vizWinMgr->forceRender(rParams, false);
		return;
	}
	
	if (nowEnabled && !prevEnabled ){//For case 2.:  create a renderer in the active window:
		int rc = ce->ActivateRender(winnum,ArrowParams::_arrowParamsTag,instance,true);
		
		//force the renderer to refresh 
		if (!rc) vizWinMgr->forceRender(rParams, true);
	
		return;
	}
	
	assert(prevEnabled && !nowEnabled); //case 6, disable 
	int rc = ce->ActivateRender(winnum,ArrowParams::_arrowParamsTag,instance,false);
	if (!rc) vizWinMgr->forceRender(rParams, true);

	return;
}


