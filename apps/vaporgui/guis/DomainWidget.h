//-- DomainWidget.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// OpenGL-based widget used to scale and move the a transfer function domain. 
// Also supports derived class: IsoSlider, with a limited subset of the functionality
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
protected:
  enum
  {
    LEFT,
    RIGHT,
    BAR,
	VERTLINE
  };

public:

  DomainWidget(MappingFrame *parent, float min=0.0, float max=1.0);
  virtual ~DomainWidget();

  virtual void paintGL();

  void move(float dx, float dy=0.0, float dz=0.0);
  virtual void drag(float dx, float dy=0.0, float dz=0.0);

  float minValue() const  { return _minValue; }
  float maxValue() const  { return _maxValue; }

  void setDomain(float minv, float maxv) {_minValue = minv; _maxValue = maxv;}

  virtual void setGeometry(float x0, float x1, float y0, float y1);

 signals:

  //
  // Signals that the widget is being moved/edited.  Currently not connected.
  //
  void changingDomain(float min, float max);

 protected:

  float width();

  float right();
  float left();
  float mid();
    
  float dataToWorld(float x);
  float worldToData(float x);
  float _minValue;
  float _maxValue;

  MappingFrame *_parent;

 private:

  //
  // Frame data
  //
  float         _handleRadius;
  GLUquadric   *_quadHandle;
};

 class IsoSlider : public DomainWidget {
 public:
	IsoSlider(MappingFrame *parent, float min=0.0, float max=1.0);
	virtual void drag(float dx, float dy=0.0, float dz=0.0);
	virtual void paintGL();
	void setIsoValue(float val){
		setDomain(worldToData(dataToWorld(val) - 0.01), worldToData(dataToWorld(val) + 0.01));
	}
 protected:
	 float _lineWidth;
 };
};

#endif // DomainWidget_H

