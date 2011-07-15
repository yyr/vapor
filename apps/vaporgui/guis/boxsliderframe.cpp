//************************************************************************
//																		*
//		     Copyright (C)  2011										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		boxsliderframe.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2011
//
//	Description:	Implements the TwoDFrame class.  This provides
//		a frame in which the twoD texture is drawn
//		Principally involved in drawing and responding to mouse events.
//
#include "boxsliderframe.h"
#include <QFrame>
#include <qwidget.h>
#include <vector>
#include <QString>
using namespace VAPoR;

BoxSliderFrame::BoxSliderFrame( QWidget * parent) : QFrame(parent), Ui_boxframe(){
		setupUi(this);
		connect(xCenterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(boxTextChanged(const QString&)));
		connect(yCenterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(boxTextChanged(const QString&)));
		connect(zCenterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(boxTextChanged(const QString&)));
		connect(xSizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(boxTextChanged(const QString&)));
		connect(ySizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(boxTextChanged(const QString&)));
		connect(zSizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(boxTextChanged(const QString&)));
		connect(xCenterEdit,SIGNAL(returnPressed()), this, SLOT(boxReturnPressed()));
		connect(yCenterEdit,SIGNAL(returnPressed()), this, SLOT(boxReturnPressed()));
		connect(zCenterEdit,SIGNAL(returnPressed()), this, SLOT(boxReturnPressed()));
		connect(xSizeEdit,SIGNAL(returnPressed()), this, SLOT(boxReturnPressed()));
		connect(ySizeEdit,SIGNAL(returnPressed()), this, SLOT(boxReturnPressed()));
		connect(zSizeEdit,SIGNAL(returnPressed()), this, SLOT(boxReturnPressed()));
		connect(xCenterSlider, SIGNAL(valueChanged(int)),this, SLOT(xSliderCtrChange(int)));
		connect(yCenterSlider, SIGNAL(valueChanged(int)),this, SLOT(ySliderCtrChange(int)));
		connect(zCenterSlider, SIGNAL(valueChanged(int)),this, SLOT(zSliderCtrChange(int)));
		connect(xSizeSlider, SIGNAL(valueChanged(int)),this, SLOT(xSliderSizeChange(int)));
		connect(ySizeSlider, SIGNAL(valueChanged(int)),this, SLOT(ySliderSizeChange(int)));
		connect(zSizeSlider, SIGNAL(valueChanged(int)),this, SLOT(zSliderSizeChange(int)));
}
BoxSliderFrame::~BoxSliderFrame() {
}


void BoxSliderFrame::getBoxExtents(double ext[6]){
	for (int i = 0; i<6; i++) ext[i] = boxExtents[i];
}
void BoxSliderFrame::setFullDomain(const double ext[6]){
	for (int i = 0; i<6; i++) domainExtents[i] = ext[i];
}
void BoxSliderFrame::setVoxelDims(const int dims[3]){
	for (int i = 0; i<6; i++) voxelDims[i] = dims[i];
}
void BoxSliderFrame::setBoxExtents(const std::vector<double>& exts){
	for (int i = 0; i<6; i++) boxExtents[i] = exts[i];
	double mid[3], sz[3];
	for (int i = 0; i<3; i++) {
		mid[i] = 0.5*(boxExtents[i]+boxExtents[i+3]);
		sz[i] = boxExtents[i+3]-boxExtents[i];
	}
	silenceSignals=true;
	updateGuiValues(mid,sz);
	silenceSignals=false;
}
void BoxSliderFrame::confirmText(){
	if (!textChangedFlag) return;
	textChangedFlag = false;
	silenceSignals = true;
	//Force the text to be valid
	double centers[3], sizes[3];
	centers[0] = xCenterEdit->text().toDouble();
	centers[1] = yCenterEdit->text().toDouble(); 
	centers[2] = zCenterEdit->text().toDouble();
	sizes[0] = xSizeEdit->text().toDouble();
	sizes[1] = ySizeEdit->text().toDouble();
	sizes[2] = zSizeEdit->text().toDouble();
	for (int i = 0; i<3; i++){ 
		if (sizes[i] > domainExtents[i+3]-domainExtents[i])
			sizes[i] = domainExtents[i+3]-domainExtents[i];
		if (sizes[i] < 0.0)sizes[i] = 0.0;
		if (centers[i] < domainExtents[i]) centers[i] = domainExtents[i];
		if (centers[i] > domainExtents[i+3]) centers[i] = domainExtents[i+3];
		if ((centers[i]+ 0.5*sizes[i])> domainExtents[i+3])
			centers[i] = domainExtents[i+3]-0.5*sizes[i];
		if ((centers[i]- 0.5*sizes[i])< domainExtents[i])
			centers[i] = domainExtents[i]+0.5*sizes[i];
		boxExtents[i] = centers[i]-0.5*sizes[i];
		boxExtents[i+3] = centers[i]+0.5*sizes[i];
	}
	updateGuiValues(centers,sizes);
	emit extentsChanged();
	silenceSignals = false;

}
// slots:
void BoxSliderFrame::boxTextChanged(const QString&){
	if (silenceSignals) return;
	textChangedFlag = true;
}

