//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		session.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Defines the Session class.  The session class is 
//  responsible for the undo/redo queue as well as maintaining all
//  session-wide parameters.  In particular, the session owns the DataMgr and
//  the Metadata, since these are session-wide objects (only one is open at one time).
//  The information about the current metadata (e.g. histograms) is stored in 
//  an auxiliary datastructure, the DataStatus class.
//  The Session includes in its state the following:
//	Data (Metadata, Datamgr, and the file names associated)
//	Command queue (for undo/redo)
//	TransferFunction list (saved in state)
//	Image capturing state
//	VizWinMgr
//

/**
 * The session class owns all the data associated with a VAPoR session.
 * All of the panel states are in the VizWinMgr class
 * The data state (metafile) is in the DataMgr class
 * In addition to creating these classes, this class can
 * save and restore session files, and this class contains the methods
 * that maintain the Undo/Redo history
 * To support Undo/Redo, every user action that can be undone or redone must
 * correspond to an instance of a Command class.  Commands that are
 * specific to settings in the gui tabbed parameter panels are supported in the
 * PanelCommand class
 *
 * The way this works is as follows:
 * The state of each panel is managed in the corresponding Params class.  The params class has the following
 * Methods:
 * Params::updateDialog:  This updates the gui using all the state stored in the Params class.  This method 
 *     will generate gui events that should not affect the history; therefore the method must call
 *	   Session::stopRecording() and Session::startRecording() at its beginning and end.
 * Params::updatePanelState:  This changes the Params based on values in the gui that have not yet been saved.
 *	   Whenever such a text change occurs, the method Params::guiSetTextChanged(true) is called, setting
 *	   a flag indicating that there are text changes that have not taken effect.
 *	   These changes are from text boxes (LineEdit's) that have changed but not yet saved.  The 
 *     Panel::updatePanelState method *may* need to suspend 
 *	   the undo/redo recording while the Panel::updatePanelState method is executing, 
 *     using Session::stopRecording() and Session::startRecording().  That will be the case if the
 *     result of calling this method is to trigger an event that would be recorded in the history, if
 *     for example a spin box setting changes due to a lineedit change.  There is no harm done in
 *     suspending the Undo/Redo recording for this method.
 *     It is important that in addition, at the end of this method, the method
 *     Params::guiSetTextChanged(false) be called so that the memory of text changes is restarted.
 * Params::makeCurrent():  This method is called when the panel is changed during undo/redo,
 *	   and the values in the params object
 *     need to be be reflected in the session state.  makeCurrent() will call updateDialog to make the gui
 *     current, but it must also update the state of any visualizers that depend on the panel state.
 *   Changes to the panels can include any of the following types:
 *     a.Changes that occur when a gui line edit changes.  These changes have no effect until the user presses
 *		 "return" (on a line-edit) or another gui element changes.  These changes set a flag in the params class, 
 *       "textChangedFlag" and the changes are only remembered in the state of the dialog.  When the user
 *       presses "return" in a text box, or when another element changes, all of the pending changes must
 *       be applied.  The method "updatePanelState" applies these changes.  To cause these to occur 
 *       the line: "if(textChangedFlag) confirmText(false);" should be inserted prior to other actions.
 *       the method confirmText() adds the latest text changes to the history, puts them into the panel,
 *       and optionally causes these values to go to the renderer (if the argument is true).
 *     b.Changes that occur when a mouse event occurs.  The state before and after the mouse event should be
 *       captured by calling PanelCommand::captureStart() when the mouse goes down and PanelCommand::captureEnd()
 *       when the mouse comes up.  The effect of the mouse movement will be saved in data values in the
 *       Params class that will be captured by these methods.  Before captureStart() is called, the line
 *       "if(textChangedFlag) confirmText(false);" must be inserted to cause any previous text changes to
 *       take effect before the mouse moves.
 *     c.Changes that occur when users change widgets in the tabbed panels (sliders, spin boxes, combo boxes, 
 *       etc.) must take place instantly (although sliders can wait until the user releases the slider).
 *       accordingly the Params methods that handle these events will call captureStart() and captureEnd() before
 *       and after the change is made in the panel.  Also,the text changes are recorded before the capture by
 *       inserting  "if(textChangedFlag) confirmText(false);"
 *
 *  Updating the renderer:  There are several ways in which the gui interacts with the renderers:
 *     a.  Direct connection:  When users navigate or otherwise affect the rendering immediately without
 *			using gui input , the gui needs to eventually get the updates from the scene.  These will be
 *	        captured during mouse up events as described above, although the scene will already be current
 *          when the gui is informed of the new values.  Undo/redo work as above.
 *     b.  When the user changes position by typing in coordinates, the method VizWin::setValuesFromGui() is applied
 *			causing an immediate change in coords and redraw.  In addition other windows that share the
 *          state call updateGL() to refresh from the new values.  This also works with undo/redo, since
 *          updatePanelState will call updateRenderer(), which invokes setValuesFromGui().
 *	   c.  The region setting have a "dirty" bit, the renderers query this bit each rendering and request the
 *          new region settings whenever they have changed.  The dirty bit needs to be reset when there
 *          is a change in region settings from the Undo/Redo.
 *     d.  When a renderer is enabled or disabled, the appropriate construction/destruction is immediately
 *          invoked.  Similar results must occur when the local/global switch is changed.
 *     e.  Other renderer settings (colors, etc.)
 */
