//************************************************************************
//																		*
//		     Copyright (C)  2007										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isoeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2007
//
//	Description:	Implements the IsoEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the iso tab
//
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
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qlabel.h>

#include "instancetable.h"
#include "mappingframe.h"
#include "regionparams.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "isorenderer.h"
#include "DVRRayCaster.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "isotab.h"

#include <vapor/XmlNode.h>
#include "tabmanager.h"
#include "ParamsIso.h"
#include "isoeventrouter.h"
#include "animationeventrouter.h"
#include "eventrouter.h"



using namespace VAPoR;
const char* IsoEventRouter::webHelpText[] = 
{
	"Isosurface overview",
	"Renderer control",
	"Data accuracy control",
	"Controlling the Iso-value",
	"Isosurface color and transparency",
	"Isosurface quality",
	"<>"
};
const char* IsoEventRouter::webHelpURL[] =
{

	"http://www.vapor.ucar.edu/docs/vapor-gui-help/isosurfaces",
	"http://www.vapor.ucar.edu/docs/vapor-how-guide/renderer-instances",
	"http://www.vapor.ucar.edu/docs/vapor-how-guide/refinement-and-lod-control",
	"http://www.vapor.ucar.edu/docs/vapor-gui-general-guide/controlling-iso-value",
	"http://www.vapor.ucar.edu/docs/vapor-gui-general-guide/isosurface-color-and-transparency",
	"http://www.vapor.ucar.edu/docs/vapor-gui-general-guide/isosurface-quality",
};

IsoEventRouter::IsoEventRouter(QWidget* parent): QWidget(parent), Ui_IsoTab(),  EventRouter(){
	setupUi(this);
	
	myParamsBaseType = Params::GetTypeFromTag(Params::_isoParamsTag);
	fidelityButtons = 0;
	savedCommand = 0;  
	renderTextChanged = false;
	isoSelectionFrame->setOpacityMapping(true);
	isoSelectionFrame->setColorMapping(false);
	isoSelectionFrame->setIsoSlider(true);
	MessageReporter::infoMsg("IsoEventRouter::IsoEventRouter()");
	myWebHelpActions = makeWebHelpActions(webHelpText, webHelpURL);
#if defined(Darwin) && (QT_VERSION < QT_VERSION_CHECK(4,8,0))
	opacityMapShown = false;
	isoShown = false;
	transferFunctionFrame->hide();
	isoSelectionFrame->hide();
#endif
}


IsoEventRouter::~IsoEventRouter(){
	if (savedCommand) delete savedCommand;
}
/**********************************************************
 * Whenever a new Isotab is created it must be hooked up here
 ************************************************************/
