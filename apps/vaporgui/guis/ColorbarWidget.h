//-- ColorbarWidget.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// OpenGL-based colorbar with interactive color control points. 
// 
//----------------------------------------------------------------------------

#ifndef ColorbarWidget_H
#define ColorbarWidget_H

#include "GLWidget.h"
#include <vapor/MyBase.h>

#include <qobject.h>
#include <QColor>
#include <set>
#include <list>

class GLUquadric;

class MappingFrame;
namespace VAPoR {

class Params;
class VColormap;
class ColorMapBase;

class ColorbarWidget : public GLWidget
{
  Q_OBJECT

public:

  ColorbarWidget(MappingFrame *parent, ColorMapBase *colormap);
  virtual ~ColorbarWidget();

  void paintGL();
  void initializeGL();

  virtual void select(int handle, Qt::KeyboardModifiers);
  virtual void deselect();

  std::list<float> selectedPoints();

  bool controlPointSelected() { return selected(); }
  int  selectedControlPoint() { return _selected; }
  void deleteSelectedControlPoint();

  void move(float dx, float dy=0.0, float dz=0.0);
  void drag(float dx, float dy=0.0, float dz=0.0);

  float minValue() const  { return _minValue; }
  float maxValue() const  { return _maxValue; }

  void setColormap(ColorMapBase *colormap);
  ColorMapBase* colormap() { return _colormap; }

  void setDirty() { _updateTexture = true; }

 public slots:

  void newHsv(int h, int s, int v);

 signals:

  //
  // Signals that a color control point has be selected
  //
  void sendRgb(QRgb color);

  //
  // Signals that the widget has changed the mapping function.
  //
  void mapChanged();

 protected:

  void drawControlPoint(int index);
  void updateTexture();

  float width();

  float right();
  float left();
    
  float dataToWorld(float x);
  float worldToData(float x);

private:

  const int _NUM_BINS;

  MappingFrame *_parent;
  ColorMapBase     *_colormap;

  unsigned int   _texid;
  unsigned char *_texture;
  bool           _updateTexture;

  float _minValue;
  float _maxValue;

  std::set<int> _selectedCPs;
};
};

#endif // ColorbarWidget_H

