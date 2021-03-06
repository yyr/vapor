//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		dvreventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Defines the DvrEventRouter class.
//		This class handles events for the dvr params
//
#ifndef DVREVENTROUTER_H
#define DVREVENTROUTER_H


#include <qobject.h>
#include "GL/glew.h"
#include "params.h"
#include "dvrparams.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "dvr.h"

class QTimer;


using namespace VetsUtil;
QT_USE_NAMESPACE

namespace VAPoR {

class ViewpointParams;
class XmlNode;
class ParamNode;
class PanelCommand;
class Params;
class DvrEventRouter : public QWidget, public Ui_DVR, public EventRouter {
	Q_OBJECT

    enum 
    {
      PREAMBLE,
      RENDER,
      TEMPORAL,
      TFEDIT,
      DONE
    };

public: 
	
	void initTypes();
	DvrEventRouter(QWidget* parent);
	virtual ~DvrEventRouter();
	
	virtual void updateMapBounds(RenderParams* p);
	virtual void updateClut(RenderParams* p){
		VizWinMgr::getInstance()->setClutDirty((DvrParams*)p);
	}
	
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1, bool reEnable = false);
	
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	
	virtual void cleanParams(Params* p); 
	void sessionLoadTF(QString* name);
	
	//Following methods are set from gui, have undo/redo support:
	//
	
	virtual void guiSetEnabled(bool value, int instance, bool undoredo = true);
	
	void guiSetEditMode(bool val); //edit versus navigate mode
	DvrParams::DvrType getType(int num) {return typemap[num];}

	virtual void updateRenderer(RenderParams* dParams, bool prevEnabled,  bool newWindow);
	virtual void setEditorDirty(RenderParams* p = 0);
		
	virtual void reinitTab(bool doOverride);
	virtual void refreshTab();
	//Required method to create the tab:
	static EventRouter* CreateTab(){
		TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
		return (EventRouter*)(new DvrEventRouter((QWidget*)tMgr));
	}
#ifdef Darwin
	void paintEvent(QPaintEvent*);
#endif

	
public slots:

	//respond to changes in TF (for undo/redo):
	//

	virtual void guiStartChangeMapFcn(QString s);
	virtual void guiEndChangeMapFcn();
	void guiSetOpacityScale(int val);
	void setBindButtons(bool canBind);

protected slots:
	void startColorChange();
	void endColorChange();
	void guiSetFidelity(int buttonID);
	void guiSetFidelityDefault();
	void guiFitTFToData();
	void guiChangeInstance(int);
	void guiNewInstance();
	void guiDeleteInstance();
	void guiCopyInstanceTo(int toViz);
	void guiSetType(int val);
	void guiSetComboVarNum(int val);
	void guiSetNumBits(int val);
	void guiSetLighting(bool val);
	void guiSetPreintegration(bool val);
	void guiSetNumRefinements(int num);
	void guiSetCompRatio(int num);
	void guiBindColorToOpac();
	void guiBindOpacToColor();
	void guiToggleColorInterpType(bool);

	void setDvrTabTextChanged(const QString& qs);
	void dvrReturnPressed();
	//Slots for dvr panel:

    void setClutDirty();

	void setDvrEnabled(bool on, int instance);
	void setDvrEditMode(bool);
	void setDvrNavigateMode(bool);
	
	void dvrLoadTF();
	void dvrLoadInstalledTF();
	void dvrSaveTF();
	void refreshHisto();

    // Benchmark slots
    void runBenchmarks();
    void benchmarkTimeout();
    void nextBenchmark();
    void benchmarkPreamble();
    void renderBenchmark();
    void temporalBenchmark();
    void tfeditBenchmark();


protected:
	//Drop wheel events (they cause confusion here)
	virtual void wheelEvent(QWheelEvent* e){}
	static const char* webHelpText[];
	static const char* webHelpURL[];
    int     benchmark;
    QTimer *benchmarkTimer;
	
	virtual void setDatarangeDirty(RenderParams* dParams);
	std::map<int, DvrParams::DvrType> typemap;
    std::map<DvrParams::DvrType, int> typemapi; // inverse typemap
	int copyCount[MAXVIZWINS+1];
};

};

#endif //DVREVENTROUTER_H 



