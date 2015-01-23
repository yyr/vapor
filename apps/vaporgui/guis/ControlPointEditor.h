//-- ControlPointEditor -------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// Simple GUI dialog used to modify transfer function control points (i.e.,
// color and opacity control points).
//
//----------------------------------------------------------------------------


#ifndef ControlPointEditor_h
#define ControlPointEditor_h

#include "ControlPointEditorBase.h"
#include <qdialog.h>

class MappingFrame;

namespace VAPoR {

class MapperFunction;
class OpacityMap;
class OpacityMapBase;
class VColormap;
class ColorMapBase;

class ControlPointEditor : public QDialog, public Ui_ControlPointEditorBase
{
  Q_OBJECT

 public:

  ControlPointEditor(MappingFrame* parent, OpacityMapBase *map, int cp);
  ControlPointEditor(MappingFrame* parent, ColorMapBase *map, int cp);

  ~ControlPointEditor();

  void update() { initWidgets(); }

 protected:

  QColor tempColor;
  void initWidgets();
  void initConnections();

  float dataValue();
  int   toIndex(float);
  float toData(int);

 protected slots:

  void dataValueChanged();
  void indexValueChanged();
  void pickColor();
  void okHandler();
  void cancelHandler();

 private:

  int _controlPoint;

  MapperFunction *_mapper;
  OpacityMapBase     *_omap;
  ColorMapBase       *_cmap;
};

}; // VAPoR namespace

#endif
