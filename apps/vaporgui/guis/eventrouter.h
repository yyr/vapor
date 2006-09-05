//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		eventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:  Definition of EventRouter class	
//		This (pure virtual) class manages communication between
//		gui elements, visualizers, and params.

#ifndef EVENTROUTER_H
#define EVENTROUTER_H
#include "assert.h"
#include "params.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "histo.h"
#include "mapperfunction.h"
#include "panelcommand.h"
#include <qobject.h>
class QWidget;



namespace VAPoR{
	class PanelCommand;
	class Histo;
	class EventRouter
{
	
public:
	EventRouter() {
		textChangedFlag = 0; savedCommand = 0;
		histogramList = 0;
		numHistograms = 0;
	}
	virtual ~EventRouter() {
		for (int i = 0; i<numHistograms; i++) 
			if (histogramList[i]) delete histogramList[i];
		if (histogramList) delete histogramList;
	}
	//Connect signals and slots from tab
	virtual void hookUpTab() = 0;
	//Set all the fields in the tab based on current params
	virtual void updateTab(Params* p) = 0;
	//Method to install a new params for undo and redo.
	//Instance is -1 for non-render params
	virtual void makeCurrent(Params* prevParams, Params* newParams, bool newWin, int instance = -1) = 0;
	//make sure the params cleanly detaches from gui, to
	//handle possible connections from editors, frames, etc.
	virtual void cleanParams(Params*) {}
	
	//Clone the params and any other related classes.
	//Default just clones the params:
	virtual Params* deepCopy(Params* p) {return (p->deepCopy());}
	//Dirty bit comes on when any text changes in the tab.
	//Comes off as soon as it gets confirmed, or updateTab called.
	//
	void guiSetTextChanged(bool on) {textChangedFlag = on;}

	virtual void guiSetEnabled(bool) {assert(0);}

	void guiSetLocal(Params* p, bool lg){
		if (textChangedFlag) confirmText(false);
	
		PanelCommand* cmd;
		Params* localParams = VizWinMgr::getInstance()->getCorrespondingLocalParams(p);
		if (lg){
			cmd = PanelCommand::captureStart(localParams,  "set Global to Local");
		}
		else cmd = PanelCommand::captureStart(localParams,  "set Local to Global");
		localParams->setLocal(lg);
		PanelCommand::captureEnd(cmd, localParams);
	}
	//confirm a change in a text box.  the render argument
	//is true if this requires updateRenderer()
	virtual void confirmText(bool /*render*/) = 0;
	virtual void reinitTab(bool doOverride) = 0;

	//Methods to support maintaining a list of histograms
	//in each router (at least those with a TFE)
	virtual Histo* getHistogram(RenderParams*, bool /*mustGet*/) { assert(0); return 0;}
	virtual void refreshHistogram(RenderParams* ) {assert(0);}
	//If there is a mapEditor, need to implement this:
	virtual void setEditorDirty(){assert(0);}
	MapEditor* getMapEditor(RenderParams* params){
		if (params->getMapperFunc())
			return (params->getMapperFunc()->getEditor());
		else return 0;
	}
	virtual void updateMapBounds(RenderParams*) {assert (0);}
	virtual void updateClut(RenderParams*){assert(0);}

	//Method for classes that capture mouse event events in viz win:
	virtual void captureMouseUp() {assert(0);}
	virtual void captureMouseDown() {assert(0);}
	virtual void guiStartChangeMapFcn(char* ) {assert(0);}
	virtual void guiEndChangeMapFcn(){assert(0);}

	
	
protected:
	//for subclasses with a datarange:
	virtual void setDatarangeDirty(RenderParams* ) {assert(0);}
	//Routers with histograms keep an array, one for each variable,
	// or variable combination
	Histo** histogramList;
	int numHistograms; //how large is histo array..
	//There is one tabbed panel for each class of Params
	
	Params::ParamType myParamsType;
	bool textChangedFlag;
	PanelCommand* savedCommand;
};
};
#endif // EVENTROUTER_H

