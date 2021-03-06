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
#include <qdialog.h>
#include <vapor/XmlNode.h>
#include <vapor/ExpatParseMgr.h>
#include "preferences.h"


class QWidget;
class VizFeatures;
class ScrollContainer;


QT_USE_NAMESPACE
namespace VAPoR{
class ParamNode;
class PreferencesCommand;
class UserPreferences : public QDialog, public Ui_Preferences, public ParsedXml {
	Q_OBJECT
public: 
	//Constructor gets starting state from the Session
	UserPreferences();
	UserPreferences* clone();
	//Launch a UserPreferences dialog.  Put its values into state, and offer to save if it succeeds.
	void launch();
	
	//Save to file.  can be saved from outside this dialog when directory changes.
	//If filename not specified then save to default path/name 
	//void save(QString* filename = 0);
	//When user requests save state, launch file save dialog:
	static void requestSave(bool prompt = true);
	//Make these preferences apply to state.  If orderTabs is false a tabOrdering change won't be applied
	void applyToState(bool orderTabs = true);
	void getTextChanges();
	static bool saveToFile(ofstream& ofs );
	static ParamNode* buildNode(); 

	virtual bool elementStartHandler(ExpatParseMgr*, int depth, 
                                     std::string&, const char **);

	virtual bool elementEndHandler(ExpatParseMgr*, int, std::string&);
	//Create instance used to make a UserPreferences with no dialog
	//Purpose is to support Undo/Redo when UserPreferences are changed outside the dialog.
	static UserPreferences* getInstance(){
		UserPreferences* up = new UserPreferences();
		up->setDialog();
		return up;
	}
	static bool loadPreferences(const char* filename);
	static bool savePreferences(const char* filename);
	static bool loadDefault();
	static void setDefault();
	static bool depthPeelIsInState(){return depthPeelInState;}
	//Data fidelity settings:
	float getDefaultLODFidelity2D(){return defaultLODFidelity2D;}
	float getDefaultRefinementFidelity2D(){return defaultRefFidelity2D;}
	float getDefaultLODFidelity3D(){return defaultLODFidelity3D;}
	float getDefaultRefinementFidelity3D(){return defaultRefFidelity3D;}
	void setDefaultLODFidelity2D(float q) {defaultLODFidelity2D = q;}
	void setDefaultLODFidelity3D(float q) {defaultLODFidelity3D = q;}
	void setDefaultRefinementFidelity2D(float q){defaultRefFidelity2D = q;}
	void setDefaultRefinementFidelity3D(float q){defaultRefFidelity3D = q;}
	
signals: 
	void doneWithIt();

protected slots:
	
