//-- OpacityWidget.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// OpenGL-based widget used to define an opacity map. The opacity map can
// be defined by a series of linear ramps controlled by control points, or
// Gaussian curve controlled by two sliders (i.e., mean and sigma^2). 
// 
//----------------------------------------------------------------------------

#ifndef OpacityWidget_H
#define OpacityWidget_H

#include "GLWidget.h"
#include <vapor/MyBase.h>

#include <set>

struct GLUquadric;
class MappingFrame;

namespace VAPoR {

class OpacityMap;

class OpacityWidget : public GLWidget
{
  Q_OBJECT

  enum
  {
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    TOP_RIGHT,
    TOP_LEFT,
    CURVE,
    BORDER,
    TOP_SLIDER,
    BOTTOM_SLIDER,
    LEFT_SLIDER,
    RIGHT_SLIDER,
    CONTROL_POINT,
  };

public:

  OpacityWidget(MappingFrame *parent, OpacityMap *omap);
  virtual ~OpacityWidget();

  void paintGL();
  void drawLines();
  virtual void drawCurve();

  bool controlPointSelected() { return _selected >= CONTROL_POINT; }
  int  selectedControlPoint() { return _selected - CONTROL_POINT; }
  void deleteSelectedControlPoint();

  void move(float dx, float dy=0.0, float dz=0.0);
  void drag(float dx, float dy=0.0, float dz=0.0);

  OpacityMap *opacityMap() { return _opacityMap; }

  virtual void select(int handle, Qt::ButtonState state);
  virtual void deselect();

  virtual bool enabled() const;   
  virtual void enable(bool flag); 

  std::list<float> selectedPoints();

  virtual void setGeometry(float x0, float x1, float y0, float y1);

 signals:

  //
  // Signals that the widget has changed the mapping function.
  //
  void mapChanged();

 protected:

  float width();

  float top()    { return _maxY; }
  float bottom() { return _minY; }
  float right();
  float left();

  bool  hasTopSlider();
  float topSlider();
  void  topSlider(float value);

  bool  hasBottomSlider();
  float bottomSlider();
  void  bottomSlider(float value);

  bool  hasRightSlider();
  float rightSlider();
  void  rightSlider(float value);

  bool  hasLeftSlider();
  float leftSlider();
  void  leftSlider(float value);

  float dataToWorld(float x);
  float worldToData(float x);

private:

  MappingFrame *_parent;
  OpacityMap   *_opacityMap;

  //
  // Frame data
  //
  float         _handleRadius;
  GLUquadric   *_handle;

  std::set<int> _selectedCPs;
};
};

#endif // OpacityWidget_H

