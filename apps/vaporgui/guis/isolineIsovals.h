#ifndef ISOLINEISOVALS_H
#define ISOLINEISOVALS_H


#include <qobject.h>
#include "contourIsovalues.h"
#include <vapor/MyBase.h>
using namespace VAPoR;


using namespace VetsUtil;

QT_USE_NAMESPACE

namespace VAPoR {


class IsolineIsovals : public QWidget, public Ui_ContourIsovalues {

	Q_OBJECT

public: 
	
	IsolineIsovals(QWidget* parent): QWidget(parent), Ui_ContourIsovalues(){
        setupUi(this);
	
}

	virtual ~IsolineIsovals() {}

   };

};

#endif //ISOLINEISOVALS_H 



