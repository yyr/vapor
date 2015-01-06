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
#include "vizwinparams.h"
#include "arrowrenderer.h"
#include "arroweventrouter.h"
#include "eventrouter.h"
#include "vapor/ControlExecutive.h"


using namespace VAPoR;


ArrowEventRouter::ArrowEventRouter(QWidget* parent): QWidget(parent), Ui_Arrow(), EventRouter(){
        setupUi(this);
	myParamsBaseType = ControlExec::GetTypeFromTag(ArrowParams::_arrowParamsTag);

	showLayout = false;
	
	QTabWidget* myTabWidget = new QTabWidget(this);
	myTabWidget->setTabPosition(QTabWidget::West);
	myAppearance = new ArrowAppearance(myTabWidget);
	myTabWidget->addTab(myAppearance, "Appearance");
	myLayout = new ArrowLayout(myTabWidget);
	myTabWidget->addTab(myLayout,"Layout");
	tabHolderLayout->addWidget(myTabWidget);
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
	
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(setNumRefinements(int)));
	connect (lodCombo,SIGNAL(activated(int)), this, SLOT(setCompRatio(int)));
	
	//Unique connections for ArrowTab:
	//Connect all line edits to textChanged and return pressed: 
	connect (myAppearance->thicknessEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (myAppearance->thicknessEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (myAppearance->scaleEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (myAppearance->scaleEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	
	connect (myLayout->xDimEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (myLayout->xDimEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (myLayout->yDimEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (myLayout->yDimEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (myLayout->zDimEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (myLayout->zDimEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));

	connect (myLayout->xStrideEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (myLayout->xStrideEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	connect (myLayout->yStrideEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setArrowTextChanged(const QString&)));
	connect (myLayout->yStrideEdit, SIGNAL(returnPressed()), this, SLOT(arrowReturnPressed()));
	
	//Connect variable combo boxes to their own slots:
	connect (xVarCombo,SIGNAL(activated(int)), this, SLOT(setXVarNum(int)));
	connect (yVarCombo,SIGNAL(activated(int)), this, SLOT(setYVarNum(int)));
	connect (zVarCombo,SIGNAL(activated(int)), this, SLOT(setZVarNum(int)));
	connect (myLayout->heightCombo, SIGNAL(activated(int)),this,SLOT(setHeightVarNum(int)));
	connect (variableDimCombo, SIGNAL(activated(int)), this, SLOT(setVariableDims(int)));
	//checkboxes
	connect(myLayout->terrainAlignCheckbox,SIGNAL(toggled(bool)), this, SLOT(toggleTerrainAlign(bool)));
	connect(myLayout->alignDataCheckbox,SIGNAL(toggled(bool)),this, SLOT(alignToData(bool)));
	//buttons:
	connect (myAppearance->colorSelectButton, SIGNAL(pressed()), this, SLOT(selectColor()));
	connect (myLayout->boxSliderFrame, SIGNAL(extentsChanged()), this, SLOT(changeExtents()));
	

	connect (myLayout->fitDataButton, SIGNAL(pressed()), this, SLOT(fitToData()));
	//slider:
	connect (myAppearance->barbLengthSlider, SIGNAL(sliderMoved(int)),this,SLOT(moveScaleSlider(int)));
	connect (myAppearance->barbLengthSlider, SIGNAL(sliderReleased()),this,SLOT(releaseScaleSlider()));

}

/*********************************************************************************
 * Slots associated with ArrowTab:
 *********************************************************************************/


void ArrowEventRouter::
setArrowTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void ArrowEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	Command* cmd = Command::CaptureStart(aParams,"barbs text edit");
	
	
	QString strn;
	//Get all text values from gui, apply to params
	int gridsize[3];
	gridsize[0] = myLayout->xDimEdit->text().toInt();
	gridsize[1] = myLayout->yDimEdit->text().toInt();
	gridsize[2] = myLayout->zDimEdit->text().toInt();
	
	aParams->SetRakeGrid(gridsize);
	
	//Check if the strides have changed; if so will need to updateTab.
	if (aParams->IsAlignedToData()){
		const vector<long> currStrides = aParams->GetGridAlignStrides();
		long xstride = (myLayout->xStrideEdit)->text().toInt();
		long ystride = (myLayout->yStrideEdit)->text().toInt();
		if (currStrides[0] != xstride || currStrides[1] != ystride){
			vector<long> strides;
			strides.push_back( xstride);
			strides.push_back( ystride);
			aParams->SetGridAlignStrides(strides);
		}
	}

	float thickness = myAppearance->thicknessEdit->text().toFloat();
	aParams->SetLineThickness((double)thickness);

	double scale = myAppearance->scaleEdit->text().toDouble();
	
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
	int viznum = vwm->getActiveViz();
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetCurrentParams(viznum,ArrowParams::_arrowParamsTag);
	//Update the tab if it's in front:
	if(MainForm::getTabManager()->isFrontTab(this)) {
		if (viznum >= 0)
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
setVariableDims(int is3D){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
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
	myLayout->heightCombo->clear();
	myLayout->heightCombo->setMaxCount(ds->getNumActiveVariables2D());
	for (int i = 0; i< ds->getNumActiveVariables2D(); i++){
		const std::string& s = dataMgr->GetVariables2DXY()[i];
		myLayout->heightCombo->addItem(QString::fromStdString(s));
	}
}


void ArrowEventRouter::
setXVarNum(int vnum){
	confirmText(true);
	
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
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
setYVarNum(int vnum){
	
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
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
setZVarNum(int vnum){
	
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
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
setHeightVarNum(int vnum){
	
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	
	aParams->SetHeightVariableName(DataStatus::getInstance()->getDataMgr()->GetVariables2DXY()[vnum-1]);
	updateTab();
	
	if (!aParams->IsTerrainMapped()) return;
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
toggleTerrainAlign(bool isOn){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	aParams->SetTerrainMapped(isOn);
	
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
selectColor(){
	confirmText(true);
	QPalette pal(myAppearance->colorBox->palette());
	QColor newColor = QColorDialog::getColor(pal.color(QPalette::Base), this);
	if (!newColor.isValid()) return;
	pal.setColor(QPalette::Base, newColor);
	myAppearance->colorBox->setPalette(pal);
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	qreal rgb[3];
	newColor.getRgbF(rgb,rgb+1,rgb+2);
	double rgbd[3];
	for (int i = 0; i<3; i++) rgbd[i] = rgb[i];
	aParams->SetConstantColor(rgbd);
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
changeExtents(){
	confirmText(true);
	if (!DataStatus::getInstance()->getDataMgr()) return;
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	double newExts[6];
	myLayout->boxSliderFrame->getBoxExtents(newExts);
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
alignToData(bool doAlign){
	confirmText(true);
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	aParams->AlignGridToData(doAlign);
	myLayout->xStrideEdit->setEnabled(doAlign);
	myLayout->yStrideEdit->setEnabled(doAlign);
	
	updateTab();
	VizWinMgr::getInstance()->forceRender(aParams);	
}
void ArrowEventRouter::
setArrowEnabled(bool val, int instance){

	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	ArrowParams* dParams = (ArrowParams*)(ControlExec::GetParams(activeViz,ArrowParams::_arrowParamsTag, instance));
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
	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	ArrowParams* arrowParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	int currentTimeStep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	//Find all the visualizers other than the one we are using
	int winnum = vizMgr->getActiveViz();
	vector<long> viznums = VizWinParams::GetVisualizerNums();
	
		
	if (viznums.size() > 1) {
		copyCount.clear();
		//The first two entries should not be referenced
		copyCount.push_back(-1);
		copyCount.push_back(-1);
		for (int i = 0; i< viznums.size(); i++){
			int vizwin = viznums[i];
			if (winnum != vizwin){
				//Remember the viznum corresponding to a combo item:
				copyCount.push_back(vizwin);
			}
		}
	}
	
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
	myLayout->heightCombo->setCurrentIndex(hNum);

	//Set the constant color box
	const vector<double>& clr = arrowParams->GetConstantColor();
	QColor newColor;
	newColor.setRgbF((qreal)clr[0],(qreal)clr[1],(qreal)clr[2]);
	QPalette pal(myAppearance->colorBox->palette());
	pal.setColor(QPalette::Base, newColor);
	
	myAppearance->colorBox->setPalette(pal);
	
	//Set the rake extents
	const double* fullSizes = ds->getFullSizes();
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
	myLayout->boxSliderFrame->setFullDomain(fullUsrExts);
	myLayout->boxSliderFrame->setBoxExtents(usrRakeExts);
	myLayout->boxSliderFrame->setNumRefinements(numRefs);

	

	const vector<long>rakeGrid = arrowParams->GetRakeGrid();
	myLayout->xDimEdit->setText(QString::number(rakeGrid[0]));
	myLayout->yDimEdit->setText(QString::number(rakeGrid[1]));
	myLayout->zDimEdit->setText(QString::number(rakeGrid[2]));

	vector<long> strides = arrowParams->GetGridAlignStrides();
	myLayout->xStrideEdit->setText(QString::number(strides[0]));
	myLayout->yStrideEdit->setText(QString::number(strides[1]));
	
	myAppearance->thicknessEdit->setText(QString::number(arrowParams->GetLineThickness()));
	myAppearance->scaleEdit->setText(QString::number(arrowParams->GetVectorScale()));
	//Only allow terrainAlignCheckbox if height variable exists
	
	if (ds->getDataMgr()->VariableExists(currentTimeStep,arrowParams->GetHeightVariableName().c_str())){
		myLayout->terrainAlignCheckbox->setEnabled(true);
		myLayout->terrainAlignCheckbox->setChecked(arrowParams->IsTerrainMapped());
	} else {
		myLayout->terrainAlignCheckbox->setEnabled(false);
		myLayout->terrainAlignCheckbox->setChecked(false);
		if(arrowParams->IsTerrainMapped())
			arrowParams->SetTerrainMapped(false);
	}
	myLayout->heightCombo->setEnabled(ds->getNumActiveVariables2D() > 0);
	bool isAligned = arrowParams->IsAlignedToData();
	myLayout->alignDataCheckbox->setChecked(isAligned);
	myLayout->xStrideEdit->setEnabled(isAligned);
	myLayout->yStrideEdit->setEnabled(isAligned);
	myLayout->xDimEdit->setEnabled(!isAligned);
	myLayout->yDimEdit->setEnabled(!isAligned);
	
	if (isAligned){
		double exts[6];
		int grdExts[3];
		arrowParams->getDataAlignment(exts,grdExts,(size_t)currentTimeStep);
		//Display the new aligned grid extents
		myLayout->xDimEdit->setText(QString::number(grdExts[0]));
		myLayout->yDimEdit->setText(QString::number(grdExts[1]));
	}
	
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
	string tag = ParamsBase::GetTagFromType(myParamsBaseType);
	if(ControlExec::GetActiveParams(tag))updateTab();
}

void ArrowEventRouter::
setCompRatio(int num){
	confirmText(false);
	//make sure we are changing it
	ArrowParams* dParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	if (num == dParams->GetCompressionLevel()) return;
	
	dParams->SetCompressionLevel(num);
	lodCombo->setCurrentIndex(num);
	
	VizWinMgr::getInstance()->forceRender(dParams);
}
void ArrowEventRouter::
moveScaleSlider(int sliderval){
	//just update the 
	confirmText(false);
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	double defaultScale = aParams->calcDefaultScale();
	// find the new length as 10**(sliderVal)*defaultLength
	// note that the slider goes from -100 to +100
	double newVal = defaultScale*pow(10.,(double)sliderval/100.);
	myAppearance->scaleEdit->setText(QString::number(newVal));
	guiSetTextChanged(false); //Don't respond to text-change event
	
	
}
void ArrowEventRouter::
releaseScaleSlider(){
	confirmText(false);
	int sliderval = myAppearance->barbLengthSlider->value();
	ArrowParams* aParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	
	
	double defaultScale = aParams->calcDefaultScale();
	// find the new length as 10**(sliderVal)*defaultLength
	// note that the slider goes from -100 to +100
	double newVal = defaultScale*pow(10.,(double)sliderval/100.);
	myAppearance->scaleEdit->setText(QString::number(newVal));
	guiSetTextChanged(false); //Don't respond to text-change event
	aParams->SetVectorScale(newVal);
	
	VizWinMgr::getInstance()->forceRender(aParams);
}

void ArrowEventRouter::
setNumRefinements(int num){
	confirmText(false);
	//make sure we are changing it
	ArrowParams* dParams = (ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	if (num == dParams->GetRefinementLevel()) return;

	dParams->SetRefinementLevel(num);
	refinementCombo->setCurrentIndex(num);
	myLayout->boxSliderFrame->setNumRefinements(num);
	
	VizWinMgr::getInstance()->forceRender(dParams);
}
void ArrowEventRouter::
guiSetEnabled(bool value, int instance, bool undoredo){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = vizWinMgr->getActiveViz();
	//Ignore spurious clicks.
	ArrowParams* dParams = (ArrowParams*)(ControlExec::GetParams(winnum,ArrowParams::_arrowParamsTag, instance));
	
	if (value == dParams->IsEnabled()) return;
	confirmText(false);
	string commandText;
	if (value) commandText = "enable rendering";
	else commandText = "disable rendering";
	Command* cmd = Command::CaptureStart(dParams, commandText.c_str(), Renderer::UndoRedo);
	dParams->SetEnabled(value);
	Command::CaptureEnd(cmd,dParams);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(dParams,!value,instance, false);
	
	updateTab();
	vizWinMgr->forceRender(dParams);
	
}
void ArrowEventRouter::
fitToData(){
	ArrowParams* aParams =(ArrowParams*)ControlExec::GetActiveParams(ArrowParams::_arrowParamsTag);
	if (!DataStatus::getInstance()->getDataMgr()) return;
	
		
	const double* fullSizes = DataStatus::getInstance()->getFullSizes();
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
	myLayout->boxSliderFrame->setBoxExtents(currExts);
	MouseModeParams::mouseModeType t = MouseModeParams::GetCurrentMouseMode();
	VizWinMgr::getInstance()->forceRender(aParams,t == MouseModeParams::barbMode);
}






