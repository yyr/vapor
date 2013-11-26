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
#include "glutil.h"	// Must be included first!!!
#include "vizfeatureparams.h"
#include "userpreferences.h"
#include "preferences.h"
#include "tabmanager.h"
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
#include <vapor/ParamNode.h>

#include "../images/fileopen.xpm"
#include "messagereporter.h"

#include "mainform.h"
#include "session.h"
#include <vapor/Version.h>
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

const string UserPreferences::_fidelityDefaultsTag = "FidelityDefaults";
const string UserPreferences::_tabOrderingTag = "TabOrdering";
const string UserPreferences::_preferencesTag = "UserPreferences";
const string UserPreferences::_sceneColorsTag = "SceneColors";
const string UserPreferences::_exportFileNameTag = "ExportFileName";
const string UserPreferences::_imageCapturePathTag = "ImageCapturePath";
const string UserPreferences::_flowDirectoryPathTag = "FlowPath";
const string UserPreferences::_pythonDirectoryPathTag = "PythonPath";
const string UserPreferences::_logFileNameTag = "LogFileName";
const string UserPreferences::_tfPathTag = "TransferFunctionPath";
const string UserPreferences::_metadataPathTag  = "MetadataPath";
const string UserPreferences::_sessionPathTag  = "SessionPath";
const string UserPreferences::_autoSaveFilenameTag = "AutoSaveFilename";
const string UserPreferences::_messagesTag = "MessageSettings";
const string UserPreferences::_autoSaveIntervalAttr = "AutoSaveInterval";
const string UserPreferences::_citationRemindAttr = "RemindCitation";
const string UserPreferences::_probeDefaultsTag = "ProbeDefaults";
const string UserPreferences::_thetaAttr = "DefaultTheta";
const string UserPreferences::_phiAttr = "DefaultPhi";
const string UserPreferences::_psiAttr = "DefaultPsi";
const string UserPreferences::_alphaAttr = "DefaultAlpha";
const string UserPreferences::_scaleAttr = "DefaultScale";
const string UserPreferences::_winWidthAttr = "WindowWidth";
const string UserPreferences::_winHeightAttr = "WindowHeight";
const string UserPreferences::_lockWinAttr = "LockWindowSize";
const string UserPreferences::_depthPeelAttr = "DepthPeeling";

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
const string UserPreferences::_defaultSpinAnimateAttr = "DefaultEnableSpin";

string UserPreferences::preferencesVersionString = "";
bool UserPreferences::depthPeelInState = false;
bool UserPreferences::firstPreferences = true;