void
IsoEventRouter::hookUpTab()
{
	
	
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (lodCombo,SIGNAL(activated(int)), this, SLOT(guiSetCompRatio(int)));
	connect (variableCombo, SIGNAL( activated(int) ), this, SLOT( guiSetComboVarNum(int) ) );
	connect (mapVariableCombo, SIGNAL( activated(int) ), this, SLOT( guiSetMapComboVarNum(int) ) );
	connect (numBitsCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumBits(int)));
	connect (lightingCheckbox, SIGNAL( toggled(bool) ), this, SLOT( guiSetLighting(bool) ) );
	connect (fidelityDefaultButton, SIGNAL(clicked()), this, SLOT(guiSetFidelityDefault()));
 
	//Line edits:
	connect (histoScaleEdit,SIGNAL(textChanged(const QString&)),this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (histoScaleEdit, SIGNAL(returnPressed() ), this, SLOT( isoReturnPressed()));
	connect (TFHistoScaleEdit,SIGNAL(textChanged(const QString&)),this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (TFHistoScaleEdit, SIGNAL(returnPressed() ), this, SLOT( isoReturnPressed()));
	connect (isoValueEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (isoValueEdit, SIGNAL(returnPressed()), this, SLOT( isoReturnPressed()));
	connect (leftHistoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (leftHistoEdit, SIGNAL(returnPressed()), this, SLOT(isoReturnPressed()));
	connect (rightHistoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (rightHistoEdit, SIGNAL(returnPressed()), this, SLOT(isoReturnPressed()));
	connect (leftMappingEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (rightMappingEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (leftMappingEdit, SIGNAL(returnPressed()), this, SLOT(isoReturnPressed()));
	connect (rightMappingEdit, SIGNAL(returnPressed()), this, SLOT(isoReturnPressed()));
	connect (selectedXEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (selectedXEdit, SIGNAL( returnPressed()), this, SLOT( isoReturnPressed()));
	connect (selectedYEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (selectedYEdit, SIGNAL(returnPressed()), this, SLOT( isoReturnPressed()));
	connect (selectedZEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (selectedZEdit, SIGNAL(returnPressed()), this, SLOT( isoReturnPressed()));
	connect (constantOpacityEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (constantOpacityEdit, SIGNAL(returnPressed()), this, SLOT( isoReturnPressed()));
	
	connect(newHistoButton, SIGNAL(clicked()), this, SLOT(refreshHisto()));
	connect(editButton, SIGNAL(toggled(bool)), this, SLOT(setIsoEditMode(bool)));
	connect(navigateButton, SIGNAL(toggled(bool)), this, SLOT(setIsoNavigateMode(bool)));
	connect (constantColorButton, SIGNAL(clicked()), this, SLOT(setConstantColor()));
	connect (passThruButton, SIGNAL(clicked()), this, SLOT(guiPassThruPoint()));
	connect (copyPointButton, SIGNAL(clicked()), this, SLOT(guiCopyProbePoint()));
	// Transfer function controls:
	connect (loadButton, SIGNAL(clicked()), this, SLOT(isoLoadTF()));
	connect (loadInstalledButton, SIGNAL(clicked()), this, SLOT(isoLoadInstalledTF()));
	connect (saveButton, SIGNAL(clicked()), this, SLOT(isoSaveTF()));
	
	connect (opacityScaleSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetOpacityScale(int)));
	connect (ColorBindButton, SIGNAL(pressed()), this, SLOT(guiBindColorToOpac()));
	connect (OpacityBindButton, SIGNAL(pressed()), this, SLOT(guiBindOpacToColor()));
	connect (TFnavigateButton, SIGNAL(toggled(bool)), this, SLOT(setTFNavigateMode(bool)));
	
	connect (TFeditButton, SIGNAL(toggled(bool)), this, SLOT(setTFEditMode(bool)));
	
	connect(TFalignButton, SIGNAL(clicked()), this, SLOT(guiSetTFAligned()));
	
	connect(TFHistoButton, SIGNAL(clicked()), this, SLOT(refreshTFHisto()));
	connect(TFeditButton, SIGNAL(toggled(bool)), 
            transferFunctionFrame, SLOT(setEditMode(bool)));

	connect(TFalignButton, SIGNAL(clicked()),
            transferFunctionFrame, SLOT(fitToView()));

    connect(transferFunctionFrame, SIGNAL(startChange(QString)), 
            this, SLOT(guiStartChangeMapFcn(QString)));

    connect(transferFunctionFrame, SIGNAL(endChange()),
            this, SLOT(guiEndChangeMapFcn()));

    connect(transferFunctionFrame, SIGNAL(canBindControlPoints(bool)),
            this, SLOT(setBindButtons(bool)));

	//Instance table stuff:
	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setIsoEnabled(bool,int)));
	
	// isoSelectionFrame controls:
	connect(editButton, SIGNAL(toggled(bool)), 
            isoSelectionFrame, SLOT(setEditMode(bool)));
	connect(alignButton, SIGNAL(clicked()), this, SLOT(guiSetIsoAligned()));
	connect(alignButton, SIGNAL(clicked()),
            isoSelectionFrame, SLOT(fitToView()));
    connect(isoSelectionFrame, SIGNAL(startChange(QString)), 
            this, SLOT(guiStartChangeIsoSelection(QString)));
    connect(isoSelectionFrame, SIGNAL(endChange()),
            this, SLOT(guiEndChangeIsoSelection()));
	connect (fitDataButton, SIGNAL(clicked()), this, SLOT(guiFitTFToData()));
	connect(colorInterpCheckbox,SIGNAL(toggled(bool)), this, SLOT(guiToggleColorInterpType(bool)));
}
//Insert values from params into tab panel
//
void IsoEventRouter::updateTab(){
	if(!MainForm::getTabManager()->isFrontTab(this)) return;
	MainForm::getInstance()->buildWebTabHelpMenu(myWebHelpActions);
	if (!isEnabled()) return;
	if (GLWindow::isRendering()) return;
	Session *session = Session::getInstance();
	
	session->blockRecording();

	if (DataStatus::getInstance()->getDataMgr()) instanceTable->setEnabled(true);
	else instanceTable->setEnabled(false);
	instanceTable->rebuild(this);

	VizWinMgr* vizMgr = VizWinMgr::getInstance();
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


	ParamsIso* isoParams = getActiveIsoParams();
	deleteInstanceButton->setEnabled(vizMgr->getNumIsoInstances(winnum) > 1);

	QString strn;
	const std::string& varname = isoParams->GetMapVariableName();
    int mapComboVarNum = 0;
	if (StrCmpNoCase(varname, "Constant") != 0) {
		//1 extra for constant
		mapComboVarNum = 1+DataStatus::getInstance()->getActiveVarNum3D(varname);
	}
		
	float sliderVal = isoParams->getOpacityScale();
	opacityScaleSlider->setToolTip("Opacity Scale Value = "+QString::number(sliderVal));
	opacityScaleSlider->setValue((int)(256*(1.-sliderVal)));
	mapVariableCombo->setCurrentIndex(mapComboVarNum);
	if (DataStatus::getInstance()->getDataMgr() && fidelityDefaultChanged){
		setupFidelity(3, fidelityLayout,fidelityBox, isoParams, false);
		connect(fidelityButtons,SIGNAL(buttonClicked(int)),this, SLOT(guiSetFidelity(int)));
		fidelityDefaultChanged = false;
	}
	if (DataStatus::getInstance()->getDataMgr())updateFidelity(fidelityBox,isoParams,lodCombo,refinementCombo);
	//setup the transfer function editor:
	if(mapComboVarNum > 0 && isoParams->GetMapperFunc()) {
		transferFunctionFrame->setMapperFunction(isoParams->GetMapperFunc());
		updateMapBounds(isoParams);
		TFInterpolator::type t = isoParams->GetMapperFunc()->colorInterpType();
		if (colorInterpCheckbox->isChecked() && t != TFInterpolator::discrete) 
			colorInterpCheckbox->setChecked(false);
		if (!colorInterpCheckbox->isChecked() && t == TFInterpolator::discrete) 
			colorInterpCheckbox->setChecked(true);
	}
	else transferFunctionFrame->setMapperFunction(0);

    transferFunctionFrame->updateParams();

	
    if (session->getDataMgr())
    {
      
      transferFunctionFrame->setVariableName(varname);
    }
    else
    {
      transferFunctionFrame->setVariableName("N/A");
	  Session::getInstance()->unblockRecording();
	  return;
    }
	
	numBitsCombo->setCurrentIndex((isoParams->GetNumBits())>>4);

	int comboVarNum = DataStatus::getInstance()->getActiveVarNum3D(
		isoParams->GetIsoVariableName());
	variableCombo->setCurrentIndex(comboVarNum);
	
	histoScaleEdit->setText(QString::number(isoParams->GetIsoHistoStretch()));
	TFHistoScaleEdit->setText(QString::number(isoParams->GetHistoStretch()));
	isoValueEdit->setText(QString::number(isoParams->GetIsoValue()));
	
	const vector<double>& coords = isoParams->GetSelectedPoint();
	const float* bnds = isoParams->GetHistoBounds();
	const float* clr = isoParams->GetConstantColor();
	leftHistoEdit->setText(QString::number(bnds[0]));
	rightHistoEdit->setText(QString::number(bnds[1]));
	constantOpacityEdit->setText(QString::number(clr[3]));
	selectedXEdit->setText(QString::number(coords[0]));
	selectedYEdit->setText(QString::number(coords[1]));
	selectedZEdit->setText(QString::number(coords[2]));
	QPalette pal = constantColorEdit->palette();
	pal.setColor(QPalette::Base,QColor((int)(.5+clr[0]*255.),(int)(.5+clr[1]*255.),(int)(.5+clr[2]*255.)));
	constantColorEdit->setPalette(pal);
	if(isoParams->GetIsoControl()){
		isoParams->GetIsoControl()->setParams(isoParams);
		isoSelectionFrame->setMapperFunction(isoParams->GetIsoControl());
	}
	
	
    isoSelectionFrame->setVariableName(isoParams->GetIsoVariableName());
	updateHistoBounds(isoParams);
	isoSelectionFrame->updateParams();


	float val = evaluateSelectedPoint();
	if (val != OUT_OF_BOUNDS)
		variableValueLabel->setText(QString::number(val));
	else variableValueLabel->setText("");
	passThruButton->setEnabled((val!= OUT_OF_BOUNDS) && isoParams->isEnabled());
	guiSetTextChanged(false);
	
	//Anything that can start an event should go after we turn off the textChanged flag:
    lightingCheckbox->setChecked(isoParams->GetNormalOnOff());
	update();
	guiSetTextChanged(false);
	
	renderTextChanged = false;
	Session::getInstance()->unblockRecording();
	vizMgr->getTabManager()->update();
}
//Update the params based on changes in text boxes
void IsoEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::_isoParamsTag);
	
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "edit Iso text");
	QString strn;
	
	iParams->SetIsoHistoStretch(histoScaleEdit->text().toFloat());
	iParams->SetHistoStretch(TFHistoScaleEdit->text().toFloat());
	float opac = constantOpacityEdit->text().toFloat();
	const float *clr = iParams->GetConstantColor();
	
	if (clr[3] != opac){
		float newColor[4];
		newColor[3] = opac;
		for (int i = 0; i< 3; i++) newColor[i] = clr[i];
		iParams->SetConstantColor(newColor);
	}
	float isoval = isoValueEdit->text().toFloat();
	if(iParams->GetIsoControl())iParams->SetIsoValue(isoval);
	float bnds[2];
	bnds[0] = leftHistoEdit->text().toFloat();
	bnds[1] = rightHistoEdit->text().toFloat();
	if (bnds[0] >= isoval || bnds[1] <= isoval){ //Reset bounds to include isoval
		float iwidth = bnds[1]-bnds[0];
		if (isoval < bnds[0]+0.01*iwidth) {
			bnds[0] = isoval - 0.01*iwidth;
			leftHistoEdit->setText(QString::number(bnds[0]));
		}
		if (isoval > bnds[1]-0.01*iwidth) {
			bnds[1] = isoval + 0.01*iwidth;
			rightHistoEdit->setText(QString::number(bnds[1]));
		}
	}

	iParams->SetHistoBounds(bnds);
	
	bnds[0] = leftMappingEdit->text().toFloat();
	bnds[1] = rightMappingEdit->text().toFloat();
	
	iParams->SetMapBounds(bnds);
	
	float coords[3];
	coords[0] = selectedXEdit->text().toFloat();
	coords[1] = selectedYEdit->text().toFloat();
	coords[2] = selectedZEdit->text().toFloat();
	iParams->SetSelectedPoint(coords);
	

	float val = evaluateSelectedPoint();
	if (val != OUT_OF_BOUNDS)
		variableValueLabel->setText(QString::number(val));
	else variableValueLabel->setText("");
	passThruButton->setEnabled((val!= OUT_OF_BOUNDS) && iParams->isEnabled());
	
	guiSetTextChanged(false);
	setEditorDirty(iParams);
	
	PanelCommand::captureEnd(cmd, iParams);
	update();
	if (renderTextChanged && iParams->getVizNum()>=0)
		VizWinMgr::getInstance()->forceRender(iParams);
		
	renderTextChanged = false;
}
/*********************************************************************************
 * Slots associated with IsoTab:
 *********************************************************************************/

void IsoEventRouter::isoLoadTF(){
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::_isoParamsTag);
	if (iParams->GetMapVariableNum() < 0) return;
	loadTF(iParams, iParams->GetMapVariableNum());
	iParams->SetFlagDirty(ParamsIso::_ColorMapTag);
	if (iParams->isEnabled()&&iParams->getVizNum()>=0)
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
}
void IsoEventRouter::isoLoadInstalledTF(){
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::_isoParamsTag);
	if (iParams->GetMapVariableNum() < 0) return;
	TransferFunction* tf = (TransferFunction*)iParams->GetMapperFunc();
	assert(tf);
	float minb = tf->getMinMapValue();
	float maxb = tf->getMaxMapValue();
	if (minb >= maxb){ minb = 0.0; maxb = 1.0;}
	loadInstalledTF(iParams,iParams->GetMapVariableNum());
	tf = (TransferFunction*)iParams->GetMapperFunc();
	tf->setMinMapValue(minb);
	tf->setMaxMapValue(maxb);
	iParams->SetMinMapEdit(iParams->GetMapVariableNum(), minb);
	iParams->SetMaxMapEdit(iParams->GetMapVariableNum(), maxb);
	setEditorDirty();
	iParams->SetFlagDirty(ParamsIso::_ColorMapTag);
	if (iParams->isEnabled()&&iParams->getVizNum()>=0)
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
}

void IsoEventRouter::isoSaveTF(){
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::_isoParamsTag);
	if (iParams->GetMapVariableNum() < 0) return;
	saveTF(iParams);
}
//Respond to user request to load/save TF
//Assumes name is valid
//
void IsoEventRouter::
sessionLoadTF(QString* name){
	
	confirmText(false);
	ParamsIso* iParams = getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "Load Transfer Function from Session");
	
	//Get the transfer function from the session:
	
	std::string s((const char*)name->toAscii());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	int varNum = iParams->GetMapVariableNum();
	tf->SetRootParamNode(new ParamNode(TransferFunction::_transferFunctionTag));
	tf->GetRootNode()->SetParamsBase(tf);
	iParams->hookupTF(tf, varNum);
	PanelCommand::captureEnd(cmd, iParams);
	iParams->SetFlagDirty(ParamsIso::_ColorMapTag);
	if (iParams->isEnabled()&&iParams->getVizNum()>=0)
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
	setEditorDirty();
}

