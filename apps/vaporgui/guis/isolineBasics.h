#ifndef ISOLINEBASICS_H
#define ISOLINEBASICS_H


#include <qobject.h>
#include "params.h"
#include "arrowparams.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "contourBasics.h"
#include "tabmanager.h"


using namespace VetsUtil;
QT_USE_NAMESPACE

namespace VAPoR {


class IsolineBasics : public QWidget, public Ui_ContourBasics {

	Q_OBJECT

public: 
	
	IsolineBasics(QWidget* parent): QWidget(parent), Ui_ContourBasics(){ 
		setupUi(this);
	}
	virtual ~IsolineBasics(){}

};

};

#endif //ISOLINEBASICS_H 