//Create a new UserPreferences
UserPreferences::UserPreferences() : QDialog(0), Ui_Preferences(){
	setupUi(this);
	dialogChanged = false;
	textChangedFlag = false;
	featureHolder = 0;
	QPixmap* fileopenIcon = new QPixmap(fileopen);
	logFilePathButton->setIcon(QIcon(*fileopenIcon));
	sessionPathButton->setIcon(QIcon(*fileopenIcon));
	metadataPathButton->setIcon(QIcon(*fileopenIcon));
	jpegPathButton->setIcon(QIcon(*fileopenIcon));
	tfPathButton->setIcon(QIcon(*fileopenIcon));
	flowPathButton->setIcon(QIcon(*fileopenIcon));
	pythonPathButton->setIcon(QIcon(*fileopenIcon));
	autoSaveButton->setIcon(QIcon(*fileopenIcon));
	
}
//Just clone the exposed part, not the QT part
UserPreferences* UserPreferences::clone(){
	UserPreferences* newPrefs = new UserPreferences();
	newPrefs->textChangedFlag = false;
	newPrefs->featureHolder = 0;
	newPrefs->sessionDir=sessionDir;
	newPrefs->autoSaveFilename = autoSaveFilename;
	newPrefs->autoSaveInterval = autoSaveInterval;
	newPrefs->citationRemind = citationRemind;
	newPrefs->metadataDir=metadataDir;
	newPrefs->logFileName=logFileName;
	newPrefs->jpegPath=jpegPath;
	newPrefs->flowPath=flowPath;
	newPrefs->pythonPath=pythonPath;
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
	newPrefs->trackMouse=trackMouse;
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
	
	newPrefs->maxFPS = maxFPS;
	newPrefs->showAxisArrows = showAxisArrows;
	newPrefs->spinAnimate = spinAnimate;
	newPrefs->winWidth = winWidth;
	newPrefs->winHeight = winHeight;
	
	newPrefs->lockWin = lockWin;
	newPrefs->depthPeel = depthPeel;
	

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
	newPrefs->tabPositions = tabPositions;
	newPrefs->defaultLODFidelity2D = defaultLODFidelity2D;
	newPrefs->defaultLODFidelity3D = defaultLODFidelity3D;
	newPrefs->defaultRefFidelity2D=defaultRefFidelity2D;
	newPrefs->defaultRefFidelity3D=defaultRefFidelity3D;
	
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
	int w = 450;
	paramDefaultsFrame->hide();
	defaultDirectoryFrame->hide();
	defaultVizFeatureFrame->hide();
	
	if (w > MainForm::getInstance()->width()) w = MainForm::getInstance()->width();
	setGeometry(0, 0, w, 900);
	int swidth = sv->verticalScrollBar()->width();
	featureHolder->setGeometry(50, 50, w+swidth,h);

	sv->resize(w,h);
	featureHolder->updateGeometry();
	featureHolder->adjustSize();
//	sv->resizeContents(w,h);
	
	//Do connections for buttons
	connect (hideUnhideButton, SIGNAL(clicked()),this, SLOT(hideUnhidePressed()));
	connect (buttonLatestSession, SIGNAL(clicked()),this, SLOT(copyLatestSession()));
	connect (buttonLatestMetadata, SIGNAL(clicked()),this, SLOT(copyLatestMetadata()));
	connect (buttonLatestTF, SIGNAL(clicked()),this, SLOT(copyLatestTF()));
	connect (buttonLatestFlow, SIGNAL(clicked()),this, SLOT(copyLatestFlow()));
	connect (buttonLatestPython, SIGNAL(clicked()),this, SLOT(copyLatestPython()));
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
	connect (pythonPathButton, SIGNAL(clicked()), this, SLOT(choosePythonPath()));
	connect (backgroundColorButton, SIGNAL(clicked()), this, SLOT(selectBackgroundColor()));
	connect (regionFrameColorButton, SIGNAL(clicked()), this, SLOT(selectRegionFrameColor()));
	connect (subregionFrameColorButton, SIGNAL(clicked()), this, SLOT(selectSubregionFrameColor()));
	connect (resetCountButton, SIGNAL(clicked()), this, SLOT(resetCounts()));
	connect (enableSpinCheckbox, SIGNAL(toggled(bool)),this, SLOT(spinChanged(bool)));
	connect (lockSizeCheckBox, SIGNAL(toggled(bool)),this, SLOT(winLockChanged(bool)));
	connect (depthPeelingCheckbox, SIGNAL(toggled(bool)),this, SLOT(depthPeelChanged(bool)));

	connect (noShowCitationCheckbox, SIGNAL(toggled(bool)), this, SLOT(setNoCitation(bool)));
	connect (autoSaveCheckbox, SIGNAL(toggled(bool)), this, SLOT(setAutoSave(bool)));
	connect (regionCheckbox, SIGNAL(toggled(bool)), this, SLOT(regionChanged(bool)));
	connect (subregionCheckbox, SIGNAL(toggled(bool)), this, SLOT(subregionChanged(bool)));
	connect (textureSizeCheckbox, SIGNAL(toggled(bool)), this, SLOT(changeTextureSize(bool)));
	connect (missingDataCheckbox, SIGNAL(toggled(bool)), this, SLOT(warningChanged(bool)));
	connect (trackMouseCheckbox, SIGNAL(toggled(bool)), this, SLOT(trackMouseChanged(bool)));
	connect (lowerRefinementCheckbox, SIGNAL(toggled(bool)), this, SLOT(lowerResChanged(bool)));
	connect (dvrLightingCheckbox, SIGNAL(toggled(bool)), this, SLOT(dvrLightingChanged(bool)));
	connect (preIntegrationCheckbox, SIGNAL(toggled(bool)), this, SLOT(preIntegrationChanged(bool)));
	connect (axisArrowsCheckbox, SIGNAL(toggled(bool)), this, SLOT(axisArrowsChanged(bool)));
	
	connect (tabNameCombo,SIGNAL(currentIndexChanged(int)), this, SLOT(tabNameChanged(int)));
	connect (tabOrderSpin,SIGNAL(valueChanged(int)), this, SLOT(tabOrderChanged(int)));
	//TextBoxes:
	connect (cacheSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (textureSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (jpegQualityEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (winWidthEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (winHeightEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (sessionPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (autoSaveFilenameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (autoSaveIntervalEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (metadataPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (tfPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (logFilePathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (flowPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (pythonPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (jpegPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (maxInfoLog, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
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
	QPalette pal(regionFrameColorEdit->palette());
	QColor frameColor = QColorDialog::getColor(pal.color(QPalette::Base));
	if (!frameColor.isValid()) return;
	pal.setColor(QPalette::Base,frameColor);
	regionFrameColorEdit->setPalette(pal);
	regionFrameColor = frameColor;
	dialogChanged = true;
}

void UserPreferences::
selectSubregionFrameColor(){
	QPalette pal(subregionFrameColorEdit->palette());
	QColor frameColor = QColorDialog::getColor(pal.color(QPalette::Base));
	if (!frameColor.isValid()) return;
	pal.setColor(QPalette::Base,frameColor);
	subregionFrameColorEdit->setPalette(pal);
	subregionFrameColor = frameColor;
	dialogChanged = true;
}
void UserPreferences::
selectBackgroundColor(){
	//Launch colorselector, put result into the button
	QPalette pal(backgroundColorEdit->palette());
	QColor bgColor = QColorDialog::getColor(pal.color(QPalette::Base));
	if (!bgColor.isValid()) return;
	pal.setColor(QPalette::Base,bgColor);
	backgroundColorEdit->setPalette(pal);
	backgroundColor = bgColor;
	dialogChanged = true;
}
void UserPreferences::tabNameChanged(int val) {
	if (val< 0) return;
	currentTabIndex = val; 
	tabOrderSpin->setValue(tabPositions[val]);
	if (tabPositions[val] != 0) hideUnhideButton->setText("Hide");
	else hideUnhideButton->setText("Show");
	dialogChanged=true;
}
void UserPreferences::hideUnhidePressed(){
	if (tabPositions[currentTabIndex] > 0){ //Must hide it
		//All the values larger than this must be reduced:
		for (int i = 0; i<tabPositions.size(); i++){
			if (tabPositions[i]>tabPositions[currentTabIndex]) tabPositions[i]--;
		}
		tabPositions[currentTabIndex] = 0;
		tabOrderSpin->setValue(0);
		hideUnhideButton->setText("Show");
	} else {//Unhide it
		tabOrderSpin->setValue(1);
		hideUnhideButton->setText("Hide");
	}
	dialogChanged = true;
}
//Respond to user changing the position index of a tab, using the tabOrderSpin.
//If the position index changes, you need to swap with the tab that uses the new position index
//If it becomes 0, need to move everything else down 1.
//If a zero becomes 1, need to move everything else up 1.
void UserPreferences::tabOrderChanged(int newPosn){
	int oldPosn = tabPositions[currentTabIndex];
	if (oldPosn == newPosn) return;
	dialogChanged = false;
	//A 1 is changed to zero
	if (newPosn == 0) {
		assert(oldPosn == 1);
		tabPositions[currentTabIndex] = 0;
		for (int i = 0; i<tabPositions.size(); i++){
			if (tabPositions[i] > 0) tabPositions[i] = tabPositions[i] -1;
		}
		dialogChanged = true;
		return;
	}
	//A zero is changed to 1:
	if (oldPosn == 0) {
		assert(newPosn == 1);
		//Push everything up 1
		for (int i = 0; i<tabPositions.size(); i++){
			if (tabPositions[i] > 0) tabPositions[i] = tabPositions[i] +1;
		}
		tabPositions[currentTabIndex] = 1;
		dialogChanged=true;
		return;
	}
	//Find the one that is being displaced
	int otherPosn = -1;
	for (int i = 0; i< tabPositions.size(); i++){
		if (tabPositions[i] == newPosn){
			otherPosn = i;
			break;
		}
	}
	if(otherPosn < 0) {
		return; //Invalid index
	}
	//Now swap with otherPosn;
	tabPositions[otherPosn] = oldPosn;
	tabPositions[currentTabIndex] = newPosn;
	
	dialogChanged=true;
}
void UserPreferences::chooseSessionPath(){
	//Launch a file-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefSessionDirectory() == ".") dir = QDir::currentPath();
	else dir = Session::getInstance()->getPrefSessionDirectory().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            	"Choose the session file directory",
		dir);
	if (s != "") {
		sessionPathEdit->setText(s);
		sessionDir = s.toStdString();
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
		Session::getInstance()->setAutoSaveSessionFilename((const char*)s.toAscii());
		autoSaveFilename = s.toStdString();
		dialogChanged = true;
	}
}
void UserPreferences::chooseMetadataPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefMetadataDir() == ".") dir = QDir::currentPath();
	else dir = Session::getInstance()->getPrefMetadataDir().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            	"Choose the metadata directory",
		dir);
	if (s != "") {
		metadataPathEdit->setText(s);
		metadataDir = s.toStdString();
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
		MessageReporter::getInstance()->reset((const char*)s.toAscii());
		logFileName = s.toStdString();
		dialogChanged = true;
	}
}
void UserPreferences::chooseJpegPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefJpegDirectory() == ".") dir = QDir::currentPath();
	else dir = Session::getInstance()->getPrefJpegDirectory().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            "Choose the jpeg (image) file directory",
		dir);
	if (s != "") {
		jpegPathEdit->setText(s);
		jpegPath = s.toStdString();
		dialogChanged = true;
	}
}
void UserPreferences::chooseTFPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefTFFilePath() == ".") dir = QDir::currentPath();
	else dir = Session::getInstance()->getPrefTFFilePath().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            "Choose the transfer function file directory",
		dir);
	if (s != "") {
		tfPathEdit->setText(s);
		tfPath = s.toStdString();
		dialogChanged = true;
	}
}
void UserPreferences::chooseFlowPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefFlowDirectory() == ".") dir = QDir::currentPath();
	else dir = Session::getInstance()->getPrefFlowDirectory().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            "Choose the flow file directory",
		dir);
	if (s != "") {
		flowPathEdit->setText(s);
		flowPath = s.toStdString();
		dialogChanged = true;
	}
}
void UserPreferences::choosePythonPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString dir;
	if (Session::getInstance()->getPrefPythonDirectory() == ".") dir = QDir::currentPath();
	else dir = Session::getInstance()->getPrefPythonDirectory().c_str();
	QString s = QFileDialog::getExistingDirectory(this,
            "Choose the Python script file directory",
		dir);
	if (s != "") {
		pythonPathEdit->setText(s);
		pythonPath = s.toStdString();
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
void UserPreferences::copyLatestPython(){
	pythonPath = Session::getInstance()->getPythonDirectory();
	pythonPathEdit->setText(pythonPath.c_str());
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
void UserPreferences::winLockChanged(bool enabled){
	lockWin = enabled;
	dialogChanged = true;
}
void UserPreferences::depthPeelChanged(bool enabled){
	depthPeel = enabled;
	dialogChanged = true;
	MessageReporter::warningMsg("Note that this transparency change will not take effect until the preferences are saved and VAPOR GUI is restarted.");
}
void UserPreferences::subregionChanged(bool enabled){
	subregionFrameEnabled = enabled;
	dialogChanged = true;
}
void UserPreferences::warningChanged(bool enabled){
	warnDataMissing = enabled;
	dialogChanged = true;
}
void UserPreferences::trackMouseChanged(bool enabled){
	trackMouse = enabled;
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

void UserPreferences::spinChanged(bool val){
	spinAnimate = val;
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
void UserPreferences::setNoCitation(bool val){
	if (val) { //check to turn off citation reminder
		citationRemind = false;
	} else {
		citationRemind = true;
	}
	dialogChanged=true;
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
	defaultLODFidelity2D = ses->getDefaultLODFidelity2D();
	defaultLODFidelity3D = ses->getDefaultLODFidelity3D();
	defaultRefFidelity2D = ses->getDefaultRefinementFidelity2D();
	defaultRefFidelity3D = ses->getDefaultRefinementFidelity3D();
	
	cacheMB = ses->getCacheMB();
	winWidth = ses->getLockWinWidth();
	winHeight = ses->getLockWinHeight();
	lockWin = ses->getWindowSizeLock();
	depthPeel = depthPeelInState;
	depthPeelingCheckbox->setChecked(depthPeel);
	cacheSizeEdit->setText(QString::number(ses->getCacheMB()));
	winWidthEdit->setText(QString::number(winWidth));
	winHeightEdit->setText(QString::number(winHeight));
	lockSizeCheckBox->setChecked(lockWin);
	texSize = ses->getTextureSize();
	textureSizeEdit->setText(
			QString::number(texSize));
	jpegQuality = GLWindow::getJpegQuality();
	jpegQualityEdit->setText(QString::number(jpegQuality));
	autoSaveInterval = ses->getAutoSaveInterval();
	citationRemind = ses->getCitationRemindDefault();
	if (autoSaveInterval < 0) autoSaveInterval = 0;
	autoSaveIntervalEdit->setText(QString::number(autoSaveInterval));
	autoSaveCheckbox->setChecked(autoSaveInterval > 0);
	noShowCitationCheckbox->setChecked(!citationRemind);
	warnDataMissing = DataStatus::warnIfDataMissing();
	missingDataCheckbox->setChecked(warnDataMissing);
	trackMouse = DataStatus::trackMouse();
	trackMouseCheckbox->setChecked(trackMouse);
	useLowerRefinement = DataStatus::useLowerAccuracy();
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
	pythonPath = ses->getPrefPythonDirectory();
	autoSaveFilename = ses->getAutoSaveSessionFilename();

	//Directories:
	sessionPathEdit->setText(ses->getPrefSessionDirectory().c_str());
	autoSaveFilenameEdit->setText(autoSaveFilename.c_str());
	metadataPathEdit->setText(ses->getPrefMetadataDir().c_str());
	logFilePathEdit->setText(ses->getLogfileName().c_str());
	jpegPathEdit->setText(ses->getPrefJpegDirectory().c_str());
	tfPathEdit->setText(ses->getPrefTFFilePath().c_str());
	flowPathEdit->setText(ses->getPrefFlowDirectory().c_str());
	pythonPathEdit->setText(ses->getPrefPythonDirectory().c_str());
	MessageReporter* mReporter = MessageReporter::getInstance();
	for (int i = 0; i< 3; i++){
		popupNum[i] = mReporter->getMaxPopup((MessageReporter::messagePriority)i);
		logNum[i] = mReporter->getMaxLog((MessageReporter::messagePriority)i);
	}
	maxInfoPopup->setText("0");
	maxWarnPopup->setText(QString::number(mReporter->getMaxPopup((MessageReporter::messagePriority)1)));
	maxErrorPopup->setText(QString::number(mReporter->getMaxPopup((MessageReporter::messagePriority)2)));
	maxInfoLog->setText(QString::number(mReporter->getMaxLog((MessageReporter::messagePriority)0)));
	maxWarnLog->setText(QString::number(mReporter->getMaxLog((MessageReporter::messagePriority)1)));
	maxErrorLog->setText(QString::number(mReporter->getMaxLog((MessageReporter::messagePriority)2)));
	QPalette pal(backgroundColorEdit->palette());
	pal.setColor(QPalette::Base,DataStatus::getBackgroundColor());
	backgroundColorEdit->setPalette(pal);
	regionCheckbox->setChecked(DataStatus::regionFrameIsEnabled());
	pal.setColor(QPalette::Base,DataStatus::getRegionFrameColor());
	regionFrameColorEdit->setPalette(pal);
	subregionCheckbox->setChecked(DataStatus::subregionFrameIsEnabled());
	pal.setColor(QPalette::Base,DataStatus::getSubregionFrameColor());
	subregionFrameColorEdit->setPalette(pal);
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
	geometryCombo->setCurrentIndex(geometryType);
	isoBitsPerVoxel = ParamsIso::getDefaultBitsPerVoxel();
	dvrBitsPerVoxel = DvrParams::getDefaultBitsPerVoxel();
	dvrLighting = DvrParams::getDefaultLightingEnabled();
	dvrPreIntegration = DvrParams::getDefaultPreIntegrationEnabled();
	
	maxFPS = AnimationParams::getDefaultMaxFPS();
	showAxisArrows = GLWindow::getDefaultAxisArrowsEnabled();
	
	spinAnimate= GLWindow::getDefaultSpinAnimateEnabled();
	isoBitsCombo->setCurrentIndex((isoBitsPerVoxel == 16) ? 1 : 0);
	dvrBitsCombo->setCurrentIndex((dvrBitsPerVoxel == 16) ? 1 : 0);
	dvrLightingCheckbox->setChecked(dvrLighting);
	preIntegrationCheckbox->setChecked(dvrPreIntegration);
	
	maxFPSEdit->setText(QString::number(maxFPS));
	axisArrowsCheckbox->setChecked(showAxisArrows);
	enableSpinCheckbox->setChecked(spinAnimate);

	//Initialize the tab ordering based on current ordering in TabManager
	const vector<long>& tabOrdering = TabManager::getTabOrdering();
	tabPositions = tabOrdering;  //save a copy
	tabNameCombo->clear();
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		tabNameCombo->addItem(QString::fromStdString(Params::paramName(i)));
	}
	tabNameCombo->setCurrentIndex(0);
	currentTabIndex = 0;
	tabOrderSpin->setMaximum(Params::GetNumParamsClasses());
	tabOrderSpin->setValue(tabPositions[0]);
	if (tabPositions[0] == 0) hideUnhideButton->setText("Show");
	else hideUnhideButton->setText("Hide");

}
void UserPreferences::
applyToState(bool orderTabs){
	
	//Copy from this (not QDialog) to session
	Session* ses = Session::getInstance();
	//If fidelity default changes, need to force all fidelities to update
	bool fidelityChanged = false;
	if (ses->getDefaultRefinementFidelity2D() != defaultRefFidelity2D ||
		ses->getDefaultRefinementFidelity3D() != defaultRefFidelity3D ||
		ses->getDefaultLODFidelity2D() != defaultLODFidelity2D ||
		ses->getDefaultLODFidelity3D() != defaultLODFidelity3D){
			ses->setDefaultLODFidelity2D(defaultLODFidelity2D);
			ses->setDefaultLODFidelity3D(defaultLODFidelity3D);
			ses->setDefaultRefinementFidelity2D(defaultRefFidelity2D);
			ses->setDefaultRefinementFidelity3D(defaultRefFidelity3D);
			VizWinMgr::getInstance()->forceFidelityUpdate();
	}
	ses->setCacheMB(cacheMB);
	ses->setLockWinWidth(winWidth);
	ses->setLockWinHeight(winHeight);
	ses->setWindowSizeLock(lockWin);
	ses->setTextureSize(texSize);
	ses->specifyTextureSize(texSizeSpecified);
	ses->setAutoSaveInterval(autoSaveInterval);
	//Only set CitationRemind if it's changed:
	if (ses->getCitationRemindDefault() != citationRemind){
		ses->setCitationRemindDefault(citationRemind);
		ses->setCitationRemind(citationRemind);
	}
	GLWindow::setJpegQuality(jpegQuality);
	depthPeelInState = depthPeel;
	DataStatus::setWarnMissingData(warnDataMissing);
	DataStatus::setTrackMouse(trackMouse);
	DataStatus::setUseLowerAccuracy(useLowerRefinement);
	
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
	ses->setPrefPythonDirectory(pythonPath.c_str());
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
	
	AnimationParams::setDefaultMaxFPS(maxFPS);
	GLWindow::setDefaultAxisArrows(showAxisArrows);
	GLWindow::setDefaultSpinAnimate(spinAnimate);
	GLWindow::setSpinAnimation(spinAnimate);
	
	TabManager::setTabOrdering(tabPositions);
	if (orderTabs)MainForm::getInstance()->getTabManager()->orderTabs();
	
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
void UserPreferences::requestSave(bool prompt){
	if(prompt){
		int rc = QMessageBox::question(0, "Save User Preferences?", 
			"User Preferences have changed.\nDo you want to save them to file?",
			 QMessageBox::Yes|QMessageBox::Default,QMessageBox::No,Qt::NoButton);
		if (rc != QMessageBox::Yes) return;
	}
	
	QString filename = QFileDialog::getSaveFileName(MainForm::getInstance(),
            	"Select the filename for saving user preferences", 
		Session::getPreferencesFile().c_str(),
            	".*");
	
	if(filename.length() == 0) return;
	ofstream os;
	os.open((const char*)filename.toAscii());

	if (!os || !saveToFile(os)){//Report error if you can't open the file
		MessageReporter::errorMsg("Unable to open preferences file: \n%s", (const char*)filename.toAscii());
	}
	os.close();
	
}
bool UserPreferences::
saveToFile(ofstream& ofs ){
	ParamNode* rootNode = buildNode();
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
	        h+=300;	
		showDefaultsButton->setToolTip("Click to hide the session defaults");
		w = 950;
		paramDefaultsFrame->show();
		defaultDirectoryFrame->show();
		defaultVizFeatureFrame->show();
	}
	else {
		showDefaultsButton->setText("Session Defaults");
		showDefaultsButton->setToolTip("Display the session defaults that will apply when the application is restarted or a new dataset is loaded");
		w = 480;
		paramDefaultsFrame->hide();
		defaultDirectoryFrame->hide();
		defaultVizFeatureFrame->hide();
	}
	if (w > MainForm::getInstance()->width()) w = MainForm::getInstance()->width();
	setGeometry(0, 0, w, h);
	int swidth = sv->verticalScrollBar()->width();
	if (h>768) h = 768;
	featureHolder->setGeometry(50, 50, w+swidth,h);

	sv->resize(w,h);
	adjustSize();
	featureHolder->updateGeometry();
	featureHolder->adjustSize();
	featureHolder->update();
	
}
//Build the XML for user preferences, based on the state of the app
ParamNode* UserPreferences::buildNode(){
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
	oss << (long)ses->getLockWinWidth();
	attrs[_winWidthAttr] = oss.str();
	oss.str(empty);
	oss << (long)ses->getLockWinHeight();
	attrs[_winHeightAttr] = oss.str();
	oss.str(empty);
	if (ses->getWindowSizeLock())
		oss << "true";
	else 
		oss << "false";
	attrs[_lockWinAttr] = oss.str();
	oss.str(empty);
	if (depthPeelInState)
		oss << "true";
	else 
		oss << "false";
	attrs[_depthPeelAttr] = oss.str();

	oss.str(empty);
	if (DataStatus::textureSizeIsSpecified())
		oss << "true";
	else 
		oss << "false";
	attrs[Session::_specifyTextureSizeAttr] = oss.str();

	oss.str(empty);
	if (Session::getInstance()->getCitationRemindDefault())
		oss << "true";
	else 
		oss << "false";
	attrs[UserPreferences::_citationRemindAttr] = oss.str();

	attrs[Session::_VAPORVersionAttr] = Version::GetVersionString();


	oss.str(empty);
	oss << (long)DataStatus::getTextureSize();
	attrs[Session::_textureSizeAttr] = oss.str();
	 
	oss.str(empty);
	oss << (long)ses->getAutoSaveInterval();
	attrs[Session::_autoSaveIntervalAttr] = oss.str();

	ParamNode* mainNode = new ParamNode(_preferencesTag, attrs, 10);
//create element for each path or filename
	mainNode->SetElementString(_metadataPathTag, ses->getPrefMetadataDir());
	mainNode->SetElementString(_sessionPathTag, ses->getPrefSessionDirectory());
	mainNode->SetElementString(_autoSaveFilenameTag, ses->getAutoSaveSessionFilename());
	mainNode->SetElementString(_tfPathTag, ses->getPrefTFFilePath());
	mainNode->SetElementString(_imageCapturePathTag, ses->getPrefJpegDirectory());
	mainNode->SetElementString(_flowDirectoryPathTag, ses->getPrefFlowDirectory());
	mainNode->SetElementString(_pythonDirectoryPathTag, ses->getPrefPythonDirectory());
	mainNode->SetElementString(_exportFileNameTag, ses->getExportFile());
	mainNode->SetElementString(_logFileNameTag, ses->getLogfileName());
	mainNode->SetElementLong(_tabOrderingTag, TabManager::getTabOrdering());

	vector<double> fidelityDefaults;
	fidelityDefaults.push_back(ses->getDefaultRefinementFidelity2D());
	fidelityDefaults.push_back(ses->getDefaultRefinementFidelity3D());
	fidelityDefaults.push_back(ses->getDefaultLODFidelity2D());
	fidelityDefaults.push_back(ses->getDefaultLODFidelity3D());
	mainNode->SetElementDouble(_fidelityDefaultsTag,fidelityDefaults);
	
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
	if (DataStatus::trackMouse())
		oss << "true";
	else 
		oss << "false";
	attrs[DataStatus::_trackMouseAttr] = oss.str();

	oss.str(empty);
	if (DataStatus::useLowerAccuracy())
		oss << "true";
	else 
		oss << "false";
	attrs[DataStatus::_useLowerRefinementAttr] = oss.str();

	ParamNode* msgNode = new ParamNode(_messagesTag, attrs, 0);
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
			
	ParamNode* colorNode = new ParamNode(_sceneColorsTag, attrs, 0);

	

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
	ParamNode* defaultProbeNode = new ParamNode(_probeDefaultsTag, attrs, 0);
	
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
	
	
	ParamNode* defaultViewpointNode = new ParamNode(_viewpointDefaultsTag, attrs, 0);
	
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
	ParamNode* defaultFlowNode = new ParamNode(_flowDefaultsTag, attrs, 0);
	mainNode->AddChild(defaultFlowNode);

	//Create a node for iso defaults:
	attrs.clear();
	oss.str(empty);
	oss << (int)ParamsIso::getDefaultBitsPerVoxel();
	attrs[_isoDefaultBitsPerVoxelAttr] = oss.str();
	ParamNode* defaultIsoNode = new ParamNode(_isoDefaultsTag, attrs, 0);
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
	ParamNode* defaultDvrNode = new ParamNode(_dvrDefaultsTag, attrs, 0);
	mainNode->AddChild(defaultDvrNode);

	//Create a node for Animation Defaults
	attrs.clear();
	
	oss.str(empty);
	oss << (float)AnimationParams::getDefaultMaxFPS();
	attrs[_animationDefaultMaxFPSAttr] = oss.str();

	ParamNode* defaultAnimationNode = new ParamNode(_animationDefaultsTag, attrs, 0);
	mainNode->AddChild(defaultAnimationNode);

	//Create a node for VizFeature Defaults
	attrs.clear();
	oss.str(empty);
	str = GLWindow::getDefaultAxisArrowsEnabled() ? "true" : "false";
	oss << str;
	attrs[_defaultShowAxisArrowsAttr] = oss.str();

	oss.str(empty);
	str = GLWindow::getDefaultSpinAnimateEnabled() ? "true" : "false";
	oss << str;
	attrs[_defaultSpinAnimateAttr] = oss.str();

	ParamNode* defaultVizFeatureNode = new ParamNode(_vizFeatureDefaultsTag, attrs, 0);
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
				if (StrCmpNoCase(attr, UserPreferences::_citationRemindAttr) == 0) {
					string boolVal;
					bool val;
					ist >> boolVal;
					if (boolVal == "true") val = true;
					else val = false;
					ses->setCitationRemindDefault(val);
					ses->setCitationRemind(val);
				}
				else if (StrCmpNoCase(attr, Session::_VAPORVersionAttr) == 0){
					ist >> preferencesVersionString;
				}
				else if (StrCmpNoCase(attr, _winWidthAttr) == 0){
					ist >> winWidth;
					ses->setLockWinWidth(winWidth);
				}
				else if (StrCmpNoCase(attr, _winHeightAttr) == 0){
					ist >> winHeight;
					ses->setLockWinHeight(winHeight);
				}
				else if (StrCmpNoCase(attr, _lockWinAttr) == 0) {
					string boolVal;
					bool val;
					ist >> boolVal;
					if (boolVal == "true") val = true;
					else val = false;
					ses->setWindowSizeLock(val);
					lockWin=val;
				}
				else if (StrCmpNoCase(attr, _depthPeelAttr) == 0) {
					string boolVal;
					bool val;
					ist >> boolVal;
					if (boolVal == "true") val = true;
					else val = false;
					depthPeelInState = val;
					depthPeel=val;
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
						msgRpt->setMaxPopup(MessageReporter::Info, 0); //Always maxpopup is zero for info
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
						DataStatus::setUseLowerAccuracy(boolval);
					}
					else if (StrCmpNoCase(attr, DataStatus::_missingDataWarningAttr) == 0) {
						bool boolval;
						if (value == "true") boolval = true; 
						else boolval = false; 
						DataStatus::setWarnMissingData(boolval);
					}
					else if (StrCmpNoCase(attr, DataStatus::_trackMouseAttr) == 0) {
						bool boolval;
						if (value == "true") boolval = true; 
						else boolval = false; 
						DataStatus::setTrackMouse(boolval);
					}
					else {
					}
				}
				return true;
			// Handle various path strings and tabOrdering at end parse...
			} else if (StrCmpNoCase(tag, _exportFileNameTag) == 0 ||
				StrCmpNoCase(tag, _imageCapturePathTag) == 0 ||
				StrCmpNoCase(tag, _flowDirectoryPathTag) == 0 ||
				StrCmpNoCase(tag, _pythonDirectoryPathTag) == 0 ||
				StrCmpNoCase(tag, _logFileNameTag) == 0 ||
				StrCmpNoCase(tag, _tfPathTag) == 0 ||
				StrCmpNoCase(tag, _sessionPathTag) == 0 ||
				StrCmpNoCase(tag, _metadataPathTag) == 0 ||
				StrCmpNoCase(tag, _autoSaveFilenameTag) == 0 ||
				StrCmpNoCase(tag, _tabOrderingTag) == 0 ||
				StrCmpNoCase(tag, _fidelityDefaultsTag) == 0)
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
				} 
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
						//Obsolete, do nothing
					} else if (StrCmpNoCase(attr, _animationDefaultMaxFPSAttr) == 0) {
						ist >> fltVal;
						AnimationParams::setDefaultMaxFPS(fltVal);
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
						//obsolete
					} else if (StrCmpNoCase(attr, _defaultSpinAnimateAttr) == 0) {
						string boolVal;
						bool val;
						ist >> boolVal;
						if (boolVal == "true") val = true;
						else val = false;
						GLWindow::setDefaultSpinAnimate(val);
						GLWindow::setSpinAnimation(val);
					} 
				}
				return true;
			} else {
				pm->skipElement(tag, depth);
				return true;
			}
		}//end of child tags
		default: 
			pm->skipElement(tag, depth);
			return true;
	} //end switch (depth)
	pm->skipElement(tag, depth);
	return true;
}

