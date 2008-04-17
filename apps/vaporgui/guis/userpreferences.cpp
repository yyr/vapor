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
#include "vizfeatureparams.h"
#include "userpreferences.h"
#include "command.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

#include "messagereporter.h"
#include "mainform.h"
#include "preferences.h"
#include "session.h"
#include "vapor/Version.h"
#include <qlineedit.h>
#include <qfiledialog.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qscrollview.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qwhatsthis.h>
const string UserPreferences::_preferencesTag = "UserPreferences";
const string UserPreferences::_sceneColorsTag = "SceneColors";
const string UserPreferences::_exportFileNameTag = "ExportFileName";
const string UserPreferences::_imageCapturePathTag = "ImageCapturePath";
const string UserPreferences::_flowDirectoryPathTag = "FlowPath";
const string UserPreferences::_logFileNameTag = "LogFileName";
const string UserPreferences::_tfPathTag = "TransferFunctionPath";
const string UserPreferences::_metadataPathTag  = "MetadataPath";
const string UserPreferences::_sessionPathTag  = "SessionPath";
const string UserPreferences::_messagesTag = "MessageSettings";
string UserPreferences::preferencesVersionString = "";

using namespace VAPoR;
//Create a new UserPreferences
UserPreferences::UserPreferences() : Preferences(){
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
	return newPrefs;
}
//Set up the dialog with current parameters from session state
void UserPreferences::launch(){
	dialogChanged = false;
	
	featureHolder = new ScrollContainer((QWidget*)MainForm::getInstance(), "Set User Preferences");
	
	QScrollView* sv = new QScrollView(featureHolder);
	sv->setHScrollBarMode(QScrollView::AlwaysOff);
	sv->setVScrollBarMode(QScrollView::AlwaysOn);
	featureHolder->setScroller(sv);
	sv->addChild(this);
	
	
	//Copy values from session into this
	setDialog();
	//Put current state in command queue
	myPrefsCommand = PreferencesCommand::captureStart(this, "edit user preferences");
	
	int h = MainForm::getInstance()->height();
	if ( h > 750) h = 750;
	int w = 460;
	
	setGeometry(0, 0, w, h);
	int swidth = sv->verticalScrollBar()->width();
	featureHolder->setGeometry(50, 50, w+swidth,h);
	
	sv->resizeContents(w,h);
	
	//Do connections for buttons
	connect (sessionPathButton, SIGNAL(clicked()), this, SLOT(chooseSessionPath()));
	connect (metadataPathButton, SIGNAL(clicked()), this, SLOT(chooseMetadataPath()));
	connect (logFilePathButton, SIGNAL(clicked()), this, SLOT(chooseLogFilePath()));
	connect (jpegPathButton, SIGNAL(clicked()), this, SLOT(chooseJpegPath()));
	connect (tfPathButton, SIGNAL(clicked()), this, SLOT(chooseTFPath()));
	connect (flowPathButton, SIGNAL(clicked()), this, SLOT(chooseFlowPath()));
	connect (backgroundColorButton, SIGNAL(clicked()), this, SLOT(selectBackgroundColor()));
	connect (regionFrameColorButton, SIGNAL(clicked()), this, SLOT(selectRegionFrameColor()));
	connect (subregionFrameColorButton, SIGNAL(clicked()), this, SLOT(selectSubregionFrameColor()));
	connect (resetCountButton, SIGNAL(clicked()), this, SLOT(resetCounts()));

	connect (regionCheckbox, SIGNAL(toggled(bool)), this, SLOT(regionChanged(bool)));
	connect (subregionCheckbox, SIGNAL(toggled(bool)), this, SLOT(subregionChanged(bool)));
	connect (textureSizeCheckbox, SIGNAL(toggled(bool)), this, SLOT(changeTextureSize(bool)));
	connect (missingDataCheckbox, SIGNAL(toggled(bool)), this, SLOT(warningChanged(bool)));
	connect (lowerRefinementCheckbox, SIGNAL(toggled(bool)), this, SLOT(lowerResChanged(bool)));
	

	//TextBoxes:
	connect (cacheSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (textureSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (jpegQualityEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
	connect (sessionPathEdit, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged()));
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
	QColor prevColor = regionFrameColorButton->paletteBackgroundColor();
	QColor frameColor = QColorDialog::getColor(prevColor,0,0);
	regionFrameColorButton->setPaletteBackgroundColor(frameColor);
	regionFrameColor = frameColor;
	dialogChanged = true;
}

void UserPreferences::
selectSubregionFrameColor(){
	QColor frameColor = QColorDialog::getColor(subregionFrameColorButton->paletteBackgroundColor(),0,0);
	subregionFrameColorButton->setPaletteBackgroundColor(frameColor);
	subregionFrameColor = frameColor;
	dialogChanged = true;
}
void UserPreferences::
selectBackgroundColor(){
	//Launch colorselector, put result into the button
	QColor bgColor = QColorDialog::getColor(backgroundColorButton->paletteBackgroundColor(),0,0);
	backgroundColorButton->setPaletteBackgroundColor(bgColor);
	backgroundColor = bgColor;
	dialogChanged = true;
}
void UserPreferences::chooseSessionPath(){
	//Launch a file-chooser dialog, just choosing the directory
	QString s = QFileDialog::getExistingDirectory(
			Session::getInstance()->getSessionDirectory().c_str(),
            this,
            "directory chooser",
            "Choose the session file directory",
            TRUE );
	if (s != "") {
		sessionPathEdit->setText(s);
		sessionDir = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseMetadataPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString s = QFileDialog::getExistingDirectory(
			Session::getInstance()->getMetadataDir().c_str(),
            this,
            "directory chooser",
            "Choose the metadata directory",
            TRUE );
	if (s != "") {
		metadataPathEdit->setText(s);
		metadataDir = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseLogFilePath(){
	//Launch a file-chooser dialog, 
    QString s = QFileDialog::getSaveFileName(
				Session::getInstance()->getLogfileName().c_str(),
				"Text (*.txt)",
                this,
                "open log file dialog",
                "Choose a filename for log messages" );
	if (s != ""){
		logFilePathEdit->setText(s);
		MessageReporter::getInstance()->reset(s.ascii());
		logFileName = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseJpegPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString s = QFileDialog::getExistingDirectory(
			Session::getInstance()->getJpegDirectory().c_str(),
            this,
            "directory chooser",
            "Choose the jpeg (image) file directory",
            TRUE );
	if (s != "") {
		jpegPathEdit->setText(s);
		jpegPath = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseTFPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString s = QFileDialog::getExistingDirectory(
			Session::getInstance()->getTFFilePath().c_str(),
            this,
            "directory chooser",
            "Choose the transfer function file directory",
            TRUE );
	if (s != "") {
		tfPathEdit->setText(s);
		tfPath = s.ascii();
		dialogChanged = true;
	}
}
void UserPreferences::chooseFlowPath(){
	//Launch a directory-chooser dialog, just choosing the directory
	QString s = QFileDialog::getExistingDirectory(
			Session::getInstance()->getFlowDirectory().c_str(),
            this,
            "directory chooser",
            "Choose the flow file directory",
            TRUE );
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

	sessionDir = ses->getSessionDirectory();
	metadataDir = ses->getMetadataDir();
	logFileName = ses->getLogfileName();
	jpegPath = ses->getJpegDirectory();
	tfPath = ses->getTFFilePath();
	flowPath = ses->getFlowDirectory();

	//Directories:
	sessionPathEdit->setText(ses->getSessionDirectory().c_str());
	metadataPathEdit->setText(ses->getMetadataDir().c_str());
	logFilePathEdit->setText(ses->getLogfileName().c_str());
	jpegPathEdit->setText(ses->getJpegDirectory().c_str());
	tfPathEdit->setText(ses->getTFFilePath().c_str());
	flowPathEdit->setText(ses->getFlowDirectory().c_str());
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
}
void UserPreferences::
applyToState(){
	
	//Copy from this (not QDialog) to session
	Session* ses = Session::getInstance();
	ses->setCacheMB(cacheMB);
	ses->setTextureSize(texSize);
	ses->specifyTextureSize(texSizeSpecified);
	GLWindow::setJpegQuality(jpegQuality);
	DataStatus::setWarnMissingData(warnDataMissing);
	DataStatus::setUseLowerRefinementLevel(useLowerRefinement);
	
	//Directories:
	ses->setSessionDirectory(sessionDir.c_str());
	ses->setMetadataDirectory(metadataDir.c_str());
	ses->setLogfileName(logFileName.c_str());
	ses->setJpegDirectory(jpegPath.c_str());
	ses->setTFFilePath(tfPath.c_str());
	ses->setFlowDirectory(flowPath.c_str());

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
		 QMessageBox::Yes|QMessageBox::Default,QMessageBox::No,QMessageBox::NoButton);
	if (rc != QMessageBox::Yes) return;
	
	std::string s = Session::getPreferencesFile();
	QString filename = QFileDialog::getSaveFileName(
			Session::getPreferencesFile().c_str(),
            ".*",
            this,
            "save preferences to file",
            "Select the filename for saving user preferences" );
	
	if(filename.length() == 0) return;
	ofstream os;
	os.open(filename.ascii());

	if (!os || !saveToFile(os)){//Report error if you can't open the file
		MessageReporter::errorMsg("Unable to open preferences file: %s", filename.ascii());
	}
	os.close();
	
}
bool UserPreferences::
saveToFile(ofstream& ofs ){
	XmlNode* const rootNode = buildNode("");
	ofs << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
	XmlNode::streamOut(ofs,(*rootNode));
	if (MyBase::GetErrCode() != 0) {
		MessageReporter::errorMsg("Preferences Save Error %d, Writing to:\n %s",
			MyBase::GetErrCode(),Session::getPreferencesFile());
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

	XmlNode* mainNode = new XmlNode(_preferencesTag, attrs, 2);
//create element for each path or filename
	mainNode->SetElementString(_metadataPathTag, ses->getMetadataDir());
	mainNode->SetElementString(_sessionPathTag, ses->getSessionDirectory());
	mainNode->SetElementString(_tfPathTag, ses->getTFFilePath());
	mainNode->SetElementString(_imageCapturePathTag, ses->getJpegDirectory());
	mainNode->SetElementString(_flowDirectoryPathTag, ses->getFlowDirectory());
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
				StrCmpNoCase(tag, _metadataPathTag) == 0)
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
	//all of these are supposed to be strings:
	if (StrCmpNoCase(state->data_type, _stringType) != 0) {
		return false;
	}
	const string &strdata = pm->getStringData();
	Session* ses = Session::getInstance();
	if (StrCmpNoCase(tag, _exportFileNameTag) == 0){
		ses->setExportFile(strdata.c_str());
	} else if (StrCmpNoCase(tag, _imageCapturePathTag) == 0){
		ses->setJpegDirectory(strdata.c_str());
	} else if (StrCmpNoCase(tag, _flowDirectoryPathTag) == 0){
		ses->setFlowDirectory(strdata.c_str());
	} else if (StrCmpNoCase(tag, _logFileNameTag) == 0){
		ses->setLogfileName(strdata.c_str());
	} else if (StrCmpNoCase(tag, _tfPathTag) == 0){
		ses->setTFFilePath(strdata.c_str());
	} else if (StrCmpNoCase(tag, _sessionPathTag) == 0){
		ses->setSessionDirectory(strdata.c_str());
	} else if (StrCmpNoCase(tag, _metadataPathTag) == 0){
		ses->setMetadataDirectory(strdata.c_str());
	} else {
		pm->parseError("Invalid preferences tag  : \"%s\"", tag.c_str());
		return false;
	}
	return true;
}
bool UserPreferences::loadPreferences(const char* filename){

	ifstream is;
	is.open(filename);
	if (!is){//Report error if you can't open the file
		MessageReporter::errorMsg("Unable to open user preferences file: %s", filename);
		return false;
	}
	//Then set values from file.
	//Create a dummy userpreferences class that does the parsing
	UserPreferences* userPrefs = new UserPreferences;
	ExpatParseMgr* parseMgr = new ExpatParseMgr(userPrefs);
	parseMgr->parse(is);
	is.close();
	delete parseMgr;
	delete userPrefs;
	return true;
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
	metadataDir = metadataPathEdit->text().ascii();
	tfPath = tfPathEdit->text().ascii();
	logFileName = logFilePathEdit->text().ascii();
	flowPath = flowPathEdit->text().ascii();
	jpegPath = jpegPathEdit->text().ascii();
	logNum[0] = maxInfoLog->text().toInt();
	popupNum[0] = maxInfoPopup->text().toInt();
	logNum[1] = maxWarnLog->text().toInt();
	popupNum[1] = maxWarnPopup->text().toInt();
	logNum[2] = maxErrorLog->text().toInt();
	popupNum[2] = maxErrorPopup->text().toInt();
	
}
//Attempt to load preferences from default location(s).
//If fail, return false.
bool UserPreferences::loadDefault(){
	bool gotFile = false;
	string filename;
	struct STAT64 statbuf;
	
	char* prefPath = getenv("VAPOR_PREFS_DIR");
	if (prefPath){
		filename = string(prefPath)+"/.vapor_prefs";
		if (STAT64(filename.c_str(), &statbuf) >= 0) gotFile = true;
	}
	if (!gotFile){
		prefPath = getenv("HOME");
		if (prefPath){
			filename = string(prefPath)+"/.vapor_prefs";
			
			if (STAT64(filename.c_str(), &statbuf) >= 0) gotFile = true;
		}
	}
	if (!gotFile){
		prefPath = getenv("VAPOR_HOME");
		if (prefPath){
			filename = string(prefPath)+"/.vapor_prefs";
			if (STAT64(filename.c_str(), &statbuf) >= 0) gotFile = true;
		}
	}
	if (!gotFile) return false;
	
	bool ok = loadPreferences(filename.c_str());
	return ok;

}