//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		userpreferences.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2008
//
//	Description:	Implements the UserPreferences class.
//
#ifdef WIN32
#pragma warning(disable : 4100)
#endif
#include "vizfeatureparams.h"
#include "userpreferences.h"
#include "preferences.h"
#include "command.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include "probeparams.h"
#include "dvrparams.h"
#include "ParamsIso.h"
#include "viewpointparams.h"
#include "animationparams.h"
#include "flowparams.h"
#include "regionparams.h"


#include "messagereporter.h"
#include "mainform.h"
#include "session.h"
#include "vapor/Version.h"
#include "GetAppPath.h"
#include <qtooltip.h>
#include <qlineedit.h>
#include <QFileDialog>
#include <QScrollArea>
#include <QWhatsThis>
#include <QScrollBar>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qlayout.h>

using namespace VAPoR;

const string UserPreferences::_preferencesTag = "UserPreferences";
const string UserPreferences::_sceneColorsTag = "SceneColors";
const string UserPreferences::_exportFileNameTag = "ExportFileName";
const string UserPreferences::_imageCapturePathTag = "ImageCapturePath";
const string UserPreferences::_flowDirectoryPathTag = "FlowPath";
const string UserPreferences::_logFileNameTag = "LogFileName";
const string UserPreferences::_tfPathTag = "TransferFunctionPath";
const string UserPreferences::_metadataPathTag  = "MetadataPath";
const string UserPreferences::_sessionPathTag  = "SessionPath";
const string UserPreferences::_autoSaveFilenameTag = "AutoSaveFilename";
const string UserPreferences::_messagesTag = "MessageSettings";
const string UserPreferences::_autoSaveIntervalAttr = "AutoSaveInterval";
const string UserPreferences::_probeDefaultsTag = "ProbeDefaults";
const string UserPreferences::_thetaAttr = "DefaultTheta";
const string UserPreferences::_phiAttr = "DefaultPhi";
const string UserPreferences::_psiAttr = "DefaultPsi";
const string UserPreferences::_alphaAttr = "DefaultAlpha";
const string UserPreferences::_scaleAttr = "DefaultScale";

const string UserPreferences::_viewpointDefaultsTag = "ViewpointDefaults";
const string UserPreferences::_viewDirAttr = "DefaultViewDir";
const string UserPreferences::_upVecAttr = "DefaultUpVec";
const string UserPreferences::_numLightsAttr = "DefaultNumLights";
const string UserPreferences::_lightDirectionAttr = "DefaultLightDirections";
const string UserPreferences::_diffuseCoeffAttr = "DefaultDiffuseCoeff";
const string UserPreferences::_specularCoeffAttr = "DefaultSpecularCoeff";
const string UserPreferences::_ambientCoeffAttr = "DefaultAmbientCoeff";
const string UserPreferences::_specularExpAttr = "DefaultSpecularExp";


const string UserPreferences::_flowDefaultsTag = "FlowDefaults";
const string UserPreferences::_integrationAccuracyAttr = "DefaultIntegrationAccuracy";
const string UserPreferences::_smoothnessAttr = "DefaultSmoothness";
const string UserPreferences::_flowLengthAttr = "DefaultFlowLength";
const string UserPreferences::_flowDiameterAttr = "DefaultFlowDiameter";
const string UserPreferences::_arrowSizeAttr = "DefaultArrowSize";
const string UserPreferences::_diamondSizeAttr = "DefaultDiamondSize";
const string UserPreferences::_geometryTypeAttr = "DefaultGeometryType";

const string UserPreferences::_isoDefaultsTag = "IsoDefaults";
const string UserPreferences::_isoDefaultBitsPerVoxelAttr = "DefaultIsoBitsPerVoxel";

const string UserPreferences::_dvrDefaultsTag = "DVRDefaults";
const string UserPreferences::_dvrDefaultBitsPerVoxelAttr = "DefaultDvrBitsPerVoxel";
const string UserPreferences::_dvrDefaultLightingAttr = "DefaultDvrLighting";
const string UserPreferences::_dvrDefaultPreIntegrationAttr = "DefaultDvrPreIntegration";

const string UserPreferences::_animationDefaultsTag = "AnimationDefaults";
const string UserPreferences::_animationDefaultMaxWaitAttr = "DefaultAnimationMaxWait";
const string UserPreferences::_animationDefaultMaxFPSAttr = "DefaultAnimationMaxFPS";

const string UserPreferences::_vizFeatureDefaultsTag = "VizFeatureDefaults";
const string UserPreferences::_defaultShowAxisArrowsAttr = "DefaultShowAxisArrows";
const string UserPreferences::_defaultShowTerrainAttr = "DefaultShowTerrain";

string UserPreferences::preferencesVersionString = "";

