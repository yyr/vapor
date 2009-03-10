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
//		This class handles events for the twoD params
//
#ifndef TWODEVENTROUTER_H
#define TWODEVENTROUTER_H

#include <qthread.h>
#include <qobject.h>
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
class TwoDEventRouter : public TwoDtab, public EventRouter {
	Q_OBJECT
public: 
	
	TwoDEventRouter(QWidget* parent, const char* name);
	virtual ~TwoDEventRouter();

	virtual void updateMapBounds(RenderParams* p);
	virtual void updateClut(RenderParams* p){
		setTwoDDirty((TwoDParams*)p);
		VizWinMgr::getInstance()->setVizDirty(p,TwoDTextureBit,true);
	}
	
	//Connect signals and slots from tab
	
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1, bool reEnable = false);
	virtual void cleanParams(Params* p); 

	virtual void refreshTab();
	void sessionLoadTF(QString* name);
	
	void sliderToText(TwoDParams* pParams, int coord, int slideCenter, int slideSize);

	virtual void updateRenderer(RenderParams* dParams, bool prevEnabled,  bool newWindow);
	virtual void captureMouseDown();
	virtual void captureMouseUp();
	void textToSlider(TwoDParams* pParams, int coord, float newCenter, float newSize);
	void setZCenter(TwoDParams* pParams,int sliderval);
	void setYCenter(TwoDParams* pParams,int sliderval);
	void setXCenter(TwoDParams* pParams,int sliderval);
	
	void setYSize(TwoDParams* pParams,int sliderval);
	void setXSize(TwoDParams* pParams,int sliderval);
	
	
	//Determine the value of the variable(s) at specified point.
	//Return OUT_OF_BOUNDS if 2 relevant coords not in twoD region and in full domain
	float calcCurrentValue(TwoDParams* p, const float point[3], int* sessionVarNums, int numVars);

	//TwoD has a special version of histogramming
	virtual Histo* getHistogram(RenderParams*, bool mustGet, bool isIsoWin = false);
	virtual void refreshHistogram(RenderParams* p);


	virtual void setEditorDirty(RenderParams* p = 0);
		
	virtual void reinitTab(bool doOverride);
	bool seedIsAttached(){return seedAttached;}
	//methods with undo/redo support:
	//Following methods are set from gui, have undo/redo support:
	//
	

	virtual void guiSetEnabled(bool value, int instance);
	//respond to changes in TF (for undo/redo):
	//
	
	
	void guiAttachSeed(bool attach, FlowParams*);
	
	void guiSetXCenter(int sliderVal);
	void guiSetYCenter(int sliderVal);
	void guiSetZCenter(int sliderVal);
	void guiSetOpacityScale(int val);
	void guiSetEditMode(bool val); //edit versus navigate mode
	
	void guiSetYSize(int sliderval);
	void guiSetXSize(int sliderval);
	void guiStartCursorMove();
	void guiEndCursorMove();
	void guiCopyRegionToTwoD();
	
	void setTwoDDirty(TwoDParams* pParams){
		pParams->setTwoDDirty();
	}

public slots:
    //
	//respond to changes in TF (for undo/redo):
	//
	virtual void guiStartChangeMapFcn(QString qs);
	virtual void guiEndChangeMapFcn();

	void setBindButtons(bool canbind);

protected slots:
	void guiFitToImage();
	void guiSetLatLon(bool);
	void guiChangeType(int);
	void guiSelectImageFile();
	void guiSetOrientation(int);
	void guiSetGeoreferencing(bool);

	void guiApplyTerrain(bool);
	void guiNudgeXSize(int);
	void guiNudgeXCenter(int);
	void guiNudgeYSize(int);
	void guiNudgeYCenter(int);
	void guiNudgeZCenter(int);
	
	void guiChangeInstance(int);
	void guiNewInstance();
	void guiDeleteInstance();
	
	void guiCopyInstanceTo(int toViz);
	//Slots for TwoD panel:
	void twoDCenterRegion();
	void twoDCenterView();
	void twoDCenterRake();
	void guiCenterTwoD();
	void guiCenterProbe();
	void twoDAddSeed();
	void twoDAttachSeed(bool attach);
	
	void guiChangeVariables();
	void setTwoDXCenter();
	void setTwoDYCenter();
	void setTwoDZCenter();
	
	void setTwoDXSize();
	void setTwoDYSize();
	
	void setTwoDEnabled(bool on, int instance);
	void setTwoDEditMode(bool);
	void setTwoDNavigateMode(bool);
	
	void twoDOpacityScale();
	void guiBindColorToOpac();
	void guiBindOpacToColor();
	void twoDLoadTF();
	void twoDLoadInstalledTF();
	void twoDSaveTF();
	void refreshTwoDHisto();
	void guiSetNumRefinements(int numtrans);
	void setTwoDTabTextChanged(const QString& qs);
	void twoDReturnPressed();
	void captureImage();
	
protected:
	
	//Calculate the current cursor position, set it in the Params:
	void mapCursor();
	// update the left and right bounds text:
	void updateBoundsText(RenderParams*);
	//fix TwoD box to fit in domain:
	void adjustBoxSize(TwoDParams*);
	void resetTextureSize(TwoDParams*);
	virtual void setDatarangeDirty(RenderParams*);
	QString getMappedVariableNames(int* numvars);
	bool seedAttached;
	FlowParams* attachedFlow;
	//Flag to enable resetting of the listbox without
	//triggering a gui changed event
	bool ignoreListboxChanges;
	//Flag to ignore slider events caused by updating tab,
	//so they won't be recognized as a nudge event.
	bool notNudgingSliders;
	int numVariables;
	int copyCount[MAXVIZWINS+1];
	int lastXSizeSlider, lastYSizeSlider;
	int lastXCenterSlider, lastYCenterSlider, lastZCenterSlider;

};

};

#endif //PROBEEVENTROUTER_H 

