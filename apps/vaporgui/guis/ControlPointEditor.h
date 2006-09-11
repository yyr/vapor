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

namespace VAPoR {

class MappingFrame;
class MapperFunction;
class OpacityMap;
class Colormap;

class ControlPointEditor : public ControlPointEditorBase
{
  Q_OBJECT

 public:

  ControlPointEditor(MappingFrame* parent, OpacityMap *map, int cp);
  ControlPointEditor(MappingFrame* parent, Colormap *map, int cp);

  ~ControlPointEditor();

  void update() { initWidgets(); }

 protected:

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
  OpacityMap     *_omap;
  Colormap       *_cmap;
};

}; // VAPoR namespace

#endif