bool UserPreferences::elementEndHandler(ExpatParseMgr* pm, int depth, std::string& tag){
	//Need only to get path names
	if (depth != 1) return true;
	//ExpatStackElement *state = pm->getStateStackTop();
	//These tags don't have any data:
	if (StrCmpNoCase(tag, _messagesTag) == 0) return true;
	if (StrCmpNoCase(tag, _sceneColorsTag) == 0) return true;
	if (StrCmpNoCase(tag, _probeDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _viewpointDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _flowDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _isoDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _animationDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _vizFeatureDefaultsTag) == 0) return true;
	if (StrCmpNoCase(tag, _dvrDefaultsTag) == 0) return true;

	const string &strdata = pm->getStringData();
	const vector<long>& longdata = pm->getLongData();
	const vector<double>& doubledata = pm->getDoubleData();
	Session* ses = Session::getInstance();
	if (StrCmpNoCase(tag, _exportFileNameTag) == 0){
		ses->setExportFile(strdata.c_str());
	} else if (StrCmpNoCase(tag, _imageCapturePathTag) == 0){
		ses->setPrefJpegDirectory(strdata.c_str());
	} else if (StrCmpNoCase(tag, _flowDirectoryPathTag) == 0){
		ses->setPrefFlowDirectory(strdata.c_str());
	} else if (StrCmpNoCase(tag, _pythonDirectoryPathTag) == 0){
		ses->setPrefPythonDirectory(strdata.c_str());
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
	} else if (StrCmpNoCase(tag, _tabOrderingTag) == 0){
		tabPositions = longdata;
	} else if (StrCmpNoCase(tag, _fidelityDefaultsTag) == 0){
		defaultRefFidelity2D = doubledata[0]; 
		defaultRefFidelity3D = doubledata[1]; 
		defaultLODFidelity2D = doubledata[2]; 
		defaultLODFidelity3D = doubledata[3]; 
		ses->setDefaultLODFidelity2D(defaultLODFidelity2D);
		ses->setDefaultLODFidelity3D(defaultLODFidelity3D);
		ses->setDefaultRefinementFidelity2D(defaultRefFidelity2D);
		ses->setDefaultRefinementFidelity3D(defaultRefFidelity3D);
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
	userPrefs->tabPositions.clear();
	//INitialize to default fidelity, for pre 2.3 preferences
	userPrefs->defaultRefFidelity2D = 4.f;
	userPrefs->defaultRefFidelity3D = 4.f;
	userPrefs->defaultLODFidelity2D = 2.f;
	userPrefs->defaultLODFidelity3D = 2.f;
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
	ses->setPythonDirectory(ses->getPrefPythonDirectory().c_str());
	ses->setJpegDirectory(ses->getPrefJpegDirectory().c_str());
	if (userPrefs->tabPositions.size()> 0){
		TabManager::setTabOrdering(userPrefs->tabPositions);
		//Only apply the tab ordering if the Params have already been set up:
		if (Params::GetNumParamsClasses()>0)
			MainForm::getInstance()->getTabManager()->orderTabs();
	}

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
	vector<long> defaultOrder;
	for (int i = 1; i<=Params::GetNumParamsClasses(); i++)
		defaultOrder.push_back(i);
	TabManager::setTabOrdering(defaultOrder);
	MainForm::getInstance()->getTabManager()->orderTabs();

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
	ses->setPythonDirectory(ses->getPrefPythonDirectory().c_str());
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
	size_t newCacheMB = (size_t) cacheSizeEdit->text().toInt();
	if (newCacheMB != cacheMB){
		MessageReporter::warningMsg("Note that cache size change will not take effect until the next time data is loaded");
	}
	cacheMB = newCacheMB;
	texSize = textureSizeEdit->text().toInt();
	jpegQuality = jpegQualityEdit->text().toInt();
	winWidth = winWidthEdit->text().toInt();
	winHeight = winHeightEdit->text().toInt();
	
	sessionDir = sessionPathEdit->text().toStdString();
	if (sessionDir == "" || sessionDir == "./" || sessionDir == ".\\")
		sessionDir = ".";
	autoSaveFilename = autoSaveFilenameEdit->text().toStdString();
	metadataDir = metadataPathEdit->text().toStdString();
	if (metadataDir == "" || metadataDir == "./" || metadataDir == ".\\")
		metadataDir = ".";
	autoSaveInterval = autoSaveIntervalEdit->text().toInt();
	tfPath = tfPathEdit->text().toStdString();
	if (tfPath == "" || tfPath == "./" || tfPath == ".\\")
		tfPath = ".";
	logFileName = logFilePathEdit->text().toStdString();
	flowPath = flowPathEdit->text().toStdString();
	if (flowPath == "" || flowPath == "./" || flowPath == ".\\")
		flowPath = ".";
	pythonPath = pythonPathEdit->text().toStdString();
	if (pythonPath == "" || pythonPath == "./" || pythonPath == ".\\")
		pythonPath = ".";
	jpegPath = jpegPathEdit->text().toStdString();
	if (jpegPath == "" || jpegPath == "./" || jpegPath == ".\\")
		jpegPath = ".";
	logNum[0] = maxInfoLog->text().toInt();
	popupNum[0] = 0;
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
			if (firstPreferences) GLWindow::enableDepthPeeling(UserPreferences::depthPeelIsInState());
			firstPreferences = false;
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
	ses->setPythonDirectory(ses->getPrefPythonDirectory().c_str());
	ses->setJpegDirectory(ses->getPrefJpegDirectory().c_str());
	MessageReporter::infoMsg("Set user preferences to defaults");
	return false;
}
