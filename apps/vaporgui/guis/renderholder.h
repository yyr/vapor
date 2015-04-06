#ifndef RENDERHOLDER_H
#define RENDERHOLDER_H


#include <qobject.h>
#include "qstackedwidget.h"
#include "qpushbutton.h"
#include "qtableview.h"
#include "params.h"

#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "renderselector.h"
#include "tabmanager.h"


using namespace VetsUtil;
QT_USE_NAMESPACE

namespace VAPoR {


class RenderHolder : public QWidget, public Ui_RenderSelector {

	Q_OBJECT

public: 
	
	RenderHolder(QWidget* parent);
    
	virtual ~RenderHolder() {}
	static RenderHolder* getInstance(){
		if (!theRenderHolder) theRenderHolder = new RenderHolder(0);
		return theRenderHolder;
	}
	//Add a widget (EventRouter) to the QStackedWidget.  Return its index in the stackedWidget
	static int addWidget(QWidget*, const char* name, string tag);
	
	static void rebuildCombo();
	
	static void setCurrentIndex(int indx){
		stackedWidget->setCurrentIndex(indx);
	}
	//Make the tableWidget match the current params
	void updateTableWidget();


private slots:
	void newRenderer();
	void deleteRenderer();
	void changeChecked(int i, int j);
	void itemTextChange(QTableWidgetItem*);
	void selectInstance();
	void copyInstanceTo(int viznum);

private:
	
	static RenderHolder* theRenderHolder;
	//remember the visualizer indices in the dupCombo
	static vector<int> vizDupNums;
	//Boolean indicates whether will respond to signals
	bool signalsOn;

	//Convert name to a unique name (among renderer names)
	static std::string uniqueName(std::string name);

	static QStackedWidget* stackedWidget;
   };

};

#endif //RENDERHOLDER_H 



