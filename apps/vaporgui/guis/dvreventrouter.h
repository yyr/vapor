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
#include "params.h"
#include "eventrouter.h"
#include "vapor/MyBase.h"
#include "tfeditor.h"
#include "dvr.h"

class QTimer;


using namespace VetsUtil;

namespace VAPoR {

class ViewpointParams;
class XmlNode;
class PanelCommand;
class Params;
class DvrEventRouter : public Dvr, public EventRouter {
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
	DvrEventRouter(QWidget* parent, const char* name);
	virtual ~DvrEventRouter();

	virtual void updateMapBounds(RenderParams* p);
	virtual void updateClut(RenderParams* p){
		VizWinMgr::getInstance()->setVizDirty(p,DvrClutBit,true);
	}
	
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1);
	
	virtual void confirmText(bool /*render*/);
	virtual void updateTab(Params* p);
	
	virtual void cleanParams(Params* p); 

	
	
	void fileLoadTF(DvrParams* dParams);
	void sessionLoadTF(DvrParams* dParams, QString* name);
	void fileSaveTF(DvrParams* dParams);
	//Following methods are set from gui, have undo/redo support:
	//
	
	virtual void guiSetEnabled(bool value);
    
	
	
	
	void guiSetOpacityScale(int val);
	void guiSetEditMode(bool val); //edit versus navigate mode
	

	//respond to changes in TF (for undo/redo):
	//
	virtual void guiStartChangeMapFcn(char* s);
	virtual void guiEndChangeMapFcn();
	
	
	void setBindButtons(DvrParams*);
	
	
	DvrParams::DvrType getType(int num) {return typemap[num];}

	TFEditor* getTFEditor(DvrParams* dParams){
		if (dParams->getTransFunc())
			return ((TFEditor*)dParams->getTransFunc()->getEditor());
		else return 0;
	}
	void updateRenderer(DvrParams* dParams, bool prevEnabled,  bool newWindow);

		
	virtual Histo* getHistogram(RenderParams*, bool);
	virtual void refreshHistogram(RenderParams*);

	//There are multiple notions of "dirty" here!
	virtual void setEditorDirty();
		
	virtual void reinitTab(bool doOverride);

	
protected slots:
	void guiChangeInstance(int);
	void guiNewInstance();
	void guiDeleteInstance();
	void guiCopyInstance();
	void guiSetType(int val);
	void guiSetComboVarNum(int val);
	void guiSetNumBits(int val);
	void guiSetLighting(bool val);
	void guiSetAligned();
	void guiSetNumRefinements(int num);
	void guiBindColorToOpac();
	void guiBindOpacToColor();

	void setDvrTabTextChanged(const QString& qs);
	void dvrReturnPressed();
	//Slots for dvr panel:
	
	
	void setDvrEnabled(int on);
	void setDvrEditMode(bool);
	void setDvrNavigateMode(bool);
	
	void dvrOpacityScale();
	
	void dvrLoadTF();
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

    int     benchmark;
    QTimer *benchmarkTimer;

	virtual void setDatarangeDirty(RenderParams* dParams);
	std::map<int, DvrParams::DvrType> typemap;
    std::map<DvrParams::DvrType, int> typemapi; // inverse typemap
	
};

};

#endif //DVREVENTROUTER_H 