	void setDefaultDialog();
	void panelChanged();
	void selectRegionFrameColor();
	void selectSubregionFrameColor();
	void selectBackgroundColor();
	void chooseSessionPath();
	void chooseAutoSaveFilename();
	void chooseMetadataPath();
	void chooseLogFilePath();
	void chooseJpegPath();
	void chooseTFPath();
	void chooseFlowPath();
	void choosePythonPath();
	void changeTextureSize(bool);
	void regionChanged(bool);
	void subregionChanged(bool);
	void warningChanged(bool);
	void trackMouseChanged(bool);
	void lowerResChanged(bool);
	void textChanged();
	void resetCounts();
	void okClicked();
	void doHelp();
	void showAllDefaults();
	void setGeometryType(int);
	void setDvrBits(int);
	void setIsoBits(int);
	void dvrLightingChanged(bool);
	void preIntegrationChanged(bool);
	void axisArrowsChanged(bool);
	void spinChanged(bool);
	void winLockChanged(bool val); 
	void depthPeelChanged(bool val); 
	void setAutoSave(bool);
	void copyLatestSession();
	void copyLatestMetadata();
	void copyLatestTF();
	void copyLatestImage();
	void copyLatestFlow();
	void copyLatestPython();
	void tabNameChanged(int val); 
	void tabOrderChanged(int);
	void hideUnhidePressed();
	void setNoCitation(bool);
	
protected:
	static const string _preferencesTag;
	static const string _sceneColorsTag;
	static const string _exportFileNameTag;
	static const string _imageCapturePathTag;
	static const string _flowDirectoryPathTag;
	static const string _pythonDirectoryPathTag;
	static const string _logFileNameTag;
	static const string _tfPathTag;
	static const string _metadataPathTag;
	static const string _sessionPathTag;
	static const string _autoSaveFilenameTag;
	static const string _fidelityDefaultsTag;
	static const string _autoSaveIntervalAttr;
	static const string _messagesTag;
	static const string _probeDefaultsTag;
	static const string _alphaAttr;
	static const string _scaleAttr;
	static const string _phiAttr;
	static const string _thetaAttr;
	static const string _psiAttr;
	static const string _viewpointDefaultsTag;
	static const string _viewDirAttr;
	static const string _upVecAttr;
	static const string _flowDefaultsTag;
	static const string _tabOrderingTag;
	static const string _integrationAccuracyAttr;
	static const string _smoothnessAttr;
	static const string _flowLengthAttr;
	static const string _flowDiameterAttr;
	static const string _arrowSizeAttr;
	static const string _diamondSizeAttr;
	static const string _geometryTypeAttr;
	static const string _isoDefaultsTag;
	static const string _dvrDefaultsTag;
	static const string _animationDefaultsTag;
	static const string _vizFeatureDefaultsTag;
	static const string _isoDefaultBitsPerVoxelAttr; 
	static const string _dvrDefaultBitsPerVoxelAttr; 
	static const string _dvrDefaultLightingAttr; 
	static const string _dvrDefaultPreIntegrationAttr; 
	static const string _animationDefaultMaxWaitAttr; 
	static const string _animationDefaultMaxFPSAttr; 
	static const string _defaultShowAxisArrowsAttr; 
	static const string _defaultShowTerrainAttr;
	static const string _defaultSpinAnimateAttr;
	static const string _numLightsAttr;
	static const string _lightDirectionAttr;
	static const string _diffuseCoeffAttr;
	static const string _specularCoeffAttr;
	static const string _ambientCoeffAttr;
	static const string _specularExpAttr;
	static const string _citationRemindAttr;
	static const string _winWidthAttr;
	static const string _winHeightAttr;
	static const string _lockWinAttr;
	static const string _depthPeelAttr;

	static bool depthPeelInState;
	static bool firstPreferences;
	
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
	float defaultRefFidelity2D;
	float defaultRefFidelity3D;
	float defaultLODFidelity2D;
	float defaultLODFidelity3D;
	bool subregionFrameEnabled;
	bool regionFrameEnabled;
	QColor subregionFrameColor;
	QColor regionFrameColor;
	QColor backgroundColor;
	int popupNum[3];
	int logNum[3];
	string sessionDir;
	string autoSaveFilename;
	string metadataDir;
	string logFileName;
	string jpegPath;
	string flowPath;
	string pythonPath;
	string tfPath;
	bool texSizeSpecified;
	bool useLowerRefinement;
	bool warnDataMissing;
	bool trackMouse;
	int jpegQuality;
	int texSize;
	int winWidth, winHeight;
	bool lockWin;
	bool depthPeel;
	size_t cacheMB;
	float scale, alpha, phi, psi, theta;
	float viewDir[3], upVec[3];
	float lightDir[3][3];
	float diffuseCoeff[3];
	float specularCoeff[3];
	float ambientCoeff;
	float specularExp;
	int numLights;
	float flowLength, smoothness, integrationAccuracy, flowDiameter, arrowSize, diamondSize;
	int geometryType;
	int isoBitsPerVoxel;
	int dvrBitsPerVoxel;
	bool dvrLighting;
	bool dvrPreIntegration;
	
	float maxFPS;
	bool showAxisArrows;
	bool spinAnimate;
	bool citationRemind;
	int autoSaveInterval;
	bool showAll;
	//Indicate position of tab associated with a paramsBaseType, or 0 if it is not shown:
	int currentTabIndex;
	vector<long> tabPositions;
};


};
#endif //USERPREFERENCES_H 
