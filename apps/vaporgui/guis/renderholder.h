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
	//Add a widget (EventRouter) to the QStackedWidget.  Return its index in the stackedWidget
	int addWidget(QWidget*, const char* name, string tag);
	
	//Change current visualizer
	void changeViznum(int);
	void deleteViznum(int);
	void addViznum(int);
	
	void setCurrentIndex(int indx){
		stackedWidget->setCurrentIndex(indx);
	}

private slots:
	void newRenderer();
	void deleteRenderer();
	void changeChecked(int i, int j);
	void itemTextChange(QTableWidgetItem*);
	void selectInstance();
	void copyInstanceTo(int viznum);

private:
	//For each visualizer must remember full contents of the tableWidget.
	//It is reconstructed whenever the current visualizer is changed.
	//The following are mapped from visualizer index to TableWidget information
	//instanceIndex is the instance index of the particular renderer.
	//Not visible to user but required for params identification.
	std::map<int,std::vector<int> > instanceIndex;
	//Currentrow indicates which row is selected in the tableWidget for the particular visualizer.
	//Need to know in case the active visualizer changes
	std::map<int, int> currentRow;
	//instanceName is the name the user provides for the instance.  It need not be unique.
	std::map<int, std::vector<string> > instanceName;
	//renParams is the RenderParams* for the tableWidget entry
	std::map<int, std::vector<RenderParams*> > renParams;

	//Boolean indicates whether will respond to signals
	bool signalsOn;

	//Convert name to a unique name (among renderer names)
	std::string uniqueName(std::string name);

	QStackedWidget* stackedWidget;
   };

};

#endif //RENDERHOLDER_H 



