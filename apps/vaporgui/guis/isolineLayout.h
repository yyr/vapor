#ifndef ISOLINELAYOUT_H
#define ISOLINELAYOUT_H


#include <qobject.h>
#include "params.h"
#include "arrowparams.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
using namespace VAPoR;
#include "contourLayout.h"
#include "tabmanager.h"
#include "boxsliderframe.h"


using namespace VetsUtil;

QT_USE_NAMESPACE

namespace VAPoR {


class IsolineLayout : public QWidget, public Ui_ContourLayout {

	Q_OBJECT

public: 
	
	IsolineLayout(QWidget* parent): QWidget(parent), Ui_ContourLayout(){
        setupUi(this);
	
}

	virtual ~IsolineLayout() {}

   };

};

#endif //ARROWLAYOUT_H 



