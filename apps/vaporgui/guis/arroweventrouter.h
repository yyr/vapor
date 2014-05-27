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
	
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	
	virtual void guiSetEnabled(bool value, int instance, bool undoredo = true);
	virtual void reinitTab(bool doOverride);

	virtual void captureMouseUp();
	virtual void captureMouseDown(int button);

protected slots:

	void changeInstance(int);
	void newInstance();
	void deleteInstance();
	
	void copyInstanceTo(int toViz);
		
	void setNumRefinements(int num);
	void setCompRatio(int num);
	
	void setArrowTextChanged(const QString& qs);
	void arrowReturnPressed();
	void setArrowEnabled(bool on, int instance);
	void setXVarNum(int);
	void setYVarNum(int);
	void setZVarNum(int);
	void setHeightVarNum(int);
	void moveScaleSlider(int);
	void releaseScaleSlider();
	void toggleTerrainAlign(bool);
	void selectColor();
	void changeExtents();
	void showHideLayout();
	void showHideAppearance();
	void fitToData();
	void setVariableDims(int);
	void alignToData(bool);


	
protected:
	void populateVariableCombos(bool is3d);
	vector<int> copyCount;
	bool showLayout;
	bool showAppearance;

   };

};

#endif //ARROWEVENTROUTER_H 



