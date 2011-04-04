//************************************************************************
//																		*
//		     Copyright (C)  2007										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isoeventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2007
//
//	Description:	Defines the IsoEventRouter class.
//		This class handles events for the iso params
//
#ifndef ISOEVENTROUTER_H
#define ISOEVENTROUTER_H


#include <qobject.h>
#include "GL/glew.h"
#include "params.h"
#include "ParamsIso.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "isotab.h"
#include "transferfunction.h"



using namespace VetsUtil;

namespace VAPoR {

class ViewpointParams;
class XmlNode;
class PanelCommand;
class Params;
class IsoEventRouter : public QWidget, public Ui_IsoTab, public EventRouter {
	Q_OBJECT

public: 
	
	
	IsoEventRouter(QWidget* parent);
	virtual ~IsoEventRouter();

	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1, bool reEnable = false);
	
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	
	virtual void cleanParams(Params* p); 
	virtual QSize const sizeHint(){ return QSize(440,800);}
#ifdef Darwin
	void paintEvent(QPaintEvent*);
#endif

	//Required method to create the tab:
	static EventRouter* CreateTab(){
		TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
		return (EventRouter*)(new IsoEventRouter((QWidget*)tMgr));
	}
	//Convenience method to obtain current ParamsIso Pointer
	static ParamsIso* getActiveIsoParams(){
		int activeViz = VizWinMgr::getInstance()->getActiveViz();
		return((ParamsIso*)Params::GetParamsInstance(Params::_isoParamsTag,activeViz,-1));
	}
	//gui... methods are set from gui, have undo/redo support:
	//
	
	virtual void guiSetEnabled(bool value, int instance);

	//special version for iso, since it has two different histograms.
	virtual Histo* getHistogram(RenderParams*, bool mustGet, bool isIsoWin = false);


	virtual void updateRenderer(RenderParams* dParams, bool prevEnabled,  bool newWindow);
		
	virtual void reinitTab(bool doOverride);
	virtual void refreshTab();
	void guiSetConstantColor(QColor& c);
	
	void sessionLoadTF(QString* name);
	
protected slots:
	void guiFitTFToData();
	void guiSetOpacityScale(int val);
	void guiChangeInstance(int);
	void guiNewInstance();
	void guiDeleteInstance();
	void guiCopyInstanceTo(int toViz);
	void guiCopyProbePoint();

	void isoLoadTF();
	void isoLoadInstalledTF();
	void isoSaveTF();

	void guiSetComboVarNum(int val);
	void guiSetMapComboVarNum(int val);
	
	void guiSetLighting(bool val);
	void guiSetNumRefinements(int num);
	void guiSetCompRatio(int num);
	void guiSetNumBits(int num);

	void setIsoTabTextChanged(const QString& qs);
	void setIsoTabRenderTextChanged(const QString& qs);
	void isoReturnPressed();
	void setIsoEnabled(bool on, int instance);
	void refreshHisto();
	void setIsoEditMode(bool);
	void setIsoNavigateMode(bool);
	void setTFNavigateMode(bool);
	void setConstantColor();
	void guiPassThruPoint();
	void guiStartChangeIsoSelection(QString);
	void guiEndChangeIsoSelection();
	void guiBindColorToOpac();
	void guiBindOpacToColor();
	void setTFEditMode(bool);
	void guiSetTFAligned();
	void guiSetIsoAligned();
	void refreshTFHisto();
	void guiStartChangeMapFcn(QString);
    void guiEndChangeMapFcn();
	void guiSetTFEditMode(bool);
    void setBindButtons(bool);
	


protected:
	virtual void setEditorDirty(RenderParams *p = 0);
	void updateHistoBounds(RenderParams*);
	virtual void setDatarangeDirty(RenderParams*);
	virtual void updateMapBounds(RenderParams*);
	float evaluateSelectedPoint();
	int copyCount[MAXVIZWINS+1];
	bool editMode;
	bool renderTextChanged;
};

};

#endif //ISOEVENTROUTER_H 



