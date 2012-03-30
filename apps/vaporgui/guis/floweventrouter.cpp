//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		floweventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2006
//
//	Description:	Implements the FlowEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the flow tab
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 4996)
#endif
#include <QScrollArea>
#include <QScrollBar>
#include <algorithm>
#include <qlayout.h>
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
#include <qtooltip.h>
#include "regionparams.h"
#include "instancetable.h"
#include "mappingframe.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "flowrenderer.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "seedlisteditor.h"
#include "helpwindow.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qlabel.h>
#include <QTableWidget>
#include <QFileDialog>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "flowtab.h"
#include <vapor/XmlNode.h>
#include <vapor/flowlinedata.h>
#include <vapor/VaporFlow.h>

#include "tabmanager.h"
#include "glutil.h"
#include "flowparams.h"
#include "floweventrouter.h"
#include "eventrouter.h"


#include "VolumeRenderer.h"

using namespace VAPoR;


FlowEventRouter::FlowEventRouter(QWidget* parent): QWidget(parent), Ui_FlowTab(), EventRouter(){
	setupUi(this);
	myParamsBaseType = Params::GetTypeFromTag(Params::_flowParamsTag);
	savedCommand = 0;
	flowDataChanged = false;
	mapBoundsChanged = false;
	flowGraphicsChanged = false;
	showSeeding = false;
	showAppearance=true;
	showUnsteadyTime=false;
	MessageReporter::infoMsg("FlowEventRouter::FlowEventRouter()");
	dontUpdate=false;
#ifdef Darwin
	colorMapShown=false;
	opacityMapShown = false;
	colorMappingFrame->hide();
	opacityMappingFrame->hide();
#endif
}


FlowEventRouter::~FlowEventRouter(){
	if (savedCommand) delete savedCommand;
}
/**********************************************************
 * Whenever a new Flowtab is created it must be hooked up here
 ************************************************************/
