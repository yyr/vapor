#ifndef ARROWLAYOUT_H
#define ARROWLAYOUT_H


#include <qobject.h>
#include "params.h"
#include "arrowparams.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
using namespace VAPoR;
#include "barbLayout.h"
#include "tabmanager.h"
#include "boxsliderframe.h"


using namespace VetsUtil;

QT_USE_NAMESPACE

namespace VAPoR {


class ArrowLayout : public QWidget, public Ui_barbLayout {

	Q_OBJECT

public: 
	
	ArrowLayout(QWidget* parent): QWidget(parent), Ui_barbLayout(){
        setupUi(this);
	
}

	virtual ~ArrowLayout() {}

   };

};

#endif //ARROWLAYOUT_H 



