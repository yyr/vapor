//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vizactivatecommand.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Defines the VizActivateCommand class
//		This is a Command class that supports undo/redo of activate
//		window events from the visualizer window.
//
#ifndef VIZACTIVATECOMMAND_H
#define VIZACTIVATECOMMAND_H

#include "command.h"
#include <qcolor.h>
namespace VAPoR {
class VizWinMgr;
class VizWin;
class Session;
class RegionParams;
class ViewpointParams;
class DvrParams;
class IsosurfaceParams;
class ContourParams;
class AnimationParams;
//Simple Command to handle events associated with creating, deleting, activating visualizers
//There are only 3:  create, remove, and activate
//
class VizActivateCommand : public Command {
public:
	// on create, prevViz is former active viz, nextViz is the created one, having name winName
	// on removal, prevViz is the viz being removed, named winName, nextViz is the resulting activeViz
	// on activation, prevViz is previously active viz, nextViz is newly activated one.  Name ignored.
	//
	VizActivateCommand (VizWin*,  int prevViz, int nextViz, Command::activateType typ);
	~VizActivateCommand();
	virtual void unDo();
	virtual void reDo();

protected:
	//Whenever the undo/redo operation results in deleting a window, we need
	//to clone the params associated with that window, since they will be deleted during
	//the close event.
	//
	void cloneStateParams(VizWin*, int viznum);
	int lastActiveViznum;
	int currentActiveViznum;
	Command::activateType thisType;
	const char* windowName;
	//If this is a "remove" event, we need the following state as well:
	//
	ViewpointParams* vpParams;
	RegionParams* regionParams;
	IsosurfaceParams* isoParams;
	ContourParams* contourParams;
	DvrParams* dvrParams;
	AnimationParams* animationParams;
	QColor backgroundColor;

};
};











#endif

