//
//      $Id$
//
//	File:		TransformWidget.h
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Simple widget for inline editing of transform parameters
//
//

#ifndef TRANSFORMWIDGET_H
#define TRANSFORMWIDGET_H

#include <QWidget>
#include <QString>
#include <QLabel>
#include <QDoubleSpinBox>

QT_USE_NAMESPACE

namespace VAPoR 
{

class TransformWidget : public QWidget
{
   Q_OBJECT

public:

   TransformWidget(QWidget *parent=NULL);

public slots:

   void setLabel(QString);
   void setX(double x);
   void setY(double y);
   void setZ(double z);
   void setTheta(double t);

   QString label() { return _label->text(); }
   double x();
   double y();
   double z();
   double theta();

   void interpretText();

protected:

   QLabel         *_label;
   QDoubleSpinBox *_xSpin;
   QDoubleSpinBox *_ySpin;
   QDoubleSpinBox *_zSpin;
   QDoubleSpinBox *_thetaSpin;
};

};

#endif
