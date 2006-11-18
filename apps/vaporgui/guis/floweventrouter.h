//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		floweventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2006
//
//	Description:	Defines the FlowEventRouter class.
//		This class handles events for the flow params
//
#ifndef FLOWEVENTROUTER_H
#define FLOWEVENTROUTER_H


#include <qobject.h>
#include "params.h"
#include "eventrouter.h"
#include "vapor/MyBase.h"
#include "flowtab.h"



using namespace VetsUtil;

namespace VAPoR {

class ViewpointParams;
class XmlNode;
class PanelCommand;
class FlowEventRouter : public FlowTab, public EventRouter {
	Q_OBJECT
public: 
	
	FlowEventRouter(QWidget* parent, const char* name);
	virtual ~FlowEventRouter();

	virtual void updateMapBounds(RenderParams* p);
	virtual void updateClut(RenderParams* p){
		VizWinMgr::getInstance()->setFlowGraphicsDirty((FlowParams*)p);
	}
	
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1);
	virtual void cleanParams(Params* p); 
	
	void fileLoadTF(FlowParams* dParams);
	void sessionLoadTF(FlowParams* dParams, QString* name);
	void fileSaveTF(FlowParams* dParams);
		
	//There are multiple notions of "dirty" here!
	virtual void setEditorDirty(RenderParams* p = 0);
		
	virtual void reinitTab(bool doOverride);

	//Start to slide a handle.  Need to save direction vector
	//
	virtual void captureMouseDown();
	//When the mouse goes up, save the displacement into the params
	virtual void captureMouseUp();
	
	void guiCenterRake(const float* coords);
	void guiAddSeed(Point4 pt);
	void guiMoveLastSeed(const float* coords );
	
	
	//Methods to make sliders and text consistent for seed region:
	void textToSlider(FlowParams* fParams,int coord, float center, float size);
	void sliderToText(FlowParams* fParams,int coord, int center, int size);
	virtual void updateRenderer(RenderParams* fParams, bool prevEnabled,  bool newWindow);

public slots:

	virtual void guiStartChangeMapFcn(QString s);
	virtual void guiEndChangeMapFcn();

	void guiSetRakeToRegion();
protected slots:
	void addSample();
	void deleteSample();
	void timestepChanged1(int row, int col);
	void timestepChanged2(int row, int col);
	void toggleAdvanced();
	void toggleShowMap();
	void guiSetSteadyDirection(int comboIndex);
	void guiSetUnsteadyDirection(int comboIndex);
	void guiChangeInstance(int);
	void guiNewInstance();
	void guiDeleteInstance();
	
	void guiCopyInstanceTo(int toViz);
	//Slots for flow panel:
	//three kinds of text changed for flow tab
	void setFlowTabFlowTextChanged(const QString&);
	void setFlowTabGraphicsTextChanged(const QString&);
	void setFlowTabRangeTextChanged(const QString&);
	void flowTabReturnPressed();
	//slots for flow tab:
	void guiRefreshFlow();
	void setFlowEnabled(bool val,int instance);
	void setFlowType(int);
	void guiSetNumRefinements(int numtrans);
	void guiSetXComboSteadyVarNum(int varnum);
	void guiSetYComboSteadyVarNum(int varnum);
	void guiSetZComboSteadyVarNum(int varnum);
	void guiSetXComboUnsteadyVarNum(int varnum);
	void guiSetYComboUnsteadyVarNum(int varnum);
	void guiSetZComboUnsteadyVarNum(int varnum);
	
	void guiSetRandom(bool rand);
	void guiCheckPeriodicX(bool periodic);
	void guiCheckPeriodicY(bool periodic);
	void guiCheckPeriodicZ(bool periodic);
	void guiSetAutoRefresh(bool isOn);
	void setFlowXCenter();
	void setFlowYCenter();
	void setFlowZCenter();
	void setFlowXSize();
	void setFlowYSize();
	void setFlowZSize();
	void setFlowGeomSamples();
	void setFlowConstantColor();
	void guiSetFlowGeometry(int geomNum);
	void guiSetColorMapEntity( int entityNum);
	void guiSetOpacMapEntity( int entityNum);
	void setFlowEditMode(bool);
	void setFlowNavigateMode(bool);
	void guiSetAligned();
	void guiDoSeedList(bool isOn);
	void guiDoRake(bool isOn);
	void guiEditSeedList();
	void guiLoadSeeds();
	void guiSaveFlowLines();
	
	void setSteadyLength();
	void flowOpacityScale();
	void guiToggleAutoScale(bool isOn);
	void guiToggleTimestepSample(bool isOn);

protected:
	void guiUpdateUnsteadyTimes(QTable*, const char*);
	void populateTimestepTables();
	void setXCenter(FlowParams*,int sliderval);
	void setYCenter(FlowParams*,int sliderval);
	void setZCenter(FlowParams*,int sliderval);
	void setXSize(FlowParams*,int sliderval);
	void setYSize(FlowParams*,int sliderval);
	void setZSize(FlowParams*,int sliderval);
	void guiSetEditMode(bool val); //edit versus navigate mode
	void guiSetOpacityScale( int scale);
	void guiSetSteadyLength(int sliderpos);

	//Methods that record changes in the history are preceded by 'gui'
	//
	virtual void guiSetEnabled(bool, int);
	void guiSetFlowType(int typenum);
	
	void guiSetXCenter(int sliderval);
	void guiSetYCenter(int sliderval);
	void guiSetZCenter(int sliderval);
	void guiSetXSize(int sliderval);
	void guiSetYSize(int sliderval);
	void guiSetZSize(int sliderval);
	void guiSetConstantColor(QColor& newColor);
	void guiSetGeomSamples(int sliderVal);
	void setMapBoundsChanged(bool on){mapBoundsChanged = on; flowGraphicsChanged = on;}
	void setFlowDataChanged(bool on){flowDataChanged = on;}
	void setFlowGraphicsChanged(bool on){flowGraphicsChanged = on;}

	//Flags to know what has changed when text changes:
	bool flowDataChanged;
	bool flowGraphicsChanged;
	bool mapBoundsChanged;

	int copyCount[MAXVIZWINS+1];
	bool showAdvanced;
	bool showMapEditor;
	
};

};

#endif //FLOWEVENTROUTER_H 

