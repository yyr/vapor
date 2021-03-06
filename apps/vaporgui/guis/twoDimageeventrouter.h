//************************************************************************
//																		*
//		     Copyright (C)  2009										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDimageeventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2009
//
//	Description:	Defines the TwoDImageEventRouter class.
//		This class handles events for the twoDImage params
//
#ifndef TWODIMAGEEVENTROUTER_H
#define TWODIMAGEEVENTROUTER_H

#include <qthread.h>
#include <qobject.h>
#include "GL/glew.h"
#include "params.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "twoDimageparams.h"
#include "twoDimagetab.h"
#include "twoDeventrouter.h"


using namespace VetsUtil;

namespace VAPoR {

class TwoDImageParams;
class XmlNode;
class PanelCommand;
class TwoDImageEventRouter : public QWidget, public Ui_TwoDImageTab, public TwoDEventRouter {
	Q_OBJECT
public: 
	
	TwoDImageEventRouter(QWidget* parent);
	virtual ~TwoDImageEventRouter();
	static EventRouter* CreateTab(){
		TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
		return (EventRouter*)(new TwoDImageEventRouter((QWidget*)tMgr));
	}

	
	//Connect signals and slots from tab
	
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1, bool reEnable = false);
	virtual void cleanParams(Params* ) {} 

	virtual void refreshTab();
	//void sessionLoadTF(QString* name);
	
#ifdef Darwin
	void paintEvent(QPaintEvent*);
#endif
	void sliderToText(TwoDImageParams* pParams, int coord, int sliderval, bool isSizeSlider);

	virtual void updateRenderer(RenderParams* dParams, bool prevEnabled,  bool newWindow);
	virtual void captureMouseDown(int button);
	virtual void captureMouseUp();
	void textToSlider(TwoDImageParams* pParams, int coord, float newCenter, float newSize);
	void setZCenter(TwoDImageParams* pParams,int sliderval);
	void setYCenter(TwoDImageParams* pParams,int sliderval);
	void setXCenter(TwoDImageParams* pParams,int sliderval);
	
	void setYSize(TwoDImageParams* pParams,int sliderval);
	void setXSize(TwoDImageParams* pParams,int sliderval);
	
	
		
	virtual void reinitTab(bool doOverride);
	

	virtual void guiSetEnabled(bool value, int instance, bool undoredo = true);
	
	void guiSetXCenter(int sliderVal);
	void guiSetYCenter(int sliderVal);
	void guiSetZCenter(int sliderVal);
	
	void guiSetYSize(int sliderval);
	void guiSetXSize(int sliderval);
	void guiStartCursorMove();
	void guiEndCursorMove();
	void guiCopyRegionToTwoD();
	
	



protected slots:
	void guiSetFidelity(int buttonID);
	void guiSetFidelityDefault();
	void guiFitToRegion();
	void guiChangeInstance(int);
	void guiNewInstance();
	void guiDeleteInstance();
	void twoDReturnPressed();
	void twoDAttachSeed(bool attach);
	void setTwoDTabTextChanged(const QString& qs);
	
	void guiFitToImage();
	void guiSetCrop(bool);
	void guiSetPlacement(int);
	void guiSelectImageFile();
	void guiSelectInstalledImage();
	void guiSetOrientation(int);
	void guiSetGeoreferencing(bool);

	void guiApplyTerrain(bool);
	void guiSetHeightVarNum(int);
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

	void setTwoDXCenter();
	void setTwoDYCenter();
	void setTwoDZCenter();
	void setTwoDXSize();
	void setTwoDYSize();
	void setTwoDEnabled(bool on, int instance);
	void guiSetNumRefinements(int numtrans);
	void guiSetCompRatio(int num);
	void guiSetOpacitySlider(int);
	
protected:
	//Drop wheel events (they cause confusion here)
	virtual void wheelEvent(QWheelEvent* e){}
	static const char* webHelpText[];
	static const char* webHelpURL[];
	//Calculate the current cursor position, set it in the Params:
	void mapCursor();
	// update the left and right bounds text:
	void updateBoundsText(RenderParams*);
	//fix TwoD box to fit in domain:
	void adjustBoxSize(TwoDImageParams*);
	void resetTextureSize(TwoDImageParams*);

};

};

#endif //PROBEEVENTROUTER_H 