//Respond to a change in opacity scale factor
void IsoEventRouter::
guiSetOpacityScale(int val){
	ParamsIso* pi = getActiveIsoParams();
	if(pi->GetMapVariableNum() < 0) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(pi, "modify opacity scale slider");
	pi->setOpacityScale( ((float)(256-val))/256.f);
	float sliderVal = pi->getOpacityScale();
	opacityScaleSlider->setToolTip("Opacity Scale Value = "+QString::number(sliderVal));
	PanelCommand::captureEnd(cmd,pi);
	pi->SetFlagDirty(ParamsIso::_ColorMapTag);
	if (pi->isEnabled()&& pi->getVizNum()>= 0)
		VizWinMgr::getInstance()->getVizWin(pi->getVizNum())->updateGL();
}
void IsoEventRouter::guiBindColorToOpac(){
	confirmText(false);
	ParamsIso* iParams = getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "bind Color to Opacity");
    transferFunctionFrame->bindColorToOpacity();
	PanelCommand::captureEnd(cmd, iParams);
	iParams->SetFlagDirty(ParamsIso::_ColorMapTag);
	if (iParams->isEnabled()&& iParams->getVizNum()>=0)
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
}
void IsoEventRouter::guiBindOpacToColor(){
	confirmText(false);
	ParamsIso* iParams = getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "bind Opacity to Color");
    transferFunctionFrame->bindOpacityToColor();
	PanelCommand::captureEnd(cmd, iParams);
	iParams->SetFlagDirty(ParamsIso::_ColorMapTag);
	if (iParams->isEnabled()&& iParams->getVizNum()>=0)
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
}
void IsoEventRouter::setTFNavigateMode(bool mode){
	TFeditButton->setChecked(!mode);
	guiSetTFEditMode(!mode);
}
void IsoEventRouter::setTFEditMode(bool mode){
	TFnavigateButton->setChecked(!mode);
	guiSetTFEditMode(mode);
}
void IsoEventRouter::
guiToggleColorInterpType(bool isDiscrete){
	confirmText(false);
	ParamsIso* iParams = getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams,  "toggle discrete color interpolation");
	if (isDiscrete)
		iParams->GetMapperFunc()->setColorInterpType(TFInterpolator::discrete);
	else 
		iParams->GetMapperFunc()->setColorInterpType(TFInterpolator::linear);
	updateTab();
	
	//Force a redraw
	
	setEditorDirty();
	PanelCommand::captureEnd(cmd,iParams);
	iParams->SetFlagDirty(ParamsIso::_ColorMapTag);
	if (iParams->isEnabled())VizWinMgr::getInstance()->forceRender(iParams,true);
}
void IsoEventRouter::guiSetTFAligned(){
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::_isoParamsTag);
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "align tf in edit frame");
	setEditorDirty();
	update();
	PanelCommand::captureEnd(cmd, iParams);
}void IsoEventRouter::guiSetIsoAligned(){
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::_isoParamsTag);
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "fit iso selection window to view");
	setEditorDirty();
	update();
	PanelCommand::captureEnd(cmd, iParams);
}
void IsoEventRouter::refreshTFHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	ParamsIso* iParams = getActiveIsoParams();
	if (iParams->GetMapVariableNum()<0) return;
	if (iParams->doBypass(VizWinMgr::getActiveAnimationParams()->getCurrentTimestep())){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Unable to refresh histogram");
		return;
	}
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		const float* bnds = iParams->GetMapBounds();
		refreshHistogram(iParams, iParams->GetMapVariableNum(),bnds);
	}
	setEditorDirty();
}
void IsoEventRouter::guiStartChangeMapFcn(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	ParamsIso* pi = getActiveIsoParams();
    savedCommand = PanelCommand::captureStart(pi, qstr.toLatin1());
}

