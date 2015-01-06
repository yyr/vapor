#ifndef ARROWAPPEARANCE_H
#define ARROWAPPEARANCE_H


#include <qobject.h>
#include "params.h"
#include "arrowparams.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "barbAppearance.h"
#include "tabmanager.h"


using namespace VetsUtil;
QT_USE_NAMESPACE

namespace VAPoR {


class ArrowAppearance : public QWidget, public Ui_barbAppearance {

	Q_OBJECT

public: 
	
	ArrowAppearance(QWidget* parent): QWidget(parent), Ui_barbAppearance(){
        setupUi(this);
	
}

	virtual ~ArrowAppearance() {}

   };

};

#endif //ARROWAPPEARANCE_H 



