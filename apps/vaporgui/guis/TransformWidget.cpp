//
//      $Id$
//
//	File:		TransformWidget.cpp
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Simple widget for inline editing of transform parameters
//
//

#include "TransformWidget.h"
#include <QtGui>
#include <limits>

using namespace VAPoR;
using namespace std;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------=
TransformWidget::TransformWidget(QWidget *parent) :
        QWidget(parent)
{
   QHBoxLayout *horizontalLayout = new QHBoxLayout(this);

   _label = new QLabel(this);

   _xSpin = new QDoubleSpinBox(this);
   _ySpin = new QDoubleSpinBox(this);
   _zSpin = new QDoubleSpinBox(this);
   _thetaSpin = new QDoubleSpinBox(this);

   _xSpin->setRange(-numeric_limits<double>::max(), numeric_limits<double>::max());
   _ySpin->setRange(-numeric_limits<double>::max(), numeric_limits<double>::max());
   _zSpin->setRange(-numeric_limits<double>::max(), numeric_limits<double>::max());
   _thetaSpin->setRange(-360, 360);

   
   horizontalLayout->addWidget(_label);
   horizontalLayout->addWidget(_xSpin);
   horizontalLayout->addWidget(_ySpin);
   horizontalLayout->addWidget(_zSpin);
   horizontalLayout->addWidget(_thetaSpin);
   horizontalLayout->setContentsMargins(0, 0, 0, 0);

   setBackgroundRole(QPalette::Window);
   setAutoFillBackground(true);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void TransformWidget::setLabel(QString text)
{
   if (text == "Rotate")
   {
      _thetaSpin->show();
   }
   else
   {
      _thetaSpin->hide();
   }

   _label->setText(text);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void TransformWidget::setX(double x)
{
   _xSpin->setValue(x);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void TransformWidget::setY(double y)
{
   _ySpin->setValue(y);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void TransformWidget::setZ(double z)
{
   _zSpin->setValue(z);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void TransformWidget::setTheta(double t)
{
   _thetaSpin->setValue(t);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double TransformWidget::x()
{
   return _xSpin->value();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double TransformWidget::y()
{
   return _ySpin->value();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double TransformWidget::z()
{
   return _zSpin->value();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double TransformWidget::theta()
{
   return _thetaSpin->value();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void TransformWidget::interpretText()
{
   _xSpin->interpretText();
   _ySpin->interpretText();
   _zSpin->interpretText();
   _thetaSpin->interpretText();
}
