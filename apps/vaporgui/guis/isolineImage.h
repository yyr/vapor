#ifndef ISOLINEIMAGE_H
#define ISOLINEIMAGE_H


#include <qobject.h>
#include "contourImage.h"

using namespace VAPoR;




using namespace VetsUtil;

QT_USE_NAMESPACE

namespace VAPoR {


class IsolineImage : public QWidget, public Ui_ContourImage {

	Q_OBJECT

public: 
	
	IsolineImage(QWidget* parent): QWidget(parent), Ui_ContourImage(){
        setupUi(this);
	
}

	virtual ~IsolineImage() {}

   };

};

#endif //ISOLINEIMAGE_H 



