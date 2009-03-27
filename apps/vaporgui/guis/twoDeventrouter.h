//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDeventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Defines the twoDEventRouter class.
//		This is pure virtual class for both twoDImage and twoDData event routers
//
#ifndef TWODEVENTROUTER_H
#define TWODEVENTROUTER_H

#include "params.h"
#include "eventrouter.h"
#include "vapor/MyBase.h"
#include "twoDparams.h"
#include "twoDtab.h"


using namespace VetsUtil;

namespace VAPoR {

class TwoDParams;
class XmlNode;
class PanelCommand;
class TwoDEventRouter : public EventRouter {

public: 
	
	TwoDEventRouter();
	virtual ~TwoDEventRouter();
	//Connect signals and slots from tab
	
	virtual void hookUpTab() = 0;
	virtual void confirmText(bool /*render*/) = 0;
	virtual void updateTab() = 0;
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1, bool reEnable = false)=0;
	virtual void cleanParams(Params* p)=0; 

	virtual void refreshTab()=0;


	virtual void updateRenderer(RenderParams* dParams, bool prevEnabled,  bool newWindow)=0;
	virtual void captureMouseDown()=0;
	virtual void captureMouseUp()=0;
	
	virtual void reinitTab(bool doOverride)=0;
	bool seedIsAttached(){return seedAttached;}
	
	void guiAttachSeed(bool attach, FlowParams*);
	
	void setTwoDDirty(TwoDParams* pParams){
		pParams->setTwoDDirty();
	}
	
protected:
	float calcCurrentValue(TwoDParams* pParams, const float point[3], int* sessionVarNums, int numVars);
	bool seedAttached;
	FlowParams* attachedFlow;
	//Flag to enable resetting of the listbox without
	//triggering a gui changed event
	bool ignoreListboxChanges;
	//Flag to ignore slider events caused by updating tab,
	//so they won't be recognized as a nudge event.
	bool notNudgingSliders;
	int copyCount[MAXVIZWINS+1];
	int lastXSizeSlider, lastYSizeSlider;
	int lastXCenterSlider, lastYCenterSlider, lastZCenterSlider;

};

};

#endif //PROBEEVENTROUTER_H 