void IsoEventRouter::guiSetTFEditMode(bool mode){
	confirmText(false);
	ParamsIso* iParams = getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "set edit/navigate mode");
	iParams->setMapEditMode(mode);
	PanelCommand::captureEnd(cmd, iParams); 
}
void IsoEventRouter::setBindButtons(bool canbind){
	OpacityBindButton->setEnabled(canbind);
	ColorBindButton->setEnabled(canbind);
}

void IsoEventRouter::
guiEndChangeMapFcn(){
	if (!savedCommand) return;
	ParamsIso* iParams = getActiveIsoParams();
	PanelCommand::captureEnd(savedCommand,iParams);
	iParams->SetFlagDirty(ParamsIso::_MapBoundsTag);
	iParams->SetFlagDirty(ParamsIso::_ColorMapTag);
	if (iParams->isEnabled()&& iParams->getVizNum()>=0 ){
		VizWin* viz = VizWinMgr::getInstance()->getVizWin(iParams->getVizNum());
		viz->setColorbarDirty(true);
		viz->updateGL();
	}
	savedCommand = 0;
}
void IsoEventRouter::
guiStartChangeIsoSelection(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	ParamsIso* pi = getActiveIsoParams();
    savedCommand = PanelCommand::captureStart(pi, qstr.toLatin1());
}
//This will set dirty bits and undo/redo changes to histo bounds and eventually iso value
void IsoEventRouter::
guiEndChangeIsoSelection(){
	if (!savedCommand) return;
	ParamsIso* iParams = getActiveIsoParams();
	// Make sure histo bounds are valid
	IsoControl* icntrl = iParams->GetIsoControl();
	float minbnd = icntrl->getMinHistoValue();
	float maxbnd = icntrl->getMaxHistoValue();
	float isoval = icntrl->getIsoValue();
	float wid = maxbnd - minbnd;
	if (isoval < minbnd +0.01*wid) icntrl->setMinHistoValue(isoval - 0.01*wid);
	if (isoval > maxbnd -0.01*wid) icntrl->setMaxHistoValue(isoval + 0.01*wid);

	PanelCommand::captureEnd(savedCommand,iParams);
	iParams->SetFlagDirty(ParamsIso::_IsoValueTag);
	iParams->SetFlagDirty(ParamsIso::_HistoBoundsTag);
	
	savedCommand = 0;
	updateTab();
	if (iParams->isEnabled() && iParams->getVizNum() >=0)
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
	
}
void IsoEventRouter::
setIsoEditMode(bool mode){
	navigateButton->setChecked(!mode);
	editMode = mode;
}
void IsoEventRouter::
setIsoNavigateMode(bool mode){
	editButton->setChecked(!mode);
	editMode = !mode;
}
void IsoEventRouter::guiChangeInstance(int newCurrent){
	performGuiChangeInstance(newCurrent);
}
void IsoEventRouter::guiNewInstance(){
	performGuiNewInstance();
}
void IsoEventRouter::guiDeleteInstance(){
	performGuiDeleteInstance();
}

void IsoEventRouter::guiCopyInstanceTo(int toViz){
	if (toViz == 0) return; 
	if (toViz == 1) {performGuiCopyInstance(); return;}
	int viznum = copyCount[toViz];
	copyCombo->setCurrentIndex(0);
	performGuiCopyInstanceToViz(viznum);
}

void IsoEventRouter::
setIsoTabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void IsoEventRouter::
setIsoTabRenderTextChanged(const QString& ){
	guiSetTextChanged(true);
	renderTextChanged = true;
}

void IsoEventRouter::
isoReturnPressed(void){
	confirmText(true);
}


void IsoEventRouter::
setIsoEnabled(bool val, int instance){

	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	ParamsIso* iParams = (ParamsIso*)Params::GetParamsInstance(Params::_isoParamsTag, activeViz, instance);
	//Make sure this is a change:
	if (iParams->isEnabled() == val ) return;
	//If we are enabling, also make this the current instance:
	if (val) {
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val,instance);
	
	
}
//Slot that changes the isovalue to be the function value at the selectionPoint
//Must Evaluate the current variable at the selection point, 
//then change the iso value to that value.
//Has no effect if there is no dataMgr
void IsoEventRouter::guiPassThruPoint(){
	confirmText(false);
	ParamsIso* iParams = getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "pass iso thru point");
	float val = evaluateSelectedPoint();
	if (val != OUT_OF_BOUNDS)
		iParams->SetIsoValue(val);
	PanelCommand::captureEnd(cmd, iParams);
	updateTab(); 
	if (iParams->getVizNum()>=0)
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
}
float IsoEventRouter::evaluateSelectedPoint(){
	ParamsIso* iParams = getActiveIsoParams();
	if (!iParams->isEnabled()) return OUT_OF_BOUNDS;
	
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double> pnt = iParams->GetSelectedPoint();
	double dbpnt[3];
	int numRefinements = iParams->GetRefinementLevel();
	int lod = iParams->GetCompressionLevel();
	
	const string& varname = iParams->GetIsoVariableName();
	for (int i = 0; i<3; i++) dbpnt[i] = pnt[i];
	float val = RegionParams::calcCurrentValue(varname, dbpnt, numRefinements, lod, timeStep);
	return val;
}


