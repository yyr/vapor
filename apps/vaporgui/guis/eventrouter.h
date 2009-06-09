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
	//Refresh the displayed texture, if there is one..
	virtual void refreshGLWindow(){}
	//Connect signals and slots from tab
	virtual void hookUpTab() = 0;
	//Set all the fields in the tab based on current params
	virtual void updateTab() = 0;
	//Update only tab state that is most urgent after an error message.
	//In particular, don't update any GL widgets in the tab.
	//This is to deal with error messages coming from the rendering thread
	//that are trapped in the gui thread.
	//Default does nothing
	virtual void updateUrgentTabState() {return;}
	//Method to install a new params for undo and redo.
	//Instance is -1 for non-render params
	//reEnable is true if (render) params needs to be disabled and re-enabled
	virtual void makeCurrent(Params* prevParams, Params* newParams, bool newWin, int instance = -1, bool reEnable = false) = 0;
	
	//Methods to change instances (for undo/redo).
	//Only used by renderer params
	//Insert an instance (where there was none).
	//if instPosition is at end, appends to current instances
	//void insertCurrentInstance(Params* newParams, int instPosition);
	//Remove specified instance, disable it if necessary.
	void removeRendererInstance(int winnum, int instance);
	void newRendererInstance(int winnum);
	void copyRendererInstance(int toWinnum, RenderParams* rParams);
	void changeRendererInstance(int winnum, int newInstance);
	Params::ParamType getParamsType() {return myParamsType;}
	virtual void performGuiChangeInstance(int newCurrent);
	virtual void performGuiNewInstance();
	virtual void performGuiDeleteInstance();
	virtual void performGuiCopyInstance();
	virtual void performGuiCopyInstanceToViz(int vizwin);

	//Make the tab refresh after a scrolling operation.
	//Helpful on windows only, and only some tabs
	virtual void refreshTab() {}

	//UpdateRenderer ignores renderParams argument and uses the
	//params associated with the instance if it is nonnegative
	virtual void updateRenderer(RenderParams* , bool /*wasEnabled*/, bool /*newWindow*/){assert(0);}
	
	//make sure the params cleanly detaches from gui, to
	//handle possible connections from editors, frames, etc.
	virtual void cleanParams(Params*) {}
	
	virtual void sessionLoadTF(QString* ) {assert(0);}
	//Clone the params and any other related classes.
	//Default just clones the params:
	virtual Params* deepCopy(Params* p) {return (p->deepCopy());}
	//Dirty bit comes on when any text changes in the tab.
	//Comes off as soon as it gets confirmed, or updateTab called.
	//
	void guiSetTextChanged(bool on) {textChangedFlag = on;}

	virtual void guiSetEnabled(bool, int ) {assert(0);}

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

	virtual Histo* getHistogram(RenderParams*, bool mustGet, bool isIsoWin = false);
	virtual void refreshHistogram(RenderParams* , int sesVarNum, const float drange[2]);

	//For render params, setEditorDirty uses the current instance if Params
	//arg is null
	virtual void setEditorDirty(RenderParams*){assert(0);}
	virtual void updateMapBounds(RenderParams*) {assert (0);}
	virtual void updateClut(RenderParams*){assert(0);}

	//Method for classes that capture mouse event events in viz win:
	virtual void captureMouseUp() {assert(0);}
	virtual void captureMouseDown() {assert(0);}

//Methods for loading/saving transfer functions:
void saveTF(RenderParams* rParams);
void fileSaveTF(RenderParams* rParams);
void loadInstalledTF(RenderParams* rParams, int varnum);
void loadTF(RenderParams* rParams, int varnum);

void fileLoadTF(RenderParams* rParams, int varnum, const char* startPath, bool savePath);

public slots:

	virtual void guiStartChangeMapFcn(QString) { assert(0); }
	virtual void guiEndChangeMapFcn() { assert(0); }
	
	
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

