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
#include "params.h"
#include "ParamsIso.h"
#include "eventrouter.h"
#include "vapor/MyBase.h"
#include "isotab.h"



using namespace VetsUtil;

namespace VAPoR {

class ViewpointParams;
class XmlNode;
class PanelCommand;
class Params;
class IsoEventRouter : public IsoTab, public EventRouter {
	Q_OBJECT

public: 
	
	
	IsoEventRouter(QWidget* parent, const char* name);
	virtual ~IsoEventRouter();

	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1);
	
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	
	virtual void cleanParams(Params* p); 

	//gui... methods are set from gui, have undo/redo support:
	//
	
	virtual void guiSetEnabled(bool value, int instance);




	virtual void updateRenderer(RenderParams* dParams, bool prevEnabled,  bool newWindow);
	virtual Histo* getHistogram(RenderParams*, bool);
	virtual void refreshHistogram(RenderParams*);
		
	virtual void reinitTab(bool doOverride);
	void guiSetConstantColor(QColor& c);
	
protected slots:
	void guiChangeInstance(int);
	void guiNewInstance();
	void guiDeleteInstance();
	void guiCopyInstanceTo(int toViz);
	

	void guiSetComboVarNum(int val);
	
	void guiSetLighting(bool val);
	void guiSetNumRefinements(int num);

	void setIsoTabTextChanged(const QString& qs);
	void isoReturnPressed();
	void setIsoEnabled(bool on, int instance);
	void refreshHisto();
	void setIsoEditMode(bool);
	void setIsoNavigateMode(bool);
	void setConstantColor();
	void passThruPoint();
	


protected:

	void updateHistoBounds(RenderParams*p);
	int copyCount[MAXVIZWINS+1];
	bool editMode;
};

};

#endif //ISOEVENTROUTER_H 