void IsoEventRouter::
refreshHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::_isoParamsTag);
	if (iParams->doBypass(VizWinMgr::getActiveAnimationParams()->getCurrentTimestep())){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Unable to refresh histogram");
		return;
	}
	
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		const float* bnds = iParams->GetHistoBounds();
		refreshHistogram(iParams,iParams->GetIsoVariableNum(),bnds);
	}
	setEditorDirty(iParams);
}



//Reinitialize Iso tab settings, session has changed.
//Note that this is called after the globalIsoParams are set up, but before
//any of the localIsoParams are setup.
void IsoEventRouter::
reinitTab(bool doOverride){
	DataStatus* ds = DataStatus::getInstance();
	if (ds->dataIsPresent3D()&&!ds->sphericalTransform()) setEnabled(true);
	else setEnabled(false);
	QPalette pal(constantColorEdit->palette());
	pal.setColor(QPalette::Base,Qt::white);
	constantColorEdit->setPalette(pal);
	variableCombo->clear();
	variableCombo->setMaxCount(ds->getNumActiveVariables3D());
	int i;
	for (i = 0; i< ds->getNumActiveVariables3D(); i++){
		const std::string& s = ds->getActiveVarName3D(i);
		//Direct conversion of std::string& to QString doesn't seem to work
		//Maybe std was not enabled when QT was built?
		const QString& text = QString(s.c_str());
		variableCombo->addItem(text);
	}

	mapVariableCombo->clear();
	mapVariableCombo->setMaxCount(ds->getNumActiveVariables3D()+1);
	mapVariableCombo->addItem("Constant");
	for (i = 0; i< ds->getNumActiveVariables3D(); i++){
		const std::string& s = ds->getActiveVarName3D(i);
		//Direct conversion of std::string& to QString doesn't seem to work
		//Maybe std was not enabled when QT was built?
		const QString& text = QString(s.c_str());
		mapVariableCombo->addItem(text);
	}

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
	if (histogramList){
		for (int i = 0; i<numHistograms; i++){
			if (histogramList[i]) delete histogramList[i];
		}
		delete [] histogramList;
		histogramList = 0;
		numHistograms = 0;
	}
	
	setBindButtons(false);
	ParamsIso* dParams = (ParamsIso*)VizWinMgr::getActiveParams(Params::_isoParamsTag);
	setupFidelity(3, fidelityLayout,fidelityBox, dParams, doOverride);
	connect(fidelityButtons,SIGNAL(buttonClicked(int)),this, SLOT(guiSetFidelity(int)));
	updateTab();
}

void IsoEventRouter::
guiSetCompRatio(int num){
	confirmText(false);
	//make sure we are changing it
	ParamsIso* iParams = getActiveIsoParams();
	if (num == iParams->GetCompressionLevel()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "set compression level");
	
	iParams->SetCompressionLevel(num);
	lodCombo->setCurrentIndex(num);
	iParams->SetIgnoreFidelity(true);
	QPalette pal = QPalette(fidelityBox->palette());
	pal.setColor(QPalette::WindowText, Qt::gray);
	fidelityBox->setPalette(pal);
	PanelCommand::captureEnd(cmd, iParams);
	VizWinMgr::getInstance()->setVizDirty(iParams,RegionBit);
}
void IsoEventRouter::
guiSetNumRefinements(int num){
	confirmText(false);
	//make sure we are setting it to a valid number
	
	ParamsIso* iParams = getActiveIsoParams();
	
	if (num == iParams->GetRefinementLevel()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "set number of refinements");
	
	iParams->SetRefinementLevel(num);
	refinementCombo->setCurrentIndex(num);
	iParams->SetIgnoreFidelity(true);
	QPalette pal = QPalette(fidelityBox->palette());
	pal.setColor(QPalette::WindowText, Qt::gray);
	fidelityBox->setPalette(pal);
	PanelCommand::captureEnd(cmd, iParams);
	VizWinMgr::getInstance()->setVizDirty(iParams, RegionBit);
	
}
void IsoEventRouter::
guiSetEnabled(bool value, int instance, bool undoredo){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = vizWinMgr->getActiveViz();
	//Ignore spurious clicks.
	ParamsIso* iParams = (ParamsIso*)Params::GetParamsInstance(Params::_isoParamsTag, winnum, instance);
	
	if (value == iParams->isEnabled()) return;
	
	
	//Check if we really can do raycasting:
	if (value && !DVRRayCaster::supported()){
		MessageReporter::errorMsg("This computer's graphics capabilities \ndo not support isosurfacing.\n %s\n",
			"Update graphics drivers \nor install a supported graphics card \nto render isosurfaces.");
		return;
	}
	confirmText(false);

	if (iParams->GetMapperFunc() && iParams->GetMapVariableNum()>=0)
    {
      QString strn;

      strn.setNum(iParams->GetMapperFunc()->getMinColorMapValue(),'g',7);
      leftMappingEdit->setText(strn);

      strn.setNum(iParams->GetMapperFunc()->getMaxColorMapValue(),'g',7);
      rightMappingEdit->setText(strn);
	} 
    else 
    {
      leftMappingEdit->setText("0.0");
      rightMappingEdit->setText("1.0");
	}
	//enable the current instance:
	PanelCommand* cmd;
	if(undoredo) cmd = PanelCommand::captureStart(iParams, "toggle iso enabled", instance);
	iParams->setEnabled(value);
	if(undoredo) PanelCommand::captureEnd(cmd, iParams);

	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(iParams,!value, false);
	
	updateTab();
}

/*
 * Method to be invoked after the user has changed a variable
 * Make the labels consistent with the new left/right data limits, but
 * don't trigger a new undo/redo event
 */
