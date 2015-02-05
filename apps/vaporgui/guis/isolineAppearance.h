#ifndef ISOLINEAPPEARANCE_H
#define ISOLINEAPPEARANCE_H


#include <qobject.h>
#include "params.h"
#include "arrowparams.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "contourAppearance.h"
#include "tabmanager.h"


using namespace VetsUtil;
QT_USE_NAMESPACE

namespace VAPoR {


class IsolineAppearance : public QWidget, public Ui_ContourAppearance {

	Q_OBJECT

public: 
	
	IsolineAppearance(QWidget* parent): QWidget(parent), Ui_ContourAppearance(){ 
		setupUi(this);
	}
	virtual ~IsolineAppearance(){}

};

};

#endif //ISOLINEAPPEARANCE_H 



