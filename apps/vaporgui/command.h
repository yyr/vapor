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
//	Description:	Defines the Command class, and two of its
//		subclasses, TabChangeCommand and MouseModeCommand.  
//		Command class is abstract base class for user commands that can be
//		Redone/Undone.  Each command must have enough info to support undo/redo.
//		Also:  It is essential that the undo and redo operations not alter any of the
//		commands in the state.  For example, the undo and redo of a visualizer creation
//		must not alter any of the panels that are saved in the history.
//		The MouseModeCommand supports undo/redo of setting the mouse mode,
//		e.g. navigation versus moving contour planes in the scene.
//		The TabChangeCommand supports undo/redo of selection of the
//		tabs in the tabmanager
//
#ifndef COMMAND_H
#define COMMAND_H

#include <qstring.h>

#include "params.h"

namespace VAPoR{
class Params;
class Session;
class Command {
public:
	virtual ~Command() {}
	virtual void reDo() = 0;
	virtual void unDo() = 0;
	void setDescription(QString& str){
		description = str;
	}
	QString& getDescription(){return description;}
	
	//Enum describes various mouse modes:
	enum mouseModeType {
		unknownMode,
		navigateMode,
		regionMode,
		contourMode,
		probeMode,
		lightMode
	};

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
	MouseModeCommand(mouseModeType previousMode, mouseModeType newMode);
	virtual ~MouseModeCommand() {}
	virtual void unDo();
	virtual void reDo();
protected:
	const char* modeName(mouseModeType t);
	mouseModeType previousMode;
	mouseModeType currentMode;
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
};
#endif