void IsoEventRouter::
updateHistoBounds(RenderParams* params){
	QString strn;
	ParamsIso* iParams = (ParamsIso*)params;
	//Find out what timestep is current:
	int viznum = iParams->getVizNum();
	if (viznum < 0) return;
	DataStatus* ds = DataStatus::getInstance();
	int varnum = ds->getActiveVarNum3D(iParams->GetIsoVariableName());
	int currentTimeStep = VizWinMgr::getInstance()->getAnimationParams(viznum)->getCurrentTimestep();
	float minval, maxval;
	if (iParams->isEnabled()){
		minval = ds->getDataMin3D(varnum, currentTimeStep);
		maxval = ds->getDataMax3D(varnum, currentTimeStep);
	} else {
		minval = ds->getDefaultDataMin3D(varnum);
		maxval = ds->getDefaultDataMax3D(varnum);
	}
	

	minDataBound->setText(strn.setNum(minval));
	maxDataBound->setText(strn.setNum(maxval));
	
}
//Copy the point from the probe to the iso panel.  Don't do anything with it.
void IsoEventRouter::
guiCopyProbePoint(){
	confirmText(false);
	ParamsIso* iParams = getActiveIsoParams();
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	//Issue a warning if the probe resolution does not match iso resolution;
	if (pParams->GetRefinementLevel() != iParams->GetRefinementLevel()){
		MessageReporter::warningMsg("Note:  Refinement levels of probe and \nisosurface are different.\n%s",
			"Variable values may differ as a result");
	}
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "copy point from probe");
	const float* selectedpoint = pParams->getSelectedPointLocal();
	int ts = VizWinMgr::getInstance()->getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>& usrExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)ts);
	float userCoords[3];
	for (int i = 0; i<3; i++) userCoords[i] = selectedpoint[i]+usrExts[i];
	iParams->SetSelectedPoint(userCoords);
	PanelCommand::captureEnd(cmd, iParams);
	updateTab();
}
void IsoEventRouter::
guiSetComboVarNum(int val){
	confirmText(false);
	ParamsIso* iParams = getActiveIsoParams();
	//The comboVarNum is the metadata num, but the ParamsIso keeps
	//the session variable name
	int comboVarNum = DataStatus::getInstance()->getActiveVarNum3D(iParams->GetIsoVariableName());
	if (val == comboVarNum) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "set iso variable");
		
	//reset the display range shown on the histo window
	//also set dirty flag
	
	iParams->SetIsoVariableName(DataStatus::getInstance()->getActiveVarName3D(val));
	
	updateHistoBounds(iParams);
	
	PanelCommand::captureEnd(cmd, iParams);
	updateTab();
	VizWinMgr::getInstance()->setVizDirty(iParams, RegionBit);
	
}
		
void IsoEventRouter::
guiSetMapComboVarNum(int val){
	confirmText(false);
	ParamsIso* iParams = getActiveIsoParams();
	//The mapcomboVarNum is the metadata num +1, but the ParamsIso keeps
	//the session variable name
	DataStatus* ds = DataStatus::getInstance();
	int comboVarNum = 1+ds->getActiveVarNum3D(iParams->GetMapVariableName());
	if (val == comboVarNum) return;
	//If the change is to set the combo to a nonexistent variable, then do not make
	//the change:
	if (val != 0){ 
		int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
		int sesvarnum = ds->mapActiveToSessionVarNum3D(val-1);
		if (ds->getVDCType() != 2){
			int ref = ds->maxXFormPresent3D(sesvarnum, timeStep);
			if (ref < 0 || (ref < iParams->GetRefinementLevel() && !ds->useLowerAccuracy())){
				//don't change the variable
				MessageReporter::errorMsg("Selected variable is not available\nat required refinement level\nand at current timestep.");
				mapVariableCombo->setCurrentIndex(comboVarNum);
				return;
			}
		} else {
			int ref = ds->maxLODPresent3D(sesvarnum, timeStep);
			if (ref < 0 || (ref < iParams->GetCompressionLevel() && !ds->useLowerAccuracy())){
				//don't change the variable
				MessageReporter::errorMsg("Selected variable is not available\nat required LOD\nand at current timestep.");
				mapVariableCombo->setCurrentIndex(comboVarNum);
				return;
			}
		}
	}
	//If the change is turning on or off the constant color, then will need to disable and
	//re-enable the renderer.
	if(iParams->isEnabled()&&((val == 0 && comboVarNum != 0)||(val!=0 && comboVarNum == 0))){
		ReenablePanelCommand* cmd = ReenablePanelCommand::captureStart(iParams, "set iso mapping variable");
		//reset the display range shown on the histo window
		//also set dirty flag
		if(val > 0){
			iParams->SetMapVariableName(DataStatus::getInstance()->getActiveVarName3D(val-1));
		}
		else iParams->SetMapVariableName("Constant");
		//Disable and enable:
		iParams->setEnabled(false);
		updateRenderer(iParams,true,false);
		iParams->setEnabled(true);
		updateRenderer(iParams,false,false);
		ReenablePanelCommand::captureEnd(cmd, iParams);

		if(val > 0)	updateMapBounds(iParams);
		
	} else {
		PanelCommand* cmd = PanelCommand::captureStart(iParams, "set iso mapping variable");
		
		//reset the display range shown on the histo window
		//also set dirty flag
		if(val > 0){
			iParams->SetMapVariableName(DataStatus::getInstance()->getActiveVarName3D(val-1));
			updateMapBounds(iParams);
		}
		else iParams->SetMapVariableName("Constant");

		PanelCommand::captureEnd(cmd, iParams);
	}
	
	updateTab();
	
	if (iParams->isEnabled() && iParams->getVizNum()>=0 )
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
}

void IsoEventRouter::
guiSetLighting(bool val){
	ParamsIso* iParams = getActiveIsoParams();
	confirmText(false);
	if (iParams->GetNormalOnOff() == val) return;
	PanelCommand* cmd = PanelCommand::captureStart(iParams,  "toggle iso lighting");
	iParams->SetNormalOnOff(val);
	PanelCommand::captureEnd(cmd, iParams);
	if (iParams->isEnabled()&& iParams->getVizNum()>=0)
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();	
}




/* Handle the change of status associated with change of enablement 
 
 * If the window is new, (i.e. we are just creating a new window, use 
 * prevEnabled = false
 
 */
void IsoEventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled, bool newWindow){

	
	
	ParamsIso* iParams = (ParamsIso*)rParams;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = iParams->getVizNum();
	
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
	} else {
		viz = vizWinMgr->getActiveVisualizer();
	}
	
	//cases to consider:
	//1.  unchanged disabled renderer; do nothing.
	//  enabled renderer, just force refresh:
	
	if (prevEnabled == nowEnabled) {
		if (!prevEnabled) return;
		
		VizWinMgr::getInstance()->setVizDirty(iParams,NavigatingBit,true);
		return;
	}
	
	//2.  Change of disable->enable .  Create a new renderer in active window.
	
	
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	
	
	//For a new renderer

	
	if (nowEnabled && !prevEnabled ){//For case 2.:  create a renderer in the active window:
		
		IsoRenderer* myIso;
		if (iParams->GetMapVariableNum()< 0)
			myIso = new IsoRenderer(viz->getGLWindow(), DvrParams::DVR_RAY_CASTER,iParams);
		else
			myIso = new IsoRenderer(viz->getGLWindow(), DvrParams::DVR_RAY_CASTER_2_VAR,iParams);
        
		
		viz->getGLWindow()->insertSortedRenderer(iParams,myIso);

		//force the renderer to refresh region data  (why?)
		
		VizWinMgr::getInstance()->setVizDirty(iParams,NavigatingBit,true);
		VizWinMgr::getInstance()->setClutDirty(iParams);
		VizWinMgr::getInstance()->setVizDirty(iParams,LightingBit, true);
		
        lightingCheckbox->setEnabled(myIso->hasLighting());

		//Quit 
		return;
	}
	
	assert(prevEnabled && !nowEnabled); //case 6, disable 
	
	viz->getGLWindow()->removeRenderer(iParams);

	return;
}



