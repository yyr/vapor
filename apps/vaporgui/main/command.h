//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		command.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Defines the Command class, and four of its
//		subclasses, TabChangeCommand MouseModeCommand, VizFeatureCommand, PreferencesCommand  
//		Command class is abstract base class for user commands that can be
//		Redone/Undone.  Each command must have enough info to support undo/redo.
//		Also:  It is essential that the undo and redo operations not alter any of the
//		commands in the state.  For example, the undo and redo of a visualizer creation
//		must not alter any of the panels that are saved in the history.
//		The MouseModeCommand supports undo/redo of setting the mouse mode,
//		e.g. navigation versus moving contour planes in the scene.
//		The TabChangeCommand supports undo/redo of selection of the
//		tabs in the tabmanager
//		The VizFeatureCommand is for changing features of a visualizer window
//		The PreferencesCommand is when user preferences change
//
#ifndef COMMAND_H
#define COMMAND_H

#include <qstring.h>
#include "glwindow.h"
#include "params.h"

namespace VAPoR{
class Params;
class Session;
class VizFeatureParams;
class UserPreferences;
class Command {
public:
	virtual ~Command() {}
	virtual void reDo() = 0;
	virtual void unDo() = 0;
	void setDescription(QString& str){
		description = str;
	}
	QString& getDescription(){return description;}
	
	

	//Enum to describe various vizwin activations
	enum activateType {
		create,
		remove,
		activate  
	};
protected:
	QString description;
};
//Subclass to deal with mouse mode settings
//
class MouseModeCommand : public Command{
public:
	MouseModeCommand(GLWindow::mouseModeType previousMode, GLWindow::mouseModeType newMode);
	virtual ~MouseModeCommand() {}
	virtual void unDo();
	virtual void reDo();
protected:
	const char* modeName(GLWindow::mouseModeType t);
	GLWindow::mouseModeType previousMode;
	GLWindow::mouseModeType currentMode;
};
//Subclass to deal with changes in the front tab panel
//
class TabChangeCommand : public Command{
public:
	TabChangeCommand(Params::ParamType oldTab, Params::ParamType newTab);
	virtual ~TabChangeCommand() {}
	virtual void unDo();
	virtual void reDo();
protected:
	const char* tabName(Params::ParamType t);
	int previousTab;
	int currentTab;

};
//Subclass to deal with changes in visualizer features
//
class VizFeatureCommand : public Command{
public:
	VizFeatureCommand(VizFeatureParams* prevFeatures, const char* descr, int vizNum);
	void setNext(VizFeatureParams* nextFeatures);
	virtual ~VizFeatureCommand();
	virtual void unDo();
	virtual void reDo();
	static VizFeatureCommand* captureStart(VizFeatureParams* p,  char* description, int vizNum);
	static void captureEnd(VizFeatureCommand* pCom, VizFeatureParams *p);
protected:
	VizFeatureParams* previousFeatures;
	VizFeatureParams* currentFeatures;
	int windowNum;

};
//Subclass to deal with changes in preferences
//
class PreferencesCommand : public Command{
public:
	PreferencesCommand(UserPreferences* prefs, const char* descr);
	void setNext(UserPreferences* nextPrefs);
	virtual ~PreferencesCommand();
	virtual void unDo();
	virtual void reDo();
	static PreferencesCommand* captureStart(UserPreferences* p,  char* description);
	static void captureEnd(PreferencesCommand* pCom, UserPreferences *p);
protected:
	UserPreferences* previousPrefs;
	UserPreferences* currentPrefs;

};
};
#endif
 
