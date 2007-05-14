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
#include "vapor/MyBase.h"
#include "viztab.h"


using namespace VetsUtil;


namespace VAPoR {

class ViewpointParams;
class XmlNode;

class ViewpointEventRouter : public VizTab, public EventRouter {

	Q_OBJECT

public: 
	ViewpointEventRouter(QWidget* parent = 0, const char* name = 0);
	virtual ~ViewpointEventRouter();
	
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1);
	
	
	//Following methods are for undo/redo support:
	//Methods to capture state at start and end of mouse moves:
	//
	
	virtual void captureMouseDown();
	//When the mouse goes up, save the face displacement into the region.
	virtual void captureMouseUp();
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

	
protected slots:
	void guiSetStereoMode(int);
	void viewpointReturnPressed();
	void setVtabTextChanged(const QString& qs);

};

};
#endif //VIEWPOINTEVENTROUTER_H 
