//-- DomainWidget.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// OpenGL-based widget used to scale and move the a transfer function domain. 
//
//----------------------------------------------------------------------------

#ifndef DomainWidget_H
#define DomainWidget_H

#include "GLWidget.h"
#include <vapor/MyBase.h>

#include <qobject.h>

struct GLUquadric;

namespace VAPoR {

class Params;
class MappingFrame;

 class DomainWidget : public GLWidget
{
  Q_OBJECT

  enum
  {
    LEFT,
    RIGHT,
    BAR
  };

public:

  DomainWidget(MappingFrame *parent, float min=0.0, float max=1.0);
  virtual ~DomainWidget();

  void paintGL();

  void move(float dx, float dy=0.0, float dz=0.0);
  void drag(float dx, float dy=0.0, float dz=0.0);

  float minValue() const  { return _minValue; }
  float maxValue() const  { return _maxValue; }

  void setDomain(float minv, float maxv) {_minValue = minv; _maxValue = maxv;}

  virtual void setGeometry(float x0, float x1, float y0, float y1);

 signals:

  //
  // Signals that the widget is being moved/edited
  //
  void changingDomain(float min, float max);

 protected:

  float width();

  float right();
  float left();
    
  float dataToWorld(float x);
  float worldToData(float x);

private:

  MappingFrame *_parent;

  float _minValue;
  float _maxValue;

  //
  // Frame data
  //
  float         _handleRadius;
  GLUquadric   *_quadHandle;
};
};

#endif // DomainWidget_H

