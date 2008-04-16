//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		userpreferences.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2005
//
//	Description:	Defines the UserPreferences class.
//		Provides a temporary container for the state of the preferences dialog
//		It is derived from the dialog
//		undo/redo is supported by cloning this class whenever a preference changes.
//
#ifndef USERPREFERENCES_H
#define USERPREFERENCES_H
 
#include <qwidget.h>
#include <qscrollview.h>
#include <qdialog.h>
#include "vapor/XmlNode.h"
#include "vapor/ExpatParseMgr.h"
#include "preferences.h"

class QWidget;
class VizFeatures;
class ScrollContainer;

namespace VAPoR{
class PreferencesCommand;
class UserPreferences : public Preferences, public ParsedXml {
	Q_OBJECT
public: 
	//Constructor gets starting state from the Session
	UserPreferences();
	UserPreferences* clone();
	//Launch a UserPreferences dialog.  Put its values into state, and offer to save if it succeeds.
	void launch();
	//Save to file.  can be saved from outside this dialog when directory changes.
	//If filename not specified then save to default path/name 
	void save(QString* filename = 0);
	//When user requests save state, launch file save dialog:
	void requestSave();
	//Make these preferences apply to state
	void applyToState();
	void getTextChanges();
	static bool saveToFile(ofstream& ofs );
	static XmlNode* buildNode(const string& tfname); 

	virtual bool elementStartHandler(ExpatParseMgr*, int depth, 
                                     std::string&, const char **);

	virtual bool elementEndHandler(ExpatParseMgr*, int, std::string&);
	static bool loadPreferences(const char* filename);
	static bool savePreferences(const char* filename);
	static bool loadDefault();
	
signals: 
	void doneWithIt();

protected slots:
	
	void panelChanged();
	void selectRegionFrameColor();
	void selectSubregionFrameColor();
	void selectBackgroundColor();
	void chooseSessionPath();
	void chooseMetadataPath();
	void chooseLogFilePath();
	void chooseJpegPath();
	void chooseTFPath();
	void chooseFlowPath();
	void changeTextureSize(bool);
	void regionChanged(bool);
	void subregionChanged(bool);
	void textChanged();
	void resetCounts();
	void okClicked();
	void doHelp();
	
protected:
	static const string _preferencesTag;
	static const string _sceneColorsTag;
	static const string _exportFileNameTag;
	static const string _imageCapturePathTag;
	static const string _flowDirectoryPathTag;
	static const string _logFileNameTag;
	static const string _tfPathTag;
	static const string _metadataPathTag;
	static const string _sessionPathTag;
	static const string _messagesTag;

	//Copy data from session to dialog
	void setDialog();
	//Copy from dialog to session
	void copyFromDialog();
	ScrollContainer* featureHolder;
	bool dialogChanged;//If it changes, we will copy entire state to session??
	bool textChangedFlag;
	PreferencesCommand* myPrefsCommand;
	//So far no use for this; indicates the version of
	//VAPOR that wrote the current preferences to file.
	static string preferencesVersionString;
	bool subregionFrameEnabled;
	bool regionFrameEnabled;
	QColor subregionFrameColor;
	QColor regionFrameColor;
	QColor backgroundColor;
	int popupNum[3];
	int logNum[3];
	string sessionDir;
	string metadataDir;
	string logFileName;
	string jpegPath;
	string flowPath;
	string tfPath;
	bool texSizeSpecified;
	bool useLowerRefinement;
	bool warnDataMissing;
	int jpegQuality;
	int texSize;
	size_t cacheMB;
};


};
#endif //USERPREFERENCES_H 