//Method called when undo/redo changes params.  If prevParams is null, the
//vizwinmgr will create a new instance.
void IsoEventRouter::
makeCurrent(Params* prevParams, Params* newParams, bool newWin, int instance, bool reEnable) {
	assert(instance >= 0);
	ParamsIso* iParams = (ParamsIso*)(newParams->deepCopy());
	iParams->GetRootNode()->SetAllFlags(true);
	int vizNum = iParams->getVizNum();
	if (vizNum < 0) return; //defective session file in 2.0.0
	
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumIsoInstances(vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, iParams, Params::GetTypeFromTag(Params::_isoParamsTag), instance);
	
	setEditorDirty(iParams);
	updateTab();
	

	ParamsIso* formerParams = (ParamsIso*)prevParams;
	if (reEnable){
		assert(formerParams->isEnabled() &&  iParams->isEnabled());
		assert(!newWin);
		iParams->setEnabled(false);
		updateRenderer(iParams, true, newWin);
		iParams->setEnabled(true);
		updateRenderer(iParams, false, newWin);
	} else {
		bool wasEnabled = false;
		if (formerParams) wasEnabled = formerParams->isEnabled();
		//Check if the enabled and/or Local settings changed:
		if (newWin || (formerParams->isEnabled() != iParams->isEnabled())){
			updateRenderer(iParams, wasEnabled,  newWin);
		} else { //Just make all the renderer flags dirty
			VizWinMgr::getInstance()->setVizDirty(iParams,NavigatingBit,true);
			VizWinMgr::getInstance()->setClutDirty(iParams);
			VizWinMgr::getInstance()->setVizDirty(iParams,LightingBit, true);
			VizWinMgr::getInstance()->setDatarangeDirty(iParams);
		}
	}
}


void IsoEventRouter::cleanParams(Params* p) 
{
	isoSelectionFrame->setMapperFunction(NULL);
	isoSelectionFrame->setVariableName("");
	isoSelectionFrame->updateParams(); 
	transferFunctionFrame->setMapperFunction(NULL);
	transferFunctionFrame->setVariableName("");
	transferFunctionFrame->updateParams(); 
}
	
/*
 * Respond to user clicking the color button
 */
