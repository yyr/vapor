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
#include <vapor/MyBase.h>
#include "regiontab.h"
#include "tabmanager.h"


using namespace VetsUtil;
QT_USE_NAMESPACE


namespace VAPoR {

class ViewpointParams;
class XmlNode;

class RegionEventRouter : public QWidget, public Ui_RegionTab, public EventRouter {

	Q_OBJECT

public: 
	RegionEventRouter(QWidget* parent = 0);
	virtual ~RegionEventRouter();
	static EventRouter* CreateTab(){
		TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
		QWidget* parent = tMgr->getSubTabWidget(1);
		return (EventRouter*)(new RegionEventRouter(parent));
	}
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
    virtual void relabel();
	
	
	//Following methods are set from gui, have undo/redo support:
	//
	void setCenter(const double * coords);
	
	

	//Methods to make sliders and text valid and consistent for region:
	//If doSet is false, no setvalue is issued to the Params.
	void textToSlider(RegionParams*, int coord, float center, float size, bool doSet);
	void sliderToText(RegionParams*, int coord, int center, int size);
	//Start to slide a region face.  Need to save direction vector
	//
	virtual void captureMouseDown(int button);
	//When the mouse goes up, save the face displacement into the region.
	void captureMouseUp();
	
	virtual void reinitTab(bool doOverride);

	
protected slots:
	void loadRegionExtents();
	void saveRegionExtents();
	void adjustExtents();
	void setRegionTabTextChanged(const QString& qs);
	void regionReturnPressed();
	void setMaxSize();
	
	void setXCenter(int n);
	void setXSize(int n);
	void setYCenter(int n);
	void setYSize(int n);
	void setZCenter(int n);
	void setZSize(int n);
	void copyRegionToRake();
	void copyRakeToRegion();
	void copyProbeToRegion();

};

};
#endif //REGIONEVENTROUTER_H 
