//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		probeeventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Defines the ProbeEventRouter class.
//		This class handles events for the region params
//
#ifndef PROBEEVENTROUTER_H
#define PROBEEVENTROUTER_H


#include <qobject.h>
#include "params.h"
#include "eventrouter.h"
#include "vapor/MyBase.h"

#include "probetab.h"


using namespace VetsUtil;

namespace VAPoR {

class ProbeParams;
class XmlNode;
class PanelCommand;
class ProbeEventRouter : public ProbeTab, public EventRouter {
	Q_OBJECT
public: 
	
	ProbeEventRouter(QWidget* parent, const char* name);
	virtual ~ProbeEventRouter();

	virtual void updateMapBounds(RenderParams* p);
	virtual void updateClut(RenderParams* p){
		VizWinMgr::getInstance()->setVizDirty(p,ProbeTextureBit,true);
	}
	
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1);
	virtual void cleanParams(Params* p); 


	void fileLoadTF(ProbeParams* dParams, const char* path, bool savePath);
	void sessionLoadTF(QString* name);
	void fileSaveTF(ProbeParams* dParams);
	
	
	void sliderToText(ProbeParams* pParams, int coord, int slideCenter, int slideSize);

	virtual void updateRenderer(RenderParams* dParams, bool prevEnabled,  bool newWindow);
	virtual void captureMouseDown();
	virtual void captureMouseUp();
	void textToSlider(ProbeParams* pParams, int coord, float newCenter, float newSize);
	void setZCenter(ProbeParams* pParams,int sliderval);
	void setYCenter(ProbeParams* pParams,int sliderval);
	void setXCenter(ProbeParams* pParams,int sliderval);
	void setZSize(ProbeParams* pParams,int sliderval);
	void setYSize(ProbeParams* pParams,int sliderval);
	void setXSize(ProbeParams* pParams,int sliderval);
	
	
	//Determine the value of the variable(s) at specified point.
	//Return OUT_OF_BOUNDS if not in probe and in full domain
	float calcCurrentValue(ProbeParams* p, const float point[3]);

	//probe has a special version of histogramming
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
	void guiSetXSize(int sliderval);
	void guiSetYSize(int sliderval);
	void guiSetZSize(int sliderval);
	void guiStartCursorMove();
	void guiEndCursorMove();
	void guiCopyRegionToProbe();
	

public slots:
    //
	//respond to changes in TF (for undo/redo):
	//
	virtual void guiStartChangeMapFcn(QString qs);
	virtual void guiEndChangeMapFcn();

	void setBindButtons(bool canbind);

protected slots:
	void guiNudgeXSize(int);
	void guiNudgeXCenter(int);
	void guiNudgeYSize(int);
	void guiNudgeYCenter(int);
	void guiNudgeZSize(int);
	void guiNudgeZCenter(int);
	
	//Handle thumbwheel events:
	void rotateXWheel(int);
	void rotateYWheel(int);
	void rotateZWheel(int);
	void guiReleaseXWheel(int);
	void guiReleaseYWheel(int);
	void guiReleaseZWheel(int);
	void guiRotate90(int);

	void guiChangeInstance(int);
	void guiNewInstance();
	void guiDeleteInstance();
	void guiAxisAlign();
	void guiTogglePlanar(bool);
	
	void guiCopyInstanceTo(int toViz);
	//Slots for probe panel:
	void probeCenterRegion();
	void probeCenterView();
	void probeCenterRake();
	void guiCenterProbe();
	void probeAddSeed();
	void probeAttachSeed(bool attach);
	
	void guiChangeVariables();
	void setProbeXCenter();
	void setProbeYCenter();
	void setProbeZCenter();
	void setProbeXSize();
	void setProbeYSize();
	void setProbeZSize();
	void setProbeEnabled(bool on, int instance);
	void setProbeEditMode(bool);
	void setProbeNavigateMode(bool);
	void guiSetAligned();
	void probeOpacityScale();
	void guiBindColorToOpac();
	void guiBindOpacToColor();
	void probeLoadTF();
	void probeLoadInstalledTF();
	void probeSaveTF();
	void refreshProbeHisto();
	void guiSetNumRefinements(int numtrans);
	void setProbeTabTextChanged(const QString& qs);
	void probeReturnPressed();
	void captureImage();
	
protected:
	//fix probe box to fit in domain:
	void adjustBoxSize(ProbeParams*);
	void resetTextureSize(ProbeParams*);
	virtual void setDatarangeDirty(RenderParams*);
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
	int lastXSizeSlider, lastYSizeSlider, lastZSizeSlider;
	int lastXCenterSlider, lastYCenterSlider, lastZCenterSlider;
	float maxBoxSize[3];
};

};

#endif //PROBEEVENTROUTER_H 