void
FlowEventRouter::hookUpTab()
{
	//Connect up the sampleTable events:
	connect (addSampleButton1,SIGNAL(clicked()), this, SLOT(addSample()));
	
	connect (flowHelpButton, SIGNAL(clicked()), this, SLOT(showSetupHelp()));
	connect (deleteSampleButton1,SIGNAL(clicked()), this, SLOT(deleteSample()));
	connect (rebuildButton1, SIGNAL(clicked()), this, SLOT(guiRebuildList()));
	
	connect (timestepSampleTable1, SIGNAL(cellChanged(int,int)), this, SLOT(timestepChanged1(int,int)));
	
	connect (timestepSampleCheckbox1, SIGNAL(toggled(bool)), this, SLOT(guiToggleTimestepSample(bool)));
	
	connect (stopButton, SIGNAL(clicked()), this, SLOT(stopClicked()));
	
	connect (autoScaleCheckbox1, SIGNAL(toggled(bool)), this, SLOT(guiToggleAutoScale(bool)));
	connect (displayListCheckbox,SIGNAL(toggled(bool)),this,SLOT(guiToggleDisplayLists(bool)));
	
	connect (flaOptionCombo, SIGNAL(activated(int)), this, SLOT(guiSetFLAOption(int)));
	connect (flowTypeCombo, SIGNAL( activated(int) ), this, SLOT( guiSetFlowType(int) ) );
	connect (steadyDirectionCombo, SIGNAL( activated(int) ), this, SLOT( guiSetSteadyDirection(int) ) );
	connect (unsteadyDirectionCombo, SIGNAL( activated(int) ), this, SLOT( guiSetUnsteadyDirection(int) ) );
	
	connect (autoRefreshCheckbox, SIGNAL(toggled(bool)), this, SLOT (guiSetAutoRefresh(bool)));
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (lodCombo,SIGNAL(activated(int)), this, SLOT(guiSetCompRatio(int)));
	connect (xSteadyVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetXComboSteadyVarNum(int)));
	connect (ySteadyVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetYComboSteadyVarNum(int)));
	connect (zSteadyVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetZComboSteadyVarNum(int)));
	connect (xUnsteadyVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetXComboUnsteadyVarNum(int)));
	connect (yUnsteadyVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetYComboUnsteadyVarNum(int)));
	connect (zUnsteadyVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetZComboUnsteadyVarNum(int)));
	connect (xSeedDistCombo,SIGNAL(activated(int)), this, SLOT(guiSetXComboSeedDistVarNum(int)));
	connect (ySeedDistCombo,SIGNAL(activated(int)), this, SLOT(guiSetYComboSeedDistVarNum(int)));
	connect (zSeedDistCombo,SIGNAL(activated(int)), this, SLOT(guiSetZComboSeedDistVarNum(int)));
	
	connect (xSeedPriorityCombo,SIGNAL(activated(int)), this, SLOT(guiSetXComboPriorityVarNum(int)));
	connect (ySeedPriorityCombo,SIGNAL(activated(int)), this, SLOT(guiSetYComboPriorityVarNum(int)));
	connect (zSeedPriorityCombo,SIGNAL(activated(int)), this, SLOT(guiSetZComboPriorityVarNum(int)));
	
	connect (rakeListCombo,SIGNAL(activated(int)),this, SLOT(guiSetRakeList(int)));
	connect (xCenterSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetXCenter(int)));
	connect (yCenterSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetYCenter(int)));
	connect (zCenterSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetZCenter(int)));
	
	connect (xSizeSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetXSize(int)));
	connect (ySizeSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetYSize(int)));
	connect (zSizeSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetZSize(int)));

	connect (biasSlider1, SIGNAL(valueChanged(int)), this, SLOT(setBiasFromSlider1(int)));
	
	connect (steadyLengthSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetSteadyLength(int)));
	connect (smoothnessSlider, SIGNAL(valueChanged(int)), this, SLOT(guiSetSmoothness(int)));
	connect (steadySamplesSlider1, SIGNAL(valueChanged(int)), this, SLOT(guiSetSteadySamples(int)));
	
	
	connect (unsteadySamplesSlider, SIGNAL(valueChanged(int)), this, SLOT(guiSetUnsteadySamples(int)));
	
	connect (geometryCombo, SIGNAL(activated(int)),SLOT(guiSetFlowGeometry(int)));
	connect (constantColorButton, SIGNAL(clicked()), this, SLOT(setFlowConstantColor()));
	connect (rakeOnRegionButton, SIGNAL(clicked()), this, SLOT(guiSetRakeToRegion()));
	connect (colormapEntityCombo,SIGNAL(activated(int)),SLOT(guiSetColorMapEntity(int)));
	connect (opacmapEntityCombo,SIGNAL(activated(int)),SLOT(guiSetOpacMapEntity(int)));
	connect (refreshButton,SIGNAL(clicked()), this, SLOT(refreshFlow()));
	connect (periodicXCheck,SIGNAL(toggled(bool)), this, SLOT(guiCheckPeriodicX(bool)));
	connect (periodicYCheck,SIGNAL(toggled(bool)), this, SLOT(guiCheckPeriodicY(bool)));
	connect (periodicZCheck,SIGNAL(toggled(bool)), this, SLOT(guiCheckPeriodicZ(bool)));
	//Line edits.  Note that the textChanged may either affect the flow or the geometry
	
	connect (smoothnessSamplesEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (smoothnessSamplesEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (steadyLengthEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (steadyLengthEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (unsteadySamplesEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (unsteadySamplesEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (steadySamplesEdit1,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (steadySamplesEdit1,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	
	connect (flaSamplesEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flaSamplesEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (xSeedEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (xSeedEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (ySeedEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (ySeedEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (zSeedEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (zSeedEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (constantOpacityEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (constantOpacityEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabGraphicsTextChanged(const QString&)));
	connect (integrationAccuracyEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (integrationAccuracyEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
		
	connect (steadyScaleEdit1,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (steadyScaleEdit1,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	
	connect (unsteadyScaleEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (unsteadyScaleEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	
	connect (timesampleStartEdit1,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (timesampleStartEdit1,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (timesampleEndEdit1,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (timesampleEndEdit1,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (timesampleIncrementEdit1,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (timesampleIncrementEdit1,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	
	connect (priorityFieldMinEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (priorityFieldMinEdit, SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (priorityFieldMaxEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (priorityFieldMaxEdit, SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (biasEdit1, SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (biasEdit1, SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	
	
	connect (randomSeedEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (randomSeedEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (xCenterEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (xCenterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (yCenterEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (yCenterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (zCenterEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (zCenterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (xSizeEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (xSizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (ySizeEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (ySizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (zSizeEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (zSizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (generatorCountEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (generatorCountEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (seedtimeStartEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (seedtimeStartEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (seedtimeEndEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (seedtimeEndEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (seedtimeIncrementEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (seedtimeIncrementEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	
	connect (firstDisplayFrameEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (firstDisplayFrameEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabGraphicsTextChanged(const QString&)));
	connect (lastDisplayFrameEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (lastDisplayFrameEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabGraphicsTextChanged(const QString&)));
	connect (diameterEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (diameterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabGraphicsTextChanged(const QString&)));
	connect (diamondSizeEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (diamondSizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabGraphicsTextChanged(const QString&)));
	
	connect (arrowheadEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (arrowheadEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabGraphicsTextChanged(const QString&)));
	connect (minColormapEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (minColormapEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabRangeTextChanged(const QString&)));
	connect (maxColormapEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (maxColormapEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabRangeTextChanged(const QString&)));
	connect (minOpacmapEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (minOpacmapEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabRangeTextChanged(const QString&)));
	connect (maxOpacmapEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (maxOpacmapEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabRangeTextChanged(const QString&)));
	
	connect (opacityScaleSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetOpacityScale(int)));
	connect (navigateButton, SIGNAL(toggled(bool)), this, SLOT(setFlowNavigateMode(bool)));
	connect (editButton, SIGNAL(toggled(bool)), this, SLOT(setFlowEditMode(bool)));

	connect (seedListLoadButton, SIGNAL(clicked()), this, SLOT(guiLoadSeeds()));
	connect (saveSeedsButton, SIGNAL(clicked()), this, SLOT(saveSeeds()));
	connect (saveFlowButton, SIGNAL(clicked()), this, SLOT(saveFlowLines()));
	connect (seedListEditButton, SIGNAL(clicked()), this, SLOT(guiEditSeedList()));

	connect(editButton, SIGNAL(toggled(bool)), 
            opacityMappingFrame, SLOT(setEditMode(bool)));
	connect(alignButton, SIGNAL(clicked()),
            opacityMappingFrame, SLOT(fitToView()));
    connect(opacityMappingFrame, SIGNAL(startChange(QString)), 
            this, SLOT(guiStartChangeMapFcn(QString)));
    connect(opacityMappingFrame, SIGNAL(endChange()),
            this, SLOT(guiEndChangeMapFcn()));

	connect(editButton, SIGNAL(toggled(bool)), 
            colorMappingFrame, SLOT(setEditMode(bool)));
	connect(alignButton, SIGNAL(clicked()),
            colorMappingFrame, SLOT(fitToView()));
    connect(colorMappingFrame, SIGNAL(startChange(QString)), 
            this, SLOT(guiStartChangeMapFcn(QString)));
    connect(colorMappingFrame, SIGNAL(endChange()),
            this, SLOT(guiEndChangeMapFcn()));

	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setFlowEnabled(bool,int)));
	connect (showHideSeedingButton, SIGNAL(pressed()), this, SLOT(showHideSeeding()));
	connect (showHideTimeButton, SIGNAL(pressed()), this, SLOT(showHideUnsteadyTime()));
	connect (showHideAppearanceButton, SIGNAL(pressed()), this, SLOT(showHideAppearance()));
	dontUpdate=false;
}

//Insert values from params into tab panel.
//This is called whenever the tab is displayed
//
void FlowEventRouter::updateTab(){
	if(!MainForm::getTabManager()->isFrontTab(this)) return;
	if (!isEnabled()) return;
	if (GLWindow::isRendering())return;
	DataStatus* dStatus = DataStatus::getInstance();

	if (dStatus->getDataMgr()) instanceTable->setEnabled(true);
	else instanceTable->setEnabled(false);
	instanceTable->rebuild(this);

	FlowParams* fParams = (FlowParams*) VizWinMgr::getActiveFlowParams();
	int flowType = fParams->getFlowType();
	bool autoScale = fParams->isAutoScale();
	Session::getInstance()->blockRecording();

	stopButton->setEnabled( flowType != 0 && fParams->isEnabled());
	lodCombo->setCurrentIndex(fParams->GetCompressionLevel());
	displayListCheckbox->setChecked(fParams->usingDisplayLists());

	
	switch (flowType){
		case (0) : //steady
			steadyFieldFrame->show();
			if (showAppearance) {
				if (autoScale) {
					autoscaleOffFrame->hide();
					autoscaleOnFrame->show();
				} else {
					autoscaleOnFrame->hide();
					autoscaleOffFrame->show();
				}
				steadyAppearanceFrame->show();
			}
			unsteadyFieldFrame->hide();
			fullUnsteadyTimeFrame->hide();
			advancedLineAdvectionFrame->hide();
			
			colormapEntityCombo->setItemText(1,"Position on Flow");
			opacmapEntityCombo->setItemText(1,"Position on Flow");
			break;
		case (1) : //unsteady
			steadyFieldFrame->hide();
			steadyAppearanceFrame->hide();
			unsteadyFieldFrame->show();
			fullUnsteadyTimeFrame->show();
			advancedLineAdvectionFrame->hide();
			flowHelpButton->setText("Unsteady Flow Setup Help");
		
			seedtimeIncrementEdit->setEnabled(true);
			seedtimeEndEdit->setEnabled(true);
		
			
			colormapEntityCombo->setItemText(1,"Time Step");
			opacmapEntityCombo->setItemText(1,"Time Step");
			break;
		case(2) : //field line advection
			advancedLineAdvectionFrame->show();
			steadyFieldFrame->show();
			if (showAppearance) {
				if (autoScale) {
					autoscaleOffFrame->hide();
					autoscaleOnFrame->show();
				} else {
					autoscaleOnFrame->hide();
					autoscaleOffFrame->show();
				}
				steadyAppearanceFrame->show();
			}
			unsteadyFieldFrame->show();
			fullUnsteadyTimeFrame->show();
			
			flowHelpButton->setText("Field Line Advection Setup Help");

			
			seedtimeIncrementEdit->setEnabled(false);
			seedtimeEndEdit->setEnabled(false);
			
			colormapEntityCombo->setItemText(1,"Position on Flow");
			opacmapEntityCombo->setItemText(1,"Position on Flow");
			break;
		default :
			assert(0);
	}
	

	
	switch (flowType){
		case (0) : //steady

			
			autoScaleCheckbox1->setChecked(autoScale);
			steadyScaleEdit1->setText(QString::number(fParams->getSteadyScale()));
			
			xSeedDistCombo->setCurrentIndex(fParams->getComboSeedDistVarnum(0));
			ySeedDistCombo->setCurrentIndex(fParams->getComboSeedDistVarnum(1));
			zSeedDistCombo->setCurrentIndex(fParams->getComboSeedDistVarnum(2));
			
			break;
		case (1) : //unsteady
			
			//Set up the timestep sample table:
			timestepSampleTable1->horizontalHeader()->hide();
			timestepSampleTable1->setSelectionMode(QAbstractItemView::SingleSelection);
			timestepSampleTable1->setSelectionBehavior(QAbstractItemView::SelectRows);
			timestepSampleTable1->setColumnWidth(0,80);
			timesampleIncrementEdit1->setText(QString::number(fParams->getTimeSamplingInterval()));
			timesampleStartEdit1->setText(QString::number(fParams->getTimeSamplingStart()));
			timesampleEndEdit1->setText(QString::number(fParams->getTimeSamplingEnd()));
			populateTimestepTables();
			
			guiSetTextChanged(false);
			xSeedDistCombo->setCurrentIndex(fParams->getComboSeedDistVarnum(0));
			ySeedDistCombo->setCurrentIndex(fParams->getComboSeedDistVarnum(1));
			zSeedDistCombo->setCurrentIndex(fParams->getComboSeedDistVarnum(2));
			
			break;
		case(2) : //field line advection
			
			//Set up the timestep sample table:
			timestepSampleTable1->horizontalHeader()->hide();
			timestepSampleTable1->setSelectionMode(QAbstractItemView::SingleSelection);
			timestepSampleTable1->setSelectionBehavior(QAbstractItemView::SelectRows);

			timestepSampleTable1->setColumnWidth(0,80);
			{
				int advectBeforePriorOption = 
					fParams->getFLAAdvectBeforePrioritize() ? 1 : 0 ;
				flaOptionCombo->setCurrentIndex(advectBeforePriorOption);
				int numSamps = (advectBeforePriorOption == 0) ? 1 : fParams->getNumFLASamples() ;
				flaSamplesEdit->setEnabled(advectBeforePriorOption == 1);
				priorityFieldMinEdit->setEnabled(advectBeforePriorOption != 1);
				priorityFieldMaxEdit->setEnabled(advectBeforePriorOption != 1);
				flaSamplesEdit->setText(QString::number(numSamps));
			}
			timesampleIncrementEdit1->setText(QString::number(fParams->getTimeSamplingInterval()));
			timesampleStartEdit1->setText(QString::number(fParams->getTimeSamplingStart()));
			timesampleEndEdit1->setText(QString::number(fParams->getTimeSamplingEnd()));
			autoScaleCheckbox1->setChecked(autoScale);
			steadyScaleEdit1->setText(QString::number(fParams->getSteadyScale()));
		
			populateTimestepTables();
			
			xSeedPriorityCombo->setCurrentIndex(fParams->getComboPriorityVarnum(0));
			ySeedPriorityCombo->setCurrentIndex(fParams->getComboPriorityVarnum(1));
			zSeedPriorityCombo->setCurrentIndex(fParams->getComboPriorityVarnum(2));
			
			priorityFieldMinEdit->setText(QString::number(fParams->getPriorityMin()));
			priorityFieldMaxEdit->setText(QString::number(fParams->getPriorityMax()));
			
			
			guiSetTextChanged(false);
			
			break;
		default :
			assert(0);
	}

	float biasVal = fParams->getSeedDistBias();
	biasEdit1->setText(QString::number(biasVal));
	guiSetTextChanged(false);
	float bval = biasSlider1->value();
	if (bval != (int)(biasVal*128.f/15.f)){
		biasSlider1->setTracking(false);
		biasSlider1->setSliderPosition((int)(biasVal*128.f/15.f));
		biasSlider1->setTracking(true);
	}
			
	
	if (showAppearance) {
		if (flowType != 1) steadyAppearanceFrame->show();
		else steadyAppearanceFrame->hide();
		appearanceFrame->show();
	}
	else appearanceFrame->hide();
	if (showSeeding) seedingFrame->show();
	else seedingFrame->hide();
	if (showUnsteadyTime && flowType>0) unsteadyTimeFrame->show();
	else unsteadyTimeFrame->hide();
	adjustSize();

	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int numViz = vizMgr->getNumVisualizers();
	copyCombo->clear();
	copyCombo->addItem("Duplicate In:");
	copyCombo->addItem("This visualizer");

	//Set up the timestep sample table:
	timestepSampleTable1->horizontalHeader()->hide();
	timestepSampleTable1->setSelectionMode(QAbstractItemView::SingleSelection);
	timestepSampleTable1->setSelectionBehavior(QAbstractItemView::SelectRows);
	timestepSampleTable1->setColumnWidth(0,80);
	
	populateTimestepTables();

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
	if (fParams->GetMapperFunc()){
		fParams->GetMapperFunc()->setParams(fParams);
		opacityMappingFrame->setMapperFunction(fParams->GetMapperFunc());
		colorMappingFrame->setMapperFunction(fParams->GetMapperFunc());
	}

    opacityMappingFrame->setVariableName(opacmapEntityCombo->currentText().toStdString());
    colorMappingFrame->setVariableName(colormapEntityCombo->currentText().toStdString());

	
	
	deleteInstanceButton->setEnabled(Params::GetNumParamsInstances(Params::_flowParamsTag,winnum)>1);
	

	//Get the region corners from the current applicable region panel,
	//or the global one:
	
	
	
	flowTypeCombo->setCurrentIndex(flowType);
	int numRefs = fParams->GetRefinementLevel();
	if(numRefs <= refinementCombo->count())
		refinementCombo->setCurrentIndex(numRefs);

	float sliderVal = fParams->getOpacityScale();
	opacityScaleSlider->setToolTip("Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	sliderVal = 256.f*(1.f -sliderVal);
	if (opacityScaleSlider->value() != (int) sliderVal)
		opacityScaleSlider->setValue((int) sliderVal);
	
	//Determine if the renderer dirty flag is set:

	refreshButton->setEnabled(!fParams->refreshIsAuto() && VizWinMgr::getInstance()->flowDataIsDirty(fParams));
	autoRefreshCheckbox->setChecked(fParams->refreshIsAuto());
	QPalette pal(autoRefreshCheckbox->palette());
	if (fParams->refreshIsAuto()) {
		pal.setColor(autoRefreshCheckbox->foregroundRole(),QColor(Qt::black));
		autoRefreshCheckbox->setPalette(pal);
	}
	else {
		pal.setColor(autoRefreshCheckbox->foregroundRole(),QColor(Qt::red));
		autoRefreshCheckbox->setPalette(pal);
	}
	//Always allow at least 4 variables in combo:
	int numVars = fParams->getNumComboVariables();
	if (numVars < 4) numVars = 4;
	

	xSteadyVarCombo->setCurrentIndex(fParams->getComboSteadyVarnum(0));
	ySteadyVarCombo->setCurrentIndex(fParams->getComboSteadyVarnum(1));
	zSteadyVarCombo->setCurrentIndex(fParams->getComboSteadyVarnum(2));

	xUnsteadyVarCombo->setCurrentIndex(fParams->getComboUnsteadyVarnum(0));
	yUnsteadyVarCombo->setCurrentIndex(fParams->getComboUnsteadyVarnum(1));
	zUnsteadyVarCombo->setCurrentIndex(fParams->getComboUnsteadyVarnum(2));
	
	periodicXCheck->setChecked(fParams->getPeriodicDim(0));
	periodicYCheck->setChecked(fParams->getPeriodicDim(1));
	periodicZCheck->setChecked(fParams->getPeriodicDim(2));
	
	int comboSetting = 0;  //Using seed list
	if (fParams->rakeEnabled()) {
		comboSetting = 1;
		if (fParams->isRandom()) comboSetting = 2;
	}
	rakeListCombo->setCurrentIndex(comboSetting);
	
	randomSeedEdit->setEnabled(fParams->isRandom()&&fParams->rakeEnabled());

	float seedBoxMin[3],seedBoxMax[3];
	fParams->getBox(seedBoxMin,seedBoxMax, -1);
	
	for (int i = 0; i< 3; i++){
		textToSlider(fParams, i, (seedBoxMin[i]+seedBoxMax[i])*0.5f,
			seedBoxMax[i]-seedBoxMin[i]);
	}

	//Geometric parameters:
	geometryCombo->setCurrentIndex(fParams->getShapeType());
	
	colormapEntityCombo->setCurrentIndex(fParams->getColorMapEntityIndex());
	opacmapEntityCombo->setCurrentIndex(fParams->getOpacMapEntityIndex());

    // Disable the mapping frame if a "Constant" color is selected;
    opacityMappingFrame->setEnabled(fParams->getOpacMapEntityIndex() != 0);
    colorMappingFrame->setEnabled(fParams->getColorMapEntityIndex() != 0);

	if (fParams->isRandom() && fParams->rakeEnabled()){//random rake
		dimensionLabel->setEnabled(false);
		generatorCountEdit->setEnabled(true);
		xSeedEdit->setEnabled(false);
		ySeedEdit->setEnabled(false);
		zSeedEdit->setEnabled(false);
	} else if (fParams->rakeEnabled()) {//nonrandom rake
		dimensionLabel->setEnabled(true);
		generatorCountEdit->setEnabled(false);
		xSeedEdit->setEnabled(true);
		ySeedEdit->setEnabled(true);
		zSeedEdit->setEnabled(true);
	} else {//seed list
		dimensionLabel->setEnabled(false);
		generatorCountEdit->setEnabled(false);
		xSeedEdit->setEnabled(false);
		ySeedEdit->setEnabled(false);
		zSeedEdit->setEnabled(false);
	} 
	if (autoScale){
		float sampleRate = fParams->getSteadySmoothness();
		int sval = (int)(0.5f+256.0*(log10((float)sampleRate)+1.f)*0.25f);
		if (smoothnessSlider->value() != sval)
			smoothnessSlider->setValue(sval);
		float flowlen = fParams->getSteadyFlowLength();
		sval = (int)(0.5f+256.0*(log10((float)flowlen)+2.f)*0.25f);
		if (sval != steadyLengthSlider->value())
			steadyLengthSlider->setValue(sval);
	}
	if(flowType != 0) {
		if (unsteadySamplesSlider->value() != (int)fParams->getObjectsPerTimestep())
			unsteadySamplesSlider->setValue((int)fParams->getObjectsPerTimestep());
	} 
	if (flowType == 0) {
		int sval = (int)(0.5f+ 256.0*log10((float)fParams->getObjectsPerFlowline()/2.f)*0.33333);
		if (steadySamplesSlider1->value() != sval)
			steadySamplesSlider1->setValue(sval);
	
	}
	if (flowType == 2) {
		int sval = (int)(0.5f+256.0*log10((float)fParams->getObjectsPerFlowline()/2.f)*0.33333);
		if (steadySamplesSlider1->value() != sval)
			steadySamplesSlider1->setValue(sval);
		//Set the combo to display "As Needed" and be disabled.
		unsteadyDirectionCombo->setItemText(0,QString("As Needed"));
		unsteadyDirectionCombo->setCurrentIndex(0);
		unsteadyDirectionCombo->setEnabled(false);
	}
	
	//Put the rest of the setText messages here, will inhibit textChanged event
	if (flowType != 1) {
		steadyLengthEdit->setText(QString::number(fParams->getSteadyFlowLength()));
		steadyDirectionCombo->setCurrentIndex(fParams->getSteadyDirection()+1);
		steadyScaleEdit1->setText(QString::number(fParams->getSteadyScale()));
		
	} else {//set the combo to just show forward & backward and be enabled
		//dir = -1 results in combo position 1
		unsteadyDirectionCombo->setItemText(0,QString("Forward"));
		unsteadyDirectionCombo->setCurrentIndex((1-fParams->getUnsteadyDirection())/2);
		unsteadyDirectionCombo->setMaxCount(2);
		unsteadyDirectionCombo->setEnabled(true);
	}
	if (flowType != 0) {
		unsteadyScaleEdit->setText(QString::number(fParams->getUnsteadyScale()));
		
	}
	integrationAccuracyEdit->setText(QString::number(fParams->getIntegrationAccuracy()));

	if (fParams->rakeEnabled())
		generatorCountEdit->setText(QString::number(fParams->getNumRakeSeedPoints()));
	else 
		generatorCountEdit->setText(QString::number(fParams->getNumListSeedPoints()));
	
	xSeedEdit->setText(QString::number(fParams->getNumGenerators(0)));
	ySeedEdit->setText(QString::number(fParams->getNumGenerators(1)));
	zSeedEdit->setText(QString::number(fParams->getNumGenerators(2)));

	unsteadyScaleEdit->setText(QString::number(fParams->getUnsteadyScale()));
	
	
		
	randomSeedEdit->setText(QString::number(fParams->getRandomSeed()));

	if (autoScale){
		float sampleRate = fParams->getSteadySmoothness();
		int sval = (int)(0.5f+256.0*(log10((float)sampleRate)+1.f)*0.25f);
		if (smoothnessSlider->value()!= sval)
			smoothnessSlider->setValue(sval);
		float flowlen = fParams->getSteadyFlowLength();
		sval = (int)(0.5f+256.0*(log10((float)flowlen)+2.f)*0.25f);
		if (steadyLengthSlider->value()!= sval)
			steadyLengthSlider->setValue(sval);
	}
	if(flowType != 0) {
		unsteadySamplesEdit->setText(QString::number(fParams->getObjectsPerTimestep()));
	} 
	if (flowType == 0) {
		steadySamplesEdit1->setText(QString::number(fParams->getObjectsPerFlowline()));
		smoothnessSamplesEdit->setText(QString::number(fParams->getSteadySmoothness()));
		steadyLengthEdit->setText(QString::number(fParams->getSteadyFlowLength()));
	}
	if (flowType == 2) {
		steadySamplesEdit1->setText(QString::number(fParams->getObjectsPerFlowline()));
		smoothnessSamplesEdit->setText(QString::number(fParams->getSteadySmoothness()));
		steadyLengthEdit->setText(QString::number(fParams->getSteadyFlowLength()));
	}
	
	firstDisplayFrameEdit->setText(QString::number(fParams->getFirstDisplayFrame()));
	lastDisplayFrameEdit->setText(QString::number(fParams->getLastDisplayFrame()));
	diameterEdit->setText(QString::number(fParams->getShapeDiameter()));
	diamondSizeEdit->setText(QString::number(fParams->getDiamondDiameter()));
	arrowheadEdit->setText(QString::number(fParams->getArrowDiameter()));
	constantOpacityEdit->setText(QString::number(fParams->getConstantOpacity()));
	pal.setColor(constantColorEdit->backgroundRole(),QColor(fParams->getConstantColor()));
	constantColorEdit->setPalette(pal);
	if (fParams->GetMapperFunc()){
		minColormapEdit->setText(QString::number(fParams->GetMapperFunc()->getMinColorMapValue()));
		maxColormapEdit->setText(QString::number(fParams->GetMapperFunc()->getMaxColorMapValue()));
		minOpacmapEdit->setText(QString::number(fParams->GetMapperFunc()->getMinOpacMapValue()));
		maxOpacmapEdit->setText(QString::number(fParams->GetMapperFunc()->getMaxOpacMapValue()));
	}
	
	
	xSizeEdit->setText(QString::number(seedBoxMax[0]-seedBoxMin[0],'g', 4));
	xCenterEdit->setText(QString::number(0.5f*(seedBoxMax[0]+seedBoxMin[0]),'g',5));
	ySizeEdit->setText(QString::number(seedBoxMax[1]-seedBoxMin[1],'g', 4));
	yCenterEdit->setText(QString::number(0.5f*(seedBoxMax[1]+seedBoxMin[1]),'g',5));
	zSizeEdit->setText(QString::number(seedBoxMax[2]-seedBoxMin[2],'g', 4));
	zCenterEdit->setText(QString::number(0.5f*(seedBoxMax[2]+seedBoxMin[2]),'g',5));
	seedtimeIncrementEdit->setText(QString::number(fParams->getSeedTimeIncrement()));
	seedtimeStartEdit->setText(QString::number(fParams->getSeedTimeStart()));
	if (flowType != 2)
		seedtimeEndEdit->setText(QString::number(fParams->getSeedTimeEnd()));
	else 
		seedtimeEndEdit->setText(QString::number(fParams->getSeedTimeStart()));

	//Put the opacity and color bounds for the currently chosen mappings
	//These should be the actual range of the variables
	int var = fParams->getColorMapEntityIndex();
	int tstep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	if (var<4){
		minColorBound->setText(QString::number(fParams->minRange(var, tstep)));
		maxColorBound->setText(QString::number(fParams->maxRange(var, tstep)));
	} else {
		int varnum = DataStatus::mapActiveToSessionVarNum3D(var -4);
		float minval= -1.f, maxval = 1.f;
		if (dStatus->variableIsPresent3D(varnum)){
			if (fParams->isEnabled()){
				minval = dStatus->getDataMin3D(varnum, tstep);
				maxval = dStatus->getDataMax3D(varnum, tstep);
			} else {
				minval = dStatus->getDefaultDataMin3D(varnum);
				maxval = dStatus->getDefaultDataMax3D(varnum);
			}
		}
		minColorBound->setText(QString::number(minval));
		maxColorBound->setText(QString::number(maxval));
	}
	var = fParams->getOpacMapEntityIndex();
	if (var < 4){
		minOpacityBound->setText(QString::number(fParams->minRange(var, tstep)));
		maxOpacityBound->setText(QString::number(fParams->maxRange(var, tstep)));
	} else {
		int varnum = DataStatus::mapActiveToSessionVarNum3D(var -4);
		float minval= -1.f, maxval = 1.f;
		if (dStatus->variableIsPresent3D(varnum)){
			if (fParams->isEnabled()){
				minval = dStatus->getDataMin3D(varnum, tstep);
				maxval = dStatus->getDataMax3D(varnum, tstep);
			} else {
				minval = dStatus->getDefaultDataMin3D(varnum);
				maxval = dStatus->getDefaultDataMax3D(varnum);
			}
		}
		minOpacityBound->setText(QString::number(minval));
		maxOpacityBound->setText(QString::number(maxval));
	}

	updateMapBounds(fParams);
	update();
	guiSetTextChanged(false);
	
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
}
//the only state that needs urgent updating is the auto refresh checkbox:
void FlowEventRouter::updateUrgentTabState(){
	FlowParams* fParams = (FlowParams*) VizWinMgr::getActiveFlowParams();
	if (fParams) {
		autoRefreshCheckbox->setChecked(fParams->refreshIsAuto());
		QPalette pal(autoRefreshCheckbox->palette());
		if (fParams->refreshIsAuto()) {
			pal.setColor(autoRefreshCheckbox->foregroundRole(),QColor(Qt::black));
			autoRefreshCheckbox->setPalette(pal);
		}
		else {
			pal.setColor(autoRefreshCheckbox->foregroundRole(),QColor(Qt::red));
			autoRefreshCheckbox->setPalette(pal);
		}
	}
}

void FlowEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	bool needRegen = flowDataChanged;
	FlowParams* fParams = VizWinMgr::getInstance()->getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "edit Flow text");
	if (mapBoundsChanged){
		float colorMapMin = minColormapEdit->text().toFloat();
		float colorMapMax = maxColormapEdit->text().toFloat();
		if (colorMapMin >= colorMapMax){
			colorMapMax = colorMapMin+1.e-6;
			maxColormapEdit->setText(QString::number(colorMapMax));
		}
		float opacMapMin = minOpacmapEdit->text().toFloat();
		float opacMapMax = maxOpacmapEdit->text().toFloat();
		if (opacMapMin >= opacMapMax){
			opacMapMax = opacMapMin+1.e-6;
			maxOpacmapEdit->setText(QString::number(opacMapMax));
		}
		MapperFunction* mapperFunction = fParams->GetMapperFunc();
		if (mapperFunction){
			mapperFunction->setMaxColorMapValue(colorMapMax);
			mapperFunction->setMinColorMapValue(colorMapMin);
			mapperFunction->setMaxOpacMapValue(opacMapMax);
			mapperFunction->setMinOpacMapValue(opacMapMin);
		}
		fParams->setMinColorMapBound(colorMapMin);
		fParams->setMaxColorMapBound(colorMapMax);
		
		fParams->setMinOpacMapBound(opacMapMin);
		fParams->setMaxOpacMapBound(opacMapMax);
		
		//Align the editor:
		fParams->setMinColorEditBound(fParams->getMinColorMapBound(),fParams->getColorMapEntityIndex());
		fParams->setMaxColorEditBound(fParams->getMaxColorMapBound(),fParams->getColorMapEntityIndex());
		fParams->setMinOpacEditBound(fParams->getMinOpacMapBound(),fParams->getOpacMapEntityIndex());
		fParams->setMaxOpacEditBound(fParams->getMaxOpacMapBound(),fParams->getOpacMapEntityIndex());
		setEditorDirty();
	}
	int flowType = fParams->getFlowType();
	bool autoscale = fParams->isAutoScale();
	if (flowDataChanged){
		//Do settings that depend on flowType:
		
		if (flowType == 0){
			
			
			if (!autoscale ){
				int sampleRate = steadySamplesEdit1->text().toInt();
				if (sampleRate < 2 || sampleRate > 2000){
					sampleRate = 2;
					steadySamplesEdit1->setText(QString::number(sampleRate));
					guiSetTextChanged(false);
				}
				int sval = (int)(0.5f+256.0*log10((float)sampleRate/2.f)*0.33333);
				if (steadySamplesSlider1->value() != sval)
					steadySamplesSlider1->setValue(sval);
			
				fParams->setObjectsPerFlowline(sampleRate);

				float velocityScale = steadyScaleEdit1->text().toFloat();
				if (velocityScale < 1.e-20f){
					velocityScale = 1.e-20f;
					steadyScaleEdit1->setText(QString::number(velocityScale));
					guiSetTextChanged(false);
				}
				fParams->setSteadyScale(velocityScale);
			}
		} else {//set up unsteady flow sample rate, velocity scale
			int sampleRate = unsteadySamplesEdit->text().toInt();
			if (sampleRate < 1 || sampleRate > 256){
				sampleRate = 1;
				unsteadySamplesEdit->setText(QString::number(sampleRate));
				guiSetTextChanged(false);
			}
			if (unsteadySamplesSlider->value() != sampleRate)
				unsteadySamplesSlider->setValue(sampleRate);
			fParams->setObjectsPerTimestep(sampleRate);

			float velocityScale = unsteadyScaleEdit->text().toFloat();
			if (velocityScale < 1.e-20f){
				velocityScale = 1.e-20f;
				unsteadyScaleEdit->setText(QString::number(velocityScale));
				guiSetTextChanged(false);
			}
			fParams->setUnsteadyScale(velocityScale);		
		}

		if (flowType == 1 ){
			

			fParams->setTimeSamplingInterval(timesampleIncrementEdit1->text().toInt());
			fParams->setTimeSamplingStart(timesampleStartEdit1->text().toInt());
			fParams->setTimeSamplingEnd(timesampleEndEdit1->text().toInt());
			int minFrame = VizWinMgr::getInstance()->getActiveAnimationParams()->getStartFrameNumber();
			int unsteadyvars[3];
			for (int i = 0; i<3; i++) unsteadyvars[i] = (fParams->getUnsteadyVarNums())[i]-1;
			
			if (!fParams->validateSampling(minFrame,
				fParams->GetRefinementLevel(), unsteadyvars)){//did anything change?
				timesampleIncrementEdit1->setText(QString::number(fParams->getTimeSamplingInterval()));
				timesampleStartEdit1->setText(QString::number(fParams->getTimeSamplingStart()));
				timesampleEndEdit1->setText(QString::number(fParams->getTimeSamplingEnd()));
			}
		} else if (autoscale){ // auto steady flow settings for flow type 0 and 2:
			//Look at smoothness (samples per along flowline, per region diameter)
			//and flow length (in diameter units). The product of smoothness and flow length
			//is the total number of samples (maxPoints) and needs to be kept between
			//2 and 10000
			bool changed = false;
			
			//Make sure flowLen*smoothness is between 2 and 10000.
			//following code adapted from guiSetSteadyLength
			float steadySmoothness = smoothnessSamplesEdit->text().toFloat();
			float len = steadyLengthEdit->text().toFloat();
			//First make sure they fit within bounds of sliders:
			if (steadySmoothness < .1f){
				steadySmoothness = .1f;
				changed = true;
			}
			if (steadySmoothness > 1000.f){
				steadySmoothness = 1000.f;
				changed = true;
			}
			if (len < 0.01f) {
				len = 0.01f;
				changed = true;
			}
			if (len > 100.f){
				len = 100.f;
				changed = true;
			}
			//then adjust the product, changing length if necessary:
			if (steadySmoothness*len > 10000.f){
				changed = true;
				len = 10000.f/steadySmoothness;
			}
			if (steadySmoothness*len < 2.f){ 
				changed = true;
				len = 2.f/steadySmoothness;
			}
			if (changed){
				smoothnessSamplesEdit->setText(QString::number(steadySmoothness));
				steadyLengthEdit->setText(QString::number(len));
				guiSetTextChanged(false);
			}
			int sval = (int)(0.5f+256.0*(log10((float)steadySmoothness)+1.f)*0.25f);
			if (sval != smoothnessSlider->value())
				smoothnessSlider->setValue(sval);
			sval = (int)(0.5f+256.0*(log10((float)len)+2.f)*0.25f);
			if (steadyLengthSlider->value() != sval)
				steadyLengthSlider->setValue(sval);
			fParams->setSteadyFlowLength(len);
			fParams->setSteadySmoothness(steadySmoothness);
		}

		if(flowType == 2 ) {//Flow line advection only
			
			int numFlaSamples = 1;
			if (flaOptionCombo->currentIndex() == 1){
				numFlaSamples = flaSamplesEdit->text().toInt();
				if (numFlaSamples < 2) {
					numFlaSamples = 2;
					flaSamplesEdit->setText("2");
				}
				fParams->setNumFLASamples(numFlaSamples);
			}
			
			fParams->setPriorityMin(priorityFieldMinEdit->text().toFloat());
			fParams->setPriorityMax(priorityFieldMaxEdit->text().toFloat());
			
			fParams->setTimeSamplingInterval(timesampleIncrementEdit1->text().toInt());
			fParams->setTimeSamplingStart(timesampleStartEdit1->text().toInt());
			fParams->setTimeSamplingEnd(timesampleEndEdit1->text().toInt());
			int minFrame = VizWinMgr::getInstance()->getActiveAnimationParams()->getStartFrameNumber();
			int unsteadyvars[3];
			for (int i = 0; i<3; i++) unsteadyvars[i] = fParams->getUnsteadyVarNums()[i]-1;
			if (!fParams->validateSampling(minFrame,
				fParams->GetRefinementLevel(), unsteadyvars)){//did anything change?
				timesampleIncrementEdit1->setText(QString::number(fParams->getTimeSamplingInterval()));
				timesampleStartEdit1->setText(QString::number(fParams->getTimeSamplingStart()));
				timesampleEndEdit1->setText(QString::number(fParams->getTimeSamplingEnd()));
			}
			if (!autoscale){
				//steady scaling stuff for flow line advection
				int sampleRate = steadySamplesEdit1->text().toInt();
				if (sampleRate < 2 || sampleRate > 2000){
					sampleRate = 2;
					steadySamplesEdit1->setText(QString::number(sampleRate));
					guiSetTextChanged(false);
				}
				int sval = (int)(0.5f+256.0*log10((float)sampleRate/2.f)*0.33333);
				if (sval != steadySamplesSlider1->value())
					steadySamplesSlider1->setValue(sval);
				fParams->setObjectsPerFlowline(sampleRate);

				float velocityScale = steadyScaleEdit1->text().toFloat();
				if (velocityScale < 1.e-20f){
					velocityScale = 1.e-20f;
					steadyScaleEdit1->setText(QString::number(velocityScale));
					guiSetTextChanged(false);
				}
				fParams->setSteadyScale(velocityScale);
			}
		}
		float seedDistBias = biasEdit1->text().toFloat();
		if (seedDistBias < -15.f || seedDistBias > 15.f) seedDistBias = 0.f;
		int bval = (int)(0.5+ seedDistBias*128.f/15.f);
		if (bval != biasSlider1->value()){
			biasSlider1->setTracking(false);
			biasSlider1->setSliderPosition(bval);
			biasSlider1->setTracking(true);
		}
		float integrationAccuracy = integrationAccuracyEdit->text().toFloat();
		if (integrationAccuracy < 0.f || integrationAccuracy > 1.f) {
			if (integrationAccuracy > 1.f) integrationAccuracy = 1.f;
			if (integrationAccuracy < 0.f) integrationAccuracy = 0.f;
			integrationAccuracyEdit->setText(QString::number(integrationAccuracy));
		}
		fParams->setIntegrationAccuracy(integrationAccuracy);
		float steadyFlowLen = steadyLengthEdit->text().toFloat();
		if (steadyFlowLen != fParams->getSteadyFlowLength()){
			if (steadyFlowLen < 0.01f || steadyFlowLen > 100.f) steadyFlowLen = 1.f;
			fParams->setSteadyFlowLength(steadyFlowLen);
		}
		
		fParams->setRandomSeed(randomSeedEdit->text().toUInt());

		//Do Rake settings
		
		float boxCtr = xCenterEdit->text().toFloat();
		float boxSize = xSizeEdit->text().toFloat();
		
		textToSlider(fParams,0, boxCtr, boxSize);
		boxCtr = yCenterEdit->text().toFloat();
		boxSize = ySizeEdit->text().toFloat();
		
		textToSlider(fParams,1, boxCtr, boxSize);
		boxCtr = zCenterEdit->text().toFloat();
		boxSize = zSizeEdit->text().toFloat();
		
		textToSlider(fParams,2, boxCtr, boxSize);

		if (fParams->isRandom()){
			int genCount = generatorCountEdit->text().toInt();
			if (genCount < 1) {
				genCount = 1;
				generatorCountEdit->setText(QString::number(genCount));
			}
			fParams->setTotalNumGenerators(genCount);
		} else {
			int genCount = xSeedEdit->text().toInt();
			if (genCount<1)genCount = 1;
			fParams->setNumGenerators(0,genCount);
			genCount = ySeedEdit->text().toInt();
			if (genCount<1)genCount = 1;
			fParams->setNumGenerators(1,genCount);
			genCount = zSeedEdit->text().toInt();
			if (genCount<1)genCount = 1;
			fParams->setNumGenerators(2,genCount);
			generatorCountEdit->setText(QString::number(fParams->getNumRakeSeedPoints()));
		}	
		if(flowType != 0){//rake settings that are for non-steady flow
			int seedTimeStart = seedtimeStartEdit->text().toUInt();
			int seedTimeEnd = seedTimeStart;
			if (flowType == 1)
				seedTimeEnd = seedtimeEndEdit->text().toUInt(); 
			bool changed = false;
			VizWinMgr* vizMgr = VizWinMgr::getInstance();
			int minFrame = vizMgr->getAnimationParams(vizMgr->getActiveViz())->getStartFrameNumber();
			int maxFrame = fParams->getMaxFrame();
			if (seedTimeStart < minFrame) {seedTimeStart = minFrame; changed = true;}
			if (seedTimeEnd > maxFrame) {seedTimeEnd = maxFrame; changed = true;}
			if (seedTimeEnd < seedTimeStart) {seedTimeEnd = seedTimeStart; changed = true;}
			if (changed){
				seedtimeStartEdit->setText(QString::number(seedTimeStart));
				if(flowType == 1) seedtimeEndEdit->setText(QString::number(seedTimeEnd));
			}
			fParams->setSeedTimeStart(seedTimeStart);
			fParams->setSeedTimeEnd(seedTimeEnd);
			

			int seedTimeIncrement = seedtimeIncrementEdit->text().toUInt();
			if (seedTimeIncrement < 1) seedTimeIncrement = 1;
			fParams->setSeedTimeIncrement(seedTimeIncrement);
			
		} //end of rake settings for unsteady flow
		
	} // end of flow Data changed
	if (flowGraphicsChanged){
		//change the parameters.  
		float shapeDiameter = diameterEdit->text().toFloat();
		if (shapeDiameter < 0.f) {
			shapeDiameter = 0.f;
			diameterEdit->setText(QString::number(shapeDiameter));
		}
		float diamondDiameter = diamondSizeEdit->text().toFloat();
		if (diamondDiameter < 0.f) {
			diamondDiameter = 0.f;
			diamondSizeEdit->setText(QString::number(diamondDiameter));
		}
		float arrowDiameter = arrowheadEdit->text().toFloat();
		if (arrowDiameter < 1.f) {
			arrowDiameter = 1.f;
			arrowheadEdit->setText(QString::number(arrowDiameter));
		}
		float constantOpacity = constantOpacityEdit->text().toFloat();
		if (constantOpacity < 0.f) constantOpacity = 0.f;
		if (constantOpacity > 1.f) constantOpacity = 1.f;
		fParams->setShapeDiameter(shapeDiameter);
		fParams->setArrowDiameter(arrowDiameter);
		fParams->setDiamondDiameter(diamondDiameter);
		fParams->setConstantOpacity(constantOpacity);
		if (fParams->getFlowType() == 1){
			int lastDisplayFrame = lastDisplayFrameEdit->text().toInt();
			int firstDisplayFrame = firstDisplayFrameEdit->text().toInt();
			
			//Make sure at least one frame is displayed.
			//
			if (firstDisplayFrame >= lastDisplayFrame) {
				lastDisplayFrame = firstDisplayFrame+1;
				lastDisplayFrameEdit->setText(QString::number(lastDisplayFrame));
			}
			fParams->setFirstDisplayFrame(firstDisplayFrame);
			fParams->setLastDisplayFrame(lastDisplayFrame);
		}
	}
	
	guiSetTextChanged(false);
	//If the data changed, need to setFlowDataDirty; otherwise
	//just need to setFlowMappingDirty.
	mapBoundsChanged = false;
	flowGraphicsChanged = false;
	flowDataChanged = false;
	if(needRegen){
		if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
		VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	} else {
		VizWinMgr::getInstance()->setFlowGraphicsDirty(fParams);
	}

	PanelCommand::captureEnd(cmd, fParams);
}

/*********************************************************************************
 * Slots associated with FlowTab:
 *********************************************************************************/
//Respond to user click "STOP"
void FlowEventRouter::stopClicked(){
	//Set the "Stop" flag in the params
	FlowParams* fParams = VizWinMgr::getInstance()->getActiveFlowParams();
	if (fParams->flowIsSteady()||!fParams->isEnabled()) return;
	//Turn off autoRefresh (does this screw up undo/redo queue?
	MessageReporter::warningMsg("Flow integration stopped.\n%s",
			"Auto flow refresh has been disabled.");
	guiSetAutoRefresh(false);
	fParams->setStopFlag();
	refreshButton->setEnabled(true);
	
}
//Add a new (blank) row to the tables (they are same size)
void FlowEventRouter::addSample(){
	int numRows = timestepSampleTable1->rowCount()+1;
	timestepSampleTable1->setRowCount(numRows);
	
	QTableWidgetItem* tstepItem1 = new QTableWidgetItem("");
	
	dontUpdate=true;
	timestepSampleTable1->setItem(numRows-1,0, tstepItem1);
	
	dontUpdate=false;
	timestepSampleTable1->setCurrentCell(numRows-1,0);
	
}
//Show setup instructions for flow:
void FlowEventRouter::showSetupHelp(){
	FlowParams* fParams = VizWinMgr::getInstance()->getActiveFlowParams();
	if (fParams->getFlowType() == 1){ 
		HelpWindow::showHelp(("UnsteadyHelp.html"));
	} else {
		HelpWindow::showHelp(("FieldLineAdvectionHelp.html"));
	}
}
//Delete the current selected row
void FlowEventRouter::deleteSample(){
	int thisRow;
	thisRow = timestepSampleTable1->currentRow();
	if (thisRow <0) return;
	timestepSampleTable1->removeRow(thisRow);
	guiUpdateUnsteadyTimes(timestepSampleTable1, "remove unsteady timestep");
	if(thisRow>0)timestepSampleTable1->setCurrentCell(thisRow-1,0);
	else timestepSampleTable1->setCurrentCell(0,0);
}
//Rebuild the list of timestep samples.
void FlowEventRouter::guiRebuildList(){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getInstance()->getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "Rebuild timestep list");
	std::vector<int>& timesteplist = fParams->getUnsteadyTimesteps();
	timesteplist.clear();
	DataStatus* ds = DataStatus::getInstance();
	int minTime = ds->getMinTimestep();
	int maxTime = ds->getMaxTimestep();
	for (int i = minTime; i<= maxTime; i++){
		if (ds->dataIsPresent(i)) timesteplist.push_back(i);
	}
	populateTimestepTables();
	PanelCommand::captureEnd(cmd, fParams);
	updateTab();
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
//Respond to user has typed in a row. 
void FlowEventRouter::timestepChanged1(int row, int col){
	if (dontUpdate) return;
	guiUpdateUnsteadyTimes(timestepSampleTable1,"edit unsteady timestep list");
	return;
}
//Send the contents of the timestepTable to the params.

void FlowEventRouter::guiUpdateUnsteadyTimes(QTableWidget* tbl, const char* descr){	
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getInstance()->getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams, descr);
	std::vector<int>& timesteplist = fParams->getUnsteadyTimesteps();
	timesteplist.clear();
	
	for (int i = 0; i< tbl->rowCount(); i++){
		int newTime = tbl->item(i,0)->text().toInt();
		timesteplist.push_back(newTime);	
	}
	//Sort the times
	std::sort(timesteplist.begin(), timesteplist.end());
	//Eliminate duplicates:
	for (int i = timesteplist.size()-1; i>0; i--)
		if (timesteplist[i] == timesteplist[i-1]) timesteplist.erase(timesteplist.begin()+i);
	PanelCommand::captureEnd(cmd, fParams);
	updateTab();
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}



void FlowEventRouter::populateTimestepTables(){
	dontUpdate = true;
	FlowParams* fParams = VizWinMgr::getInstance()->getActiveFlowParams();
	std::vector<int>& tSteps = fParams->getUnsteadyTimesteps();
	timestepSampleTable1->setRowCount(tSteps.size());
	timestepSampleTable1->setColumnCount(1);
	
	for (int i = 0; i< tSteps.size(); i++){
		QTableWidgetItem* tstepItem1 = new QTableWidgetItem(QString::number(tSteps[i]));
		
		timestepSampleTable1->setItem(i,0, tstepItem1);
		
	}
	timestepSampleCheckbox1->setChecked(fParams->usingTimestepSampleList());
	
	dontUpdate = false;
}

void FlowEventRouter::
flowTabReturnPressed(void){
	confirmText(true);
}


/*************************************************************************************
 * slots associated with FlowTab
 *************************************************************************************/
void FlowEventRouter::guiChangeInstance(int newCurrent){
	//Do this in the parent class:
	performGuiChangeInstance(newCurrent);
	
}
void FlowEventRouter::guiNewInstance(){
	performGuiNewInstance();
	
}
void FlowEventRouter::guiDeleteInstance(){
	performGuiDeleteInstance();
	
}

void FlowEventRouter::guiCopyInstanceTo(int toViz){
	if (toViz == 0) return; 
	if (toViz == 1) {performGuiCopyInstance();return;}
	int viznum = copyCount[toViz];
	copyCombo->setCurrentIndex(0);
	performGuiCopyInstanceToViz(viznum);
}

//There are text changed events for flow (requiring rebuilding flow data),
//for graphics (requiring regenerating flow graphics), and
//for dataRange (requiring change of data mapping range, hence regenerating flow graphics)
void FlowEventRouter::setFlowTabFlowTextChanged(const QString&){
	setFlowDataChanged(true);
	guiSetTextChanged(true);
}


void FlowEventRouter::setFlowTabGraphicsTextChanged(const QString&){
	setFlowGraphicsChanged(true);
	guiSetTextChanged(true);
}
void FlowEventRouter::setFlowTabRangeTextChanged(const QString&){
	
	setMapBoundsChanged(true);
	guiSetTextChanged(true);
}


void FlowEventRouter::
setFlowEnabled(bool val, int instance){
	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	FlowParams* myFlowParams = vizMgr->getFlowParams(winnum, instance);
	//Make sure this is a change:
	if (myFlowParams->isEnabled() == val ) return;
	//If we are enabling, also make this the current instance:
	if (val) {
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val, instance);
	
}

void FlowEventRouter::
setBiasFromSlider1(int val){
	
	float biasVal = 15.f*val/128.f;
	biasEdit1->setText(QString::number(biasVal));
	guiSetSeedDistBias(biasVal);
}




/*
 * Respond to user clicking the color button
 */
void FlowEventRouter::
setFlowConstantColor(){
	QPalette pal(constantColorEdit->palette());
	QColor newColor = QColorDialog::getColor(pal.color(QPalette::Base), this);
	if (!newColor.isValid()) return;
	pal.setColor(QPalette::Base, newColor);
	constantColorEdit->setPalette(pal);
	//Set parameter value of the appropriate parameter set:
	guiSetConstantColor(newColor);
}


void FlowEventRouter::
setFlowEditMode(bool mode){
	navigateButton->setChecked(!mode);
	guiSetEditMode(mode);
}
void FlowEventRouter::
setFlowNavigateMode(bool mode){
	editButton->setChecked(!mode);
	guiSetEditMode(!mode);
}



//Reinitialize Flow tab settings, session has changed.
//Note that this is called after the globalFlowParams are set up, but before
//any of the localFlowParams are setup.
void FlowEventRouter::
reinitTab(bool doOverride){
	setIgnoreBoxSliderEvents(false);
	DataStatus* ds = DataStatus::getInstance();
	if (ds->dataIsPresent3D()&&!ds->sphericalTransform()) setEnabled(true);
	else setEnabled(false);

	flowDataChanged = false;
	mapBoundsChanged = false;
	flowGraphicsChanged = false;
	int maxNumRefinements = DataStatus::getInstance()->getNumTransforms();
	//Set up the refinementCombo
	refinementCombo->setMaxCount(maxNumRefinements+1);
	refinementCombo->clear();
	for (int i = 0; i<= maxNumRefinements; i++){
		refinementCombo->addItem(QString::number(i));
	}
	if (ds->getDataMgr()){
		vector<size_t> cRatios = ds->getDataMgr()->GetCRatios();
		lodCombo->clear();
		lodCombo->setMaxCount(cRatios.size());
		for (int i = 0; i<cRatios.size(); i++){
			QString s = QString::number(cRatios[i]);
			s += ":1";
			lodCombo->addItem(s);
		}
	}
	
	int newNumComboVariables = ds->getNumActiveVariables3D();
	//Set up the combo 
	
	xSteadyVarCombo->clear();
	xSteadyVarCombo->setMaxCount(newNumComboVariables+1);
	ySteadyVarCombo->clear();
	ySteadyVarCombo->setMaxCount(newNumComboVariables+1);
	zSteadyVarCombo->clear();
	zSteadyVarCombo->setMaxCount(newNumComboVariables+1);
	xUnsteadyVarCombo->clear();
	xUnsteadyVarCombo->setMaxCount(newNumComboVariables+1);
	yUnsteadyVarCombo->clear();
	yUnsteadyVarCombo->setMaxCount(newNumComboVariables+1);
	zUnsteadyVarCombo->clear();
	zUnsteadyVarCombo->setMaxCount(newNumComboVariables+1);
	xSeedPriorityCombo->clear();
	xSeedPriorityCombo->setMaxCount(newNumComboVariables);
	ySeedPriorityCombo->clear();
	ySeedPriorityCombo->setMaxCount(newNumComboVariables);
	zSeedPriorityCombo->clear();
	zSeedPriorityCombo->setMaxCount(newNumComboVariables);
	xSeedDistCombo->clear();
	xSeedDistCombo->setMaxCount(newNumComboVariables);
	ySeedDistCombo->clear();
	ySeedDistCombo->setMaxCount(newNumComboVariables);
	zSeedDistCombo->clear();
	zSeedDistCombo->setMaxCount(newNumComboVariables);
	

	//Put a "0" at the start of the variable combos
	const QString& text = QString("0");
	xSteadyVarCombo->addItem(text);
	ySteadyVarCombo->addItem(text);
	zSteadyVarCombo->addItem(text);
	xUnsteadyVarCombo->addItem(text);
	yUnsteadyVarCombo->addItem(text);
	zUnsteadyVarCombo->addItem(text);
	for (int i = 0; i< newNumComboVariables; i++){
		const std::string& s = DataStatus::getInstance()->getActiveVarName3D(i);
		//Direct conversion of std::string& to QString doesn't seem to work
		//Maybe std was not enabled when QT was built?
		const QString& text = QString(s.c_str());
		xSteadyVarCombo->addItem(text);
		ySteadyVarCombo->addItem(text);
		zSteadyVarCombo->addItem(text);
		xUnsteadyVarCombo->addItem(text);
		yUnsteadyVarCombo->addItem(text);
		zUnsteadyVarCombo->addItem(text);
		xSeedDistCombo->addItem(text);
		ySeedDistCombo->addItem(text);
		zSeedDistCombo->addItem(text);
		
		xSeedPriorityCombo->addItem(text);
		ySeedPriorityCombo->addItem(text);
		zSeedPriorityCombo->addItem(text);
	}
	
	std::vector<string> colorMapEntity;
	std::vector<string> opacMapEntity;
	colorMapEntity.clear();
	colorMapEntity.push_back("Constant");
	colorMapEntity.push_back("Timestep");
	colorMapEntity.push_back("Field Magnitude");
	colorMapEntity.push_back("Seed Index");
	opacMapEntity.clear();
	opacMapEntity.push_back("Constant");
	opacMapEntity.push_back("Timestep");
	opacMapEntity.push_back("Field Magnitude");
	opacMapEntity.push_back("Seed Index");
	for (int i = 0; i< newNumComboVariables; i++){
		colorMapEntity.push_back(DataStatus::getInstance()->getActiveVarName3D(i));
		opacMapEntity.push_back(DataStatus::getInstance()->getActiveVarName3D(i));
	}
	//Set up the color, opac map entity combos:
	
	
	colormapEntityCombo->clear();
	for (int i = 0; i< (int)colorMapEntity.size(); i++){
		colormapEntityCombo->addItem(QString(colorMapEntity[i].c_str()));
	}
	opacmapEntityCombo->clear();
	for (int i = 0; i< (int)colorMapEntity.size(); i++){
		opacmapEntityCombo->addItem(QString(opacMapEntity[i].c_str()));
	}
	updateTab();
	dontUpdate=false;
}

//Methods that record changes in the history:
//
//Move the rake center to specified coords, shrink it if necessary
void FlowEventRouter::
guiCenterRake(const float* coords){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "move rake center");
	const float* fullExtent = DataStatus::getInstance()->getExtents();
	float seedBoxMin[3],seedBoxMax[3];
	fParams->getBox(seedBoxMin, seedBoxMax, -1);
	for (int i = 0; i< 3; i++){
		float coord = coords[i];
		float regMin = fullExtent[i];
		float regMax = fullExtent[i+3];
		if (coord < regMin) coord = regMin;
		if (coord > regMax) coord = regMax;
		float boxSize = seedBoxMax[i] - seedBoxMin[i];
		if (coord + 0.5f*boxSize > seedBoxMax[i]) boxSize = 2.f*(seedBoxMax[i] - coord);
		if (coord - 0.5f*boxSize < seedBoxMin[i]) boxSize = 2.f*(coord - seedBoxMin[i]);
		seedBoxMax[i] = coord + 0.5f*boxSize;
		seedBoxMin[i] = coord - 0.5f*boxSize;
	}
	fParams->setBox(seedBoxMin, seedBoxMax, -1);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
//Add an individual seed to the set of seeds
//Is also performed when we first do a seed attachment.
void FlowEventRouter::
guiAddSeed(Point4 coords){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "Add new seed point");
	fParams->pushSeed(*(new Point4(coords)));
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	PanelCommand::captureEnd(cmd, fParams);
	VizWinMgr::getInstance()->refreshFlow(fParams);
}
//Add an individual seed to the set of seeds
void FlowEventRouter::
guiMoveLastSeed(const float coords[3]){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "Move seed point");
	fParams->moveLastSeed(coords);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	//Do not want to interrupt during flow construction here, it can ruin the probe 
	VizWinMgr::getInstance()->setFlowDataDirty(fParams, false);
	
}
//Turn on/off the rake and the seedlist:
void FlowEventRouter::guiSetRakeList(int index){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	bool doRake = fParams->rakeEnabled();
	bool isRandom = fParams->isRandom();
	int currentIndex = 0;
	if (doRake) currentIndex = 1;
	if (isRandom&&doRake) currentIndex = 2;
	if (currentIndex == index) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "change rake/list/random flow setting");
	fParams->enableRake(index > 0);
	fParams->setRandom(index > 1);
	if (index > 0)
		generatorCountEdit->setText(QString::number(fParams->getNumRakeSeedPoints()));
	else
		generatorCountEdit->setText(QString::number(fParams->getNumListSeedPoints()));
	if (index > 1){//random rake
		dimensionLabel->setEnabled(false);
		generatorCountEdit->setEnabled(true);
		xSeedEdit->setEnabled(false);
		ySeedEdit->setEnabled(false);
		zSeedEdit->setEnabled(false);
		randomSeedEdit->setEnabled(true);
	} 
	if (index == 1){//nonrandom rake
		dimensionLabel->setEnabled(true);
		generatorCountEdit->setEnabled(false);
		xSeedEdit->setEnabled(true);
		ySeedEdit->setEnabled(true);
		zSeedEdit->setEnabled(true);
		randomSeedEdit->setEnabled(false);
	}
	if (index == 0){//seedlist 
		dimensionLabel->setEnabled(false);
		generatorCountEdit->setEnabled(false);
		xSeedEdit->setEnabled(false);
		ySeedEdit->setEnabled(false);
		zSeedEdit->setEnabled(false);
		randomSeedEdit->setEnabled(false);
	}
	PanelCommand::captureEnd(cmd, fParams);
	//If there's a change, need to set flags dirty
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	VizWinMgr::getInstance()->refreshFlow(fParams);
}



void FlowEventRouter::
guiSetEnabled(bool on, int instance, bool undoredo){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	FlowParams* fParams = vizMgr->getFlowParams(winnum, instance);
	if (on == fParams->isEnabled()) return;
	
	confirmText(false);
	PanelCommand* cmd;
	if(undoredo) cmd = PanelCommand::captureStart(fParams,  "enable/disable flow render",instance);
	fParams->setEnabled(on);
	if(undoredo) PanelCommand::captureEnd(cmd, fParams);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(fParams, !on, false);
	updateTab();
}
//Respond to a change in opacity scale factor
void FlowEventRouter::
guiSetOpacityScale(int val){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "modify opacity scale slider");
	fParams->setOpacityScale(((float)(256-val))/256.f);
	float sliderVal = fParams->getOpacityScale();
	opacityScaleSlider->setToolTip("Opacity Scale Value = "+QString::number(sliderVal));
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);

	VizWinMgr::getInstance()->setFlowGraphicsDirty(fParams);

	PanelCommand::captureEnd(cmd,fParams);
}
//Make rake match region
void FlowEventRouter::
guiSetRakeToRegion(){
	confirmText(false);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "move rake to region");
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	float seedBoxMin[3],seedBoxMax[3];
	for (int i = 0; i< 3; i++){
		seedBoxMin[i] = rParams->getRegionMin(i,timestep);
		seedBoxMax[i] = rParams->getRegionMax(i,timestep);
	}
	fParams->setBox(seedBoxMin,seedBoxMax, -1);
	PanelCommand::captureEnd(cmd, fParams);
	updateTab();
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	
}
void FlowEventRouter::
guiSetFlowType(int typenum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	if (fParams->getFlowType() == typenum) return;
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set flow type");
	fParams->setFlowType(typenum);
	if (typenum == 0){
		//Need non-positive first display frame for steady flow
		int firstDisplayFrame = fParams->getFirstDisplayFrame();
		if (firstDisplayFrame > 0) {
			firstDisplayFrame = 0;
			firstDisplayFrameEdit->setText(QString::number(firstDisplayFrame));
			fParams->setFirstDisplayFrame(firstDisplayFrame);
		}
	} 
	PanelCommand::captureEnd(cmd, fParams);
	updateTab();
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	//If refresh is not auto, clear the needs refresh flags
	if (!fParams->refreshIsAuto()){
		FlowRenderer* fRenderer = (FlowRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(fParams);
		if (!fRenderer) return;
		//Clear all the needs refresh flags:
		fRenderer->setAllNeedRefresh(false);
		refreshButton->setEnabled(true);
	}
}
void FlowEventRouter::
guiSetSteadyDirection(int comboIndex){
	
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	//combo has values 0,1,2
	int flowDir = comboIndex-1;
	if (fParams->getSteadyDirection() == flowDir) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set steady flow direction");
	
	fParams->setSteadyDirection(flowDir);
	PanelCommand::captureEnd(cmd, fParams);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
}
void FlowEventRouter::
guiSetUnsteadyDirection(int comboIndex){
	
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	//combo has values 0,1, resp corresponding to +1, -1
	int flowDir = 1-2*comboIndex;
	if (flowDir == fParams->getUnsteadyDirection()) return;
	confirmText(false);

	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set unsteady flow direction");
	
	fParams->setUnsteadyDirection(flowDir);
	PanelCommand::captureEnd(cmd, fParams);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
}
void FlowEventRouter::
guiSetCompRatio(int num){
	confirmText(false);
	//make sure we are changing it
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	if (num == fParams->GetCompressionLevel()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "set compression level");
	
	fParams->SetCompressionLevel(num);
	lodCombo->setCurrentIndex(num);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetNumRefinements(int n){
	
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	if (fParams->GetRefinementLevel() == n) return;
	confirmText(false);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	int newNumTrans = ((RegionParams*)(VizWinMgr::getActiveRegionParams()))->validateNumTrans(n,timestep);
	if (newNumTrans != n) {
		MessageReporter::warningMsg("%s","Invalid number of Refinements \nfor current region, data cache size");
		refinementCombo->setCurrentIndex(newNumTrans);
	}
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "set number Refinements in Flow data");
	fParams->SetRefinementLevel(newNumTrans);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetXComboSteadyVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set X steady field variable");
	fParams->setComboSteadyVarnum(0,varnum);
	if (varnum == 0) fParams->setXSteadyVarNum(0);
	else {
		fParams->setXSteadyVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum-1)+1);
	}
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	updateTab();
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetYComboSteadyVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set Y steady field variable");
	fParams->setComboSteadyVarnum(1,varnum);
	if (varnum == 0)fParams->setYSteadyVarNum(0);
	else
		fParams->setYSteadyVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum-1)+1);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	updateTab();
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetZComboSteadyVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set Z steady field variable");
	fParams->setComboSteadyVarnum(2,varnum);
	if (varnum == 0)fParams->setZSteadyVarNum(0);
	else fParams->setZSteadyVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum-1)+1);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	updateTab();
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetXComboSeedDistVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set X seed distribution field variable");
	fParams->setComboSeedDistVarnum(0,varnum);
	fParams->setXSeedDistVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum));
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetYComboSeedDistVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set Y seed distribution field variable");
	fParams->setComboSeedDistVarnum(1,varnum);
	fParams->setYSeedDistVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum));
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetZComboSeedDistVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set Z seed distribution field variable");
	fParams->setComboSeedDistVarnum(2,varnum);
	fParams->setZSeedDistVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum));
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetXComboPriorityVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set X seed prioritization field variable");
	fParams->setComboPriorityVarnum(0,varnum);
	fParams->setXPriorityVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum));
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetYComboPriorityVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set Y seed prioritization field variable");
	fParams->setComboPriorityVarnum(1,varnum);
	fParams->setYPriorityVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum));
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetZComboPriorityVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set Z seed prioritization field variable");
	fParams->setComboPriorityVarnum(2,varnum);
	fParams->setZPriorityVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum));
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetXComboUnsteadyVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set X unsteady field variable");
	fParams->setComboUnsteadyVarnum(0,varnum);
	if (varnum == 0)fParams->setXUnsteadyVarNum(0);
	else fParams->setXUnsteadyVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum-1)+1);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetYComboUnsteadyVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set Y unsteady field variable");
	fParams->setComboUnsteadyVarnum(1,varnum);
	if (varnum == 0)fParams->setYUnsteadyVarNum(0);
	else fParams->setYUnsteadyVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum-1)+1);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetZComboUnsteadyVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set Z unsteady field variable");
	fParams->setComboUnsteadyVarnum(2,varnum);
	if (varnum == 0)fParams->setZUnsteadyVarNum(0);
	else fParams->setZUnsteadyVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varnum-1)+1);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}


void FlowEventRouter::
guiSetConstantColor(QColor& newColor){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set constant mapping color");
	fParams->setConstantColor(newColor.rgb());
	PanelCommand::captureEnd(cmd, fParams);
	VizWinMgr::getInstance()->setFlowGraphicsDirty(fParams);
	VizWinMgr::getInstance()->setFlowDisplayListDirty(fParams);
}
//Slider sets the geometry sampling rate.  This is the number of objects per flowline,
//and is between 2 and 2000
// value is 2*10**(3s)  where s is between 0 and 1

void FlowEventRouter::
guiSetSteadySamples(int sliderPos){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set steady sampling rate");
	float s = ((float)(sliderPos))/256.f;
	int sampleRate = (int)(0.5f+ 2.f*pow(10.f,3.f*s));
	fParams->setObjectsPerFlowline(sampleRate);
	steadySamplesEdit1->setText(QString::number(sampleRate));
	
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
//Respond to the bias slider release:
void FlowEventRouter::
guiSetSeedDistBias(float biasVal){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set seed distribution bias");
	fParams->setSeedDistBias(biasVal);
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);

}
	
//With unsteady flow, just go from 1 to 256
void FlowEventRouter::
guiSetUnsteadySamples(int sliderPos){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set unsteady sampling rate");
	
	fParams->setObjectsPerTimestep(sliderPos);
	unsteadySamplesEdit->setText(QString::number(fParams->getObjectsPerTimestep()));

	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
//Slider sets the smoothness, between 1 and 1000.0
//This determines the number of samples per domain diameter,
//Can be between 1 and 10000
// value is 10**(4s)  where s is between 0 and 1

void FlowEventRouter::
guiSetSmoothness(int sliderPos){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set steady flow smoothness");
	float s = ((float)(sliderPos))/256.f; // between 0 and 1
	bool changed = false;
	float sfactor = pow(10.f,s*4.f -1.f); //between .1 and 1000
	//Make sure flowLen*smoothness is between 2 and 10000
	float steadyLen = fParams->getSteadyFlowLength();
	if (sfactor*steadyLen > 10000.f){
		sfactor = 10000.f/steadyLen;
		changed = true;
	}
	if (sfactor*steadyLen < 2.f){
		sfactor = 2.f/steadyLen;
		changed = true;
	}
	if (changed) 
		smoothnessSlider->setValue((int)(0.5f+256.0*(log10(sfactor)+1.f)*0.25f));
	fParams->setSteadySmoothness(sfactor);
	smoothnessSamplesEdit->setText(QString::number(sfactor));
	
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}

//Slider sets the line length, between 0.01 and 100.0
// value is 0.01*10**(4s)  where s is between 0 and 1
void FlowEventRouter::
guiSetSteadyLength(int sliderPos){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set steady flow length");
	//logarithmic scale between .01 and 100
	float s = ((float)(sliderPos-128))/128.f;  //between -1 and 1
	float len = pow(10.f,s*2); //between .01 and 100
	bool changed = false;
	//Make sure flowLen*smoothness is between 2 and 10000
	float steadySmoothness = fParams->getSteadySmoothness();
	if (steadySmoothness*len > 10000.f){
		changed = true;
		len = 10000.f/steadySmoothness;
	}
	if (steadySmoothness*len < 2.f){ 
		changed = true;
		len = 2.f/steadySmoothness;
	}
	fParams->setSteadyFlowLength(len);
	steadyLengthEdit->setText(QString::number(len));
	if (changed) 
		steadyLengthSlider->setValue((int)(0.5f+256.0*(log10((float)len)+2.f)*0.25f));
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}

void FlowEventRouter::
guiCheckPeriodicX(bool periodic){
	
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	if (fParams->getPeriodicDim(0) == periodic) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle periodic X coords");
	fParams->setPeriodicDim(0,periodic);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	PanelCommand::captureEnd(cmd, fParams);
	
}
void FlowEventRouter::
guiCheckPeriodicY(bool periodic){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	if (fParams->getPeriodicDim(1) == periodic) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle periodic Y coords");
	fParams->setPeriodicDim(1,periodic);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	PanelCommand::captureEnd(cmd, fParams);
	
}
void FlowEventRouter::
guiCheckPeriodicZ(bool periodic){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	if (fParams->getPeriodicDim(2) == periodic) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle periodic Z coords");
	fParams->setPeriodicDim(2,periodic);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	PanelCommand::captureEnd(cmd, fParams);
	
}

void FlowEventRouter::
guiSetXCenter(int sliderval){
	if (ignoreBoxSliderEvents) return;
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake X center");
	setXCenter(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
	
}
void FlowEventRouter::
guiSetYCenter(int sliderval){
	if (ignoreBoxSliderEvents) return;
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake Y center");
	setYCenter(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
	
}
void FlowEventRouter::
guiSetZCenter(int sliderval){
	if (ignoreBoxSliderEvents) return;
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake Z center");
	setZCenter(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
}
void FlowEventRouter::
guiSetXSize(int sliderval){
	if (ignoreBoxSliderEvents) return;
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake X size");
	setXSize(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
	
}
void FlowEventRouter::
guiSetYSize(int sliderval){
	if (ignoreBoxSliderEvents) return;
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake Y size");
	setYSize(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
	
}
void FlowEventRouter::
guiSetZSize(int sliderval){
	if (ignoreBoxSliderEvents) return;
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake Z size");
	setZSize(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
}
void FlowEventRouter::
guiSetAutoRefresh(bool autoOn){
	
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	//For our purposes here we consider all frame numbers in session:
	//Check if it's a change
	if (autoOn == fParams->refreshIsAuto()) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "toggle auto flow refresh");
	fParams->setAutoRefresh(autoOn);
	QPalette pal(autoRefreshCheckbox->palette());
	if (autoOn) {
		pal.setColor(autoRefreshCheckbox->foregroundRole(),QColor(Qt::black));
		autoRefreshCheckbox->setPalette(pal);
		if (!autoRefreshCheckbox->isChecked()){
			autoRefreshCheckbox->setChecked(true);
		}
	}
	else {
		pal.setColor(autoRefreshCheckbox->foregroundRole(),QColor(Qt::red));
		autoRefreshCheckbox->setPalette(pal);
		if (autoRefreshCheckbox->isChecked()){
			autoRefreshCheckbox->setChecked(false);
		}
	}
	
	PanelCommand::captureEnd(cmd, fParams);
	
	//If we are turning off autoRefresh, turn off all the needsRefresh flags 
	//It's possible that some frames are dirty and will not be refreshed, in which case
	//we will need to leave the refresh button enabled.
	bool needEnable = false;
	int maxFrame = DataStatus::getInstance()->getMaxTimestep();
	FlowRenderer* fRenderer = (FlowRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(fParams);
	if (!fRenderer) return;
	//see if we need to schedule a render (when autoOn is enabled)
	bool needRender = false;
	if(autoOn) { 
		//flowtype != 1: set the needsRefresh for all the dirty frames
		if (fParams->getFlowType()!= 1){
			for (int i = 0; i<=maxFrame; i++) {
				if (fRenderer->flowDataIsDirty(i)) {
					fRenderer->setNeedOfRefresh(i,true);
					needRender = true;
				}
			}
		} else {
			if (fRenderer->allFlowDataIsDirty()){
				fRenderer->setAllNeedRefresh(true);
				needRender = true;
			}
		}
	}
	//Also see if button needs to be enabled (at least one frame is dirty)
	if (!autoOn){
		needEnable = fRenderer->setDirtyNeedsRefresh(fParams);
	}
	//If we are turning it on, 
	//we may need to schedule a render.  
	refreshButton->setEnabled(!autoOn && needEnable);
	if (needRender) VizWinMgr::getInstance()->refreshFlow(fParams);
}

void FlowEventRouter::
guiSetFlowGeometry(int geomNum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set flow geometry type");
	fParams->setFlowGeometry(geomNum);
	PanelCommand::captureEnd(cmd, fParams);
	updateMapBounds(fParams);
	updateTab();
	update();
	//If you change the geometry, you do not need to recalculate the flow,
	//But you need to rerender
	VizWinMgr::getInstance()->refreshFlow(fParams);
}
void FlowEventRouter::
guiSetColorMapEntity( int entityNum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set flow colormap entity");
	//set the entity, put the entity bounds into the mapper function
	fParams->setColorMapEntity(entityNum);

	//Align the color part of the editor:
	fParams->setMinColorEditBound(fParams->getMinColorMapBound(),entityNum);
	fParams->setMaxColorEditBound(fParams->getMaxColorMapBound(),entityNum);

    // Disable the mapping frame if a "Constant" color is selected;
    colorMappingFrame->setEnabled(entityNum != 0);

	PanelCommand::captureEnd(cmd, fParams);
	updateMapBounds(fParams);
	
	updateTab();
	update();
	//We only need to redo the flowData if the entity is changing to "speed"
	//or if it changes to another variable:
	if(entityNum == 2) {
		VizWinMgr::getInstance()->setFlowDataDirty(fParams);
		if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	}
	else VizWinMgr::getInstance()->setFlowGraphicsDirty(fParams);
}
//When the user clicks "refresh", 
//This triggers a rebuilding of all dirty frames 
//And then a rerendering.
void FlowEventRouter::
refreshFlow(){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	confirmText(false);
	
	//set all dirty frames to need rendering.

	FlowRenderer* fRenderer = (FlowRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(fParams);
	if (!fRenderer) return;
	bool needToRender =false;
	if (fParams->getFlowType() == 1) {
		fRenderer->setAllNeedRefresh(true);
		needToRender = true;
	}
	else { //do it selectively for the dirty frames..
		int maxFrame = DataStatus::getInstance()->getMaxTimestep();
		int minFrame = DataStatus::getInstance()->getMinTimestep();
		for (int i = minFrame; i<= maxFrame; i++){
			if (fRenderer->flowDataIsDirty(i)){
				fRenderer->setNeedOfRefresh(i, true);
				needToRender = true;
			}
		}
	}
	refreshButton->setEnabled(false);
	if(needToRender) VizWinMgr::getInstance()->refreshFlow(fParams);
}
void FlowEventRouter::
guiToggleAutoScale(bool on){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	
	bool wasOn = fParams->isAutoScale();
	if (wasOn == on) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle auto scale steady field");
	fParams->setAutoScale(on);
	if (on) {
		autoscaleOffFrame->hide();
		autoscaleOnFrame->show();
	} else {
		autoscaleOnFrame->hide();
		autoscaleOffFrame->show();
	}
	//Refresh if we are turning it on...
	if (on) VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	PanelCommand::captureEnd(cmd, fParams);
	//This has no real effect until the next rendering
	updateTab();
}
void FlowEventRouter::
guiToggleDisplayLists(bool on){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	
	bool wasOn = fParams->usingDisplayLists();
	if (wasOn == on) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle use of display lists");
	fParams->enableDisplayLists(on);
	//Refresh ...
	VizWinMgr::getInstance()->setFlowDisplayListDirty(fParams);
	PanelCommand::captureEnd(cmd, fParams);
	//This has no real effect until the next rendering
	updateTab();
}
void FlowEventRouter::
guiToggleTimestepSample(bool on){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle use of timestep sample list");
	fParams->setTimestepSampleList(on);
	timestepSampleTable1->setEnabled(on);
	
	timesampleStartEdit1->setEnabled(!on);
	timesampleEndEdit1->setEnabled(!on);
	timesampleIncrementEdit1->setEnabled(!on);

	
	
	deleteSampleButton1->setEnabled(on);
	
	addSampleButton1->setEnabled(on);
	PanelCommand::captureEnd(cmd, fParams);


	//This has no real effect until the next rendering
	updateTab();
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
	
void FlowEventRouter::
guiEditSeedList(){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "edit seed list");
	SeedListEditor sle(fParams->getNumListSeedPoints(), fParams);
	if (!sle.exec()){
		delete cmd;
		return;
	}
	if (!fParams->rakeEnabled()) updateTab();
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetOpacMapEntity( int entityNum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set flow opacity map entity");
	//Change the entity, putting entity bounds into mapperFunction
	fParams->setOpacMapEntity(entityNum);
	//Align the opacity part of the editor
	fParams->setMinOpacEditBound(fParams->getMinOpacMapBound(),entityNum);
	fParams->setMaxOpacEditBound(fParams->getMaxOpacMapBound(),entityNum);

    // Disable the mapping frame if a "Constant" color is selected;
    opacityMappingFrame->setEnabled(entityNum != 0);

	PanelCommand::captureEnd(cmd, fParams);
	updateMapBounds(fParams);
	updateTab();
	update();
	//We only need to redo the flowData if the entity is changing to "speed"
	if(entityNum == 2) {
		if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
		VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	}
	else VizWinMgr::getInstance()->setFlowGraphicsDirty(fParams);
}

//Change mouse mode to specified value
//0,1,2 correspond to edit, zoom, pan
void FlowEventRouter::
guiSetEditMode(bool mode){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "set edit/navigate mode");
	fParams->setEditMode(mode);
	PanelCommand::captureEnd(cmd, fParams);
}
void FlowEventRouter::
guiSetFLAOption(int option){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "set Field Line Advection ordering");
	fParams->setFLAAdvectBeforePrioritize(option == 1);
	if (option == 0) flaSamplesEdit->setText(QString::number(1));
	else flaSamplesEdit->setText(QString::number(fParams->getNumFLASamples()));
	guiSetTextChanged(false);
	flaSamplesEdit->setEnabled(option != 0);
	priorityFieldMinEdit->setEnabled(option == 0);
	priorityFieldMaxEdit->setEnabled(option == 0);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	PanelCommand::captureEnd(cmd, fParams);
}

//Respond to a change in mapper function (from color selection or mouse down/release events)
//These are just for undo/redo.  Also may need to update visualizer and/or editor
//
void FlowEventRouter::
guiStartChangeMapFcn(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
    savedCommand = PanelCommand::captureStart(fParams, qstr.toLatin1());
}
void FlowEventRouter::
guiEndChangeMapFcn(){
	if (!savedCommand) return;
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand::captureEnd(savedCommand,fParams);
	VizWinMgr::getInstance()->setFlowGraphicsDirty(fParams);
	savedCommand = 0;
}
// Load a (text) file of 4D (or 3D?) points, adding to the current seedList.  
//
void FlowEventRouter::guiLoadSeeds(){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "load seeds from file");
	QString filename = QFileDialog::getOpenFileName(this,
        	"Specify file name for loading list of seed points", 
		Session::getInstance()->getFlowDirectory().c_str(),
        	"Text files (*.txt)");
	//Check that user did specify a file:
	if (filename.isNull()) {
		delete cmd;
		return;
	}
	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	//Save the path for future flow I/O
	Session::getInstance()->setFlowDirectory((const char*)fileInfo->absolutePath().toAscii());
	
	//Open the file:
	FILE* seedFile = fopen((const char*)filename.toAscii(),"r");
	if (!seedFile){
		MessageReporter::errorMsg("Seed Load Error;\nUnable to open file %s",(const char*)filename.toAscii());
		delete cmd;
		return;
	}
	//Empty the seed list:
	fParams->emptySeedList();
	//Add each seed to the list:
	while (1){
		float inputVals[4];
		int numVals = fscanf(seedFile, "%g %g %g %g", inputVals,inputVals+1,
			inputVals+2,inputVals+3);
		if (numVals < 4) break;
		Point4 newPoint(inputVals);
		fParams->pushSeed(newPoint);
	}
	fclose(seedFile);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
//
//Save all the current seed points.  
//
void FlowEventRouter::saveSeeds(){
	confirmText(false);  
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	//Launch an open-file dialog
	
	 QString filename = QFileDialog::getSaveFileName(this,
        	"Specify (*.txt) file name for saving current seed points",
		Session::getInstance()->getFlowDirectory().c_str(),
        	"Text files (*.txt)");
	if (filename.isNull())
		 return;

	//If the file has no suffix, add .txt
	if (filename.indexOf(".") == -1){
		filename.append(".txt");
	}

	QFileInfo fileInfo(filename);
	if (fileInfo.exists()){
		int rc = QMessageBox::warning(0, "Seed Point File Exists.", QString("OK to replace seed point file \n%1 ?").arg(filename), QMessageBox::Ok, 
			QMessageBox::No);
		if (rc != QMessageBox::Ok) return;
	}
	//Extract the path, and the root name, from the returned string.
	//Save the path for future captures
	Session::getInstance()->setFlowDirectory((const char*)fileInfo.absolutePath().toAscii());
	
	
	//Open the save file:
	FILE* saveFile = fopen((const char*)filename.toAscii(),"w");
	if (!saveFile){
		MessageReporter::errorMsg("Seed Save Error;\nUnable to open file %s",(const char*)filename.toAscii());
		return;
	}
	float *seedPoints;
	int numSeeds;
	//If there's a rake have the flowParams generate the seeds
	if (fParams->rakeEnabled()){
		RegionParams* rParams = VizWinMgr::getActiveRegionParams();
		AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();
		seedPoints = fParams->getRakeSeeds(rParams, &numSeeds,aParams->getCurrentFrameNumber());
		if (!seedPoints || numSeeds <= 0){
			MessageReporter::errorMsg("Unable to generate rake seeds");
			return;
		}
		for (int j = 0; j< numSeeds; j++){
			int rc = fprintf(saveFile,"%8g %8g %8g %8g\n",
				seedPoints[4*j+0],seedPoints[4*j+1],seedPoints[4*j+2],seedPoints[4*j+3]);
			if (rc <= 0) {
				MessageReporter::errorMsg("Seed Save Error;\nError writing seed no. %d to file:\n%s",j,(const char*)filename.toAscii());
				break;
			}
		}
		delete seedPoints;
		fclose(saveFile);
		return;
	} else { //write the current seedList:
		std::vector<Point4>& seedList = fParams->getSeedPointList();
		for (int j = 0; j<seedList.size(); j++){
			int rc = fprintf(saveFile,"%8g %8g %8g %8g\n",
				seedList[j].getVal(0),seedList[j].getVal(1),
				seedList[j].getVal(2),seedList[j].getVal(3));
			if (rc <= 0) {
				MessageReporter::errorMsg("Seed Save Error;\nError writing seed no. %d to file:\n%s",j,(const char*)filename.toAscii());
				break;
			}
		}
		fclose(saveFile);
		return;
	}

	
}
// Save all the points of the current flow.
// Don't support undo/redo
// If flow is unsteady, save all the points of the pathlines, with their times.
// If flow is steady, or for field line advection,
//	just save the points of the streamlines for the current animation time.
// Include the current timestep 
//
void FlowEventRouter::saveFlowLines(){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	FlowRenderer* fRenderer = (FlowRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(fParams);
	if (!fRenderer){
		MessageReporter::errorMsg("Flow cannot be saved until \nrendering is enabled");
		return;
	}
	//Launch an open-file dialog
	
	 QString filename = QFileDialog::getSaveFileName(this,
        	"Specify file name for saving current flow lines",
		Session::getInstance()->getFlowDirectory().c_str(),
        	"Text files (*.txt)");
	if (filename.isNull()){
		 return;
	}
	//If the file has no suffix, add .txt
	if (filename.indexOf(".") == -1){
		filename.append(".txt");
	}

	QFileInfo fileInfo(filename);
	if (fileInfo.exists()){
		int rc = QMessageBox::warning(0, "Flow File Exists", QString("OK to replace flow point file \n%1 ?").arg(filename), QMessageBox::Ok, 
			QMessageBox::No);
		if (rc != QMessageBox::Ok) return;
	}
	//Extract the path, and the root name, from the returned string.
	//Save the path for future captures
	Session::getInstance()->setFlowDirectory((const char*)fileInfo.absolutePath().toAscii());
	
	
	//Open the save file:
	FILE* saveFile = fopen((const char*)filename.toAscii(),"w");
	if (!saveFile){
		MessageReporter::errorMsg("Flow Save Error;\nUnable to open file:\n%s",(const char*)filename.toAscii());
		return;
	}
	//Refresh the flow, if necessary
	refreshFlow();
	
	//Get min/max timesteps from applicable animation params
	VizWinMgr* vizMgr = VizWinMgr::getInstance();

	//What's the current timestep?
	int vizNum = vizMgr->getActiveViz();
	AnimationParams* myAnimationParams = vizMgr->getAnimationParams(vizNum);
	int timeStep = myAnimationParams->getCurrentFrameNumber();
	assert (fRenderer);
	//
	//Rebuild if necessary:
	if (fRenderer->flowDataIsDirty(timeStep))
		if (!fRenderer->rebuildFlowData(timeStep)) {
			fParams->setBypass(timeStep);
			MessageReporter::errorMsg("Unable to build stream lines \nfor timestep %d",timeStep);
			return;
		}
	if (fParams->getFlowType() != 1){//steady flow, or field line advection
	
		FlowLineData* flowData = fRenderer->getSteadyCache(timeStep);
		
		for (int i = 0; i<flowData->getNumLines(); i++){
			float padValue[3];
			for (int k = 0; k<3; k++) { padValue[k] = END_FLOW_FLAG;}
			//Check for a stationary point.  
			int startIndex = flowData->getStartIndex(i);
			if (startIndex > 0 && flowData->getFlowPoint(i,startIndex-1)[0] == STATIONARY_STREAM_FLAG) {
				for (int k = 0; k<3; k++) {
					padValue[k] = flowData->getFlowPoint(i,startIndex)[k];
				}
			}
			
			//For each stream line write out "END_FLOW_FLAG" before and 
			//after the actual data:
			
			for (int j = 0; j < flowData->getStartIndex(i); j++){
				int rc = fprintf(saveFile,"%8g %8g %8g %8g\n",
					padValue[0],padValue[1],padValue[2],(float)timeStep);
				if (rc <= 0){
					MessageReporter::errorMsg("Unable to write stream line \nfor timestep %d",timeStep);
					return;
				}
			}
			for (int j = flowData->getStartIndex(i); j<= flowData->getEndIndex(i); j++){
				float* point = flowData->getFlowPoint(i,j);
				int rc = fprintf(saveFile,"%8g %8g %8g %8g\n",
					point[0],point[1],point[2],
					(float)timeStep);
				if (rc <= 0){
					MessageReporter::errorMsg("Unable to write stream line\nfor timestep %d",timeStep);
					return;
				}
			}
			//Check for stationary point at end:
			for (int k = 0; k<3; k++) { padValue[k] = END_FLOW_FLAG;}
			//Check for a stationary point.  
			int endIndex = flowData->getEndIndex(i);
			if (endIndex < flowData->getMaxPoints()-1 && flowData->getFlowPoint(i,endIndex+1)[0] == STATIONARY_STREAM_FLAG) {
				for (int k = 0; k<3; k++) {
					padValue[k] = flowData->getFlowPoint(i,endIndex)[k];
				}
			}
			//Pad to end:
			for (int j = flowData->getEndIndex(i)+1; j< flowData->getMaxPoints(); j++){
				int rc = fprintf(saveFile,"%8g %8g %8g %8g\n",
					padValue[0],padValue[1],padValue[2],(float)timeStep);
				if (rc <= 0){
					MessageReporter::errorMsg("Unable to write stream line\nfor timestep %d",timeStep);
					return;
				}
			}
			
		}
		//Done with writing flow lines:
		fclose(saveFile);
		return;
	} else {
		//Write out the unsteady flow lines.
		//It's like steady flow, but we use a pathlinedata, and write the
		//times as we go.  Don't need to check for stationary points.

		PathLineData* pathData = fRenderer->getUnsteadyCache();
		for (int i = 0; i<pathData->getNumLines(); i++){
			//For each path line write out "END_FLOW_FLAG" before and 
			//after the actual data:
			for (int j = 0; j< pathData->getStartIndex(i); j++){
				int rc = fprintf(saveFile,"%8g %8g %8g %8g\n",
					END_FLOW_FLAG,END_FLOW_FLAG,END_FLOW_FLAG,
					-1.f);
				if (rc <= 0){
					MessageReporter::errorMsg("Unable to write stream line\nfor timestep %d",timeStep);
					return;
				}
			}
			for (int j = pathData->getStartIndex(i); j<= pathData->getEndIndex(i); j++){
				float* point = pathData->getFlowPoint(i,j);
				float time = pathData->getTimeInPath(i,j);
				int rc = fprintf(saveFile,"%8g %8g %8g %8g\n",
					point[0],point[1],point[2],time);
				if (rc <= 0){
					MessageReporter::errorMsg("Unable to write stream line\nfor timestep %d",timeStep);
					return;
				}
			}
			for (int j = pathData->getEndIndex(i)+1; j< pathData->getMaxPoints(); j++){
				int rc = fprintf(saveFile,"%8g %8g %8g %8g\n",
					END_FLOW_FLAG,END_FLOW_FLAG,END_FLOW_FLAG, -1.f);
				if (rc <= 0){
					MessageReporter::errorMsg("Unable to write stream line\nfor timestep %d",timeStep);
					return;
				}
			}
			
		}
		//Done with writing flow lines:
		fclose(saveFile);
		return;
	}
	
}
//Save undo/redo state when user grabs a rake handle
//
void FlowEventRouter::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	savedCommand = PanelCommand::captureStart(fParams,  "slide rake handle");
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshFlow(fParams);
}
void FlowEventRouter::
captureMouseUp(){
	VizWinMgr* vwm = VizWinMgr::getInstance();
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	//Update the tab if it's in front:
	if(MainForm::getTabManager()->isFrontTab(this)) {
		
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (fParams == vwm->getFlowParams(viznum)))
			updateTab();
	}
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, fParams);
	//Set rake data dirty
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	vwm->setFlowDataDirty(fParams);
	savedCommand = 0;
	
	
}
//Set slider position, based on text change. 
//
void FlowEventRouter::
textToSlider(FlowParams* fParams,int coord, float newCenter, float newSize){
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	setIgnoreBoxSliderEvents(true);
	bool centerChanged = false;
	bool sizeChanged = false;
	DataStatus* ds = DataStatus::getInstance();
	const float* extents; 
	float regMin = newCenter - 0.5f*newSize;
	float regMax = newCenter  + 0.5f*newSize;
	

	if (ds->getDataMgr()){
		extents = DataStatus::getInstance()->getExtents();
		regMin = extents[coord];
		regMax = extents[coord+3];
		double newRegion[6];
		fParams->GetBox()->GetExtents(newRegion);

		if (newSize > (regMax-regMin)){
			newSize = regMax-regMin;
			sizeChanged = true;
		}
		if (newSize < 0.f) {
			newSize = 0.f;
			sizeChanged = true;
		}
		if (newCenter < regMin) {
			newCenter = regMin;
			centerChanged = true;
		}
		if (newCenter > regMax) {
			newCenter = regMax;
			centerChanged = true;
		}
		if ((newCenter - newSize*0.5f) < regMin){
			newCenter = regMin+ newSize*0.5f;
			centerChanged = true;
		}
		if ((newCenter + newSize*0.5f) > regMax){
			newCenter = regMax- newSize*0.5f;
			centerChanged = true;
		}
		//For small size make generator count 1 in that dimension
		if (newSize <= 0.f && !fParams->isRandom()){
			if (fParams->getNumGenerators(coord) != 1) {
				fParams->setNumGenerators(coord,1);
				switch(coord){
					case 0: xSeedEdit->setText("1"); break;
					case 1: ySeedEdit->setText("1"); break;
					case 2: zSeedEdit->setText("1"); break;
				}
			}
		}
		newRegion[coord] = newCenter - newSize*0.5;
		newRegion[coord+3] = newCenter + newSize*0.5;
		fParams->GetBox()->SetExtents(newRegion);
	}
	
	
	int sliderSize = (int)(0.5f+ 256.f*newSize/(regMax - regMin));
	int sliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	int oldSliderSize, oldSliderCenter;
	switch(coord) {
		case 0:
			oldSliderSize = xSizeSlider->value();
			oldSliderCenter = xCenterSlider->value();
			if (oldSliderSize != sliderSize)
				xSizeSlider->setValue(sliderSize);
			if(sizeChanged) xSizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				xCenterSlider->setValue(sliderCenter);
			if(centerChanged) xCenterEdit->setText(QString::number(newCenter));
			
			break;
		case 1:
			oldSliderSize = ySizeSlider->value();
			oldSliderCenter = yCenterSlider->value();
			if (oldSliderSize != sliderSize)
				ySizeSlider->setValue(sliderSize);
			if(sizeChanged) ySizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				yCenterSlider->setValue(sliderCenter);
			if(centerChanged) yCenterEdit->setText(QString::number(newCenter));
			
			break;
		case 2:
			oldSliderSize = zSizeSlider->value();
			oldSliderCenter = zCenterSlider->value();
			if (oldSliderSize != sliderSize)
				zSizeSlider->setValue(sliderSize);
			if(sizeChanged) zSizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				zCenterSlider->setValue(sliderCenter);
			if(centerChanged) zCenterEdit->setText(QString::number(newCenter));
			
			break;
		default:
			assert(0);
	}
	setIgnoreBoxSliderEvents(false);
	guiSetTextChanged(false);
	update();
	return;
	
}
//Set text when a slider changes.
//Move the center if the size is too big
//
void FlowEventRouter::
sliderToText(FlowParams* fParams,int coord, int slideCenter, int slideSize){
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	const float* extents; 
	float regMin = 0.f;
	float regMax = 1.f;
	DataStatus* ds = DataStatus::getInstance();
	if (ds){
		extents = ds->getExtents();
		regMin = extents[coord];
		regMax = extents[coord+3];
	}
	bool sliderChanged = false;
	
	float newSize = slideSize*(regMax-regMin)/256.f;
	float newCenter = regMin+ slideCenter*(regMax-regMin)/256.f;
	
	if (newCenter < regMin) {
		newCenter = regMin;
	}
	if (newCenter > regMax) {
		newCenter = regMax;
	}
	if ((newCenter - newSize*0.5f) < regMin){
		newCenter = regMin+ newSize*0.5f;
		sliderChanged = true;
	}
	if ((newCenter + newSize*0.5f) > regMax){
		newCenter = regMax- newSize*0.5f;
		sliderChanged = true;
	}
	double curBox[6];
	fParams->GetBox()->GetExtents(curBox);
	curBox[coord] = newCenter - newSize*0.5;
	curBox[coord+3] = newCenter + newSize*0.5;
	fParams->GetBox()->SetExtents(curBox);
	
	//For small size make generator count 1 in that dimension
	if (newSize <= 0.f && !fParams->isRandom()){
		if (fParams->getNumGenerators(coord) != 1) {
			fParams->setNumGenerators(coord,1);
			switch(coord){
				case 0: xSeedEdit->setText("1"); break;
				case 1: ySeedEdit->setText("1"); break;
				case 2: zSeedEdit->setText("1"); break;
			}
		}
	}
	int newSliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	//Always need to change text.  Possibly also change slider if it was moved
	switch(coord) {
		case 0:
			if (sliderChanged) 
				xCenterSlider->setValue(newSliderCenter);
			xSizeEdit->setText(QString::number(newSize));
			xCenterEdit->setText(QString::number(newCenter));
			break;
		case 1:
			if (sliderChanged) 
				yCenterSlider->setValue(newSliderCenter);
			ySizeEdit->setText(QString::number(newSize));
			yCenterEdit->setText(QString::number(newCenter));
			break;
		case 2:
			if (sliderChanged) 
				zCenterSlider->setValue(newSliderCenter);
			zSizeEdit->setText(QString::number(newSize));
			zCenterEdit->setText(QString::number(newCenter));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	update();
	//force a new render with new flow data
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	return;
}	
void FlowEventRouter::
showHideAppearance(){
	if (showAppearance) {
		showAppearance = false;
		showHideAppearanceButton->setText("Show Flow Appearance Settings");
	} else {
		showAppearance = true;
		showHideAppearanceButton->setText("Hide Flow Appearance Settings");
	}
	//Following HACK is needed to convince Qt to remove the extra space in the tab:
	updateTab();
	VizWinMgr::getInstance()->getTabManager()->toggleFrontTabs(Params::GetTypeFromTag(FlowParams::_flowParamsTag));
	updateTab();
}
void FlowEventRouter::
showHideSeeding(){
	if (showSeeding) {
		showSeeding = false;
		showHideSeedingButton->setText("Show Flow Seeding Settings");
	} else {
		showSeeding = true;
		showHideSeedingButton->setText("Hide Flow Seeding Settings");
	}
	//Following HACK is needed to convince Qt to remove the extra space in the tab:
	updateTab();
	VizWinMgr::getInstance()->getTabManager()->toggleFrontTabs(Params::GetTypeFromTag(FlowParams::_flowParamsTag));
	updateTab();
}
void FlowEventRouter::
showHideUnsteadyTime(){
	if (showUnsteadyTime) {
		showUnsteadyTime = false;
		showHideTimeButton->setText("Show Unsteady Flow Time Settings");
	} else {
		showUnsteadyTime = true;
		showHideTimeButton->setText("Hide Unsteady Flow Time Settings");
	}
	//Following HACK is needed to convince Qt to remove the extra space in the tab:
	updateTab();
	VizWinMgr::getInstance()->getTabManager()->toggleFrontTabs(Params::GetTypeFromTag(FlowParams::_flowParamsTag));
	updateTab();
}
/* Handle the change of status associated with change of enablement 
 * This is identical to code for dvrparams
 *  If we are enabling , only active one is created.
 * If we change from local to global, (no change in enablement) then new renderers are
 * created for every additional global window.  Similar for disable.
 * It can occur that both enablement and local/global change, if the local and global enablement
 * are different, during a local/global change
 * This assumes that the VizWinMgr already is set with the current (new) local/global
 * dvr settings.  
 * If the window is new, (i.e. we are just creating a new window, use: 
 * prevEnabled = false, wasLocal = isLocal = true,
 * even if the renderer is really global, since we don't want to affect other global renderers.
 */
void FlowEventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled,  bool newWindow){
	FlowParams* fParams = (FlowParams*)rParams;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	
	if (newWindow) {
		prevEnabled = false;
	}
	
	//The actual enabled state of "this" :
	bool nowEnabled = fParams->isEnabled();
	
	if (prevEnabled == nowEnabled) return;
	
	VizWin* viz = 0;
	if(fParams->getVizNum() >= 0){//Find the viz that this applies to:
		//Note that this is only for the cases below where one particular
		//visualizer is needed
		viz = vizWinMgr->getVizWin(fParams->getVizNum());
	} 
	//int minFrame = vizWinMgr->getActiveAnimationParams()->getStartFrameNumber();
	
	//Four cases to consider:
	//1.  change of local/global with unchanged disabled renderer; do nothing.
	// If change of local/global with enabled renderer, just force refresh:
	
	if (prevEnabled == nowEnabled) {
		if (!prevEnabled) return;
		if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
		vizWinMgr->setFlowDataDirty(fParams);
		return;
	}
	
	//2.  Change of disable->enable with unchanged local renderer.  Create a new renderer in active window.
	// Also applies to double change: disable->enable and local->global 
	// Also applies to disable->enable with global->local
	//3.  change of disable->enable with unchanged global renderer.  Create new renderers in all global windows, 
	//    including active window, but not if one is already enabled
	
	
	//5.  Change of enable->disable with unchanged global , disable all global renderers, provided the
	//   VizWinMgr already has the current local/global renderer settings
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	//  change of enable->disable with global->local.  (Must disable the local renderer)
	//  change of enable->disable with local->global (Must disable the local renderer)
	
	fParams->setEnabled(nowEnabled);

	
	if (nowEnabled && !prevEnabled ){//For case 2:  create a renderer in the active window:

		//This was intended to find if there was a field specification error.
		//But it will be tested later anyway, maybe it's not necessary?
		/*
		bool rc1=true, rc2=true;
		if (fParams->getFlowType() != 1)
			rc1 = fParams->validateSampling(minFrame, fParams->GetRefinementLevel(), fParams->getSteadyVarNums());
		if (fParams->getFlowType() != 0)
			rc2 = fParams->validateSampling(minFrame, fParams->GetRefinementLevel(), fParams->getUnsteadyVarNums());

		if (!rc1 || !rc2) { //Something was wrong; don't enable:
			MyBase::SetErrMsg(VAPOR_ERROR_FLOW,"Vector field setup error. Flow not enabled");
			fParams->setEnabled(false);
			updateTab();
			return;
		}
		*/
		FlowRenderer* myRenderer = new FlowRenderer (viz->getGLWindow(), fParams);
		viz->getGLWindow()->prependRenderer(fParams,myRenderer);
		myRenderer->setAllNeedRefresh(fParams->refreshIsAuto());
		if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
		vizWinMgr->setFlowDataDirty(fParams);
		return;
	}
	
	
	
	//case 6, disable 
	assert(prevEnabled && !nowEnabled); 
	viz->getGLWindow()->removeRenderer(fParams);

	return;
}
//When the center slider moves, set the seedBoxMin and seedBoxMax
void FlowEventRouter::
setXCenter(FlowParams* fParams, int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(fParams,0, sliderval, xSizeSlider->value());
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
setYCenter(FlowParams* fParams,int sliderval){
	sliderToText(fParams, 1, sliderval, ySizeSlider->value());
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
setZCenter(FlowParams* fParams,int sliderval){
	sliderToText(fParams,2, sliderval, zSizeSlider->value());
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
//Min and Max are center -+ size/2
//size is regionsize*sliderval/256
void FlowEventRouter::
setXSize(FlowParams* fParams,int sliderval){
	sliderToText(fParams,0, xCenterSlider->value(),sliderval);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
setYSize(FlowParams* fParams,int sliderval){
	sliderToText(fParams,1, yCenterSlider->value(),sliderval);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
setZSize(FlowParams* fParams,int sliderval){
	sliderToText(fParams,2, zCenterSlider->value(),sliderval);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}

/*
 * Method to be invoked after the user has moved the right or left bounds
 * (e.g. From the MapEditor. ) 
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void FlowEventRouter::
updateMapBounds(RenderParams* params){
	FlowParams* fParams = (FlowParams*)params;
	QString strn;
	MapperFunction* mpFunc = fParams->GetMapperFunc();
	if (mpFunc){
		minOpacmapEdit->setText(strn.setNum(mpFunc->getMinOpacMapValue(),'g',4));
		maxOpacmapEdit->setText(strn.setNum(mpFunc->getMaxOpacMapValue(),'g',4));
		minColormapEdit->setText(strn.setNum(mpFunc->getMinColorMapValue(),'g',4));
		maxColormapEdit->setText(strn.setNum(mpFunc->getMaxColorMapValue(),'g',4));
	} else {
		minOpacmapEdit->setText("0.0");
		maxOpacmapEdit->setText("1.0");
		minColormapEdit->setText("0.0");
		maxColormapEdit->setText("1.0");
	}
	setEditorDirty();

}
void FlowEventRouter::
setEditorDirty(RenderParams* p ){
	FlowParams* fp = (FlowParams*)p;
	if (!fp) fp = VizWinMgr::getInstance()->getActiveFlowParams();
	if(fp->GetMapperFunc())fp->GetMapperFunc()->setParams(fp);
    opacityMappingFrame->setMapperFunction(fp->GetMapperFunc());
    opacityMappingFrame->setVariableName(opacmapEntityCombo->currentText().toStdString());
    opacityMappingFrame->updateParams();
    
    colorMappingFrame->setMapperFunction(fp->GetMapperFunc());
    colorMappingFrame->setVariableName(colormapEntityCombo->currentText().toStdString());
    colorMappingFrame->updateParams();
}
//Make the new params current
void FlowEventRouter::
makeCurrent(Params* prevParams, Params* newParams, bool newWin, int instance,bool) {
	assert(instance >= 0);
	FlowParams* fParams = (FlowParams*)(newParams->deepCopy());
	int vizNum = fParams->getVizNum();
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(Params::GetNumParamsInstances(Params::_flowParamsTag,vizNum) == instance);
	
	VizWinMgr::getInstance()->setParams(vizNum, fParams, Params::GetTypeFromTag(Params::_flowParamsTag), instance);
	setEditorDirty();
	//if (fParams->GetMapperFunc())fParams->GetMapperFunc()->setParams(fParams);
	updateTab();
	
	//Need to create/destroy renderer if there's a change in local/global or enable/disable
	//or if the window is new
	//
	FlowParams* formerParams = (FlowParams*)prevParams;
	bool wasEnabled = false;
	if (formerParams) wasEnabled = formerParams->isEnabled();
	//Check if the enabled  changed:
	if (newWin || (formerParams->isEnabled() != fParams->isEnabled())){
		updateRenderer(fParams, wasEnabled,  newWin);
	}
	
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	VizWinMgr::getInstance()->getVizWin(vizNum)->updateGL();
}


void FlowEventRouter::cleanParams(Params* p) 
{
  opacityMappingFrame->setMapperFunction(NULL);
  opacityMappingFrame->setVariableName("");
  opacityMappingFrame->updateParams();

  colorMappingFrame->setMapperFunction(NULL);
  colorMappingFrame->setVariableName("");
  colorMappingFrame->updateParams();
}
//Fix for clean Windows scrolling:
void FlowEventRouter::refreshTab(){
	if(showAppearance){
		appearanceFrame->hide();
		appearanceFrame->show();
	}
}
//Check to see if all the flow variables are zero
//below the terrain.
bool FlowEventRouter::
flowVarsZeroBelow(){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	//check steady vars:
	if (fParams->getFlowType() != 1){
		for (int i = 0; i< 3; i++){
			int vnum = fParams->getSteadyVarNums()[i];
			if (vnum == 0) continue;
			if(DataStatus::isExtendedDown(vnum-1)) return false;
			if(DataStatus::getBelowValue(vnum-1) != 0.f) return false;
		}
	}
	if (fParams->getFlowType() != 0){
		for (int i = 0; i< 3; i++){
			int vnum = fParams->getUnsteadyVarNums()[i];
			if (vnum == 0) continue;
			if(DataStatus::isExtendedDown(vnum-1)) return false;
			if(DataStatus::getBelowValue(vnum-1) != 0.f) return false;
		}
	}
	return true;
}
//Workaround for Qt/Cocoa bug: postpone showing of OpenGL widgets 

#ifdef Darwin
void FlowEventRouter::paintEvent(QPaintEvent* ev){
	if (showAppearance){
		QScrollArea* sArea = (QScrollArea*)MainForm::getTabManager()->currentWidget();
 		if(!colorMapShown ){
			sArea->ensureWidgetVisible(colorMappingFrame);
			colorMappingFrame->show();
			colorMapShown = true;
			update();
			QWidget::paintEvent(ev);
			return;
		}
		if(!opacityMapShown ){
			sArea->ensureWidgetVisible(opacityMappingFrame);
			opacityMappingFrame->show();
			opacityMapShown = true;
			update();
		}
	}
	QWidget::paintEvent(ev);
}
#endif
QSize FlowEventRouter::sizeHint() const {
	FlowParams* fParams = (FlowParams*) VizWinMgr::getActiveFlowParams();
	if (!fParams) return QSize(460,1500);
	int vertsize = 555;//Full basic panel plus instance panel , with both steady and unsteady field frames
	//add showAppearance button, showSeeding button, frames
	vertsize += 100;
	switch (fParams->getFlowType()){
		case(0): 
			vertsize -= 130; //no unsteady field frame
			if (showSeeding) vertsize += (384 - 98); //seeding, without field line advection
			if (showAppearance) vertsize += 186;
			break;
		case(1): 
			vertsize -= 70; //no steady field frame
			if (showSeeding) vertsize += (384 - 98); //seeding, without field line advection
			vertsize += 50; //show unsteady times button + frame
			if (showUnsteadyTime) vertsize += 257;
			break;
		case(2): 
			if (showSeeding) vertsize += 384; //seeding, with field line advection
			vertsize += 50; //show unsteady times button + frame
			if (showUnsteadyTime) vertsize += 257;
			if (showAppearance) vertsize += 186;
			break;
	}
	if (showAppearance) vertsize += (665 - 186);  //Add in appearance panel minus steady appearance.
	//Mac and Linux have gui elements fatter than windows by about 10%
#ifndef WIN32
	vertsize = (int)(1.1*vertsize);
#endif

	return QSize(460,vertsize);
}