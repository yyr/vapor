//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		viewpointeventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Defines the ViewpointEventRouter class.
//		This class handles events for the viewpoint params
//
#ifndef VIEWPOINTEVENTROUTER_H
#define VIEWPOINTEVENTROUTER_H

#include <qobject.h>
#include "params.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "viztab.h"
#include "viewpointparams.h"
#define TEST_KEYFRAMING

using namespace VetsUtil;


namespace VAPoR {

class ViewpointParams;
class XmlNode;

class ViewpointEventRouter : public QWidget, public Ui_VizTab, public EventRouter {

	Q_OBJECT

public: 
	ViewpointEventRouter(QWidget* parent = 0);
	virtual ~ViewpointEventRouter();
	static EventRouter* CreateTab(){
		TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
		return (EventRouter*)(new ViewpointEventRouter((QWidget*)tMgr));
	}
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1, bool reEnable = false);
	
	
	//Following methods are for undo/redo support:
	//Methods to capture state at start and end of mouse moves:
	//
	
	virtual void captureMouseDown(int button);
	//When the mouse goes up, save the face displacement into the region.
	virtual void captureMouseUp();
	//When the spin is ended, it replaces captureMouseUp:
	void endSpin();
	void navigate (ViewpointParams* vpParams, float* posn, float* viewDir, float* upVec);

	
	//Methods to handle home viewpoint
	void setHomeViewpoint();
	void useHomeViewpoint();
	//Following are only accessible from main menu
	void guiCenterFullRegion(RegionParams* rParams);
	void guiCenterSubRegion(RegionParams* rParams);
	void guiAlignView(int axis);

	//Set from probe:
	void guiSetCenter(const float* centerCoords);
	
	virtual void reinitTab(bool doOverride);
	void updateRenderer(ViewpointParams* dParams, bool prevEnabled,  bool newWindow);

protected:
	float lastCamPos[3];
	bool panChanged;

#ifdef TEST_KEYFRAMING
	FILE* viewpointOutputFile;
#endif
	
protected slots:
	void guiSetStereoMode(int);
	void guiToggleLatLon(bool);
	void viewpointReturnPressed();
	void setVtabTextChanged(const QString& qs);
#ifdef TEST_KEYFRAMING
	void writeKeyframe();
	void readKeyframes();
#endif

};

};
#endif //VIEWPOINTEVENTROUTER_H 
