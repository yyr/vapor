//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		regioneventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Defines the RegionEventRouter class.
//		This class handles events for the region params
//
#ifndef REGIONEVENTROUTER_H
#define REGIONEVENTROUTER_H


#include <qobject.h>
#include "params.h"
#include "eventrouter.h"
#include "vapor/MyBase.h"
#include "regiontab.h"


using namespace VetsUtil;


namespace VAPoR {

class ViewpointParams;
class XmlNode;
class PanelCommand;
class RegionEventRouter : public RegionTab, public EventRouter {

	Q_OBJECT

public: 
	RegionEventRouter(QWidget* parent = 0, const char* name = 0);
	virtual ~RegionEventRouter();
	
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab(Params* p);
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1);

	
	//Following methods are set from gui, have undo/redo support:
	//
	void guiSetCenter(const float * coords);
	
	
	void guiSetXCenter(int n);
	void guiSetXSize(int n);
	void guiSetYCenter(int n);
	void guiSetYSize(int n);
	void guiSetZCenter(int n);
	void guiSetZSize(int n);
	void guiCopyRakeToRegion();
	void guiCopyProbeToRegion();

	

	//Methods to make sliders and text valid and consistent for region:
	void textToSlider(RegionParams*, int coord, float center, float size);
	void sliderToText(RegionParams*, int coord, int center, int size);
	//Start to slide a region face.  Need to save direction vector
	//
	virtual void captureMouseDown();
	//When the mouse goes up, save the face displacement into the region.
	void captureMouseUp();
	
	//Methods to make sliders and text valid and consistent for region:
	void textToSlider(int coord, float center, float size);
	void sliderToText(int coord, int center, int size);
	void refreshRegionInfo(RegionParams* rp);
	
	
	virtual void reinitTab(bool doOverride);

	
protected slots:
	
	void setRegionTabTextChanged(const QString& qs);
	void regionReturnPressed();
	void guiSetMaxSize();
	void guiSetNumRefinements(int n);
	void guiSetVarNum(int n);
	void guiSetTimeStep(int n);
	//Sliders set these:
	void setRegionXCenter();
	void setRegionYCenter();
	void setRegionZCenter();
	void setRegionXSize();
	void setRegionYSize();
	void setRegionZSize();
	void copyRegionToRake();
	void copyRakeToRegion();
	void copyRegionToProbe();
	void copyProbeToRegion();

};

};
#endif //REGIONEVENTROUTER_H 