void IsoEventRouter::
setConstantColor(){
	
	//Bring up a color selector dialog:
	QPalette pal(constantColorEdit->palette());
	QColor newColor = QColorDialog::getColor(pal.color(QPalette::Base), this);
	if (!newColor.isValid()) return;
	//Set button color
	pal.setColor(QPalette::Base,newColor);
	constantColorEdit->setPalette(pal);
	//Set parameter value of the appropriate parameter set:
	guiSetConstantColor(newColor);
}
void IsoEventRouter::
guiSetConstantColor(QColor& newColor){
	confirmText(false);
	ParamsIso* iParams = getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams,  "set constant iso color");
	const float* prevColor = iParams->GetConstantColor();
	float fColor[4];
	fColor[0] = (float)newColor.red()/255.f;
	fColor[1] = (float)newColor.green()/255.f;
	fColor[2] = (float)newColor.blue()/255.f;
	fColor[3] = prevColor[3]; //opacity
	iParams->SetConstantColor(fColor);
	PanelCommand::captureEnd(cmd, iParams);
	if (iParams->getVizNum()>=0 && iParams->isEnabled())
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
}
/*
 * Method to be invoked after the user has moved the right or left bounds
 * of the transfer function window
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void IsoEventRouter::
updateMapBounds(RenderParams* params){
	ParamsIso* isoParams = (ParamsIso*)params;
	int currentTimeStep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	int varNum = DataStatus::getInstance()->getActiveVarNum3D(isoParams->GetMapVariableName());
	QString strn;
	
	if (varNum >= 0){
		float minval, maxval;
		if (isoParams->isEnabled()){
			minval = DataStatus::getInstance()->getDataMin3D(varNum, currentTimeStep);
			maxval = DataStatus::getInstance()->getDataMax3D(varNum, currentTimeStep);
		} else {
			minval = DataStatus::getInstance()->getDefaultDataMin3D(varNum);
			maxval = DataStatus::getInstance()->getDefaultDataMax3D(varNum);
		}
		minTFDataBound->setText(strn.setNum(minval));
		maxTFDataBound->setText(strn.setNum(maxval));
	
		if (isoParams->GetMapperFunc()){
			leftMappingEdit->setText(strn.setNum(isoParams->GetMapperFunc()->getMinOpacMapValue(),'g',4));
			rightMappingEdit->setText(strn.setNum(isoParams->GetMapperFunc()->getMaxOpacMapValue(),'g',4));
		} else {
			leftMappingEdit->setText("0.0");
			rightMappingEdit->setText("1.0");
		}
	}
	setEditorDirty(isoParams);
}
//Set TF editor as well as isoControl editor dirty.
void IsoEventRouter::
setEditorDirty(RenderParams* p){
	ParamsIso* ip = (ParamsIso*)p;
	if(!ip) ip = getActiveIsoParams();
	if(ip->GetIsoControl())ip->GetIsoControl()->setParams(ip);
    isoSelectionFrame->setMapperFunction(ip->GetIsoControl());
	isoSelectionFrame->setVariableName(ip->GetIsoVariableName());
	isoSelectionFrame->setIsoValue(ip->GetIsoValue());
    isoSelectionFrame->updateParams();
	if(ip->GetMapperFunc()&& ip->GetMapVariableNum() >= 0){
		VizWin* viz = VizWinMgr::getInstance()->getVizWin(ip->getVizNum());
		viz->setColorbarDirty(true);
		ip->GetMapperFunc()->setParams(ip);
		transferFunctionFrame->setMapperFunction(ip->GetMapperFunc());
		transferFunctionFrame->updateParams();
		TFInterpolator::type t = ip->GetMapperFunc()->colorInterpType();
		if (colorInterpCheckbox->isChecked() && t != TFInterpolator::discrete) 
			colorInterpCheckbox->setChecked(false);
		if (!colorInterpCheckbox->isChecked() && t == TFInterpolator::discrete) 
			colorInterpCheckbox->setChecked(true);
	}

    Session *session = Session::getInstance();

	if (session->getNumSessionVariables()){
		const std::string& varname1 = ip->GetMapVariableName();
		const std::string& varname2 = ip->GetIsoVariableName();
		transferFunctionFrame->setVariableName(varname1);
		isoSelectionFrame->setVariableName(varname2);
	} else {
		isoSelectionFrame->setVariableName("N/A");
		transferFunctionFrame->setVariableName("N/A");
	}
	if(ip) {
		MapperFunction* mf = ip->GetMapperFunc();
		if (mf) {
			leftMappingEdit->setText(QString::number(mf->getMinOpacMapValue()));
			rightMappingEdit->setText(QString::number(mf->getMaxOpacMapValue()));
		}
	}
	isoSelectionFrame->update();
	transferFunctionFrame->update();
}
void IsoEventRouter::
guiSetNumBits(int val){
	ParamsIso* iParams = getActiveIsoParams();
	confirmText(false);
	if(iParams->isEnabled()){
		ReenablePanelCommand* cmd = ReenablePanelCommand::captureStart(iParams, "set iso voxel bits");
		
		//Value is 0 or 1, corresponding to 8 or 16 bits
		iParams->SetNumBits(1<<(val+3));
		//Disable and enable:
		iParams->setEnabled(false);
		updateRenderer(iParams,true,false);
		iParams->setEnabled(true);
		updateRenderer(iParams,false,false);
		ReenablePanelCommand::captureEnd(cmd, iParams);
		
	} else {
		PanelCommand* cmd = PanelCommand::captureStart(iParams, "set iso voxel bits");
		//Value is 0 or 1, corresponding to 8 or 16 bits
		iParams->SetNumBits(1<<(val+3));
		
		PanelCommand::captureEnd(cmd, iParams);
	}
		
	VizWinMgr::getInstance()->setVizDirty(iParams, RegionBit);
}
//Method to invalidate a datarange, and to force a rendering
//with new data quantization
void IsoEventRouter::
setDatarangeDirty(RenderParams* params)
{
	ParamsIso* iParams = (ParamsIso*)params;
	VizWinMgr::getInstance()->setDatarangeDirty(iParams);
}
//Obtain the current valid histogram.  if mustGet is false, don't build a new one.
//Boolean flag is only used by isoeventrouter version
Histo* IsoEventRouter::getHistogram(RenderParams* renParams, bool mustGet, bool isIsoControl ){
	
	ParamsIso* iParams = (ParamsIso*)renParams;
	int numVariables = DataStatus::getInstance()->getNumSessionVariables();
	int varNum;
	if (isIsoControl) varNum = iParams->GetIsoVariableNum();
	else varNum = iParams->GetMapVariableNum();
	
	if (varNum >= numVariables || varNum < 0) return 0;
	if (varNum >= numHistograms || !histogramList){
		if (!mustGet) return 0;
		histogramList = new Histo*[numVariables];
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
		numHistograms = numVariables;
	}
	
	const float* currentDatarange;
	if (isIsoControl) currentDatarange = iParams->GetHistoBounds();
	else currentDatarange = iParams->GetMapBounds();
	if (histogramList[varNum]) return histogramList[varNum];
	
	if (!mustGet) return 0;
	histogramList[varNum] = new Histo(256,currentDatarange[0],currentDatarange[1]);
	refreshHistogram(renParams, varNum,currentDatarange);
	return histogramList[varNum];
	
}
//Fix for clean Windows scrolling:
void IsoEventRouter::refreshTab(){
#ifdef WIN32
	isoSelectFrame->hide();
	isoSelectFrame->show();
	mappingFrame->hide();
	mappingFrame->show();
#endif
}
void IsoEventRouter::guiFitTFToData(){
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	confirmText(false);
	ParamsIso* pParams = getActiveIsoParams();
	int sesvarnum = pParams->GetMapVariableNum();
	if(sesvarnum < 0) return;
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "fit TF to data");
	//Get bounds from DataStatus:
	int ts = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	
	float minBound = ds->getDataMin3D(sesvarnum,ts);
	float maxBound = ds->getDataMax3D(sesvarnum,ts);
	
	if (minBound > maxBound){ //no data
		maxBound = 1.f;
		minBound = 0.f;
	}
	
	((TransferFunction*)pParams->GetMapperFunc())->setMinMapValue(minBound);
	((TransferFunction*)pParams->GetMapperFunc())->setMaxMapValue(maxBound);
	PanelCommand::captureEnd(cmd, pParams);
	setDatarangeDirty(pParams);
	updateTab();
	
}
//Workaround for Qt/Cocoa bug: postpone showing of OpenGL widgets until paintEvent 

#ifdef Darwin
void IsoEventRouter::paintEvent(QPaintEvent* ev){
		if(!isoShown ){
#if (QT_VERSION < QT_VERSION_CHECK(4,8,0))
			QScrollArea* sArea = (QScrollArea*)MainForm::getTabManager()->currentWidget();
			sArea->ensureWidgetVisible(isoSelectFrame);
			isoShown = true;
#endif
			isoSelectionFrame->show();
			QWidget::paintEvent(ev);
			return;
		}
		if(!opacityMapShown ){
#if (QT_VERSION < QT_VERSION_CHECK(4,8,0))
			QScrollArea* sArea = (QScrollArea*)MainForm::getTabManager()->currentWidget();
			sArea->ensureWidgetVisible(transferFunctionFrame);
			opacityMapShown = true;
#endif
			transferFunctionFrame->show();
		}
	QWidget::paintEvent(ev);
	return;
	}
#endif
//Occurs when user clicks a fidelity radio button
void IsoEventRouter::guiSetFidelity(int buttonID){
	// Recalculate LOD and refinement based on current slider value and/or current text value
	//.  If they don't change, then don't 
	// generate an event.
	confirmText(false);
	ParamsIso* dParams = (ParamsIso*)VizWinMgr::getActiveParams(Params::_isoParamsTag);
	int newLOD = fidelityLODs[buttonID];
	int newRef = fidelityRefinements[buttonID];
	int lodSet = dParams->GetCompressionLevel();
	int refSet = dParams->GetRefinementLevel();
	if (lodSet == newLOD && refSet == newRef) return;
	float fidelity = buttonID;
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "Set Data Fidelity");
	dParams->SetCompressionLevel(newLOD);
	dParams->SetRefinementLevel(newRef);
	dParams->SetFidelityLevel(fidelity);
	dParams->SetIgnoreFidelity(false);
	QPalette pal = QPalette(fidelityBox->palette());
	pal.setColor(QPalette::WindowText, Qt::black);
	fidelityBox->setPalette(pal);
	//change values of LOD and refinement combos using setCurrentIndex().
	lodCombo->setCurrentIndex(newLOD);
	refinementCombo->setCurrentIndex(newRef);
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->forceRender(dParams, false);
}

//User clicks on SetDefault button, need to make current fidelity settings the default.
void IsoEventRouter::guiSetFidelityDefault(){
	//Check current values of LOD and refinement and their combos.
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	confirmText(false);
	ParamsIso* dParams = (ParamsIso*)VizWinMgr::getActiveParams(Params::_isoParamsTag);
	UserPreferences *prePrefs = UserPreferences::getInstance();
	PreferencesCommand* pcommand = PreferencesCommand::captureStart(prePrefs, "Set Fidelity Default Preference");

	setFidelityDefault(3,dParams);
	
	UserPreferences *postPrefs = UserPreferences::getInstance();
	PreferencesCommand::captureEnd(pcommand,postPrefs);
	delete prePrefs;
	delete postPrefs;
	updateTab();
}
