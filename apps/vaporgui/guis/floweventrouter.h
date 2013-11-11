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
#include "GL/glew.h"
#include "params.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "flowtab.h"

using namespace VetsUtil;

namespace VAPoR {

class ViewpointParams;
class XmlNode;
class ParamNode;
class PanelCommand;
class FlowEventRouter : public QWidget, public Ui_FlowTab, public EventRouter {
	Q_OBJECT
public: 
	
	FlowEventRouter(QWidget* parent);
	virtual ~FlowEventRouter();
	static EventRouter* CreateTab(){
		TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
		return (EventRouter*)(new FlowEventRouter((QWidget*)tMgr));
	}

	virtual void updateMapBounds(RenderParams* p);
	virtual void updateClut(RenderParams* p){
		VizWinMgr::getInstance()->setFlowGraphicsDirty((FlowParams*)p);
	}
	
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	virtual void updateUrgentTabState();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1, bool reEnable = false);
	virtual void cleanParams(Params* p); 
	virtual void refreshTab();
	virtual QSize sizeHint() const;
	
	void sessionLoadTF(QString*);
		
	//There are multiple notions of "dirty" here!
	virtual void setEditorDirty(RenderParams* p = 0);
	
	virtual void setDatarangeDirty(RenderParams* fp) {VizWinMgr::getInstance()->setFlowGraphicsDirty((FlowParams*)fp);}
		
	virtual void reinitTab(bool doOverride);

	//Start to slide a handle.  Need to save direction vector
	//
	virtual void captureMouseDown(int button);
	//When the mouse goes up, save the displacement into the params
	virtual void captureMouseUp();
#ifdef Darwin
	void paintEvent(QPaintEvent*);
#endif
	
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
	void stopClicked();

protected slots:
	void guiBindColorToOpac();
	void guiBindOpacToColor();
	void guiFitTFToData();
	void flowLoadTF();
	void flowLoadInstalledTF();
	void flowSaveTF();
	void refreshHisto();
	void showSetupHelp();
	void addSample();
	void deleteSample();
	void guiRebuildList();
	void timestepChanged1(int row, int col);
	
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
	void refreshFlow();
	void setFlowEnabled(bool val,int instance);
	void guiSetFlowType(int typenum);
	void guiSetFLAOption(int choice);
	void guiSetNumRefinements(int numtrans);
	void guiSetCompRatio(int num);
	void guiSetXComboSteadyVarNum(int varnum);
	void guiSetYComboSteadyVarNum(int varnum);
	void guiSetZComboSteadyVarNum(int varnum);
	void guiSetXComboUnsteadyVarNum(int varnum);
	void guiSetYComboUnsteadyVarNum(int varnum);
	void guiSetZComboUnsteadyVarNum(int varnum);
	void guiSetXComboSeedDistVarNum(int varnum);
	void guiSetYComboSeedDistVarNum(int varnum);
	void guiSetZComboSeedDistVarNum(int varnum);
	void guiSetXComboPriorityVarNum(int varnum);
	void guiSetYComboPriorityVarNum(int varnum);
	void guiSetZComboPriorityVarNum(int varnum);
	void guiToggleColorInterpType(bool);
	
	void guiCheckPeriodicX(bool periodic);
	void guiCheckPeriodicY(bool periodic);
	void guiCheckPeriodicZ(bool periodic);
	void guiSetAutoRefresh(bool isOn);
	
	void setBiasFromSlider();
	void setBiasText(int);
	void setFlowConstantColor();
	void guiSetFlowGeometry(int geomNum);
	void guiSetColorMapEntity( int entityNum);
	
	void setFlowEditMode(bool);
	void setFlowNavigateMode(bool);
	
	void guiSetRakeList(int index);
	
	void guiEditSeedList();
	void guiLoadSeeds();
	void saveFlowLines();
	void saveSeeds();
	
	void guiToggleDisplayLists(bool on);
	void guiToggleAutoScale(bool isOn);
	void guiToggleTimestepSample(bool isOn);
	void guiSetOpacityScale( int scale);
	void guiSetXCenter(int sliderval);
	void guiSetYCenter(int sliderval);
	void guiSetZCenter(int sliderval);
	void guiSetXSize(int sliderval);
	void guiSetYSize(int sliderval);
	void guiSetZSize(int sliderval);
	void guiSetSteadyLength(int sliderpos);
	void guiSetSmoothness(int sliderpos);
	void guiSetSteadySamples(int sliderVal);
	void guiSetUnsteadySamples(int sliderVal);
	void showHideAppearance();
	void showHideSeeding();
	void showHideUnsteadyTime();

protected:
	static const char* webHelpText[];
	static const char* webHelpURL[];
	bool dontUpdate;
	void guiUpdateUnsteadyTimes(QTableWidget*, const char*);
	void populateTimestepTables();
	void setXCenter(FlowParams*,int sliderval);
	void setYCenter(FlowParams*,int sliderval);
	void setZCenter(FlowParams*,int sliderval);
	void setXSize(FlowParams*,int sliderval);
	void setYSize(FlowParams*,int sliderval);
	void setZSize(FlowParams*,int sliderval);
	void guiSetEditMode(bool val); //edit versus navigate mode
	
	//Methods that record changes in the history are preceded by 'gui'
	//
	virtual void guiSetEnabled(bool, int, bool undoredo = true);
	
	
	void guiSetConstantColor(QColor& newColor);
	
	void guiSetSeedDistBias(float biasVal);
	void setMapBoundsChanged(bool on){mapBoundsChanged = on; flowGraphicsChanged = on;}
	void setFlowDataChanged(bool on){flowDataChanged = on;}
	void setFlowGraphicsChanged(bool on){flowGraphicsChanged = on;}

	//Flags to know what has changed when text changes:
	bool flowDataChanged;
	bool flowGraphicsChanged;
	bool mapBoundsChanged;

	int copyCount[MAXVIZWINS+1];
	bool showSeeding, showAppearance, showUnsteadyTime;
	
};

};

#endif //FLOWEVENTROUTER_H 

