//--mappingframe.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// A QLGWidget that provides 1D transfer function interface. This frame can
// be used to map color and/or opacity to data values.
//
// Note: The interface can map either opacity, color, or both opacity and 
// color. The enabledness of these mappings needs to be set BEFORE 
// initializeGL is called. Since QtDesigner does not provide a way to pass
// in constructor arguments, these are being set as properties in designer.
// If designer is not being used call setOpacityMapping and setColorMapping
// immediately after construction. 
//
//----------------------------------------------------------------------------

#ifndef MappingFrame_H
#define MappingFrame_H

#include <GL/glew.h>
#ifdef Darwin
#include <gl.h>
#include <glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <qgl.h>

#include <QContextMenuEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include "glwindow.h"

#include <qpoint.h>
#include <list>
#include <map>
#include <set>


class QAction;
class QLabel;
class QMenu;



namespace VAPoR {
	class MapperFunction;
	class RenderParams;
	class Histo;
	class DomainWidget;
	class IsoSlider;
	class GLWidget;
	class OpacityWidget;
	class OpacityMap;
	class ColorbarWidget;
};

using namespace VAPoR;


class MappingFrame : public QGLWidget
{
  Q_OBJECT

  Q_PROPERTY(bool colorMapping   READ colorMapping   WRITE setColorMapping)
  Q_PROPERTY(bool opacityMapping READ opacityMapping WRITE setOpacityMapping)

  enum
  {
    BOOLEAN=0,
    LINEAR=1,
    LOG=2
  };

  enum
  {
    ENABLED=0,
    DISABLED=1
  };

  enum
  {
    OPACITY_WIDGETS,
    DOMAIN_WIDGET,
    COLORBAR_WIDGET,
	ISO_WIDGET
  };

public:

  MappingFrame(QWidget* parent, const char* name = 0);
  virtual ~MappingFrame();

  void setMapperFunction(MapperFunction *mapper);
  MapperFunction* mapperFunction() { return _mapper; }

  void setColorMapping(bool flag);
  bool colorMapping() const { return _colorMappingEnabled; }

  void setOpacityMapping(bool flag);
  bool opacityMapping() const { return _opacityMappingEnabled; }

  void setIsoSlider(bool flag) {_isoSliderEnabled = flag;}
  bool isoSliderEnabled() const { return _isoSliderEnabled; }

  void setVariableName(std::string name);

  void setIsoValue(float val){_isoVal = val;}

  void updateParams();

  QString tipText(const QPoint &pos);

  float minDataValue() { return _minValue; }
  float maxDataValue() { return _maxValue; }
	
  int   histoValue(const QPoint &pos);

  float xVariable(const QPoint &pos);
  float yVariable(const QPoint &pos);

  bool  canBind();

public slots:

  void setHistogramScale(QAction*);
  void setCompositionType(QAction*);
  void setWidgetEnabled(QAction*);
  void setEditMode(bool flag);
  
  void fitToView();
  void updateMap();

  void newHsv(int h, int s, int v);

  void bindColorToOpacity();
  void bindOpacityToColor();
  void updateGL();
  void update(){
	  if (!GLWindow::isRendering()) QGLWidget::update();
  }

signals:

  //
  // Signals that the mapping function has changed, update the visualizer
  // on this signal if you want transfer function edits visualized in real-time
  //
  void mappingChanged();

  //
  // Signals that a color control point has been selected
  //
  void sendRgb(QRgb color);

  //
  // Signals that one color control point and one opacity control point have
  // been selected.
  //
  void canBindControlPoints(bool);

  //
  // Signals for undo/redo
  //
  void startChange(QString description);
  void endChange();

protected:

  void initWidgets();
  void initConnections();

  OpacityWidget* createOpacityWidget(OpacityMap *map);
  void deleteOpacityWidgets();

  void initializeGL();
  void paintGL();
  void resizeGL( int w, int h );
  //Virtual, Reimplemented here:
  void paintEvent(QPaintEvent* event);

  void drawOpacityCurve();
  void drawOpacityWidgets();
  void drawDomainSlider();
  void drawIsoSlider();
  void drawColorbar();

  void updateTexture();

  void updateAxisLabels();
  void addAxisLabel(int x, int y, const QString &text);

  void select(int x, int y, Qt::KeyboardModifiers);
  void select(int hits, GLuint *selectionBuffer, Qt::KeyboardModifiers);

  void deselectWidgets();

  int  totalFixedHeight();
  void resize();

  virtual void mousePressEvent(QMouseEvent *event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent *event);
  virtual void mouseDoubleClickEvent(QMouseEvent *event);
  virtual void contextMenuEvent(QContextMenuEvent *e);

  float xDataToWorld(float x);
  float xWorldToData(float x);
  float xViewToWorld(float x);
  float xWorldToView(float x);

  float yDataToWorld(float y);
  float yWorldToData(float y);
  float yViewToWorld(float y);
  float yWorldToView(float y);

  virtual float getMinEditBound();
  virtual float getMaxEditBound();
  virtual void  setMinEditBound(float v);
  virtual void  setMaxEditBound(float v);

  virtual float getMinDomainBound();
  virtual float getMaxDomainBound();

  virtual float getOpacity(float val);
  virtual RenderParams* getParams();
  virtual Histo* getHistogram();

protected slots:

  void addOpacityWidget(QAction*);
  void deleteOpacityWidget();

  void addColorControlPoint();

  void addOpacityControlPoint();
  void editControlPoint();
  void deleteControlPoint();

  void setDomain();
  void setIsoSlider();

private:

  const int _NUM_BINS;

  MapperFunction *_mapper;
  Histo          *_histogram;

  bool            _opacityMappingEnabled;
  bool            _colorMappingEnabled;
  bool			  _isoSliderEnabled;

  std::string     _variableName;

  std::map<int, OpacityWidget*> _opacityWidgets;
  DomainWidget                 *_domainSlider;
  IsoSlider					   *_isoSlider;
  ColorbarWidget               *_colorbarWidget;
  GLWidget                     *_lastSelected;
  std::set<GLWidget*>           _selectedWidgets;

  unsigned int   _texid;
  unsigned char *_texture;
  bool           _updateTexture;
  int            _histogramScale;

    
  QPoint      _contextPoint;
  QMenu *_contextMenu;
  QMenu *_addOpacityWidgetSubMenu;
  QMenu *_histogramScalingSubMenu;
  QMenu *_compTypeSubMenu;
  QMenu *_widgetEnabledSubMenu;
  QAction    *_editOpacityWidgetAction;
  QAction    *_deleteOpacityWidgetAction;
  QAction    *_addColorControlPointAction;
  QAction    *_addOpacityControlPointAction;
  QAction    *_editControlPointAction;
  QAction    *_deleteControlPointAction;

  float _lastx;
  float _lasty;

  bool   _editMode;
  QPoint _clickedPos;

  float  _minValueStart;
  float  _maxValueStart;
  float  _isoVal;

  Qt::MouseButtons _button;

  float _minX;
  float _maxX;
  float _minY;
  float _maxY;

  float _minValue;
  float _maxValue;

  const int _colorbarHeight;
  const int _domainBarHeight;
  const int _domainLabelHeight;
  const int _domainHeight;
  const int _axisRegionHeight;
  const int _opacityGap;
  const int _bottomGap;

  QStringList _axisTexts;
  QList<QPoint*> _axisTextPos;
  
};

#endif // MappingFrame_H
