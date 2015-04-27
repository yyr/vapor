//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isolineeventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Defines the IsolineEventRouter class.
//		This class handles events for the region params
//
#ifndef ISOLINEEVENTROUTER_H
#define ISOLINEEVENTROUTER_H

#include <qthread.h>
#include <qobject.h>
#include "GL/glew.h"
#include "params.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "isolineAppearance.h"
#include "isolineLayout.h"
#include "isolineIsovals.h"
#include "isolineImage.h"
#include "isolineBasics.h"


using namespace VetsUtil;

namespace VAPoR {

class IsolineParams;
class XmlNode;
class PanelCommand;
class TransferFunction;

class IsolineEventRouter : public QTabWidget, public EventRouter {
	Q_OBJECT
public: 
	
	IsolineEventRouter(QWidget* parent);
	virtual ~IsolineEventRouter();
	static EventRouter* CreateTab(){
		TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
		return (EventRouter*)(new IsolineEventRouter((QWidget*)tMgr));
	}
	
	
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	
	virtual void setEditorDirty(RenderParams*);
	
	void sliderToText(IsolineParams* pParams, int coord, int slideCenter, int slideSize);

	virtual void captureMouseDown(int button);
	virtual void captureMouseUp();

	virtual void updateMapBounds(RenderParams* rParams);
#ifdef Darwin
	void paintEvent(QPaintEvent*);
#endif
	void sessionLoadTF(QString* name);
	virtual void setDatarangeDirty(RenderParams* ){}
	void textToSlider(IsolineParams* pParams, int coord, float newCenter, float newSize);
	void setZCenter(IsolineParams* pParams,int sliderval);
	void setYCenter(IsolineParams* pParams,int sliderval);
	void setXCenter(IsolineParams* pParams,int sliderval);
	void setYSize(IsolineParams* pParams,int sliderval);
	void setXSize(IsolineParams* pParams,int sliderval);
	
	
	//Determine the value of the  current variable at specified point.
	//Return OUT_OF_BOUNDS if not in Isoline and in full domain
	float calcCurrentValue(IsolineParams* p, const double point[3]);


	virtual void reinitTab(bool doOverride);
	bool seedIsAttached(){return seedAttached;}
	//methods with undo/redo support:
	//Following methods are set from gui, have undo/redo support:
	//
	virtual void guiSetEnabled(bool value, int instance);
	
	
	void guiSetXCenter(int sliderVal);
	void guiSetYCenter(int sliderVal);
	void guiSetZCenter(int sliderVal);
	
	void guiSetEditMode(bool val); //edit versus navigate mode
	void guiSetXSize(int sliderval);
	void guiSetYSize(int sliderval);
	void guiSetZSize(int sliderval);
	void guiStartCursorMove();
	void guiEndCursorMove();
	void guiCopyRegionToIsoline();
	
	void setIsolineDirty(IsolineParams* iParams){
		invalidateRenderer(iParams);
	}


protected slots:
	void changeExtents();
	void guiSetSingleColor();
	void guiSetUseSingleColor(bool val);
	void guiSpaceIsovalues();
	void guiEditIsovalues();
	void guiSetIsolineDensity();
	void isolineLoadTF();
	void isolineLoadInstalledTF();
	void isolineSaveTF();
	void setIsolineEditMode(bool);
	void setIsolineNavigateMode(bool);
	void refreshHisto();
	void guiSetFidelity(int buttonID);
	void guiSetFidelityDefault();
	void guiNudgeXSize(int);
	void guiNudgeXCenter(int);
	void guiNudgeYSize(int);
	void guiNudgeYCenter(int);
	void guiNudgeZCenter(int);
	
	//Handle thumbwheel events:
	void pressXWheel();
	void pressYWheel();
	void pressZWheel();
	void rotateXWheel(int);
	void rotateYWheel(int);
	void rotateZWheel(int);
	void guiReleaseXWheel(int);
	void guiReleaseYWheel(int);
	void guiReleaseZWheel(int);
	void guiRotate90(int);
	void guiFitRegion();
	void guiFitDomain();
	void guiCropToRegion();
	void guiCropToDomain();

	
	void guiAxisAlign(int);

	void copyToProbeOr2D();
	
	//Slots for Isoline panel:
	void isolineCenterRegion();
	void isolineCenterView();
	void isolineCenterRake();
	void guiCenterIsolines();
	void isolineAddSeed();
	void isolineAttachSeed(bool attach);
	void guiEnableText(bool val);
	
	void guiChangeVariable(int);
	void setIsolineXCenter();
	void setIsolineYCenter();
	void setIsolineZCenter();
	void setIsolineXSize();
	void setIsolineYSize();
	
	void guiSetNumRefinements(int numtrans);
	void guiSetCompRatio(int num);
	void setIsolineTabTextChanged(const QString& qs);
	void isolineReturnPressed();
	
	void guiSetPanelBackgroundColor();
	void guiSetDimension(int);
	void guiSetAligned();
	void guiStartChangeIsoSelection(QString);
	void guiEndChangeIsoSelection();
	void guiFitToData();
	void guiCopyToProbe();
	void guiCopyTo2D();
	
	
protected:
	virtual void wheelEvent(QWheelEvent*) {}
	IsolineBasics *myBasics;
	IsolineAppearance * myAppearance;
	IsolineLayout * myLayout;
	IsolineIsovals * myIsovals;
	IsolineImage *myImage;
	static const char* webHelpText[];
	static const char* webHelpURL[];
	void setIsolineToExtents(const double* extents, IsolineParams* pparams);
	void fitIsovalsToHisto(IsolineParams* p);
	//Modify the colormap so that isovalues occur at discrete color changes:
	void convertIsovalsToColors(TransferFunction* tf);
	void mapCursor();
	void updateBoundsText(RenderParams*);
	//Convert rotation about axis between actual and viewed in stretched coords:
	double convertRotStretchedToActual(int axis, double angle);
	void resetImageSize(IsolineParams* iParams);
	void invalidateRenderer(IsolineParams* iParams);
	void updateHistoBounds(RenderParams*);
	virtual void refreshHistogram(RenderParams* p);
	
	//fix Isoline box to fit in domain:
	void adjustBoxSize(IsolineParams*);
	
	static const float thumbSpeedFactor;
	bool seedAttached;
	//FlowParams* attachedFlow;
	//Flag to enable resetting of the listbox without
	//triggering a gui changed event
	bool ignoreComboChanges;
	//Flag to ignore slider events caused by updating tab,
	//so they won't be recognized as a nudge event.
	bool notNudgingSliders;
	
	vector<int> copyCount;
	int lastXSizeSlider, lastYSizeSlider;
	int lastXCenterSlider, lastYCenterSlider, lastZCenterSlider;
	float maxBoxSize[3];
	
	float startRotateActualAngle;
	float startRotateViewAngle;
	bool renormalizedRotate;
	bool showImage, showAppearance, showLayout;
	bool editMode;
	double prevIsoMax,prevIsoMin;  //
	
};

};


#endif //ISOLINEEVENTROUTER_H 

