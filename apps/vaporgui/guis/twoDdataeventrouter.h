//************************************************************************
//																		*
//		     Copyright (C)  2009										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDdataeventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2009
//
//	Description:	Defines the twoDDataEventRouter class.
//		This class handles events for the twoDDataParams
//
#ifndef TWODDATAEVENTROUTER_H
#define TWODDATAEVENTROUTER_H

#include <qthread.h>
#include <qobject.h>
#include "params.h"
#include "eventrouter.h"
#include "vapor/MyBase.h"
#include "twoDdataparams.h"
#include "twoDdatatab.h"
#include "twoDeventrouter.h"


using namespace VetsUtil;

namespace VAPoR {

class TwoDDataParams;
class XmlNode;
class PanelCommand;
class TwoDDataEventRouter : public TwoDDataTab, public TwoDEventRouter {
	Q_OBJECT
public: 
	
	TwoDDataEventRouter(QWidget* parent, const char* name);
	virtual ~TwoDDataEventRouter();

	virtual void updateMapBounds(RenderParams* p);
	virtual void updateClut(RenderParams* p){
		setTwoDDirty((TwoDDataParams*)p);
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
	
	void sliderToText(TwoDDataParams* pParams, int coord, int slideCenter, int slideSize);

	virtual void updateRenderer(RenderParams* dParams, bool prevEnabled,  bool newWindow);
	virtual void captureMouseDown();
	virtual void captureMouseUp();
	void textToSlider(TwoDDataParams* pParams, int coord, float newCenter, float newSize);
	void setZCenter(TwoDDataParams* pParams,int sliderval);
	void setYCenter(TwoDDataParams* pParams,int sliderval);
	void setXCenter(TwoDDataParams* pParams,int sliderval);
	
	void setYSize(TwoDDataParams* pParams,int sliderval);
	void setXSize(TwoDDataParams* pParams,int sliderval);
	
	virtual Histo* getHistogram(RenderParams*, bool mustGet, bool isIsoWin = false);
	virtual void refreshHistogram(RenderParams* p);
	virtual void setEditorDirty(RenderParams* p = 0);
		
	virtual void reinitTab(bool doOverride);
	virtual void guiSetEnabled(bool value, int instance);
	
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
	
public slots:
    //
	//respond to changes in TF (for undo/redo):
	//
	virtual void guiStartChangeMapFcn(QString qs);
	virtual void guiEndChangeMapFcn();

	void setBindButtons(bool canbind);

protected slots:
	void guiFitToRegion();
	void guiChangeInstance(int);
	void guiNewInstance();
	void guiDeleteInstance();
	
	void twoDAttachSeed(bool attach);
	void setTwoDTabTextChanged(const QString& qs);
	void twoDReturnPressed();
	
	void guiSetOrientation(int);
	void guiApplyTerrain(bool);
	void guiNudgeXSize(int);
	void guiNudgeXCenter(int);
	void guiNudgeYSize(int);
	void guiNudgeYCenter(int);
	void guiNudgeZCenter(int);
	
	void guiCopyInstanceTo(int toViz);
	
	void twoDCenterRegion();
	void twoDCenterView();
	void twoDCenterRake();
	void guiCenterTwoD();
	void guiCenterProbe();
	void twoDAddSeed();
	
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
	
	void captureImage();
	
protected:
	
	//Calculate the current cursor position, set it in the Params:
	void mapCursor();
	// update the left and right bounds text:
	void updateBoundsText(RenderParams*);
	//fix TwoD box to fit in domain:
	void adjustBoxSize(TwoDDataParams*);
	void resetTextureSize(TwoDDataParams*);
	virtual void setDatarangeDirty(RenderParams*);
	QString getMappedVariableNames(int* numvars);
	
	int numVariables;

};

};

#endif //TWODDATAEVENTROUTER_H 

