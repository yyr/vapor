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
 * Params::makeCurrent():  This method is called whenever the panel is changed and the values in the params object
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
#include "vapor/DataMgr.h"
namespace VAPoR {
class VizWinMgr;
class Histo;
class DataMgr;
class MainForm;
class Params;
class Command;
class Metadata;
class DataStatus;
class WaveletBlock3DRegionReader;
class AnimationController;
// Class used by session to keep track of variables, timesteps	
class DataStatus{
public:
	DataStatus(int numvariables, int numtimesteps);
	~DataStatus();
	//Set methods:
	//Either there is a minimum xform present, or none
	//
	void setMinXFormPresent(int varnum, int timestep, int xf){
		dataPresent[timestep + varnum*numTimesteps] = xf;
	}
	void setDataAbsent(int varnum, int timestep){
		dataPresent[timestep + varnum*numTimesteps] = -1;
	}
	void setDataMax(int varnum, int timestep, double val){
		maxData[timestep + varnum*numTimesteps] = val;
	}
	void setDataMin(int varnum, int timestep, double val){
		minData[timestep + varnum*numTimesteps] = val;
	}
	void setMaximum(int varnum, float maxval)
		{dataRange[varnum][1] = maxval;}
	void setMinimum(int varnum, float minval)
		{dataRange[varnum][0] = minval;}
	void setNumTransforms(int numtrans) {numTransforms = numtrans;}
	void setFullDataSize(int dim, size_t size){fullDataSize[dim]=size;}
	// Get methods:
	//
	bool dataIsPresent(int varnum, int timestep){
		return (dataPresent[timestep + varnum*numTimesteps] >= 0);
	}
	double getDataMax(int varnum, int timestep){
		return maxData[timestep + varnum*numTimesteps];
	}
	double getDataMin(int varnum, int timestep){
		return minData[timestep + varnum*numTimesteps];
	}
	double getDataMaxOverTime(int varnum){return dataRange[varnum][1];}
	double getDataMinOverTime(int varnum){return dataRange[varnum][0];}
	
	int minXFormPresent(int varnum, int timestep){
		return dataPresent[timestep + varnum*numTimesteps];
	}
	int getNumVariables() {return numVariables;}
	int getNumTimesteps() {return numTimesteps;}
	int getNumTransforms() {return numTransforms;}
	size_t getFullDataSize(int dim){return fullDataSize[dim];}
	float* getDataRange(int varNum){
		return dataRange[varNum];
	}
	

private:
	size_t minTimeStep;
	size_t maxTimeStep;
	int numTransforms;
	int numVariables;
	int numTimesteps;
	double* minData;
	double* maxData;
	int* dataPresent;
	size_t fullDataSize[3];
	float** dataRange;
	
};
class Session {
public:
	Session(MainForm* mf);
	~Session();
	void save(char* filename);
	void restore(char* filename);
	DataMgr* getDataMgr() {return dataMgr;}
	VizWinMgr* getWinMgr() {return vizWinMgr;}
	MainForm* getMainWindow() {return mainWin;}
	const Metadata* getCurrentMetadata() {return currentMetadata;}
	//Datarange is a 2-element float vector, one for each variable
	//
	float* getDataRange(int variableNum){
		if (currentDataStatus)
			return currentDataStatus->getDataRange(variableNum);
		else return 0;
	}
	size_t getMinTimestep(){return minTimeStep;}
	size_t getMaxTimestep(){return maxTimeStep;}
	//Set range mapped for a variable.  Currently sets all
	//variables to same range.
	//
	void setMappingRange(int variableNum, float leftRight[2]){
		dataMgr->SetDataRange(currentMetadata->GetVariableNames()[variableNum].c_str(),leftRight);
	}
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
	const WaveletBlock3DRegionReader* getRegionReader() {return myReader;}
	//Create a new Metadata, by specifying vmf file
	void resetMetadata(const char* vmfile);
	void setCacheMB(size_t size){cacheMB = size;}
	size_t getCacheMB() {return cacheMB;}
	AnimationController* getAnimationController() {return theAnimationController;}


protected:
	AnimationController* theAnimationController;
	WaveletBlock3DRegionReader* myReader;
	
	//setup the DataStatus:
	DataStatus* setupDataStatus();
	DataMgr* dataMgr;
	DataStatus* currentDataStatus;
	VizWinMgr* vizWinMgr;
	MainForm* mainWin;
	Command* commandQueue[MAX_HISTORY];
	int currentQueuePos;
	int endQueuePos;
	int startQueuePos;
	int recordingCount;
	Histo** currentHistograms;
	const Metadata* currentMetadata;
	//Cache size in megabytes
	size_t cacheMB;
	size_t maxTimeStep, minTimeStep;
	
};

}; //end VAPoR namespace
#endif //SESSION_H