void BoxSliderFrame::boxReturnPressed(){
	confirmText();
}
void BoxSliderFrame::xSliderCtrChange(int pos){
	if (silenceSignals) return;
	silenceSignals = true;
	//Force new center to conform to current size
	double center = domainExtents[0]+pos*(domainExtents[3]-domainExtents[0])/256.;
	if ((center + 0.5*(boxExtents[3]-boxExtents[0])) > domainExtents[3])
		center = domainExtents[3]-0.5*(boxExtents[3]-boxExtents[0]);
	if ((center - 0.5*(boxExtents[3]-boxExtents[0])) < domainExtents[0])
		center = domainExtents[0]+0.5*(boxExtents[3]-boxExtents[0]);
	double size = boxExtents[3]-boxExtents[0];
	boxExtents[0] = center - 0.5*size;
	boxExtents[3] = center + 0.5*size;
	xCenterEdit->setText(QString::number(center));
	xCenterSlider->setValue((int)(0.5+256*(center-domainExtents[0])/(domainExtents[3]-domainExtents[0])));
	emit extentsChanged();
	silenceSignals = false;
}
void BoxSliderFrame::xSliderSizeChange(int pos){
	if (silenceSignals) return;
	silenceSignals = true;
	double newSize = pos*(domainExtents[3]-domainExtents[0])/256.;
	//force center to conform to new size.
	double center = 0.5*(boxExtents[3]+boxExtents[0]);
	if ((center + 0.5*(newSize)) > domainExtents[3])
		center = domainExtents[3]-0.5*(newSize);
	if ((center - 0.5*(newSize)) < domainExtents[0])
		center = domainExtents[0]+0.5*(newSize);
	boxExtents[0] = center - 0.5*newSize;
	boxExtents[3] = center + 0.5*newSize;
	xCenterSlider->setValue((int)(0.5+256*(center-domainExtents[0])/(domainExtents[3]-domainExtents[0])));
	xCenterEdit->setText(QString::number(center));
	xSizeEdit->setText(QString::number(newSize));
	emit extentsChanged();
	silenceSignals = false;
}
void BoxSliderFrame::ySliderCtrChange(int pos){
	if (silenceSignals) return;
	silenceSignals = true;
	//Force new center to conform to current size
	double center = domainExtents[1]+pos*(domainExtents[4]-domainExtents[1])/256.;
	if ((center + 0.5*(boxExtents[4]-boxExtents[1])) > domainExtents[4])
		center = domainExtents[4]-0.5*(boxExtents[4]-boxExtents[1]);
	if ((center - 0.5*(boxExtents[4]-boxExtents[1])) < domainExtents[1])
		center = domainExtents[1]+0.5*(boxExtents[4]-boxExtents[1]);
	double size = boxExtents[4]-boxExtents[1];
	boxExtents[1] = center - 0.5*size;
	boxExtents[4] = center + 0.5*size;
	yCenterEdit->setText(QString::number(center));
	yCenterSlider->setValue((int)(0.5+256*(center-domainExtents[1])/(domainExtents[4]-domainExtents[1])));
	emit extentsChanged();
	silenceSignals = false;
}
void BoxSliderFrame::ySliderSizeChange(int pos){
	if (silenceSignals) return;
	silenceSignals = true;
	double newSize = pos*(domainExtents[4]-domainExtents[1])/256.;
	//force center to conform to new size.
	double center = 0.5*(boxExtents[4]+boxExtents[1]);
	if ((center + 0.5*(newSize)) > domainExtents[4])
		center = domainExtents[4]-0.5*(newSize);
	if ((center - 0.5*(newSize)) < domainExtents[1])
		center = domainExtents[1]+0.5*(newSize);
	boxExtents[1] = center - 0.5*newSize;
	boxExtents[4] = center + 0.5*newSize;
	yCenterSlider->setValue((int)(0.5+256*(center-domainExtents[1])/(domainExtents[4]-domainExtents[1])));
	yCenterEdit->setText(QString::number(center));
	ySizeEdit->setText(QString::number(newSize));
	emit extentsChanged();
	silenceSignals = false;
}
void BoxSliderFrame::zSliderCtrChange(int pos){
	if (silenceSignals) return;
	silenceSignals = true;
	//Force new center to conform to current size
	double center = domainExtents[2]+pos*(domainExtents[5]-domainExtents[2])/256.;
	if ((center + 0.5*(boxExtents[5]-boxExtents[2])) > domainExtents[5])
		center = domainExtents[5]-0.5*(boxExtents[5]-boxExtents[2]);
	if ((center - 0.5*(boxExtents[5]-boxExtents[2])) < domainExtents[2])
		center = domainExtents[2]+0.5*(boxExtents[5]-boxExtents[2]);
	double size = boxExtents[5]-boxExtents[2];
	boxExtents[2] = center - 0.5*size;
	boxExtents[5] = center + 0.5*size;
	zCenterEdit->setText(QString::number(center));
	zCenterSlider->setValue((int)(0.5+256*(center-domainExtents[2])/(domainExtents[5]-domainExtents[2])));
	emit extentsChanged();
	silenceSignals = false;
}
void BoxSliderFrame::zSliderSizeChange(int pos){
	if (silenceSignals) return;
	silenceSignals = true;
	double newSize = pos*(domainExtents[5]-domainExtents[2])/256.;
	//force center to conform to new size.
	double center = 0.5*(boxExtents[5]+boxExtents[2]);
	if ((center + 0.5*(newSize)) > domainExtents[5])
		center = domainExtents[5]-0.5*(newSize);
	if ((center - 0.5*(newSize)) < domainExtents[2])
		center = domainExtents[2]+0.5*(newSize);
	boxExtents[2] = center - 0.5*newSize;
	boxExtents[5] = center + 0.5*newSize;
	zCenterSlider->setValue((int)(0.5+256*(center-domainExtents[2])/(domainExtents[5]-domainExtents[2])));
	zCenterEdit->setText(QString::number(center));
	zSizeEdit->setText(QString::number(newSize));
	emit extentsChanged();
	silenceSignals = false;
}
void BoxSliderFrame::updateGuiValues(const double centers[3], const double sizes[3]){
	xCenterEdit->setText(QString::number(centers[0]));
	yCenterEdit->setText(QString::number(centers[1]));
	zCenterEdit->setText(QString::number(centers[2]));
	xSizeEdit->setText(QString::number(sizes[0]));
	ySizeEdit->setText(QString::number(sizes[1]));
	zSizeEdit->setText(QString::number(sizes[2]));
	xCenterSlider->setValue((int)(0.5+256*(centers[0]-domainExtents[0])/(domainExtents[3]-domainExtents[0])));
	yCenterSlider->setValue((int)(0.5+256*(centers[1]-domainExtents[1])/(domainExtents[4]-domainExtents[1])));
	zCenterSlider->setValue((int)(0.5+256*(centers[2]-domainExtents[2])/(domainExtents[5]-domainExtents[2])));
	xSizeSlider->setValue((int)(0.5+256*sizes[0]/(domainExtents[3]-domainExtents[0])));
	ySizeSlider->setValue((int)(0.5+256*sizes[1]/(domainExtents[4]-domainExtents[1])));
	zSizeSlider->setValue((int)(0.5+256*sizes[2]/(domainExtents[5]-domainExtents[2])));
}