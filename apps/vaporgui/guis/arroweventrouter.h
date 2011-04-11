#ifndef ARROWEVENTROUTER_H
#define ARROWEVENTROUTER_H


#include <qobject.h>
#include "params.h"
#include "arrowparams.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "arrow.h"


using namespace VetsUtil;
QT_USE_NAMESPACE

namespace VAPoR {


class XmlNode;
class ParamNode;
class PanelCommand;
class Params;
class ArrowEventRouter : public QWidget, public Ui_Arrow, public EventRouter {

	Q_OBJECT

public: 
	
	ArrowEventRouter(QWidget* parent);
	virtual ~ArrowEventRouter();
	static EventRouter* CreateTab(){
		TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
		return (EventRouter*)(new ArrowEventRouter((QWidget*)tMgr));
	}
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1, bool reEnable = false);
	
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	
	virtual void guiSetEnabled(bool value, int instance);
	

	virtual void updateRenderer(RenderParams* dParams, bool prevEnabled,  bool newWindow);
	virtual void reinitTab(bool doOverride);

	virtual void captureMouseUp();
	virtual void captureMouseDown();
	//Following needed if tab has embedded opengl frames
	//virtual void refreshTab(){}


protected slots:

	void guiChangeInstance(int);
	void guiNewInstance();
	void guiDeleteInstance();
	
	void guiCopyInstanceTo(int toViz);
		
	void guiSetNumRefinements(int num);
	void guiSetCompRatio(int num);
	
	void setArrowTextChanged(const QString& qs);
	void arrowReturnPressed();
	void setArrowEnabled(bool on, int instance);
	void guiSetXVarNum(int);
	void guiSetYVarNum(int);
	void guiSetZVarNum(int);
	void guiToggleTerrainAlign(bool);
	void guiSelectColor();

	
protected:
	int copyCount[MAXVIZWINS+1];

   };

};

#endif //ARROWEVENTROUTER_H 


