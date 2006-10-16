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
#include <qtooltip.h>
#include "regionparams.h"

#include "MappingFrame.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "flowrenderer.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "seedlisteditor.h"
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
#include "flowtab.h"
#include "vapor/Metadata.h"
#include "vapor/XmlNode.h"
#include "vapor/VDFIOBase.h"
#include "tabmanager.h"
#include "glutil.h"
#include "flowparams.h"
#include "floweventrouter.h"
#include "eventrouter.h"
#include "savetfdialog.h"
#include "loadtfdialog.h"

#include "VolumeRenderer.h"

using namespace VAPoR;


FlowEventRouter::FlowEventRouter(QWidget* parent,const char* name): FlowTab(parent, name), EventRouter(){
	myParamsType = Params::FlowParamsType;
	savedCommand = 0;
	flowDataChanged = false;
	mapBoundsChanged = false;
	flowGraphicsChanged = false;
	QToolTip::add(scaleFieldEdit,
		"This factor scales the vector field magnitude.\nIf too small, display will just show diamonds (fixed points).\nIf too large, flow will consist of straight lines due to flow rapidly exiting region.");
	
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
	connect (instanceCombo, SIGNAL(activated(int)), this, SLOT(guiChangeInstance(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (copyInstanceButton, SIGNAL(clicked()), this, SLOT(guiCopyInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (EnableDisable, SIGNAL(activated(int)), this, SLOT(setFlowEnabled(int)));
	connect (flowTypeCombo, SIGNAL( activated(int) ), this, SLOT( setFlowType(int) ) );
	connect (autoRefreshCheckbox, SIGNAL(toggled(bool)), this, SLOT (guiSetAutoRefresh(bool)));
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (xCoordVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetXComboVarNum(int)));
	connect (yCoordVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetYComboVarNum(int)));
	connect (zCoordVarCombo,SIGNAL(activated(int)), this, SLOT(guiSetZComboVarNum(int)));
	connect (randomCheckbox,SIGNAL(toggled(bool)),this, SLOT(guiSetRandom(bool)));
	connect (xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowXCenter()));
	connect (yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowYCenter()));
	connect (zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowZCenter()));
	connect (xSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowXSize()));
	connect (ySizeSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowYSize()));
	connect (zSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowZSize()));
	connect (geometrySamplesSlider, SIGNAL(sliderReleased()), this, SLOT(setFlowGeomSamples()));
	connect (geometryCombo, SIGNAL(activated(int)),SLOT(guiSetFlowGeometry(int)));
	connect (constantColorButton, SIGNAL(clicked()), this, SLOT(setFlowConstantColor()));
	connect (rakeOnRegionButton, SIGNAL(clicked()), this, SLOT(guiSetRakeToRegion()));
	connect (colormapEntityCombo,SIGNAL(activated(int)),SLOT(guiSetColorMapEntity(int)));
	connect (opacmapEntityCombo,SIGNAL(activated(int)),SLOT(guiSetOpacMapEntity(int)));
	connect (refreshButton,SIGNAL(clicked()), this, SLOT(guiRefreshFlow()));
	connect (periodicXCheck,SIGNAL(toggled(bool)), this, SLOT(guiCheckPeriodicX(bool)));
	connect (periodicYCheck,SIGNAL(toggled(bool)), this, SLOT(guiCheckPeriodicY(bool)));
	connect (periodicZCheck,SIGNAL(toggled(bool)), this, SLOT(guiCheckPeriodicZ(bool)));
	//Line edits.  Note that the textChanged may either affect the flow or the geometry
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
	connect (scaleFieldEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (scaleFieldEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (timesampleStartEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (timesampleStartEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (timesampleEndEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (timesampleEndEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (timesampleIncrementEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (timesampleIncrementEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
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
	connect (geometrySamplesEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (geometrySamplesEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (firstDisplayFrameEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (firstDisplayFrameEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (lastDisplayFrameEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (lastDisplayFrameEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (diameterEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (diameterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabGraphicsTextChanged(const QString&)));
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
	connect (seedListCheckbox, SIGNAL(toggled(bool)), this, SLOT(guiDoSeedList(bool)));
	connect (rakeCheckbox, SIGNAL(toggled(bool)), this, SLOT(guiDoRake(bool)));
	connect (opacityScaleSlider, SIGNAL(sliderReleased()), this, SLOT (flowOpacityScale()));
	
	connect (navigateButton, SIGNAL(toggled(bool)), this, SLOT(setFlowNavigateMode(bool)));
	connect (editButton, SIGNAL(toggled(bool)), this, SLOT(setFlowEditMode(bool)));

	connect (alignButton, SIGNAL(clicked()), this, SLOT(guiSetAligned()));
	connect (seedListLoadButton, SIGNAL(clicked()), this, SLOT(guiLoadSeeds()));
	connect (saveFlowButton, SIGNAL(clicked()), this, SLOT(guiSaveFlowLines()));
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

}

/*********************************************************************************
 * Slots associated with FlowTab:
 *********************************************************************************/


void FlowEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
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
		MapperFunction* mapperFunction = fParams->getMapperFunc();
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
	if (flowDataChanged){
		float integrationAccuracy = integrationAccuracyEdit->text().toFloat();
		if (integrationAccuracy < 0.f || integrationAccuracy > 1.f) {
			if (integrationAccuracy > 1.f) integrationAccuracy = 1.f;
			if (integrationAccuracy < 0.f) integrationAccuracy = 0.f;
			integrationAccuracyEdit->setText(QString::number(integrationAccuracy));
		}
		fParams->setIntegrationAccuracy(integrationAccuracy);
		float velocityScale = scaleFieldEdit->text().toFloat();
		if (velocityScale < 1.e-20f){
			velocityScale = 1.e-20f;
			scaleFieldEdit->setText(QString::number(velocityScale));
		}
		fParams->setVelocityScale(velocityScale);
		
		if (fParams->getFlowType() == 1){ //unsteady flow
			fParams->setTimeSamplingInterval(timesampleIncrementEdit->text().toInt());
			fParams->setTimeSamplingStart(timesampleStartEdit->text().toInt());
			fParams->setTimeSamplingEnd(timesampleEndEdit->text().toInt());
			int minFrame = VizWinMgr::getInstance()->getActiveAnimationParams()->getStartFrameNumber();
			if (!fParams->validateSampling(minFrame)){//did anything change?
				timesampleIncrementEdit->setText(QString::number(fParams->getTimeSamplingInterval()));
				timesampleStartEdit->setText(QString::number(fParams->getTimeSamplingStart()));
				timesampleEndEdit->setText(QString::number(fParams->getTimeSamplingEnd()));
			}
		}
		fParams->setRandomSeed(randomSeedEdit->text().toUInt());

		//Get slider positions from text boxes:
		
		float boxCtr = xCenterEdit->text().toFloat();
		float boxSize = xSizeEdit->text().toFloat();
		float seedBoxMin[3],seedBoxMax[3];
		seedBoxMin[0] = boxCtr - 0.5*boxSize;
		seedBoxMax[0] = boxCtr + 0.5*boxSize;
		textToSlider(fParams,0, boxCtr, boxSize);
		boxCtr = yCenterEdit->text().toFloat();
		boxSize = ySizeEdit->text().toFloat();
		seedBoxMin[1] = boxCtr - 0.5*boxSize;
		seedBoxMax[1] = boxCtr + 0.5*boxSize;
		textToSlider(fParams,1, boxCtr, boxSize);
		boxCtr = zCenterEdit->text().toFloat();
		boxSize = zSizeEdit->text().toFloat();
		seedBoxMin[2] = boxCtr - 0.5*boxSize;
		seedBoxMax[2] = boxCtr + 0.5*boxSize;
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
		
		int seedTimeStart = seedtimeStartEdit->text().toUInt();
		int seedTimeEnd = seedtimeEndEdit->text().toUInt(); 
		bool changed = false;
		VizWinMgr* vizMgr = VizWinMgr::getInstance();
		int minFrame = vizMgr->getAnimationParams(vizMgr->getActiveViz())->getStartFrameNumber();
		int maxFrame = fParams->getMaxFrame();
		if (seedTimeStart < minFrame) {seedTimeStart = minFrame; changed = true;}
		if (seedTimeEnd >= maxFrame) {seedTimeEnd = maxFrame-1; changed = true;}
		if (seedTimeEnd < seedTimeStart) {seedTimeEnd = seedTimeStart; changed = true;}
		if (changed){
			seedtimeStartEdit->setText(QString::number(seedTimeStart));
			seedtimeEndEdit->setText(QString::number(seedTimeEnd));
		}
		fParams->setSeedTimeStart(seedTimeStart);
		fParams->setSeedTimeEnd(seedTimeEnd);
		

		int seedTimeIncrement = seedtimeIncrementEdit->text().toUInt();
		if (seedTimeIncrement < 1) seedTimeIncrement = 1;
		fParams->setSeedTimeIncrement(seedTimeIncrement);
		int lastDisplayFrame = lastDisplayFrameEdit->text().toInt();
		int firstDisplayFrame = firstDisplayFrameEdit->text().toInt();
		//Make sure that steady flow has non-positive firstDisplayAge
		if (firstDisplayFrame > 0 && fParams->flowIsSteady()){
			firstDisplayFrame = 0;
			firstDisplayFrameEdit->setText(QString::number(firstDisplayFrame));
		}
		
		//Make sure at least one frame is displayed.
		//
		if (firstDisplayFrame >= lastDisplayFrame) {
			lastDisplayFrame = firstDisplayFrame+1;
			lastDisplayFrameEdit->setText(QString::number(lastDisplayFrame));
		}
		fParams->setFirstDisplayFrame(firstDisplayFrame);
		fParams->setLastDisplayFrame(lastDisplayFrame);
		int objectsPerFlowline = geometrySamplesEdit->text().toInt();
		if (objectsPerFlowline < 1 || objectsPerFlowline > 1000) {
			objectsPerFlowline = (lastDisplayFrame-firstDisplayFrame);
			geometrySamplesEdit->setText(QString::number(objectsPerFlowline));
		}
		geometrySamplesSlider->setValue((int)(256.0*log10((float)objectsPerFlowline)*0.33333));
		fParams->setObjectsPerFlowline(objectsPerFlowline);
		
	}
	if (flowGraphicsChanged){
		//change the parameters.  
		float shapeDiameter = diameterEdit->text().toFloat();
		if (shapeDiameter < 0.f) {
			shapeDiameter = 0.f;
			diameterEdit->setText(QString::number(shapeDiameter));
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
		fParams->setConstantOpacity(constantOpacity);
	}
	
	guiSetTextChanged(false);
	//If the data changed, need to setFlowDataDirty; otherwise
	//just need to setFlowMappingDirty.
	mapBoundsChanged = false;
	flowGraphicsChanged = false;
	if(flowDataChanged){
		flowDataChanged = false;
		if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
		VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	} else {
		VizWinMgr::getInstance()->setFlowGraphicsDirty(fParams);
	}

	PanelCommand::captureEnd(cmd, fParams);
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
void FlowEventRouter::guiCopyInstance(){
	performGuiCopyInstance();
}

//There are text changed events for flow (requiring rebuilding flow data),
//for graphics (requiring regenerating flow graphics), and
//for dataRange (requiring change of data mapping range, hence regenerating flow graphics)
void FlowEventRouter::setFlowTabFlowTextChanged(const QString&){
	setFlowDataChanged(true);
	guiSetTextChanged(true);
}

/*
 * Respond to a slider release
 */
void FlowEventRouter::
flowOpacityScale() {
	guiSetOpacityScale(opacityScaleSlider->value());
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
setFlowEnabled(int val){
	//If there's no window, or no datamgr, ignore this.
	
	if (Session::getInstance()->getDataMgr() == 0) {
		EnableDisable->setCurrentItem(0);
		return;
	}
	FlowParams* myFlowParams = VizWinMgr::getActiveFlowParams();
	//Make sure this is a change:
	if (myFlowParams->isEnabled() == (val==1) ) return;
	guiSetEnabled(val==1);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(myFlowParams, !val, false);
}



void FlowEventRouter::
setFlowType(int typenum){
	guiSetFlowType(typenum);
	//Activate/deactivate components associated with flow type:
	if(typenum == 0){//steady:
		seedtimeStartEdit->setEnabled(false);
		seedtimeEndEdit->setEnabled(false);
		seedtimeIncrementEdit->setEnabled(false);
		timesampleStartEdit->setEnabled(false);
		timesampleEndEdit->setEnabled(false);
		timesampleIncrementEdit->setEnabled(false);
	} else {//unsteady:
		seedtimeStartEdit->setEnabled(true);
		seedtimeEndEdit->setEnabled(true);
		seedtimeIncrementEdit->setEnabled(true);
		timesampleStartEdit->setEnabled(true);
		timesampleEndEdit->setEnabled(true);
		timesampleIncrementEdit->setEnabled(true);
	}
}




void FlowEventRouter::
setFlowXCenter(){
	guiSetXCenter(
		xCenterSlider->value());
}
void FlowEventRouter::
setFlowYCenter(){
	guiSetYCenter(
		yCenterSlider->value());
}
void FlowEventRouter::
setFlowZCenter(){
	guiSetZCenter(
		zCenterSlider->value());
}
void FlowEventRouter::
setFlowXSize(){
	guiSetXSize(
		xSizeSlider->value());
}
void FlowEventRouter::
setFlowYSize(){
	guiSetYSize(
		ySizeSlider->value());
}
void FlowEventRouter::
setFlowZSize(){
	guiSetZSize(
		zSizeSlider->value());
}
void FlowEventRouter::
setFlowGeomSamples(){
	int sliderPos = geometrySamplesSlider->value();
	guiSetGeomSamples(sliderPos);
}
/*
 * Respond to user clicking the color button
 */
void FlowEventRouter::
setFlowConstantColor(){
	
	//Bring up a color selector dialog:
	QColor newColor = QColorDialog::getColor(constantColorButton->paletteBackgroundColor(), this, "Constant Color Selection");
	//Set button color
	constantColorButton->setPaletteBackgroundColor(newColor);
	//Set parameter value of the appropriate parameter set:
	guiSetConstantColor(newColor);
}




void FlowEventRouter::
setFlowEditMode(bool mode){
	navigateButton->setOn(!mode);
	guiSetEditMode(mode);
}
void FlowEventRouter::
setFlowNavigateMode(bool mode){
	editButton->setOn(!mode);
	guiSetEditMode(!mode);
}


//Insert values from params into tab panel.
//This is called whenever the tab is displayed.
//
void FlowEventRouter::updateTab(){
	FlowParams* fParams = (FlowParams*) VizWinMgr::getActiveFlowParams();
	Session::getInstance()->blockRecording();

    opacityMappingFrame->setMapperFunction(fParams->getMapperFunc());
    colorMappingFrame->setMapperFunction(fParams->getMapperFunc());

    opacityMappingFrame->setVariableName(opacmapEntityCombo->currentText().latin1());
    colorMappingFrame->setVariableName(colormapEntityCombo->currentText().latin1());

	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int instCount = vizMgr->getNumFlowInstances(winnum);
	if(instCount != instanceCombo->count()){
		instanceCombo->clear();
		instanceCombo->setMaxCount(instCount);
		for (int i = 0; i<instCount; i++){
			instanceCombo->insertItem(QString::number(i), i);
		}
	}
	instanceCombo->setCurrentItem(vizMgr->getCurrentFlowInstIndex(winnum));
	deleteInstanceButton->setEnabled(vizMgr->getNumFlowInstances(winnum) > 1);
	

	//Get the region corners from the current applicable region panel,
	//or the global one:
	
	EnableDisable->setCurrentItem((fParams->isEnabled()) ? 1 : 0);
	
	int flowType = fParams->flowIsSteady()? 0 : 1 ;
	flowTypeCombo->setCurrentItem(flowType);
	
	refinementCombo->setCurrentItem(fParams->getNumRefinements());

	float sliderVal = fParams->getOpacityScale();
	QToolTip::add(opacityScaleSlider,"Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	sliderVal = 256.f*(1.f -sliderVal);
	opacityScaleSlider->setValue((int) sliderVal);
	
	//Determine if the renderer dirty flag is set:

	refreshButton->setEnabled(!fParams->refreshIsAuto() && VizWinMgr::getInstance()->flowDataIsDirty(fParams));
	autoRefreshCheckbox->setChecked(fParams->refreshIsAuto());
	//Always allow at least 4 variables in combo:
	int numVars = fParams->getNumComboVariables();
	if (numVars < 4) numVars = 4;
	
	xCoordVarCombo->setCurrentItem(fParams->getComboVarnum(0));
	yCoordVarCombo->setCurrentItem(fParams->getComboVarnum(1));
	zCoordVarCombo->setCurrentItem(fParams->getComboVarnum(2));
	
	periodicXCheck->setChecked(fParams->getPeriodicDim(0));
	periodicYCheck->setChecked(fParams->getPeriodicDim(1));
	periodicZCheck->setChecked(fParams->getPeriodicDim(2));
	
	timesampleStartEdit->setEnabled(flowType == 1);
	timesampleEndEdit->setEnabled(flowType == 1);
	timesampleIncrementEdit->setEnabled(flowType == 1);

	seedtimeIncrementEdit->setEnabled(flowType == 1);
	seedtimeEndEdit->setEnabled(flowType == 1);
	seedtimeStartEdit->setEnabled(flowType == 1);
	if (flowType == 0){
		displayIntervalLabel->setText("Begin/end display interval relative to seed time step");
	} else {
		displayIntervalLabel->setText("Begin/end display interval relative to current time step");
	}

	rakeCheckbox->setChecked(fParams->rakeEnabled());
	seedListCheckbox->setChecked(fParams->listEnabled());

	randomCheckbox->setChecked(fParams->isRandom());
	
	randomSeedEdit->setEnabled(fParams->isRandom());

	float seedBoxMin[3],seedBoxMax[3];
	fParams->getBox(seedBoxMin,seedBoxMax);
	for (int i = 0; i< 3; i++){
		textToSlider(fParams, i, (seedBoxMin[i]+seedBoxMax[i])*0.5f,
			seedBoxMax[i]-seedBoxMin[i]);
	}

	//Geometric parameters:
	geometryCombo->setCurrentItem(fParams->getShapeType());
	
	colormapEntityCombo->setCurrentItem(fParams->getColorMapEntityIndex());
	opacmapEntityCombo->setCurrentItem(fParams->getOpacMapEntityIndex());

    // Disable the mapping frame if a "Constant" color is selected;
    opacityMappingFrame->setEnabled(fParams->getOpacMapEntityIndex() != 0);
    colorMappingFrame->setEnabled(fParams->getColorMapEntityIndex() != 0);

	if (fParams->isRandom()){
		dimensionLabel->setEnabled(false);
		generatorCountEdit->setEnabled(true);
		xSeedEdit->setEnabled(false);
		ySeedEdit->setEnabled(false);
		zSeedEdit->setEnabled(false);
	} else {
		dimensionLabel->setEnabled(true);
		generatorCountEdit->setEnabled(false);
		xSeedEdit->setEnabled(true);
		ySeedEdit->setEnabled(true);
		zSeedEdit->setEnabled(true);
	}
	
	//Put all the setText messages here, so they won't trigger a textChanged message
	generatorCountEdit->setText(QString::number(fParams->getNumRakeSeedPoints()));
	
	xSeedEdit->setText(QString::number(fParams->getNumGenerators(0)));
	ySeedEdit->setText(QString::number(fParams->getNumGenerators(1)));
	zSeedEdit->setText(QString::number(fParams->getNumGenerators(2)));
	integrationAccuracyEdit->setText(QString::number(fParams->getIntegrationAccuracy()));
	scaleFieldEdit->setText(QString::number(fParams->getVelocityScale()));
	timesampleIncrementEdit->setText(QString::number(fParams->getTimeSamplingInterval()));
	timesampleStartEdit->setText(QString::number(fParams->getTimeSamplingStart()));
	timesampleEndEdit->setText(QString::number(fParams->getTimeSamplingEnd()));
	randomSeedEdit->setText(QString::number(fParams->getRandomSeed()));
	geometrySamplesEdit->setText(QString::number(fParams->getObjectsPerFlowline()));
	
	geometrySamplesSlider->setValue((int)(256.0*log10((float)fParams->getObjectsPerFlowline())*0.33333));
	firstDisplayFrameEdit->setText(QString::number(fParams->getFirstDisplayFrame()));
	lastDisplayFrameEdit->setText(QString::number(fParams->getLastDisplayFrame()));
	diameterEdit->setText(QString::number(fParams->getShapeDiameter()));
	arrowheadEdit->setText(QString::number(fParams->getArrowDiameter()));
	constantOpacityEdit->setText(QString::number(fParams->getConstantOpacity()));
	constantColorButton->setPaletteBackgroundColor(QColor(fParams->getConstantColor()));
	if (fParams->getMapperFunc()){
		minColormapEdit->setText(QString::number(fParams->getMapperFunc()->getMinColorMapValue()));
		maxColormapEdit->setText(QString::number(fParams->getMapperFunc()->getMaxColorMapValue()));
		minOpacmapEdit->setText(QString::number(fParams->getMapperFunc()->getMinOpacMapValue()));
		maxOpacmapEdit->setText(QString::number(fParams->getMapperFunc()->getMaxOpacMapValue()));
	}
	
	
	xSizeEdit->setText(QString::number(seedBoxMax[0]-seedBoxMin[0],'g', 4));
	xCenterEdit->setText(QString::number(0.5f*(seedBoxMax[0]+seedBoxMin[0]),'g',5));
	ySizeEdit->setText(QString::number(seedBoxMax[1]-seedBoxMin[1],'g', 4));
	yCenterEdit->setText(QString::number(0.5f*(seedBoxMax[1]+seedBoxMin[1]),'g',5));
	zSizeEdit->setText(QString::number(seedBoxMax[2]-seedBoxMin[2],'g', 4));
	zCenterEdit->setText(QString::number(0.5f*(seedBoxMax[2]+seedBoxMin[2]),'g',5));
	seedtimeIncrementEdit->setText(QString::number(fParams->getSeedTimeIncrement()));
	seedtimeStartEdit->setText(QString::number(fParams->getSeedTimeStart()));
	seedtimeEndEdit->setText(QString::number(fParams->getSeedTimeEnd()));

	//Put the opacity and color bounds for the currently chosen mappings
	//These should be the actual range of the variables
	int var = fParams->getColorMapEntityIndex();
	int tstep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	minColorBound->setText(QString::number(fParams->minRange(var, tstep)));
	maxColorBound->setText(QString::number(fParams->maxRange(var, tstep)));
	var = fParams->getOpacMapEntityIndex();
	minOpacityBound->setText(QString::number(fParams->minRange(var, tstep)));
	maxOpacityBound->setText(QString::number(fParams->maxRange(var, tstep)));
	
	guiSetTextChanged(false);

	Session::getInstance()->unblockRecording();
	
	VizWinMgr::getInstance()->getTabManager()->update();
}




//Reinitialize Flow tab settings, session has changed.
//Note that this is called after the globalFlowParams are set up, but before
//any of the localFlowParams are setup.
void FlowEventRouter::
reinitTab(bool doOverride){
	flowDataChanged = false;
	mapBoundsChanged = false;
	flowGraphicsChanged = false;
	int maxNumRefinements = DataStatus::getInstance()->getNumTransforms();
	//Set up the refinementCombo
	refinementCombo->setMaxCount(maxNumRefinements+1);
	refinementCombo->clear();
	for (int i = 0; i<= maxNumRefinements; i++){
		refinementCombo->insertItem(QString::number(i));
	}
	
	int newNumComboVariables = DataStatus::getInstance()->getNumMetadataVariables();
	//Set up the combo 
	
	xCoordVarCombo->clear();
	xCoordVarCombo->setMaxCount(newNumComboVariables);
	yCoordVarCombo->clear();
	yCoordVarCombo->setMaxCount(newNumComboVariables);
	zCoordVarCombo->clear();
	zCoordVarCombo->setMaxCount(newNumComboVariables);
	for (int i = 0; i< newNumComboVariables; i++){
		const std::string& s = DataStatus::getInstance()->getMetadataVarName(i);
		//Direct conversion of std::string& to QString doesn't seem to work
		//Maybe std was not enabled when QT was built?
		const QString& text = QString(s.c_str());
		xCoordVarCombo->insertItem(text);
		yCoordVarCombo->insertItem(text);
		zCoordVarCombo->insertItem(text);
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
		colorMapEntity.push_back(DataStatus::getInstance()->getMetadataVarName(i));
		opacMapEntity.push_back(DataStatus::getInstance()->getMetadataVarName(i));
	}
	//Set up the color, opac map entity combos:
	
	
	colormapEntityCombo->clear();
	for (int i = 0; i< (int)colorMapEntity.size(); i++){
		colormapEntityCombo->insertItem(QString(colorMapEntity[i].c_str()));
	}
	opacmapEntityCombo->clear();
	for (int i = 0; i< (int)colorMapEntity.size(); i++){
		opacmapEntityCombo->insertItem(QString(opacMapEntity[i].c_str()));
	}
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
	fParams->getBox(seedBoxMin, seedBoxMax);
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
	fParams->setBox(seedBoxMin, seedBoxMax);
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
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	VizWinMgr::getInstance()->refreshFlow(fParams);
}
//Turn on/off the rake and the seedlist:
void FlowEventRouter::guiDoRake(bool isOn){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	bool doRake = fParams->rakeEnabled();
	if (isOn == doRake) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle rake on/off");
	fParams->enableRake(isOn);
	PanelCommand::captureEnd(cmd, fParams);
	//If there's a change, need to set flags dirty
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	VizWinMgr::getInstance()->refreshFlow(fParams);
}

//Turn on/off the rake and the seedlist:
void FlowEventRouter::guiDoSeedList(bool isOn){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	bool doSeedList = fParams->listEnabled();
	if (isOn == doSeedList) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle seed list on/off");
	fParams->enableList(isOn);
	PanelCommand::captureEnd(cmd, fParams);
	//Invalidate just the list data
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	VizWinMgr::getInstance()->refreshFlow(fParams);
}

void FlowEventRouter::
guiSetEnabled(bool on){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	if (on == fParams->isEnabled()) return;
	confirmText(false);
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int inst = vizMgr->getActiveInstanceIndex(myParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "enable/disable flow render",inst);
	fParams->setEnabled(on);
	PanelCommand::captureEnd(cmd, fParams);
}
//Respond to a change in opacity scale factor
void FlowEventRouter::
guiSetOpacityScale(int val){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "modify opacity scale slider");
	fParams->setOpacityScale(((float)(256-val))/256.f);
	float sliderVal = fParams->getOpacityScale();
	QToolTip::add(opacityScaleSlider,"Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);

	VizWinMgr::getInstance()->setFlowGraphicsDirty(fParams);

	PanelCommand::captureEnd(cmd,fParams);
}
//Make rake match region
void FlowEventRouter::
guiSetRakeToRegion(){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "move rake to region");
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	float seedBoxMin[3],seedBoxMax[3];
	for (int i = 0; i< 3; i++){
		seedBoxMin[i] = rParams->getRegionMin(i);
		seedBoxMax[i] = rParams->getRegionMax(i);
	}
	fParams->setBox(seedBoxMin,seedBoxMax);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	updateTab();
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	
}
void FlowEventRouter::
guiSetFlowType(int typenum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
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
		displayIntervalLabel->setText("Begin/end display interval relative to seed time step");
	} else {
		displayIntervalLabel->setText("Begin/end display interval relative to current time step");
	}
	PanelCommand::captureEnd(cmd, fParams);
	
}
void FlowEventRouter::
guiSetNumRefinements(int n){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	int newNumTrans = ((RegionParams*)(VizWinMgr::getActiveRegionParams()))->validateNumTrans(n);
	if (newNumTrans != n) {
		MessageReporter::warningMsg("%s","Invalid number of Refinements for current region, data cache size");
		refinementCombo->setCurrentItem(newNumTrans);
	}
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "set number Refinements in Flow data");
	fParams->setNumRefinements(newNumTrans);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	
}
void FlowEventRouter::
guiSetXComboVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set X field variable");
	fParams->setComboVarnum(0,varnum);
	fParams->setXVarNum(DataStatus::getInstance()->mapMetadataToRealVarNum(varnum));
	PanelCommand::captureEnd(cmd, fParams);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}
void FlowEventRouter::
guiSetYComboVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set Y field variable");
	fParams->setComboVarnum(1,varnum);
	fParams->setYVarNum(DataStatus::getInstance()->mapMetadataToRealVarNum(varnum));
	PanelCommand::captureEnd(cmd, fParams);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);

}
void FlowEventRouter::
guiSetZComboVarNum(int varnum){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set Z field variable");
	fParams->setComboVarnum(2,varnum);
	fParams->setZVarNum(DataStatus::getInstance()->mapMetadataToRealVarNum(varnum));
	PanelCommand::captureEnd(cmd, fParams);
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
}
//Slider sets the geometry sampling rate, between 0.01 and 100.0
// value is 0.01*10**(4s)  where s is between 0 and 1
void FlowEventRouter::
guiSetGeomSamples(int sliderPos){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "set geometry sampling rate");
	float s = ((float)(sliderPos))/256.f;
	fParams->setObjectsPerFlowline((int)pow(10.f,3.f*s));
	
	geometrySamplesEdit->setText(QString::number(fParams->getObjectsPerFlowline()));
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}

void FlowEventRouter::
guiSetRandom(bool rand){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle random setting");
	fParams->setRandom(rand);
	generatorCountEdit->setText(QString::number(fParams->getNumRakeSeedPoints()));
	if (rand){
		dimensionLabel->setEnabled(false);
		generatorCountEdit->setEnabled(true);
		xSeedEdit->setEnabled(false);
		ySeedEdit->setEnabled(false);
		zSeedEdit->setEnabled(false);
	} else {
		dimensionLabel->setEnabled(true);
		generatorCountEdit->setEnabled(false);
		xSeedEdit->setEnabled(true);
		ySeedEdit->setEnabled(true);
		zSeedEdit->setEnabled(true);
	}
	guiSetTextChanged(false);
	update();
	PanelCommand::captureEnd(cmd, fParams);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
}

void FlowEventRouter::
guiCheckPeriodicX(bool periodic){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle periodic X coords");
	fParams->setPeriodicDim(0,periodic);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	PanelCommand::captureEnd(cmd, fParams);
	
}
void FlowEventRouter::
guiCheckPeriodicY(bool periodic){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle periodic Y coords");
	fParams->setPeriodicDim(1,periodic);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	PanelCommand::captureEnd(cmd, fParams);
	
}
void FlowEventRouter::
guiCheckPeriodicZ(bool periodic){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "toggle periodic Z coords");
	fParams->setPeriodicDim(2,periodic);
	VizWinMgr::getInstance()->setFlowDataDirty(fParams);
	PanelCommand::captureEnd(cmd, fParams);
	
}

void FlowEventRouter::
guiSetXCenter(int sliderval){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake X center");
	setXCenter(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
	
}
void FlowEventRouter::
guiSetYCenter(int sliderval){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake Y center");
	setYCenter(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
	
}
void FlowEventRouter::
guiSetZCenter(int sliderval){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake Z center");
	setZCenter(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
}
void FlowEventRouter::
guiSetXSize(int sliderval){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake X size");
	setXSize(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
	
}
void FlowEventRouter::
guiSetYSize(int sliderval){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake Y size");
	setYSize(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
	
}
void FlowEventRouter::
guiSetZSize(int sliderval){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	PanelCommand* cmd = PanelCommand::captureStart(fParams,  "slide flow rake Z size");
	setZSize(fParams,sliderval);
	PanelCommand::captureEnd(cmd, fParams);
	if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	
}
void FlowEventRouter::
guiSetAutoRefresh(bool autoOn){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	//For our purposes here we consider all frame numbers in session:
	//Check if it's a change
	if (autoOn == fParams->refreshIsAuto()) return;
	PanelCommand* cmd = PanelCommand::captureStart(fParams, "toggle auto flow refresh");
	fParams->setAutoRefresh(autoOn);
	PanelCommand::captureEnd(cmd, fParams);
	
	//If we are turning off autoRefresh, turn off all the needsRefresh flags 
	//It's possible that some frames are dirty and will not be refreshed, in which case
	//we will need to leave the refresh button enabled.
	bool needEnable = false;
	int maxFrame = DataStatus::getInstance()->getMaxTimestep();
	FlowRenderer* fRenderer = (FlowRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(Params::FlowParamsType);
	if (!fRenderer) return;
	fRenderer->setAllNeedRefresh(autoOn);
	//See if button needs to be enabled (at least one frame is dirty)
	if (!autoOn){
		for (int i = 0; i<=maxFrame; i++) {
			if (fRenderer->flowDataIsDirty(i)) needEnable = true;
		}
	}
	//If we are turning it on, 
	//we may need to schedule a render.  
	bool madeDirty = false;
	if (autoOn){
		for (int i = 0; i<= maxFrame; i++){
			if (fRenderer->flowDataIsDirty(i)) {
				madeDirty = true;
			}
		}
	}

	
	refreshButton->setEnabled((!fParams->refreshIsAuto()) && needEnable);
	if (madeDirty) VizWinMgr::getInstance()->refreshFlow(fParams);
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
	if(entityNum == 2) {
		VizWinMgr::getInstance()->setFlowDataDirty(fParams);
		if (!fParams->refreshIsAuto()) refreshButton->setEnabled(true);
	}
	else VizWinMgr::getInstance()->setFlowGraphicsDirty(fParams);
}
//When the user clicks "refresh", 
//This triggers a rebuilding of current frame 
//And then a rerendering.
void FlowEventRouter::
guiRefreshFlow(){
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	confirmText(false);
	
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	//Determine if the renderer dirty flag is set:
	
	if (!vizWinMgr->flowDataIsDirty(fParams)) {
		refreshButton->setEnabled(false);
		return;
	}
	
	refreshButton->setEnabled(true);
	vizWinMgr->refreshFlow(fParams);
	
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
guiSetAligned(){
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
    savedCommand = PanelCommand::captureStart(fParams, qstr.latin1());
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
	QString filename = QFileDialog::getOpenFileName(
		Session::getInstance()->getFlowDirectory().c_str(),
        "Text files (*.txt)",
        this,
        "Load Flow Seeds Dialog",
        "Specify file name for loading list of seed points" );
	//Check that user did specify a file:
	if (filename.isNull()) {
		delete cmd;
		return;
	}
	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	//Save the path for future flow I/O
	Session::getInstance()->setFlowDirectory(fileInfo->dirPath(true).ascii());
	
	//Open the file:
	FILE* seedFile = fopen(filename.ascii(),"r");
	if (!seedFile){
		MessageReporter::errorMsg("Seed Load Error;\nUnable to open file %s",filename.ascii());
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
// Save all the points of the current flow.
// Don't support undo/redo
// If flow is unsteady, save all the points of all the pathlines, with their times.
// If flow is steady, just save the points of the streamlines for the current animation time,
// Include their timesteps (relative to the current animation timestep) 
//
void FlowEventRouter::guiSaveFlowLines(){
	confirmText(false);
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	//Launch an open-file dialog
	
	 QString filename = QFileDialog::getSaveFileName(
		Session::getInstance()->getFlowDirectory().c_str(),
        "Text files (*.txt)",
        this,
        "Save Flowlines Dialog",
        "Specify file name for saving current flow lines" );
	if (filename.isNull()){
		 return;
	}
	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	//Save the path for future captures
	Session::getInstance()->setFlowDirectory(fileInfo->dirPath(true).ascii());
	
	//If the file has no suffix, add .txt
	if (filename.find(".") == -1){
		filename.append(".txt");
	}
	//Open the save file:
	FILE* saveFile = fopen(filename.ascii(),"w");
	if (!saveFile){
		MessageReporter::errorMsg("Flow Save Error;\nUnable to open file %s",filename.ascii());
		return;
	}
	//Refresh the flow, if necessary
	guiRefreshFlow();
	FlowRenderer* fRenderer = (FlowRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(Params::FlowParamsType);
	//Get min/max timesteps from applicable animation params
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int minFrame = vizMgr->getAnimationParams(vizMgr->getActiveViz())->getStartFrameNumber();
	if (fParams->flowIsSteady()){
		//What's the current timestep?
		int vizNum = VizWinMgr::getInstance()->getActiveViz();
		AnimationParams* myAnimationParams = VizWinMgr::getInstance()->getAnimationParams(vizNum);
		int timeStep = myAnimationParams->getCurrentFrameNumber();
		assert (fRenderer);
		//
		//Save the steady flowlines resulting from the rake (in reverse order)
		//Get the rake flow data:
		if (fParams->rakeEnabled()){
			if (fRenderer->flowDataIsDirty(timeStep))
				fRenderer->rebuildFlowData(timeStep,true);
			float* flowDataArray = fRenderer->getFlowData(timeStep,true);
			assert(flowDataArray);
			int numSeedPoints = fRenderer->getNumRakeSeedPointsUsed();
			for (int i = numSeedPoints -1; i>=0; i--){
				if (!fParams->writeStreamline(saveFile,i,timeStep,flowDataArray)){
					MessageReporter::errorMsg("Flow Save Error;\nUnable to write data to %s",filename.ascii());
					return;
				}
			}
		}
		if (fParams->listEnabled()&&(fRenderer->getNumListSeedPointsUsed(timeStep) > 0)){
			if (fRenderer->flowDataIsDirty(timeStep))
				fRenderer->rebuildFlowData(timeStep,false);
			float* flowDataArray = fRenderer->getFlowData(timeStep, false);
			
			assert(flowDataArray);
			int numSeedPoints = fRenderer->getNumListSeedPointsUsed(timeStep);
			for (int i = 0; i<numSeedPoints; i++){
				if (!fParams->writeStreamline(saveFile,i,timeStep,flowDataArray)){
					MessageReporter::errorMsg("Flow Save Error;\nUnable to write data to %s",filename.ascii());
					return;
				}
			}
		}
	} else {
		//Write the (entire) set of pathlines to file.
		//Loop over all injections:
		int maxPoints = fParams->calcMaxPoints();
		if (fParams->rakeEnabled()){
			if (fRenderer->flowDataIsDirty(0))
				fRenderer->rebuildFlowData(0,true);
			float* flowDataArray = fRenderer->getFlowData(0, true);
			
			assert(flowDataArray);
			for (int injNum = 0; injNum < fParams->getNumInjections(); injNum++){
				
				int numSeedPoints = fRenderer->getNumRakeSeedPointsUsed();
				
				//Offset to start of the injection:
				float* flowDataPtr = flowDataArray + 3*injNum*numSeedPoints*maxPoints;
				for (int i = numSeedPoints -1; i>=0; i--){
					if (!fParams->writePathline(saveFile,i, minFrame, injNum,flowDataPtr)){
						MessageReporter::errorMsg("Flow Save Error;\nUnable to write data to %s",filename.ascii());
						return;
					}
				}
			}
		}
		if (fParams->listEnabled()&&(fRenderer->getNumListSeedPointsUsed(0) > 0)){
			if (fRenderer->flowDataIsDirty(0))
				fRenderer->rebuildFlowData(0,false);
			float* flowDataArray = fRenderer->getFlowData(0, false);
			
			assert(flowDataArray);
			for (int injNum = 0; injNum < fParams->getNumInjections(); injNum++){
				//This appears to be wrong!!
				int numSeedPoints = fRenderer->getNumListSeedPointsUsed(0);
				float* flowDataPtr = flowDataArray+ 3*injNum*numSeedPoints*maxPoints;
				for (int i = 0; i<numSeedPoints; i++){
					if (!fParams->writePathline(saveFile,i,minFrame, injNum,flowDataPtr)){
						MessageReporter::errorMsg("Flow Save Error;\nUnable to write data to %s",filename.ascii());
						return;
					}
				}
			}
		}
	}
	fclose(saveFile);
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
	if(MainForm::getInstance()->getTabManager()->isFrontTab(this)) {
		
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
	bool centerChanged = false;
	bool sizeChanged = false;
	DataStatus* ds = DataStatus::getInstance();
	const float* extents; 
	float regMin = 0.f;
	float regMax = 1.f;
	if (ds){
		extents = DataStatus::getInstance()->getExtents();
		regMin = extents[coord];
		regMax = extents[coord+3];
	}
	
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
	fParams->setSeedRegionMin(coord, newCenter - newSize*0.5f); 
	fParams->setSeedRegionMax(coord, newCenter + newSize*0.5f); 
	
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
	guiSetTextChanged(false);
	update();
	
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
	fParams->setSeedRegionMin(coord,newCenter - newSize*0.5f);
	fParams->setSeedRegionMax(coord,newCenter + newSize*0.5f);
	
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
	int minFrame = vizWinMgr->getActiveAnimationParams()->getStartFrameNumber();
	
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

		//First, make sure we have valid fielddata:
		fParams->validateSampling(minFrame);

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
	MapperFunction* mpFunc = fParams->getMapperFunc();
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
	if(fp->getMapperFunc())fp->getMapperFunc()->setParams(fp);
    opacityMappingFrame->setMapperFunction(fp->getMapperFunc());
    opacityMappingFrame->setVariableName(opacmapEntityCombo->currentText().latin1());
    opacityMappingFrame->update();
    
    colorMappingFrame->setMapperFunction(fp->getMapperFunc());
    colorMappingFrame->setVariableName(colormapEntityCombo->currentText().latin1());
    colorMappingFrame->update();
}
//Make the new params current
void FlowEventRouter::
makeCurrent(Params* prevParams, Params* newParams, bool newWin, int instance) {
	assert(instance >= 0);
	FlowParams* fParams = (FlowParams*)(newParams->deepCopy());
	int vizNum = fParams->getVizNum();
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumFlowInstances(vizNum) == instance);
	
	VizWinMgr::getInstance()->setParams(vizNum, fParams, Params::FlowParamsType, instance);
	setEditorDirty();
	//if (fParams->getMapperFunc())fParams->getMapperFunc()->setParams(fParams);
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
  opacityMappingFrame->update();

  colorMappingFrame->setMapperFunction(NULL);
  colorMappingFrame->setVariableName("");
  colorMappingFrame->update();
}
	