#ifndef SESSION_H
#define SESSION_H
#define MAX_HISTORY 1000

#include <vector>
#include <string>
#include <assert.h>
#include "vapor/DataMgr.h"
#include "vapor/MyBase.h"
#include "vapor/ExpatParseMgr.h"
#include "datastatus.h"
class QString;
namespace VAPoR {
class VizWinMgr;
class Histo;
class DataMgr;
class MainForm;
class Params;
class RenderParams;
class Command;
class Metadata;
class WaveletBlock3DRegionReader;
class DataStatus;
class VDFIOBase;
class TransferFunction;
class DvrParams;
class RegionParams;


class Session : public ParsedXml {
public:
	static Session* getInstance(){
		if (!theSession){
			theSession = new Session();
		}
		return theSession;
	}
	virtual ~Session();
	
	DataMgr* getDataMgr() {return dataMgr;}
	
	
	const Metadata* getCurrentMetadata() {return currentMetadata;}
	/* moved to DataStatus...
	float getDataMin(int varNum, int timestep){
		if (currentDataStatus){
			return (float)currentDataStatus->getDataMin(varNum, timestep);
		}
		else return 0;
	}
	float getDataMax(int varNum, int timestep){
		if (currentDataStatus){
			return (float)currentDataStatus->getDataMax(varNum, timestep);
		}
		else return 0;
	}
	bool dataIsPresent(int varnum, int timeStep){
		if (currentDataStatus)
			return currentDataStatus->dataIsPresent(varnum, timeStep);
		else return false;
	}
	*/
	int getNumVariables(){return DataStatus::getNumVariables();}
	/*	
	size_t getMinTimestep(){
		if (currentDataStatus) return currentDataStatus->getMinTimestep();
		return 0;
	}
	size_t getMaxTimestep(){
		if (currentDataStatus)return currentDataStatus->getMaxTimestep();
		return 1;
	}
	*/
	
	//Methods to control the command queue:
	//When a new command is issued, call "addToHistory"
	//The backup and advance methods are invoked from the
	//menu or from a script.
	//
	void addToHistory(Command* cmd);
	void backupQueue();
	void advanceQueue();
	void resetCommandQueue();
	Command* currentUndoCommand() {return commandQueue[(currentQueuePos)%MAX_HISTORY];}
	Command* currentRedoCommand() {return commandQueue[(currentQueuePos+1)%MAX_HISTORY];}
	//Anytime someone wants to block recording, they call blockRecording,
	//then unblock when they are done.  No recording happens until everyone has stopped
	//blocking it.
	//
	
	void blockRecording() {recordingCount++;}
	void unblockRecording() {recordingCount--;}
	bool isRecording() {return (recordingCount == 0);}
	Histo* getCurrentHistogram(int var) {
		return (currentHistograms ? currentHistograms[var]: 0);}
	
	//Setup session for a new Metadata, by specifying vdf file
	//If the argument is null, it resets to default state
	//
	bool resetMetadata(const char* vmfile, bool restoringSession, bool isMerged = false, int mergeOffset = 0);
	
	void setMetadataSaved(bool isSaved) { metadataSaved = isSaved;}
	bool metadataIsSaved() {return metadataSaved;}
	//Export current data in active visualizer:
	void exportData();
	void setCacheMB(size_t size){cacheMB = size;}
	size_t getCacheMB() {return cacheMB;}
	
	
	int getNumTFs() { return numTFs;}
	std::string* getTFName(int i) { return tfNames[i];}
	void addTF(const char* tfName, RenderParams* );
	void addTF(const std::string tfName, TransferFunction* tf);
	bool removeTF(const std::string* name);
	//Obtain the Transfer function associated with a name,
	//plus, return left end right map limits.
	TransferFunction* getTF(const std::string* tfName);
	bool isValidTFName(const std::string* tfName);
	