//Create a new UserPreferences
UserPreferences::UserPreferences() : QDialog(0), Ui_Preferences(){
	setupUi(this);
	dialogChanged = false;
	textChangedFlag = false;
	featureHolder = 0;
}
//Just clone the exposed part, not the QT part
UserPreferences* UserPreferences::clone(){
	UserPreferences* newPrefs = new UserPreferences();
	newPrefs->textChangedFlag = false;
	newPrefs->featureHolder = 0;
	newPrefs->sessionDir=sessionDir;
	newPrefs->autoSaveFilename = autoSaveFilename;
	newPrefs->autoSaveInterval = autoSaveInterval;
	newPrefs->metadataDir=metadataDir;
	newPrefs->logFileName=logFileName;
	newPrefs->jpegPath=jpegPath;
	newPrefs->flowPath=flowPath;
	newPrefs->tfPath=tfPath;
	newPrefs->subregionFrameEnabled=subregionFrameEnabled;
	newPrefs->regionFrameEnabled=regionFrameEnabled;
	newPrefs->subregionFrameColor=subregionFrameColor;
	newPrefs->regionFrameColor=regionFrameColor;
	newPrefs->backgroundColor=backgroundColor;
	for (int i = 0; i< 3; i++){
		newPrefs->popupNum[i]=popupNum[i];
		newPrefs->logNum[i]=logNum[i];
	}
	
	newPrefs->texSizeSpecified=texSizeSpecified;
	newPrefs->useLowerRefinement=useLowerRefinement;
	newPrefs->warnDataMissing=warnDataMissing;
	newPrefs->jpegQuality=jpegQuality;
	newPrefs->texSize=texSize;
	newPrefs->cacheMB=cacheMB;
	newPrefs->theta=theta;
	newPrefs->phi=phi;
	newPrefs->psi=psi;
	newPrefs->scale=scale;
	newPrefs->alpha=alpha;
	newPrefs->flowLength= flowLength; 
	newPrefs->smoothness= smoothness;
	newPrefs->integrationAccuracy= integrationAccuracy;
	newPrefs->flowDiameter= flowDiameter;
	newPrefs->arrowSize= arrowSize;
	newPrefs->diamondSize=diamondSize;
	newPrefs->geometryType=geometryType;
	newPrefs->isoBitsPerVoxel= isoBitsPerVoxel;
	newPrefs->dvrBitsPerVoxel= dvrBitsPerVoxel;
	newPrefs->dvrLighting= dvrLighting;
	newPrefs->dvrPreIntegration = dvrPreIntegration;
	newPrefs->maxWait = maxWait;
	newPrefs->maxFPS = maxFPS;
	newPrefs->showAxisArrows = showAxisArrows;
	newPrefs->showTerrain = showTerrain;
	

	for (int i = 0; i<3; i++){ 
		newPrefs->viewDir[i] = viewDir[i];
		newPrefs->upVec[i] = upVec[i];
		for (int j = 0; j<3;j++){
			newPrefs->lightDir[i][j] = lightDir[i][j];
		}
		newPrefs->diffuseCoeff[i] = diffuseCoeff[i];
		newPrefs->specularCoeff[i] = specularCoeff[i];
	}
	newPrefs->numLights = numLights;
	newPrefs->ambientCoeff = ambientCoeff;
	newPrefs->specularExp = specularExp;
	return newPrefs;
}
//Set up the dialog with current parameters from session state
void UserPreferences::launch(){
	dialogChanged = false;
	showAll = false;
	featureHolder = new ScrollContainer((QWidget*)MainForm::getInstance(), "Set User Preferences");
	
	QScrollArea* sv = new QScrollArea(featureHolder);
	featureHolder->setScroller(sv);
	sv->setWidget(this);
	
	
	//Copy values from session into this
	setDialog();
	//Put current state in command queue
	myPrefsCommand = PreferencesCommand::captureStart(this, "edit user preferences");
	
	int h = MainForm::getInstance()->height();
	if ( h > 900) h = 900;
	int w = 400;
	paramDefaultsFrame->hide();
	defaultDirectoryFrame->hide();
	defaultVizFeatureFrame->hide();
	
	if (w > MainForm::getInstance()->width()) w = MainForm::getInstance()->width();
	setGeometry(0, 0, w, 1300);
	int swidth = sv->verticalScrollBar()->width();
	featureHolder->setGeometry(50, 50, w+swidth,h);
//	sv->resizeContents(w,h);
	
	//Do connections for buttons
	connect (buttonLatestSession, SIGNAL(clicked()),this, SLOT(copyLatestSession()));
	connect (buttonLatestMetadata, SIGNAL(clicked()),this, SLOT(copyLatestMetadata()));
	connect (buttonLatestTF, SIGNAL(clicked()),this, SLOT(copyLatestTF()));
	connect (buttonLatestFlow, SIGNAL(clicked()),this, SLOT(copyLatestFlow()));
	connect (buttonLatestImage, SIGNAL(clicked()),this, SLOT(copyLatestImage()));
	connect (showDefaultsButton, SIGNAL(clicked()),this, SLOT(showAllDefaults()));
	connect (buttonDefault, SIGNAL(clicked()), this, SLOT(setDefaultDialog()));
	connect (buttonDefault_2, SIGNAL(clicked()), this, SLOT(setDefaultDialog()));
	connect (sessionPathButton, SIGNAL(clicked()), this, SLOT(chooseSessionPath()));
	connect (autoSaveButton, SIGNAL(clicked()), this, SLOT(chooseAutoSaveFilename()));
	connect (metadataPathButton, SIGNAL(clicked()), this, SLOT(chooseMetadataPath()));
	connect (logFilePathButton, SIGNAL(clicked()), this, SLOT(chooseLogFilePath()));
	connect (jpegPathButton, SIGNAL(clicked()), this, SLOT(chooseJpegPath()));
	connect (tfPathButton, SIGNAL(clicked()), this, SLOT(chooseTFPath()));
	connect (flowPathButton, SIGNAL(clicked()), this, SLOT(chooseFlowPath()));
	connect (backgroundColorButton, SIGNAL(clicked()), this, SLOT(selectBackgroundColor()));
	connect (regionFrameColorButton, SIGNAL(clicked()), this, SLOT(selectRegionFrameColor()));
	connect (subregionFrameColorButton, SIGNAL(clicked()), this, SLOT(selectSubregionFrameColor()));
	connect (resetCountButton, SIGNAL(clicked()), this, SLOT(resetCounts()));

	connect (autoSaveCheckbox, SIGNAL(toggled(bool)), this, SLOT(setAutoSave(bool)));
	connect (regionCheckbox, SIGNAL(toggled(bool)), this, SLOT(regionChanged(bool)));
	connect (subregionCheckbox, SIGNAL(toggled(bool)), this, SLOT(subregionChanged(bool)));
	connect (textureSizeCheckbox, SIGNAL(toggled(bool)), this, SLOT(changeTextureSize(bool)));
	connect (missingDataCheckbox, SIGNAL(toggled(bool)), this, SLOT(warningChanged(bool)));
	connect (lowerRefinementCheckbox, SIGNAL(toggled(bool)), this, SLOT(lowerResChanged(bool)));
	connect (dvrLightingCheckbox, SIGNAL(toggled(bool)), this, SLOT(dvrLightingChanged(bool)));
	connect (preIntegrationCheckbox, SIGNAL(toggled(bool)), this, SLOT(preIntegrationChanged(bool)));
	connect (axisArrowsCheckbox, SIGNAL(toggled(bool)), this, SLOT(axisArrowsChanged(bool)));
	connect (surfaceCheckbox, SIGNAL(toggled(bool)), this, SLOT(showSurfaceChanged(bool)));
	

	//TextBoxes:
	connect (cacheSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (textureSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (jpegQualityEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (sessionPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (autoSaveFilenameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (autoSaveIntervalEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (metadataPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (tfPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (logFilePathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (flowPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (jpegPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (maxInfoLog, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (maxInfoPopup, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (maxWarnLog, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (maxWarnPopup, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (maxErrorLog, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (maxErrorPopup, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (alphaEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (phiEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (thetaEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (psiEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (scaleEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (viewDir0, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (viewDir1, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (viewDir2, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightPos00, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightPos01, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightPos02, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightPos10, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightPos11, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightPos12, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightPos20, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightPos21, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightPos22, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightDiff0, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightDiff1, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightDiff2, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightSpec0, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightSpec1, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lightSpec2, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (ambientEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (shininessEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (numLightsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (upVec0, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (upVec1, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (upVec2, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));

	connect (accuracyEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (lengthEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (diameterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (diamondEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (smoothnessEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (arrowEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (maxWaitEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (maxFPSEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	
	
	connect (geometryCombo, SIGNAL(activated(int)),SLOT(setGeometryType(int)));
	connect (isoBitsCombo, SIGNAL(activated(int)),SLOT(setIsoBits(int)));
	connect (dvrBitsCombo, SIGNAL(activated(int)),SLOT(setDvrBits(int)));
		

	connect (buttonOk, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect (buttonCancel,SIGNAL(clicked()), featureHolder, SLOT(reject()));
	connect (this, SIGNAL(doneWithIt()), featureHolder, SLOT(reject()));
	connect (buttonHelp, SIGNAL(released()), this, SLOT(doHelp()));	
	connect (buttonHelp2, SIGNAL(released()), this, SLOT(doHelp()));	
	connect (buttonOk2, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect (buttonCancel2,SIGNAL(clicked()), featureHolder, SLOT(reject()));

	buttonOk->setDefault(true);

	featureHolder->exec();
	
}
//Slot to identify that a change has occurred
void UserPreferences::
panelChanged(){
	dialogChanged = true;
	update();
}

//Slots to respond to color changes:
void UserPreferences::
selectRegionFrameColor(){
	QColor prevColor = regionFrameColorButton->palette().color(QPalette::Background);
	QColor frameColor = QColorDialog::getColor(prevColor,0,0);
	regionFrameColorButton->setPaletteBackgroundColor(frameColor);
	regionFrameColor = frameColor;
	dialogChanged = true;
}

void UserPreferences::
selectSubregionFrameColor(){
	QColor frameColor = QColorDialog::getColor(subregionFrameColorButton->palette().color(QPalette::Background));
	subregionFrameColorButton->setPaletteBackgroundColor(frameColor);
	subregionFrameColor = frameColor;
	dialogChanged = true;
}
void UserPreferences::
selectBackgroundColor(){
	//Launch colorselector, put result into the button
	
	QColor bgColor = QColorDialog::getColor(backgroundColorButton->palette().color(QPalette::Background));
	backgroundColorButton->setPaletteBackgroundColor(bgColor);
	backgroundColor = bgColor;
	dialogChanged = true;
}
void UserPreferences::chooseSessionPath(){
	//Launch a file-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefSessionDirectory() == ".") dir = QDir::currentDirPath();
	else dir = Session::getInstance()->getPrefSessionDirectory().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            	"Choose the session file directory",
		dir);
	if (s != "") {
		sessionPathEdit->setText(s);
		sessionDir = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseAutoSaveFilename(){
	//Launch a file-chooser dialog, 
    QString s = QFileDialog::getSaveFileName(this,
                "Choose a filename for auto saving sessions", 
		Session::getInstance()->getAutoSaveSessionFilename().c_str(),
		"Vapor Saved Sessions (*.vss)");
	if (s != ""){
		autoSaveFilenameEdit->setText(s);
		Session::getInstance()->setAutoSaveSessionFilename(s.ascii());
		autoSaveFilename = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseMetadataPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefMetadataDir() == ".") dir = QDir::currentDirPath();
	else dir = Session::getInstance()->getPrefMetadataDir().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            	"Choose the metadata directory",
		dir);
	if (s != "") {
		metadataPathEdit->setText(s);
		metadataDir = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseLogFilePath(){
	//Launch a file-chooser dialog, 
    QString s = QFileDialog::getSaveFileName(this,
                "Choose a filename for log messages", 
		Session::getInstance()->getLogfileName().c_str(),
		"Text (*.txt)");
	if (s != ""){
		logFilePathEdit->setText(s);
		MessageReporter::getInstance()->reset(s.ascii());
		logFileName = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseJpegPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefJpegDirectory() == ".") dir = QDir::currentDirPath();
	else dir = Session::getInstance()->getPrefJpegDirectory().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            "Choose the jpeg (image) file directory",
		dir);
	if (s != "") {
		jpegPathEdit->setText(s);
		jpegPath = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseTFPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefTFFilePath() == ".") dir = QDir::currentDirPath();
	else dir = Session::getInstance()->getPrefTFFilePath().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            "Choose the transfer function file directory",
		dir);
	if (s != "") {
		tfPathEdit->setText(s);
		tfPath = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseFlowPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefFlowDirectory() == ".") dir = QDir::currentDirPath();
	else dir = Session::getInstance()->getPrefFlowDirectory().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            "Choose the flow file directory",
		dir);
	if (s != "") {
		flowPathEdit->setText(s);
		flowPath = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::changeTextureSize(bool canChange){
	textureSizeEdit->setEnabled(canChange);
	if (canChange) 
		textureSizeEdit->setText(
			QString::number(Session::getInstance()->getTextureSize()));
	else 
		textureSizeEdit->setText("");
	texSizeSpecified = canChange;
	dialogChanged = true;
}
void UserPreferences::copyLatestSession(){
	sessionDir = Session::getInstance()->getSessionDirectory();
	sessionPathEdit->setText(sessionDir.c_str());
	dialogChanged = true;
}
void UserPreferences::copyLatestMetadata(){
	metadataDir = Session::getInstance()->getMetadataDir();
	metadataPathEdit->setText(metadataDir.c_str());
	dialogChanged = true;
}
void UserPreferences::copyLatestTF(){
	tfPath = Session::getInstance()->getTFFilePath();
	tfPathEdit->setText(tfPath.c_str());
	dialogChanged = true;
}
void UserPreferences::copyLatestImage(){
	jpegPath = Session::getInstance()->getJpegDirectory();
	jpegPathEdit->setText(jpegPath.c_str());
	dialogChanged = true;
}
void UserPreferences::copyLatestFlow(){
	flowPath = Session::getInstance()->getFlowDirectory();
	flowPathEdit->setText(flowPath.c_str());
	dialogChanged = true;
}
void UserPreferences::
setGeometryType(int type){
	geometryType = type;
	dialogChanged = true;
}
void UserPreferences::regionChanged(bool enabled){
	regionFrameEnabled = enabled;
	dialogChanged = true;
}
void UserPreferences::subregionChanged(bool enabled){
	subregionFrameEnabled = enabled;
	dialogChanged = true;
}
void UserPreferences::warningChanged(bool enabled){
	warnDataMissing = enabled;
	dialogChanged = true;
}
void UserPreferences::lowerResChanged(bool enabled){
	useLowerRefinement = enabled;
	dialogChanged = true;
}
void UserPreferences::dvrLightingChanged(bool val){
	dvrLighting = val;
	dialogChanged = true;
}
void UserPreferences::preIntegrationChanged(bool val){
	dvrPreIntegration = val;
	dialogChanged = true;
}
void UserPreferences::axisArrowsChanged(bool val){
	showAxisArrows = val;
	dialogChanged = true;
}
void UserPreferences::showSurfaceChanged(bool val){
	showTerrain = val;
	dialogChanged = true;
}
void UserPreferences::setDvrBits(int comboPos){
	dvrBitsPerVoxel = 8 + 8*comboPos;
	dialogChanged = true;
}
void UserPreferences::setIsoBits(int comboPos){
	isoBitsPerVoxel = 8 + 8*comboPos;
	dialogChanged = true;
}
void UserPreferences::setAutoSave(bool val){
	if (val) { //set interval to default:
		autoSaveInterval = 10;
	} else {
		autoSaveInterval = 0;
	}
	autoSaveIntervalEdit->setText(QString::number(autoSaveInterval));
}
void UserPreferences::textChanged(){
	textChangedFlag = true;
}
void UserPreferences::resetCounts(){
	MessageReporter::getInstance()->resetCounts();
}

//Copy values into 'this'  dialog, using current session state.
//Copy data values into this as well, needed for cloning.
//
void UserPreferences::
setDialog(){

	Session* ses = Session::getInstance();
	cacheMB = ses->getCacheMB();
	cacheSizeEdit->setText(QString::number(ses->getCacheMB()));
	texSize = ses->getTextureSize();
	textureSizeEdit->setText(
			QString::number(texSize));
	jpegQuality = GLWindow::getJpegQuality();
	jpegQualityEdit->setText(QString::number(jpegQuality));
	autoSaveInterval = ses->getAutoSaveInterval();
	if (autoSaveInterval < 0) autoSaveInterval = 0;
	autoSaveIntervalEdit->setText(QString::number(autoSaveInterval));
	autoSaveCheckbox->setChecked(autoSaveInterval > 0);
	warnDataMissing = DataStatus::warnIfDataMissing();
	missingDataCheckbox->setChecked(warnDataMissing);
	useLowerRefinement = DataStatus::useLowerRefinementLevel();
	lowerRefinementCheckbox->setChecked(useLowerRefinement);
	texSizeSpecified = ses->textureSizeIsSpecified();
	textureSizeCheckbox->setChecked(texSizeSpecified);
	if (texSizeSpecified){
		textureSizeEdit->setEnabled(true);
	} else {
		textureSizeEdit->setEnabled(false);
	}

	sessionDir = ses->getPrefSessionDirectory();
	metadataDir = ses->getPrefMetadataDir();
	logFileName = ses->getLogfileName();
	jpegPath = ses->getPrefJpegDirectory();
	tfPath = ses->getPrefTFFilePath();
	flowPath = ses->getPrefFlowDirectory();
	autoSaveFilename = ses->getAutoSaveSessionFilename();

	//Directories:
	sessionPathEdit->setText(ses->getPrefSessionDirectory().c_str());
	autoSaveFilenameEdit->setText(autoSaveFilename.c_str());
	metadataPathEdit->setText(ses->getPrefMetadataDir().c_str());
	logFilePathEdit->setText(ses->getLogfileName().c_str());
	jpegPathEdit->setText(ses->getPrefJpegDirectory().c_str());
	tfPathEdit->setText(ses->getPrefTFFilePath().c_str());
	flowPathEdit->setText(ses->getPrefFlowDirectory().c_str());
	MessageReporter* mReporter = MessageReporter::getInstance();
	for (int i = 0; i< 3; i++){
		popupNum[i] = mReporter->getMaxPopup((MessageReporter::messagePriority)i);
		logNum[i] = mReporter->getMaxLog((MessageReporter::messagePriority)i);
	}
	maxInfoPopup->setText(QString::number(mReporter->getMaxPopup((MessageReporter::messagePriority)0)));
	maxWarnPopup->setText(QString::number(mReporter->getMaxPopup((MessageReporter::messagePriority)1)));
	maxErrorPopup->setText(QString::number(mReporter->getMaxPopup((MessageReporter::messagePriority)2)));
	maxInfoLog->setText(QString::number(mReporter->getMaxLog((MessageReporter::messagePriority)0)));
	maxWarnLog->setText(QString::number(mReporter->getMaxLog((MessageReporter::messagePriority)1)));
	maxErrorLog->setText(QString::number(mReporter->getMaxLog((MessageReporter::messagePriority)2)));
	
	backgroundColorButton->setPaletteBackgroundColor(DataStatus::getBackgroundColor());
	regionCheckbox->setChecked(DataStatus::regionFrameIsEnabled());
	regionFrameColorButton->setPaletteBackgroundColor(DataStatus::getRegionFrameColor());
	subregionCheckbox->setChecked(DataStatus::subregionFrameIsEnabled());
	subregionFrameColorButton->setPaletteBackgroundColor(DataStatus::getSubregionFrameColor());
	backgroundColor = DataStatus::getBackgroundColor();
	regionFrameColor = DataStatus::getRegionFrameColor();
	subregionFrameColor = DataStatus::getSubregionFrameColor();
	regionFrameEnabled = DataStatus::regionFrameIsEnabled();
	subregionFrameEnabled = DataStatus::subregionFrameIsEnabled();

	//defaults:
	alpha = ProbeParams::getDefaultAlpha();
	scale = ProbeParams::getDefaultScale();
	phi = ProbeParams::getDefaultTheta();
	psi = ProbeParams::getDefaultPsi();
	theta = ProbeParams::getDefaultTheta();
	alphaEdit->setText(QString::number(alpha));
	scaleEdit->setText(QString::number(scale));
	thetaEdit->setText(QString::number(theta));
	phiEdit->setText(QString::number(phi));
	psiEdit->setText(QString::number(psi));
	vcopy(ViewpointParams::getDefaultDiffuseCoeff(), diffuseCoeff);
	vcopy(ViewpointParams::getDefaultSpecularCoeff(), specularCoeff);
	ambientCoeff = ViewpointParams::getDefaultAmbientCoeff();
	specularExp = ViewpointParams::getDefaultSpecularExp();
	numLights = ViewpointParams::getDefaultNumLights();
	for (int i = 0; i<3; i++)
		vcopy(ViewpointParams::getDefaultLightDirection(i), lightDir[i]);
	vcopy(ViewpointParams::getDefaultUpVec(),upVec);
	vcopy(ViewpointParams::getDefaultViewDir(),viewDir);
	viewDir0->setText(QString::number(viewDir[0]));
	viewDir1->setText(QString::number(viewDir[1]));
	viewDir2->setText(QString::number(viewDir[2]));
	upVec0->setText(QString::number(upVec[0]));
	upVec1->setText(QString::number(upVec[1]));
	upVec2->setText(QString::number(upVec[2]));
	lightPos00->setText(QString::number(lightDir[0][0]));
	lightPos01->setText(QString::number(lightDir[0][1]));
	lightPos02->setText(QString::number(lightDir[0][2]));
	lightPos10->setText(QString::number(lightDir[1][0]));
	lightPos11->setText(QString::number(lightDir[1][1]));
	lightPos12->setText(QString::number(lightDir[1][2]));
	lightPos20->setText(QString::number(lightDir[2][0]));
	lightPos21->setText(QString::number(lightDir[2][1]));
	lightPos22->setText(QString::number(lightDir[2][2]));
	lightDiff0->setText(QString::number(diffuseCoeff[0]));
	lightDiff1->setText(QString::number(diffuseCoeff[1]));
	lightDiff2->setText(QString::number(diffuseCoeff[2]));
	lightSpec0->setText(QString::number(specularCoeff[0]));
	lightSpec1->setText(QString::number(specularCoeff[1]));
	lightSpec2->setText(QString::number(specularCoeff[2]));
	numLightsEdit->setText(QString::number(numLights));
	shininessEdit->setText(QString::number(specularExp));
	ambientEdit->setText(QString::number(ambientCoeff));

	diamondSize = FlowParams::getDefaultDiamondSize();
	arrowSize = FlowParams::getDefaultArrowSize();
	flowLength = FlowParams::getDefaultFlowLength();
	smoothness= FlowParams::getDefaultSmoothness();
	integrationAccuracy= FlowParams::getDefaultIntegrationAccuracy(); 
	flowDiameter= FlowParams::getDefaultFlowDiameter();
	geometryType= FlowParams::getDefaultGeometryType();
	diamondEdit->setText(QString::number(diamondSize));
	arrowEdit->setText(QString::number(arrowSize));
	lengthEdit->setText(QString::number(flowLength));
	smoothnessEdit->setText(QString::number(smoothness));
	accuracyEdit->setText(QString::number(integrationAccuracy));
	diameterEdit->setText(QString::number(flowDiameter));
	geometryCombo->setCurrentItem(geometryType);
	isoBitsPerVoxel = ParamsIso::getDefaultBitsPerVoxel();
	dvrBitsPerVoxel = DvrParams::getDefaultBitsPerVoxel();
	dvrLighting = DvrParams::getDefaultLightingEnabled();
	dvrPreIntegration = DvrParams::getDefaultPreIntegrationEnabled();
	maxWait = AnimationParams::getDefaultMaxWait();
	maxFPS = AnimationParams::getDefaultMaxFPS();
	showAxisArrows = GLWindow::getDefaultAxisArrowsEnabled();
	showTerrain= GLWindow::getDefaultTerrainEnabled();
	isoBitsCombo->setCurrentItem((isoBitsPerVoxel == 16) ? 1 : 0);
	dvrBitsCombo->setCurrentItem((dvrBitsPerVoxel == 16) ? 1 : 0);
	dvrLightingCheckbox->setChecked(dvrLighting);
	preIntegrationCheckbox->setChecked(dvrPreIntegration);
	maxWaitEdit->setText(QString::number(maxWait));
	maxFPSEdit->setText(QString::number(maxFPS));
	axisArrowsCheckbox->setChecked(showAxisArrows);
	surfaceCheckbox->setChecked(showTerrain);

}
void UserPreferences::
applyToState(){
	
	//Copy from this (not QDialog) to session
	Session* ses = Session::getInstance();
	ses->setCacheMB(cacheMB);
	ses->setTextureSize(texSize);
	ses->specifyTextureSize(texSizeSpecified);
	ses->setAutoSaveInterval(autoSaveInterval);
	GLWindow::setJpegQuality(jpegQuality);
	DataStatus::setWarnMissingData(warnDataMissing);
	DataStatus::setUseLowerRefinementLevel(useLowerRefinement);
	
	//Directories:
	ses->setPrefSessionDirectory(sessionDir.c_str());
	ses->setPrefMetadataDirectory(metadataDir.c_str());
	if (ses->getLogfileName() != logFileName){
		ses->setLogfileName(logFileName.c_str());
		MessageReporter::getInstance()->reset(logFileName.c_str());
	}
	ses->setPrefJpegDirectory(jpegPath.c_str());
	ses->setPrefTFFilePath(tfPath.c_str());
	ses->setPrefFlowDirectory(flowPath.c_str());
	ses->setAutoSaveSessionFilename(autoSaveFilename.c_str());
	

	MessageReporter* mReporter = MessageReporter::getInstance();
	mReporter->setMaxLog((MessageReporter::messagePriority)0,logNum[0]);
	mReporter->setMaxLog((MessageReporter::messagePriority)1,logNum[1]);
	mReporter->setMaxLog((MessageReporter::messagePriority)2,logNum[2]);
	mReporter->setMaxPopup((MessageReporter::messagePriority)0,popupNum[0]);
	mReporter->setMaxPopup((MessageReporter::messagePriority)1,popupNum[1]);
	mReporter->setMaxPopup((MessageReporter::messagePriority)2,popupNum[2]);
	DataStatus::setBackgroundColor(backgroundColor);
	DataStatus::setRegionFrameColor(regionFrameColor);
	DataStatus::setSubregionFrameColor(subregionFrameColor);
	DataStatus::enableRegionFrame(regionFrameEnabled);
	DataStatus::enableSubregionFrame(subregionFrameEnabled);
	ProbeParams::setDefaultAlpha(alpha);
	ProbeParams::setDefaultPhi(phi);
	ProbeParams::setDefaultScale(scale);
	ProbeParams::setDefaultPsi(psi);
	ProbeParams::setDefaultTheta(theta);
	ViewpointParams::setDefaultViewDir(viewDir);
	ViewpointParams::setDefaultUpVec(upVec);
	ViewpointParams::setDefaultAmbientCoeff(ambientCoeff);
	ViewpointParams::setDefaultDiffuseCoeff(diffuseCoeff);
	ViewpointParams::setDefaultSpecularCoeff(specularCoeff);
	ViewpointParams::setDefaultNumLights(numLights);
	ViewpointParams::setDefaultSpecularExp(specularExp);
	for (int i = 0; i< 3; i++) 
		ViewpointParams::setDefaultLightDirection(i,lightDir[i]);

	FlowParams::setDefaultArrowSize(arrowSize);
	FlowParams::setDefaultDiamondSize(diamondSize);
	FlowParams::setDefaultFlowDiameter(flowDiameter);
	FlowParams::setDefaultFlowLength(flowLength);
	FlowParams::setDefaultIntegrationAccuracy(integrationAccuracy);
	FlowParams::setDefaultSmoothness(smoothness);
	FlowParams::setDefaultGeometryType(geometryType);
	ParamsIso::setDefaultBitsPerVoxel(isoBitsPerVoxel);
	DvrParams::setDefaultBitsPerVoxel(dvrBitsPerVoxel);
	DvrParams::setDefaultLighting(dvrLighting);
	DvrParams::setDefaultPreIntegration(dvrPreIntegration);
	AnimationParams::setDefaultMaxWait(maxWait);
	AnimationParams::setDefaultMaxFPS(maxFPS);
	GLWindow::setDefaultAxisArrows(showAxisArrows);
	GLWindow::setDefaultShowTerrain(showTerrain);
	
}
void UserPreferences::okClicked(){

	featureHolder->hide();
	//If user clicks OK, need to store values back to visualizer, plus
	//save changes in history
	if (dialogChanged || textChangedFlag) {
		if(textChangedFlag){
			getTextChanges();
			textChangedFlag = false;
		}
		applyToState();
		PreferencesCommand::captureEnd(myPrefsCommand, this);
		//Launch a file-save dialog
		requestSave();
	} 
	emit doneWithIt();
	
}
void UserPreferences::requestSave(){
	int rc = QMessageBox::question(0, "Save User Preferences?", 
		"User Preferences have changed.\nDo you want to save them to file?",
		 QMessageBox::Yes|QMessageBox::Default,QMessageBox::No,Qt::NoButton);
	if (rc != QMessageBox::Yes) return;
	
	QString filename = QFileDialog::getSaveFileName(this,
            	"Select the filename for saving user preferences", 
		Session::getPreferencesFile().c_str(),
            	".*");
	
	if(filename.length() == 0) return;
	ofstream os;
	os.open(filename.ascii());

	if (!os || !saveToFile(os)){//Report error if you can't open the file
		MessageReporter::errorMsg("Unable to open preferences file: \n%s", filename.ascii());
	}
	os.close();
	
}
bool UserPreferences::
saveToFile(ofstream& ofs ){
	XmlNode* const rootNode = buildNode("");
	ofs << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
	XmlNode::streamOut(ofs,(*rootNode));
	if (MyBase::GetErrCode() != 0) {
		MessageReporter::errorMsg("Preferences Save Error %d, \nWriting to:\n%s\n%s",
			MyBase::GetErrCode(),Session::getPreferencesFile().c_str(),
			"Redefine $HOME or $VAPOR_PREFS variable to save to writable directory");
		MyBase::SetErrCode(0);
		delete rootNode;
		return false;
	}
	delete rootNode;
	return true;
}
void UserPreferences::
doHelp(){
	QWhatsThis::enterWhatsThisMode();
}
void UserPreferences::
showAllDefaults(){
	QScrollArea* sv = featureHolder->getScroller();
	showAll = !showAll;
	int h = MainForm::getInstance()->height();
	if ( h > 768) h = 768;
	int w;
	if(showAll) {
		showDefaultsButton->setText("Hide Defaults");
		QToolTip::add(showDefaultsButton,"Click to hide the session defaults");
		w = 950;
		paramDefaultsFrame->show();
		defaultDirectoryFrame->show();
		defaultVizFeatureFrame->show();
	}
	else {
		showDefaultsButton->setText("Session Defaults");
		QToolTip::add(showDefaultsButton,"Display the session defaults that will apply when the application is restarted or a new dataset is loaded");
		w = 460;
		paramDefaultsFrame->hide();
		defaultDirectoryFrame->hide();
		defaultVizFeatureFrame->hide();
	}
	if (w > MainForm::getInstance()->width()) w = MainForm::getInstance()->width();
	setGeometry(0, 0, w, h);
	int swidth = sv->verticalScrollBar()->width();
	featureHolder->setGeometry(50, 50, w+swidth,h);
//	sv->resizeContents(w,h);
	sv->resize(w,h);
	featureHolder->updateGeometry();
	featureHolder->adjustSize();
	featureHolder->update();
	
}
//Build the XML for user preferences, based on the state of the app
XmlNode* UserPreferences::buildNode(const string& ){
	Session* ses = Session::getInstance();
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	ostringstream oss;

	oss.str(empty);
	oss << (long)ses->getCacheMB();
	attrs[Session::_cacheSizeAttr] = oss.str();
	oss.str(empty);
	oss << (long)GLWindow::getJpegQuality();
	attrs[Session::_jpegQualityAttr] = oss.str();

	oss.str(empty);
	if (DataStatus::textureSizeIsSpecified())
		oss << "true";
	else 
		oss << "false";
	attrs[Session::_specifyTextureSizeAttr] = oss.str();

	attrs[Session::_VAPORVersionAttr] = Version::GetVersionString();


	oss.str(empty);
	oss << (long)DataStatus::getTextureSize();
	attrs[Session::_textureSizeAttr] = oss.str();
	 
	oss.str(empty);
	oss << (long)ses->getAutoSaveInterval();
	attrs[Session::_autoSaveIntervalAttr] = oss.str();

	XmlNode* mainNode = new XmlNode(_preferencesTag, attrs, 10);
//create element for each path or filename
	mainNode->SetElementString(_metadataPathTag, ses->getPrefMetadataDir());
	mainNode->SetElementString(_sessionPathTag, ses->getPrefSessionDirectory());
	mainNode->SetElementString(_autoSaveFilenameTag, ses->getAutoSaveSessionFilename());
	mainNode->SetElementString(_tfPathTag, ses->getPrefTFFilePath());
	mainNode->SetElementString(_imageCapturePathTag, ses->getPrefJpegDirectory());
	mainNode->SetElementString(_flowDirectoryPathTag, ses->getPrefFlowDirectory());
	mainNode->SetElementString(_exportFileNameTag, ses->getExportFile());
	mainNode->SetElementString(_logFileNameTag, ses->getLogfileName());
	
	//Create a node for message reporting:

	MessageReporter* msgRpt = MessageReporter::getInstance();
	attrs.clear();
	
	oss.str(empty);
	
	oss.str(empty);
	oss << (long)msgRpt->getMaxPopup(MessageReporter::Info) << " " <<
		(long)msgRpt->getMaxPopup(MessageReporter::Warning) << " " <<
		(long)msgRpt->getMaxPopup(MessageReporter::Error);
	attrs[Session::_maxPopupAttr] = oss.str();

	oss.str(empty);
	oss << (long)msgRpt->getMaxLog(MessageReporter::Info) << " "
		<< (long)msgRpt->getMaxLog(MessageReporter::Warning) << " "
		<< (long)msgRpt->getMaxLog(MessageReporter::Error);
	attrs[Session::_maxLogAttr] = oss.str();

	oss.str(empty);
	if (DataStatus::warnIfDataMissing())
		oss << "true";
	else 
		oss << "false";
	attrs[DataStatus::_missingDataWarningAttr] = oss.str();

	oss.str(empty);
	if (DataStatus::useLowerRefinementLevel())
		oss << "true";
	else 
		oss << "false";
	attrs[DataStatus::_useLowerRefinementAttr] = oss.str();

	XmlNode* msgNode = new XmlNode(_messagesTag, attrs, 0);
	mainNode->AddChild(msgNode);

	//Now handle colors etc.
	attrs.clear();
	oss.str(empty);
	QColor clr = DataStatus::getBackgroundColor();
	oss << (long)clr.red() << " "
		<< (long)clr.green() << " "
		<< (long)clr.blue();
	attrs[DataStatus::_backgroundColorAttr] = oss.str();

	oss.str(empty);
	clr = DataStatus::getRegionFrameColor();
	oss << (long)clr.red() << " "
		<< (long)clr.green() << " "
		<< (long)clr.blue();
	attrs[DataStatus::_regionFrameColorAttr] = oss.str();

	oss.str(empty);
	clr = DataStatus::getSubregionFrameColor();
	oss << (long)clr.red() << " "
		<< (long)clr.green() << " "
		<< (long)clr.blue();
	attrs[DataStatus::_subregionFrameColorAttr] = oss.str();

	oss.str(empty);
	if (DataStatus::regionFrameIsEnabled()) oss<<"true";
		else oss<<"false";
	attrs[DataStatus::_regionFrameEnabledAttr] = oss.str();

	oss.str(empty);
	if (DataStatus::subregionFrameIsEnabled()) oss<<"true";
		else oss<<"false";
	attrs[DataStatus::_subregionFrameEnabledAttr] = oss.str();
			
	XmlNode* colorNode = new XmlNode(_sceneColorsTag, attrs, 0);

	

	mainNode->AddChild(colorNode);

	//Create a node for probedefaults:
	attrs.clear();
	oss.str(empty);
	oss << (double)ProbeParams::getDefaultAlpha();
	attrs[_alphaAttr] = oss.str();
	oss.str(empty);
	oss << (double)ProbeParams::getDefaultScale();
	attrs[_scaleAttr] = oss.str();
	oss.str(empty);
	oss << (double)ProbeParams::getDefaultTheta();
	attrs[_thetaAttr] = oss.str();
	oss.str(empty);
	oss << (double)ProbeParams::getDefaultPhi();
	attrs[_phiAttr] = oss.str();
	oss.str(empty);
	oss << (double)ProbeParams::getDefaultPsi();
	attrs[_psiAttr] = oss.str();
	XmlNode* defaultProbeNode = new XmlNode(_probeDefaultsTag, attrs, 0);
	
	mainNode->AddChild(defaultProbeNode);

	//Create a node for viewpoint defaults and lighting defaults:
	attrs.clear();
	
	const float* vec1 = ViewpointParams::getDefaultUpVec();
	const float* vec2 = ViewpointParams::getDefaultViewDir();
	oss.str(empty);
	oss << vec1[0] <<" "<<vec1[1]<<" "<<vec1[2];
	attrs[_upVecAttr] = oss.str();
	oss.str(empty);
	oss << vec2[0] <<" "<<vec2[1]<<" "<<vec2[2];
	attrs[_viewDirAttr] = oss.str();
	oss.str(empty);
	
	for (int i = 0; i<3; i++){
		const float* lightVec = ViewpointParams::getDefaultLightDirection(i);
		for (int j = 0; j<3; j++){
			oss <<lightVec[j]<<" ";
		}
	}
	attrs[_lightDirectionAttr] = oss.str();
	oss.str(empty);
	vec1 = ViewpointParams::getDefaultDiffuseCoeff();
	oss << vec1[0] <<" "<<vec1[1]<<" "<<vec1[2];
	attrs[_diffuseCoeffAttr] = oss.str();
	oss.str(empty);
	vec1 = ViewpointParams::getDefaultSpecularCoeff();
	oss << vec1[0] <<" "<<vec1[1]<<" "<<vec1[2];
	attrs[_specularCoeffAttr] = oss.str();
	oss.str(empty);
	oss << ViewpointParams::getDefaultAmbientCoeff();
	attrs[_ambientCoeffAttr] = oss.str();
	oss.str(empty);
	oss << ViewpointParams::getDefaultSpecularExp();
	attrs[_specularExpAttr] = oss.str();
	
	
	XmlNode* defaultViewpointNode = new XmlNode(_viewpointDefaultsTag, attrs, 0);
	
	mainNode->AddChild(defaultViewpointNode);

	//Create a node for flow defaults:
	attrs.clear();
	oss.str(empty);
	oss << (float)FlowParams::getDefaultIntegrationAccuracy();
	attrs[_integrationAccuracyAttr] = oss.str();
	oss.str(empty);
	oss << (double)FlowParams::getDefaultSmoothness();
	attrs[_smoothnessAttr] = oss.str();
	oss.str(empty);
	oss << (double)FlowParams::getDefaultDiamondSize();
	attrs[_diamondSizeAttr] = oss.str();
	oss.str(empty);
	oss << (double)FlowParams::getDefaultFlowDiameter();
	attrs[_flowDiameterAttr] = oss.str();
	oss.str(empty);
	oss << (double)FlowParams::getDefaultFlowLength();
	attrs[_flowLengthAttr] = oss.str();
	oss.str(empty);
	oss << (double)FlowParams::getDefaultArrowSize();
	attrs[_arrowSizeAttr] = oss.str();
	oss.str(empty);
	oss << (int)FlowParams::getDefaultGeometryType();
	attrs[_geometryTypeAttr] = oss.str();
	XmlNode* defaultFlowNode = new XmlNode(_flowDefaultsTag, attrs, 0);
	mainNode->AddChild(defaultFlowNode);

	//Create a node for iso defaults:
	attrs.clear();
	oss.str(empty);
	oss << (int)ParamsIso::getDefaultBitsPerVoxel();
	attrs[_isoDefaultBitsPerVoxelAttr] = oss.str();
	XmlNode* defaultIsoNode = new XmlNode(_isoDefaultsTag, attrs, 0);
	mainNode->AddChild(defaultIsoNode);
	
	//Create a node for dvr defaults:
	attrs.clear();
	oss.str(empty);
	oss << (int)DvrParams::getDefaultBitsPerVoxel();
	attrs[_dvrDefaultBitsPerVoxelAttr] = oss.str();
	oss.str(empty);
	string str = DvrParams::getDefaultLightingEnabled() ? "true" : "false";
	oss << str;
	attrs[_dvrDefaultLightingAttr] = oss.str();
	oss.str(empty);
	str = DvrParams::getDefaultPreIntegrationEnabled() ? "true" : "false";
	oss << str;
	attrs[_dvrDefaultPreIntegrationAttr] = oss.str();
	XmlNode* defaultDvrNode = new XmlNode(_dvrDefaultsTag, attrs, 0);
	mainNode->AddChild(defaultDvrNode);

	//Create a node for Animation Defaults
	attrs.clear();
	oss.str(empty);
	oss << (float)AnimationParams::getDefaultMaxWait();
	attrs[_animationDefaultMaxWaitAttr] = oss.str();
	oss.str(empty);
	oss << (float)AnimationParams::getDefaultMaxFPS();
	attrs[_animationDefaultMaxFPSAttr] = oss.str();

	XmlNode* defaultAnimationNode = new XmlNode(_animationDefaultsTag, attrs, 0);
	mainNode->AddChild(defaultAnimationNode);

	//Create a node for VizFeature Defaults
	attrs.clear();
	oss.str(empty);
	str = GLWindow::getDefaultAxisArrowsEnabled() ? "true" : "false";
	oss << str;
	attrs[_defaultShowAxisArrowsAttr] = oss.str();
	oss.str(empty);
	str = GLWindow::getDefaultTerrainEnabled() ? "true" : "false";
	oss << str;
	attrs[_defaultShowTerrainAttr] = oss.str();

	XmlNode* defaultVizFeatureNode = new XmlNode(_vizFeatureDefaultsTag, attrs, 0);
	mainNode->AddChild(defaultVizFeatureNode);

	
	return mainNode;
}

bool UserPreferences::elementStartHandler(ExpatParseMgr* pm, int depth, 
		std::string& tag, const char ** attrs)
{
	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	Session* ses = Session::getInstance();										  
	switch (depth){
	//Parse the global session parameters, depth = 0
		case(0): 
		{
			if (StrCmpNoCase(tag, _preferencesTag) != 0) return false;
			
			while (*attrs) {
				string attr = *attrs;
				attrs++;
				string value = *attrs;
				attrs++;

				istringstream ist(value);
				
				if (StrCmpNoCase(attr, Session::_specifyTextureSizeAttr) == 0) {
					string boolVal;
					bool val;
					ist >> boolVal;
					if (boolVal == "true") val = true;
					else val = false;
					ses->specifyTextureSize(val);
				}
				else if (StrCmpNoCase(attr, Session::_VAPORVersionAttr) == 0){
					ist >> preferencesVersionString;
				}
				else if (StrCmpNoCase(attr, Session::_textureSizeAttr) == 0){
					int val;
					ist >> val;
					ses->setTextureSize(val);
				}
				else if (StrCmpNoCase(attr, Session::_cacheSizeAttr) == 0) {
					size_t val;
					ist >> val;
					ses->setCacheMB(val);
				}
				else if (StrCmpNoCase(attr, Session::_jpegQualityAttr) == 0) {
					int qual;
					ist >> qual;
					GLWindow::setJpegQuality(qual);
				}
				else if (StrCmpNoCase(attr, Session::_autoSaveIntervalAttr) == 0) {
					int intvl;
					ist >> intvl;
					ses->setAutoSaveInterval(intvl);
				}
				else {
					pm->parseError("Invalid preferences tag attribute : \"%s\"", attr.c_str());
					return false;
				}
			}
			return true;
		}
		case (1) :
		{
			//Parse child tags
			if (StrCmpNoCase(tag, _sceneColorsTag) == 0){
				//Parse attributes...
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);	
			
					if (StrCmpNoCase(attr, DataStatus::_backgroundColorAttr) == 0) {
						int r,g,b;
						ist >> r; ist>>g; ist>>b;
						QColor bColor(r,g,b);
						DataStatus::setBackgroundColor(bColor);
					}
			
					else if (StrCmpNoCase(attr, DataStatus::_regionFrameColorAttr) == 0) {
						int r,g,b;
						ist >> r; ist>>g; ist>>b;
						QColor rColor(r,g,b);
						DataStatus::setRegionFrameColor(rColor);
					}
					else if (StrCmpNoCase(attr, DataStatus::_subregionFrameColorAttr) == 0) {
						int r,g,b;
						ist >> r; ist>>g; ist>>b;
						QColor sColor(r,g,b);
						DataStatus::setSubregionFrameColor(sColor);
					}
					else if (StrCmpNoCase(attr, DataStatus::_regionFrameEnabledAttr) == 0) {
						bool boolval;
						if (value == "true") boolval = true; 
						else boolval = false; 
						DataStatus::enableRegionFrame(boolval);
					}
					else if (StrCmpNoCase(attr, DataStatus::_subregionFrameEnabledAttr) == 0) {
						bool boolval;
						if (value == "true") boolval = true; 
						else boolval = false; 
						DataStatus::enableSubregionFrame(boolval);
					}
					else {
						pm->parseError("Invalid preferences tag attribute : \"%s\"", attr.c_str());
						return false;
					}
				}
				return true;
			} else if (StrCmpNoCase(tag, _messagesTag) == 0){
				int int1,int2,int3;
				MessageReporter* msgRpt = MessageReporter::getInstance();
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);	
			
					if (StrCmpNoCase(attr, Session::_maxPopupAttr) == 0) {
						ist >> int1; ist>>int2; ist>>int3;
						msgRpt->setMaxPopup(MessageReporter::Info, int1);
						msgRpt->setMaxPopup(MessageReporter::Warning, int2);
						msgRpt->setMaxPopup(MessageReporter::Error, int3);
					}
					else if (StrCmpNoCase(attr, Session::_maxLogAttr) == 0) {
						ist >> int1; ist>>int2; ist>>int3;
						msgRpt->setMaxLog(MessageReporter::Info, int1);
						msgRpt->setMaxLog(MessageReporter::Warning, int2);
						msgRpt->setMaxLog(MessageReporter::Error, int3);
					}
					else if (StrCmpNoCase(attr, DataStatus::_useLowerRefinementAttr) == 0) {
						bool boolval;
						if (value == "true") boolval = true; 
						else boolval = false; 
						DataStatus::setUseLowerRefinementLevel(boolval);
					}
					else if (StrCmpNoCase(attr, DataStatus::_missingDataWarningAttr) == 0) {
						bool boolval;
						if (value == "true") boolval = true; 
						else boolval = false; 
						DataStatus::setWarnMissingData(boolval);
					}
					else {
						pm->parseError("Invalid preferences tag attribute : \"%s\"", attr.c_str());
						return false;
					}
				}
				return true;
			// Handle various path strings at end parse...
			} else if (StrCmpNoCase(tag, _exportFileNameTag) == 0 ||
				StrCmpNoCase(tag, _imageCapturePathTag) == 0 ||
				StrCmpNoCase(tag, _flowDirectoryPathTag) == 0 ||
				StrCmpNoCase(tag, _logFileNameTag) == 0 ||
				StrCmpNoCase(tag, _tfPathTag) == 0 ||
				StrCmpNoCase(tag, _sessionPathTag) == 0 ||
				StrCmpNoCase(tag, _metadataPathTag) == 0 ||
				StrCmpNoCase(tag, _autoSaveFilenameTag) == 0)
			{
				if (*attrs) {
					if (StrCmpNoCase(*attrs, _typeAttr) != 0) {
						pm->parseError("Invalid attribute : %s", *attrs);
						return(false);
					}
					attrs++;
					state->data_type = *attrs;
					attrs++;
					state->has_data = 1;
					return true;
				} else return false;
			} else if (StrCmpNoCase(tag, _probeDefaultsTag) == 0){
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);	
					float fltVal;
					ist >> fltVal;
					if (StrCmpNoCase(attr, _alphaAttr) == 0) {
						ProbeParams::setDefaultAlpha(fltVal);
					} else if (StrCmpNoCase(attr, _scaleAttr) == 0) {
						ProbeParams::setDefaultScale(fltVal);
					} else if (StrCmpNoCase(attr, _thetaAttr) == 0) {
						ProbeParams::setDefaultTheta(fltVal);
					} else if (StrCmpNoCase(attr, _phiAttr) == 0) {
						ProbeParams::setDefaultPhi(fltVal);
					} else if (StrCmpNoCase(attr, _psiAttr) == 0) {
						ProbeParams::setDefaultPsi(fltVal);
					}
					else {
						pm->parseError("Invalid preferences tag attribute : \"%s\"", attr.c_str());
						return false;
					}
				}
				return true;
			} else if (StrCmpNoCase(tag, _viewpointDefaultsTag) == 0){
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);	
					float fltVal[3];
					
					if (StrCmpNoCase(attr, _viewDirAttr) == 0) {
						ist >> fltVal[0];
						ist >> fltVal[1];
						ist >> fltVal[2];
						ViewpointParams::setDefaultViewDir(fltVal);
					} else if (StrCmpNoCase(attr, _upVecAttr) == 0) {
						ist >> fltVal[0];
						ist >> fltVal[1];
						ist >> fltVal[2];
						ViewpointParams::setDefaultUpVec(fltVal);
					} else if (StrCmpNoCase(attr, _diffuseCoeffAttr) == 0) {
						ist >> fltVal[0];
						ist >> fltVal[1];
						ist >> fltVal[2];
						ViewpointParams::setDefaultDiffuseCoeff(fltVal);
					} else if (StrCmpNoCase(attr, _specularCoeffAttr) == 0) {
						ist >> fltVal[0];
						ist >> fltVal[1];
						ist >> fltVal[2];
						ViewpointParams::setDefaultSpecularCoeff(fltVal);
					} else if (StrCmpNoCase(attr, _ambientCoeffAttr) == 0) {
						float val;
						ist >> val;
						ViewpointParams::setDefaultAmbientCoeff(val);
					} else if (StrCmpNoCase(attr, _specularExpAttr) == 0) {
						float val;
						ist >> val;
						ViewpointParams::setDefaultSpecularExp(val);
					} else if (StrCmpNoCase(attr, _numLightsAttr) == 0) {
						int val;
						ist >> val;
						ViewpointParams::setDefaultNumLights(val);
					} else if (StrCmpNoCase(attr, _lightDirectionAttr) == 0) {
						//Pull off 3 light direction vectors, one coord at a time
						for (int light = 0; light < 3; light++){
							ist >> fltVal[0];
							ist >> fltVal[1];
							ist >> fltVal[2];
							ViewpointParams::setDefaultLightDirection(light, fltVal);
						}
					}
					else {
						pm->parseError("Invalid preferences tag attribute : \"%s\"", attr.c_str());
						return false;
					}
				}
				return true;
			} else if (StrCmpNoCase(tag, _flowDefaultsTag) == 0){
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);	
					float fltVal;
					int intVal;
					
					if (StrCmpNoCase(attr, _smoothnessAttr) == 0) {
						ist >> fltVal;
						FlowParams::setDefaultSmoothness(fltVal);
					} else if (StrCmpNoCase(attr, _diamondSizeAttr) == 0) {
						ist >> fltVal;
						FlowParams::setDefaultDiamondSize(fltVal);
					} else if (StrCmpNoCase(attr, _integrationAccuracyAttr) == 0) {
						ist >> fltVal;
						FlowParams::setDefaultIntegrationAccuracy(fltVal);
					} else if (StrCmpNoCase(attr, _flowLengthAttr) == 0) {
						ist >> fltVal;
						FlowParams::setDefaultFlowLength(fltVal);
					} else if (StrCmpNoCase(attr, _flowDiameterAttr) == 0) {
						ist >> fltVal;
						FlowParams::setDefaultFlowDiameter(fltVal);
					} else if (StrCmpNoCase(attr, _arrowSizeAttr) == 0) {
						ist >> fltVal;
						FlowParams::setDefaultArrowSize(fltVal);
					} else if (StrCmpNoCase(attr, _geometryTypeAttr) == 0) {
						ist >> intVal;
						FlowParams::setDefaultGeometryType(intVal);
					} else {
						pm->parseError("Invalid preferences tag attribute : \"%s\"", attr.c_str());
						return false;
					}
				}
				return true;
			} else if (StrCmpNoCase(tag, _isoDefaultsTag) == 0){
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);	
					int intVal;
					ist >> intVal;
					if (StrCmpNoCase(attr, _isoDefaultBitsPerVoxelAttr) == 0) {
						ParamsIso::setDefaultBitsPerVoxel(intVal);
					} 
					else {
						pm->parseError("Invalid iso preferences tag attribute : \"%s\"", attr.c_str());
						return false;
					}
				}
				return true;
			} else if (StrCmpNoCase(tag, _dvrDefaultsTag) == 0){
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);	
					if (StrCmpNoCase(attr, _dvrDefaultBitsPerVoxelAttr) == 0) {
						int intVal;
						ist >> intVal;
						DvrParams::setDefaultBitsPerVoxel(intVal);
					} else if (StrCmpNoCase(attr, _dvrDefaultLightingAttr) == 0) {
						string boolVal;
						bool val;
						ist >> boolVal;
						if (boolVal == "true") val = true;
						else val = false;
						DvrParams::setDefaultLighting(val);
					} else if (StrCmpNoCase(attr, _dvrDefaultPreIntegrationAttr) == 0) {
						string boolVal;
						bool val;
						ist >> boolVal;
						if (boolVal == "true") val = true;
						else val = false;
						DvrParams::setDefaultPreIntegration(val);
					} else {
						pm->parseError("Invalid dvr preferences tag attribute : \"%s\"", attr.c_str());
						return false;
					}
				}
				return true;
			} else if (StrCmpNoCase(tag, _animationDefaultsTag) == 0){
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);	
					float fltVal;
					if (StrCmpNoCase(attr, _animationDefaultMaxWaitAttr) == 0) {
						ist >> fltVal;
						AnimationParams::setDefaultMaxWait(fltVal);
					} else if (StrCmpNoCase(attr, _animationDefaultMaxFPSAttr) == 0) {
						ist >> fltVal;
						AnimationParams::setDefaultMaxFPS(fltVal);
					} else {
						pm->parseError("Invalid animation preferences tag attribute : \"%s\"", attr.c_str());
						return false;
					}
				}
				return true;
			} else if (StrCmpNoCase(tag, _vizFeatureDefaultsTag) == 0){
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);	
					if (StrCmpNoCase(attr, _defaultShowAxisArrowsAttr) == 0) {
						string boolVal;
						bool val;
						ist >> boolVal;
						if (boolVal == "true") val = true;
						else val = false;
						GLWindow::setDefaultAxisArrows(val);
					} else if (StrCmpNoCase(attr, _defaultShowTerrainAttr) == 0) {
						string boolVal;
						bool val;
						ist >> boolVal;
						if (boolVal == "true") val = true;
						else val = false;
						GLWindow::setDefaultShowTerrain(val);
					} else {
						pm->parseError("Invalid viz feature preferences tag attribute : \"%s\"", attr.c_str());
						return false;
					}
				}
				return true;
			} else {
				pm->parseError("Invalid preferences tag  : \"%s\"", tag.c_str());
				return false;
			}
		}//end of child tags
		return false;
	} //end switch (depth)
	return false;
}

bool UserPreferences::elementEndHandler(ExpatParseMgr* pm, int depth, std::string& tag){
	//Need only to get path names
	if (depth != 1) return true;
	ExpatStackElement *state = pm->getStateStackTop();
	//Two tags don't have any data:
	if (StrCmpNoCase(tag, _messagesTag) == 0) return true;
	if (StrCmpNoCase(tag, _sceneColorsTag) == 0) return true;
	if (StrCmpNoCase(tag, _probeDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _viewpointDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _flowDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _isoDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _animationDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _vizFeatureDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _dvrDefaultsTag) == 0) return true;
	//all of these are supposed to be strings:
	if (StrCmpNoCase(state->data_type, _stringType) != 0) {
		return false;
	}
	const string &strdata = pm->getStringData();
	Session* ses = Session::getInstance();
	if (StrCmpNoCase(tag, _exportFileNameTag) == 0){
		ses->setExportFile(strdata.c_str());
	} else if (StrCmpNoCase(tag, _imageCapturePathTag) == 0){
		ses->setPrefJpegDirectory(strdata.c_str());
	} else if (StrCmpNoCase(tag, _flowDirectoryPathTag) == 0){
		ses->setPrefFlowDirectory(strdata.c_str());
	} else if (StrCmpNoCase(tag, _logFileNameTag) == 0){
		ses->setLogfileName(strdata.c_str());
	} else if (StrCmpNoCase(tag, _tfPathTag) == 0){
		ses->setPrefTFFilePath(strdata.c_str());
	} else if (StrCmpNoCase(tag, _sessionPathTag) == 0){
		ses->setPrefSessionDirectory(strdata.c_str());
	} else if (StrCmpNoCase(tag, _autoSaveFilenameTag) == 0){
		ses->setAutoSaveSessionFilename(strdata.c_str());
	} else if (StrCmpNoCase(tag, _metadataPathTag) == 0){
		ses->setPrefMetadataDirectory(strdata.c_str());
	} else {
		pm->parseError("Invalid preferences tag  : \"%s\"", tag.c_str());
		return false;
	}
	return true;
}
bool UserPreferences::loadPreferences(const char* filename){

	MessageReporter::infoMsg("Loading Preferences from %s", filename);
	ifstream is;
	is.open(filename);
	if (!is){//Report error if you can't open the file
		MessageReporter::errorMsg("Unable to open user preferences file: \n%s", filename);
		return false;
	}
	//Then set values from file.
	//Create a dummy userpreferences class that does the parsing
	UserPreferences* userPrefs = new UserPreferences;
	ExpatParseMgr* parseMgr = new ExpatParseMgr(userPrefs);
	parseMgr->parse(is);
	is.close();
	delete parseMgr;
	//Copy the preference paths to the session current paths:
	Session* ses = Session::getInstance();
	ses->setSessionDirectory(ses->getPrefSessionDirectory().c_str());
	ses->setMetadataDirectory(ses->getPrefMetadataDir().c_str());
	ses->setTFFilePath(ses->getPrefTFFilePath().c_str());
	ses->setFlowDirectory(ses->getPrefFlowDirectory().c_str());
	ses->setJpegDirectory(ses->getPrefJpegDirectory().c_str());

	delete userPrefs;
	MessageReporter::getInstance()->reset(Session::getInstance()->getLogfileName().c_str());
	MessageReporter::infoMsg("UserPreferences::loadPreferences() end");
	return true;
}
void UserPreferences::setDefault(){
	//Set all the params to defaults
	ProbeParams::setDefaultPrefs();
	DataStatus::setDefaultPrefs();
	FlowParams::setDefaultPrefs();
	DvrParams::setDefaultPrefs();
	ParamsIso::setDefaultPrefs();
	ViewpointParams::setDefaultPrefs();
	AnimationParams::setDefaultPrefs();
	RegionParams::setDefaultPrefs();
	GLWindow::setDefaultPrefs();
	MessageReporter::getInstance()->setDefaultPrefs();
	Session::getInstance()->setDefaultPrefs();
}
void UserPreferences::setDefaultDialog(){
	//Set all the params to defaults
	setDefault();
	//Copy the preference paths to the session current paths:
	Session* ses = Session::getInstance();
	ses->setSessionDirectory(ses->getPrefSessionDirectory().c_str());
	ses->setMetadataDirectory(ses->getPrefMetadataDir().c_str());
	ses->setTFFilePath(ses->getPrefTFFilePath().c_str());
	ses->setFlowDirectory(ses->getPrefFlowDirectory().c_str());
	ses->setJpegDirectory(ses->getPrefJpegDirectory().c_str());

	//Then load the values into this:
	setDialog();
	//Then exit, with offer to save to file:
	featureHolder->hide();
	PreferencesCommand::captureEnd(myPrefsCommand, this);
	requestSave();
	emit doneWithIt();
	

}
bool UserPreferences::savePreferences(const char* filename){
	
	ofstream os;
	os.open(filename);
	bool rc = saveToFile(os);
	os.close();
	return rc;
}
void UserPreferences::getTextChanges(){
	//copy values from text boxes to values in class:
	cacheMB = (size_t) cacheSizeEdit->text().toInt();
	texSize = textureSizeEdit->text().toInt();
	jpegQuality = jpegQualityEdit->text().toInt();
	sessionDir = sessionPathEdit->text().ascii();
	if (sessionDir == "" || sessionDir == "./" || sessionDir == ".\\")
		sessionDir = ".";
	autoSaveFilename = autoSaveFilenameEdit->text().ascii();
	metadataDir = metadataPathEdit->text().ascii();
	if (metadataDir == "" || metadataDir == "./" || metadataDir == ".\\")
		metadataDir = ".";
	autoSaveInterval = autoSaveIntervalEdit->text().toInt();
	tfPath = tfPathEdit->text().ascii();
	if (tfPath == "" || tfPath == "./" || tfPath == ".\\")
		tfPath = ".";
	logFileName = logFilePathEdit->text().ascii();
	flowPath = flowPathEdit->text().ascii();
	if (flowPath == "" || flowPath == "./" || flowPath == ".\\")
		flowPath = ".";
	jpegPath = jpegPathEdit->text().ascii();
	if (jpegPath == "" || jpegPath == "./" || jpegPath == ".\\")
		jpegPath = ".";
	logNum[0] = maxInfoLog->text().toInt();
	popupNum[0] = maxInfoPopup->text().toInt();
	logNum[1] = maxWarnLog->text().toInt();
	popupNum[1] = maxWarnPopup->text().toInt();
	logNum[2] = maxErrorLog->text().toInt();
	popupNum[2] = maxErrorPopup->text().toInt();
	scale = scaleEdit->text().toFloat();
	alpha = alphaEdit->text().toFloat();
	theta = thetaEdit->text().toFloat();
	phi = phiEdit->text().toFloat();
	psi = psiEdit->text().toFloat();
	viewDir[0] = viewDir0->text().toFloat();
	viewDir[1] = viewDir1->text().toFloat();
	viewDir[2] = viewDir2->text().toFloat();
	upVec[0] = upVec0->text().toFloat();
	upVec[1] = upVec1->text().toFloat();
	upVec[2] = upVec2->text().toFloat();
	//Lighting:
	lightDir[0][0] = lightPos00->text().toFloat();
	lightDir[0][1] = lightPos01->text().toFloat();
	lightDir[0][2] = lightPos02->text().toFloat();
	lightDir[1][0] = lightPos10->text().toFloat();
	lightDir[1][1] = lightPos11->text().toFloat();
	lightDir[1][2] = lightPos12->text().toFloat();
	lightDir[2][0] = lightPos20->text().toFloat();
	lightDir[2][1] = lightPos21->text().toFloat();
	lightDir[2][2] = lightPos22->text().toFloat();
	diffuseCoeff[0] = lightDiff0->text().toFloat();
	diffuseCoeff[1] = lightDiff1->text().toFloat();
	diffuseCoeff[2] = lightDiff2->text().toFloat();
	specularCoeff[0] = lightSpec0->text().toFloat();
	specularCoeff[1] = lightSpec1->text().toFloat();
	specularCoeff[2] = lightSpec2->text().toFloat();
	specularExp = shininessEdit->text().toFloat();
	ambientCoeff = ambientEdit->text().toFloat();
	numLights = numLightsEdit->text().toInt();

	flowLength = lengthEdit->text().toFloat();
	smoothness = smoothnessEdit->text().toFloat();
	integrationAccuracy = accuracyEdit->text().toFloat(); 
	flowDiameter = diameterEdit->text().toFloat();
	arrowSize = arrowEdit->text().toFloat(); 
	diamondSize =diamondEdit->text().toFloat();
	maxWait = maxWaitEdit->text().toFloat();
	maxFPS = maxFPSEdit->text().toFloat();;
	
}
//Attempt to load preferences from default location(s).
//If fail, return false.
bool UserPreferences::loadDefault(){
	bool gotFile = false;
	string filename;
	struct STAT64 statbuf;

	string prefFile = Session::getInstance()->getPreferencesFile();
	if (STAT64(prefFile.c_str(), &statbuf) >= 0) gotFile = true;
	
	if (gotFile){ 
		bool ok = loadPreferences(prefFile.c_str());
		if (ok) {
			MessageReporter::infoMsg("Loaded default preferences from %s",
				prefFile.c_str());
			return true;
		}
	}
	
	//Set preferences to defaults:
	setDefault();
	//Now copy the default to current:
	Session* ses = Session::getInstance();
	ses->setSessionDirectory(ses->getPrefSessionDirectory().c_str());
	ses->setMetadataDirectory(ses->getPrefMetadataDir().c_str());
	ses->setTFFilePath(ses->getPrefTFFilePath().c_str());
	ses->setFlowDirectory(ses->getPrefFlowDirectory().c_str());
	ses->setJpegDirectory(ses->getPrefJpegDirectory().c_str());
	MessageReporter::infoMsg("Set user preferences to defaults");
	return false;
	

}
