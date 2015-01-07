#ifndef ARROWBASIC_H
#define ARROWBASIC_H


#include <qobject.h>
#include "params.h"
#include "arrowparams.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "barbBasic.h"

using namespace VetsUtil;
QT_USE_NAMESPACE

namespace VAPoR {


class ArrowBasic : public QWidget, public Ui_BarbBasic {

	Q_OBJECT

public: 
	
	ArrowBasic(QWidget* parent): QWidget(parent), Ui_BarbBasic(){ 
		setupUi(this);
	}
	virtual ~ArrowBasic(){}

};

};

#endif //ARROWBASIC_H 