	static void errorCallbackFcn(const char* msg, int err_code);
	static void infoCallbackFcn(const char* msg);
	static void pauseErrorCallback() {
		DataMgr::SetErrMsgCB(0);
	}
	static void resumeErrorCallback(){
		DataMgr::SetErrMsgCB(errorCallbackFcn);
	}
	//Session save/restore:
	bool saveToFile(ofstream&);
	bool loadFromFile(ifstream&);
	string& getTFFilePath() {return currentTFPath;}
	void updateTFFilePath(QString* newPath);
	string& getMetadataFile(){return currentMetadataFile;}
	string& getJpegDirectory() {return currentJpegDirectory;}
	string& getFlowDirectory() {return currentFlowDirectory;}
	void setJpegDirectory(const char* dir) {currentJpegDirectory = dir;}
	void setFlowDirectory(const char* dir) {currentFlowDirectory = dir;}
	string& getExportFile() {return currentExportFile;}
	string& getLogfileName() {return currentLogfileName;}
	void setLogfileName(const char* newname){currentLogfileName = newname;}
	const float* getExtents() {return extents;}
	float getExtents(int i) {return extents[i];}
	//Better version of getExtents, uses refinement level
	void getExtents(int refLevel, float extents[6]);
		
	std::string& getVariableName(int varNum) {
		return DataStatus::getVariableName(varNum);}

	//Find the session num of a name, or -1 if it's not metadata:
	int getSessionVariableNum(const string& str){
		return DataStatus::getSessionVariableNum(str);
	}
	//Insert variableName if necessary; return index
	int mergeVariableName(const string& str){
		return DataStatus::mergeVariableName(str);}


	void addVarName(const string newName) {DataStatus::addVarName(newName);}
	//"Metadata" variables are those that are in current metadata, as opposed to
	//"real" variables are those in session
	int getNumMetadataVariables() {return DataStatus::getNumMetadataVariables();}
	int mapMetadataToRealVarNum(int varnum){
		return DataStatus::mapMetadataToRealVarNum(varnum);
	}
	int mapRealToMetadataVarNum(int var) {return DataStatus::mapRealToMetadataVarNum(var);}
	string& getMetadataVarName(int varnum) {
		return DataStatus::getMetadataVarName(varnum);
	}
		
	
protected:
	static const string _cacheSizeAttr;
	static const string _jpegQualityAttr;
	static const string _metadataPathAttr;
	static const string _transferFunctionPathAttr;
	static const string _imageCapturePathAttr;
	static const string _flowDirectoryPathAttr;
	static const string _logFileNameAttr;
	static const string _maxPopupAttr;
	static const string _maxLogAttr;
	static const string _exportFileNameAttr;
	static const string _sessionTag;
	static const string _globalParameterPanelsTag;
	static const string _globalTransferFunctionsTag;
	static const string _dataExtentsAttr;

	XmlNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int depth , std::string& tag, const char **attr);
	bool elementEndHandler(ExpatParseMgr*, int depth , std::string& );

	Session();
	void init();
	static Session* theSession;
	
	
	//setup the DataStatus:
	void setupDataStatus();
	//Reset the dataStatus after a merge()
	void resetDataStatus();
	
	
	DataMgr* dataMgr;
	
	Command* commandQueue[MAX_HISTORY];
	int currentQueuePos;
	int endQueuePos;
	int startQueuePos;
	int recordingCount;
	Histo** currentHistograms;
	const Metadata* currentMetadata;
	//Cache size in megabytes
	size_t cacheMB;
	
	
	bool dataExists;
	bool newSession;
	bool renderOK;
	float extents[6];
	//TransferFunctions are kept, by name, in the session:
	TransferFunction** keptTFs;
	//hold a transfer function during parsing (between when the start and end tags
	//are parsed.)
	TransferFunction* tempParsedTF;
	Params* tempParsedPanel;
	std::string** tfNames;
	int numTFs, tfListSize;
	// Various filepath and directory paths are kept in session:
	// the most recent successfully accessed one is part of the state
	string currentTFPath;
	string currentMetadataFile;
	string currentExportFile;
	string currentJpegDirectory;
	string currentFlowDirectory;
	string currentLogfileName;

	// the variableNames array specifies the name associated with each variable num.
	//Note that this
	//contains (possibly properly) the corresponding variableNames in the
	//dataStatus.  The number of metadataVariables should coincide with the 
	//number of variables in the datastatus that have actual data associated with them.
	//std::vector<string> variableNames;
	//int numMetadataVariables;
	//int* mapMetadataVars;
	bool metadataSaved;
};

}; //end VAPoR namespace
#endif //SESSION_H

