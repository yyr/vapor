//--MappingFrame.cpp -------------------------------------------------------
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

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <math.h>
#include <qcursor.h>
#include <qpopupmenu.h>
#include <qaction.h>
#include <qcursor.h>
#include <qlabel.h>

#include "params.h"
#include "ParamsIso.h"
#include "vizwinmgr.h"
#include "mapperfunction.h"
#include "OpacityWidget.h"
#include "OpacityMap.h"
#include "DomainWidget.h"
#include "ColorbarWidget.h"
#include "ControlPointEditor.h"
#include "TFLocationTip.h"
#include "histo.h"
#include "eventrouter.h"
#include "session.h"


#include "assert.h"
#include "glutil.h"

#include "mappingframe.h"

#ifndef MAX
#define MAX(a,b)        ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)        ((a) < (b) ? (a) : (b))
#endif

using namespace VAPoR;
using namespace std;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
MappingFrame::MappingFrame(QWidget* parent, const char* name)
  : QGLWidget(parent, name),
    _NUM_BINS(256),
    _mapper(NULL),
    _histogram(NULL),
    _opacityMappingEnabled(false),
    _colorMappingEnabled(false),
	_isoSliderEnabled(false),
    _variableName(""),
    _domainSlider(new DomainWidget(this)),
	_isoSlider(new IsoSlider(this)),
    _colorbarWidget(new ColorbarWidget(this, NULL)),
    _lastSelected(NULL),
    _texid(0),
    _texture(NULL),
    _updateTexture(true),
    _histogramScale(LINEAR),
    _variableLabel(NULL),
    _contextMenu(NULL),
    _addOpacityWidgetSubMenu(NULL),
    _histogramScalingSubMenu(NULL),
    _compTypeSubMenu(NULL),
    _widgetEnabledSubMenu(NULL),
    _deleteOpacityWidgetAction(NULL),
    _addColorControlPointAction(NULL),
    _addOpacityControlPointAction(NULL),
    _deleteControlPointAction(NULL),
    _lastx(0),
    _lasty(0),
    _editMode(true),
    _clickedPos(0,0),
    _minValueStart(0.0),
    _maxValueStart(1.0),
	_isoVal(0.0),
    _button(Qt::LeftButton),
    _minX(-0.035),
    _maxX(1.035),
    _minY(-0.35),
    _maxY(1.3),
    _minValue(0.0),
    _maxValue(1.0),
    _colorbarHeight(16),
    _domainBarHeight(16),
    _domainLabelHeight(10),
    _domainHeight(_domainBarHeight + _domainLabelHeight + 3),
    _axisRegionHeight(20),
    _opacityGap(4),
    _bottomGap(10),
    _tooltip(NULL)
{
  initWidgets();
  initConnections();
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
MappingFrame::~MappingFrame()
{
  makeCurrent();

  //
  // Clean up GLWidgets
  //
  deleteOpacityWidgets();

  delete _domainSlider;
  _domainSlider = NULL;

  delete _colorbarWidget;
  _colorbarWidget = NULL;

  //
  // Clean up texture information
  //
  if (_texture)
  {
    glDeleteTextures(1, &_texid);

    delete [] _texture;
    _texture = NULL;
  }
  
}

//----------------------------------------------------------------------------
// Set the underlying mapper function that this frame represents
//----------------------------------------------------------------------------
void MappingFrame::setMapperFunction(MapperFunction *mapper)
{
  deleteOpacityWidgets();
  
  _mapper = mapper;
  
  if (_mapper && _opacityMappingEnabled)
  {
    //
    // Create a new opacity widget for each opacity map in the mapper function
    //
    for(int i=0; i<_mapper->getNumOpacityMaps(); i++)
    {
      OpacityWidget *widget = createOpacityWidget(_mapper->getOpacityMap(i));
      
      connect((QObject*)widget, SIGNAL(startChange(QString)),
              this, SIGNAL(startChange(QString)));

      connect((QObject*)widget, SIGNAL(endChange()),
              this, SIGNAL(endChange()));

      connect((QObject*)widget, SIGNAL(mapChanged()),
              this, SLOT(updateMap()));
    }

    MapperFunction::CompositionType type = _mapper->getOpacityComposition();

    _compTypeSubMenu->setItemChecked(MapperFunction::ADDITION, 
                                     type==MapperFunction::ADDITION);
    _compTypeSubMenu->setItemChecked(MapperFunction::MULTIPLICATION, 
                                     type==MapperFunction::MULTIPLICATION);
  }

  if (_colorMappingEnabled)
  {
    _colorbarWidget->setColormap(_mapper ? _mapper->getColormap() : NULL);
  }
  
  updateParams();
}


//----------------------------------------------------------------------------
// Opacity mapping property. This property controls the enabledness of the
// opacity mapping capabilities (default = true). 
//
// Note: This function should be called before initializeGL() is called. 
// Ideally, it would be a flag passed in the construction, but that would 
// preclude using it as custom widget in designer. Therefore, set this property
// in designer.
//----------------------------------------------------------------------------
void MappingFrame::setOpacityMapping(bool flag)
{
  if (flag == _opacityMappingEnabled)
  {
    return;
  }

  _opacityMappingEnabled = flag;

  if (!_opacityMappingEnabled)
  {
    deleteOpacityWidgets();
  }
  else
  {
    // Error condition. Can't enable opacity mapping after it has been
    // disabled. 
    //assert(0);
  }
}

//----------------------------------------------------------------------------
// Opacity mapping property. This property controls the enabledness of the
// opacity mapping capabilities (default = true). 
//
// Note: This function should be called before initializeGL() is called. 
// Ideally, it would be a flag passed in the construction, but that would 
// preclude using it as custom widget in designer. Therefore, set this property
// in designer. 
//----------------------------------------------------------------------------
void MappingFrame::setColorMapping(bool flag)
{
  if (flag == _colorMappingEnabled)
  {
    return;
  }

  _colorMappingEnabled = flag;

  if (!_colorMappingEnabled)
  {
    delete _colorbarWidget;
    _colorbarWidget = NULL;
  }
  else
  {
    // Error condition. Can't enable opacity mapping after it has been
    // disabled. 
    //assert(0);
  }    
}

//----------------------------------------------------------------------------
// Set the variable name
//----------------------------------------------------------------------------
void MappingFrame::setVariableName(std::string name)
{
  _variableName = name;
  if (_variableName.size()> 40)
	  _variableName.resize(40);

  if (name != "")
  {
    _variableLabel->setText(_variableName.c_str());
    _variableLabel->adjustSize();
    _variableLabel->show();
  }
}

//----------------------------------------------------------------------------
// Synchronize the frame with the underlying params
//----------------------------------------------------------------------------
void MappingFrame::updateParams()
{
  deselectWidgets();

  RenderParams *params = getParams();

  if (params)
  {
	int viznum = params->getVizNum();

	if (viznum < 0) 
    {
      return;
	}

    _histogram = getHistogram();
    _minValue = getMinEditBound();
    _maxValue = getMaxEditBound();

	if (_isoSliderEnabled) 
    {
	   _isoVal = ((ParamsIso*)params)->GetIsoValue();
	   _isoSlider->setIsoValue(xDataToWorld(_isoVal));
	}

    _domainSlider->setDomain(xDataToWorld(getMinDomainBound()), 
                             xDataToWorld(getMaxDomainBound()));

    _updateTexture = true;
	updateGL();
  }
   
  
}

//----------------------------------------------------------------------------
// Return a tool tip
//----------------------------------------------------------------------------
QString MappingFrame::tipText(const QPoint &pos)
{
  QString text;

  if (!isEnabled())
  {
    return "Disabled";
  }

  int histo = histoValue(pos);
      
  float variable = xVariable(pos);
  float yopacity = yVariable(pos);
  float opacity  = getOpacity(variable);
      
  float hf,sf,vf;
  int hue, sat, val;
  
  if (_mapper)
  {
    _mapper->hsvValue(variable, &hf, &sf, &vf);
  }
      
  hue = (int)(hf*359.99f);
  sat = (int)(sf*255.99f);
  val = (int)(vf*255.99f);
  
  text  = _variableName.c_str();
  text += QString(": %1;\n").arg(variable, 0, 'g', 4);
  
  if (_opacityMappingEnabled)
  {
    text += QString("Opacity: %1; ").arg(opacity, 0, 'g', 4);
    text += QString("Y-Coord: %1;\n").arg(yopacity, 0, 'g', 4);
  }
  
  if (_colorMappingEnabled)
  {
    text += QString("Color(HSV): %1 %2 %3\n")
      .arg(hue, 3)
      .arg(sat, 3)
      .arg(val, 3);
  }
  
  if (histo >= 0)
  {
    text += QString("Histogram: %1").arg(histo);
  }

  return text;
}

//----------------------------------------------------------------------------
// Return the index value of the histogram at the window position.
//----------------------------------------------------------------------------
int MappingFrame::histoValue(const QPoint &p)
{
  if (!_histogram)
  {
    return -1;
  }
  
  QPoint pos = mapFromParent(p);
 
  float x = xWorldToData(xViewToWorld(pos.x()));

  float ind = (x - _histogram->getMinData())
    / (_histogram->getMaxData() - _histogram->getMinData());

  if (ind < 0.f || ind >= 1.f)
  {
    return 0;
  }

  int index = (int)(ind*255.999f);

  return _histogram->getBinSize(index); 
}

//----------------------------------------------------------------------------
// Map the window position to x
//----------------------------------------------------------------------------
float MappingFrame::xVariable(const QPoint &pos)
{
  return xWorldToData(xViewToWorld(pos.x()));
}

//----------------------------------------------------------------------------
// Map the window position to y
//----------------------------------------------------------------------------
float MappingFrame::yVariable(const QPoint &pos)
{
  return yWorldToData(yViewToWorld(height() - pos.y()));
}

//----------------------------------------------------------------------------
// Set the scaling type of the histogram.
//----------------------------------------------------------------------------
void MappingFrame::setHistogramScale(int scale)
{
  _histogramScale = scale;

  _updateTexture = true;

  _histogramScalingSubMenu->setItemChecked(BOOLEAN, scale==BOOLEAN);
  _histogramScalingSubMenu->setItemChecked(LINEAR, scale==LINEAR);
  _histogramScalingSubMenu->setItemChecked(LOG, scale==LOG);

  updateGL();
}

//----------------------------------------------------------------------------
// Set the composition type of the transfer function
//----------------------------------------------------------------------------
void MappingFrame::setCompositionType(int type)
{
  emit startChange("Opacity composition type changed");

  _compTypeSubMenu->setItemChecked(MapperFunction::ADDITION, 
                                   type==MapperFunction::ADDITION);
  _compTypeSubMenu->setItemChecked(MapperFunction::MULTIPLICATION, 
                                   type==MapperFunction::MULTIPLICATION);

  _mapper->setOpacityComposition((MapperFunction::CompositionType)type);

  emit endChange();

  updateGL();
}

//----------------------------------------------------------------------------
// Enable/disable TF widget
//----------------------------------------------------------------------------
void MappingFrame::setWidgetEnabled(int enabled)
{
  OpacityWidget *opacWidget = dynamic_cast<OpacityWidget*>(_lastSelected);

  if (opacWidget)
  {
    if (enabled)
    {
      emit startChange("Opacity widget enabled");
    }
    else
    {
      emit startChange("Opacity widget disabled");
    }

    _widgetEnabledSubMenu->setItemChecked(ENABLED, enabled==ENABLED);
    _widgetEnabledSubMenu->setItemChecked(DISABLED, enabled==DISABLED);

    opacWidget->enable(enabled==ENABLED);

    emit endChange();

    updateGL();
  }
}


//----------------------------------------------------------------------------
// Enable/disable the edit mode. When edit mode is disabled, mouse clicks
// will scale and pan the mapping space. When edit mode is enabled, mouse 
// clicks operate on the GLWidgets. 
//----------------------------------------------------------------------------
void MappingFrame::setEditMode(bool flag)
{
  _editMode = flag;

  if (_editMode)
  {
    setCursor(QCursor(Qt::ArrowCursor));
  }
  else
  {
    setCursor(QCursor(Qt::SizeAllCursor));
  }
}
  
//----------------------------------------------------------------------------
// Fit the mapping space to the current domain.
//----------------------------------------------------------------------------
void MappingFrame::fitToView()
{
  emit startChange("Mapping window fit-to-view");
  
  _minValue = getMinDomainBound();
  _maxValue = getMaxDomainBound();
  
  setMinEditBound(_minValue);
  setMaxEditBound(_maxValue);
  
  _domainSlider->setDomain(xDataToWorld(_minValue), xDataToWorld(_maxValue));
  
  emit endChange();
  if(_colorbarWidget) _colorbarWidget->setDirty();
  updateGL();
}

//----------------------------------------------------------------------------
// Force a redraw in the vizualization window.
//----------------------------------------------------------------------------
void MappingFrame::updateMap()
{
  emit mappingChanged();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void MappingFrame::newHsv(int h, int s, int v)
{
  if (_colorbarWidget)
  {
    _colorbarWidget->newHsv(h, s, v);

    emit mappingChanged();

    updateGL();
  }
}

//----------------------------------------------------------------------------
// Returns true if the selected color control points and opacity control 
// points can be bound. False otherwise. 
//
// Control points can be bound iff exactly one color control and exactly one 
// opacity control point are selected.
//----------------------------------------------------------------------------
bool MappingFrame::canBind()
{
  if (_selectedWidgets.size() == 2 && 
      _selectedWidgets.find(_colorbarWidget) != _selectedWidgets.end() &&
      _colorbarWidget->selectedPoints().size() == 1)
  {
    set<GLWidget*>::iterator iter;

    for (iter = _selectedWidgets.begin(); iter!=_selectedWidgets.end(); iter++)
    {
      OpacityWidget *owidget = dynamic_cast<OpacityWidget*>(*iter);

      if (owidget)
      {
        return (owidget->selectedPoints().size() == 1);
      }
    }
  }

  return false;
}

//----------------------------------------------------------------------------
// Set the selected color control point to the value of the selected opacity
// control point. 
//----------------------------------------------------------------------------
void MappingFrame::bindColorToOpacity()
{
  map<int, OpacityWidget*>::iterator iter;

  for (iter = _opacityWidgets.begin(); iter != _opacityWidgets.end(); iter++)
  {
    OpacityWidget *owidget = (*iter).second;

    if (owidget->selected())
    {
      int oIndex = owidget->selectedControlPoint();
      OpacityMap *omap = owidget->opacityMap();

      int cIndex = _colorbarWidget->selectedControlPoint();
      Colormap   *cmap = _colorbarWidget->colormap();
      
      cmap->controlPointValue(cIndex, omap->controlPointValue(oIndex));

      updateGL();

      return;
    }
  }  
}

//----------------------------------------------------------------------------
// Set the selected opacity control point to the data value of the selected
// color control point. 
//----------------------------------------------------------------------------
void MappingFrame::bindOpacityToColor()
{
  map<int, OpacityWidget*>::iterator iter;

  for (iter = _opacityWidgets.begin(); iter != _opacityWidgets.end(); iter++)
  {
    OpacityWidget *owidget = (*iter).second;

    if (owidget->selected())
    {
      int oIndex = owidget->selectedControlPoint();
      OpacityMap *omap = owidget->opacityMap();

      int cIndex = _colorbarWidget->selectedControlPoint();
      Colormap   *cmap = _colorbarWidget->colormap();
      
      omap->controlPointValue(oIndex, cmap->controlPointValue(cIndex));

      updateGL();

      return;
    }
  }  
}

//----------------------------------------------------------------------------
// Initialize the frame's contents
//----------------------------------------------------------------------------
void MappingFrame::initWidgets()
{
  //
  // Create the 2D histogram texture
  //
  _texture = new unsigned char[_NUM_BINS*_NUM_BINS];

  //
  // Create variable label
  //
  _variableLabel = new QLabel(this);
  _variableLabel->setAlignment(Qt::AlignHCenter);
  _variableLabel->hide();
  _variableLabel->setPaletteForegroundColor(Qt::red);
  _variableLabel->setPaletteBackgroundColor(Qt::black);

  //
  // Create the context sensitive menu
  //
  _contextMenu = new QPopupMenu(this);

  _addOpacityWidgetSubMenu = new QPopupMenu(_contextMenu);
  _addOpacityWidgetSubMenu->insertItem("Control Points", 
                                       OpacityMap::CONTROL_POINT);
  _addOpacityWidgetSubMenu->insertItem("Gaussian", OpacityMap::GAUSSIAN);
  _addOpacityWidgetSubMenu->insertItem("Inverted Gaussian", 
                                       OpacityMap::INVERTED_GAUSSIAN);

  _histogramScalingSubMenu = new QPopupMenu(_contextMenu);
  _histogramScalingSubMenu->setCheckable(true);
  _histogramScalingSubMenu->insertItem("Boolean", BOOLEAN);
  _histogramScalingSubMenu->setItemChecked(BOOLEAN, false);
  _histogramScalingSubMenu->insertItem("Linear", LINEAR);
  _histogramScalingSubMenu->setItemChecked(LINEAR, true);
  _histogramScalingSubMenu->insertItem("Log", LOG);
  _histogramScalingSubMenu->setItemChecked(LOG, false);

  _compTypeSubMenu = new QPopupMenu(_contextMenu);
  _compTypeSubMenu->setCheckable(true);
  _compTypeSubMenu->insertItem("Addition", 
                               MapperFunction::ADDITION);
  _compTypeSubMenu->setItemChecked(MapperFunction::ADDITION, true);
  _compTypeSubMenu->insertItem("Multiplication", 
                               MapperFunction::MULTIPLICATION);
  _compTypeSubMenu->setItemChecked(MapperFunction::MULTIPLICATION, false);

  _widgetEnabledSubMenu = new QPopupMenu(_contextMenu);
  _widgetEnabledSubMenu->setCheckable(true);
  _widgetEnabledSubMenu->insertItem("Enabled", ENABLED);
  _widgetEnabledSubMenu->setItemChecked(ENABLED, true);
  _widgetEnabledSubMenu->insertItem("Disabled", DISABLED);
  _widgetEnabledSubMenu->setItemChecked(DISABLED, false);

  _deleteOpacityWidgetAction = new QAction(this);
  _deleteOpacityWidgetAction->setText("Delete Opacity Widget");

  _addColorControlPointAction = new QAction(this);
  _addColorControlPointAction->setText("New Color Control Point");

  _addOpacityControlPointAction = new QAction(this);
  _addOpacityControlPointAction->setText("New Opacity Control Point");

  _editControlPointAction = new QAction(this);
  _editControlPointAction->setText("Edit Control Point");

  _deleteControlPointAction = new QAction(this);
  _deleteControlPointAction->setText("Delete Control Point");

  //
  // Tooltips
  //
  _tooltip = new TFLocationTip(this);
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void MappingFrame::initConnections()
{
  connect(_addOpacityControlPointAction, SIGNAL(activated()), 
          this, SLOT(addOpacityControlPoint()));

  connect(_addColorControlPointAction, SIGNAL(activated()), 
          this, SLOT(addColorControlPoint()));

  connect(_editControlPointAction, SIGNAL(activated()), 
          this, SLOT(editControlPoint()));

  connect(_deleteControlPointAction, SIGNAL(activated()), 
          this, SLOT(deleteControlPoint()));

  connect(_addOpacityWidgetSubMenu, SIGNAL(activated(int)),
          this, SLOT(addOpacityWidget(int)));

  connect(_histogramScalingSubMenu, SIGNAL(activated(int)),
          this, SLOT(setHistogramScale(int)));

  connect(_compTypeSubMenu, SIGNAL(activated(int)),
          this, SLOT(setCompositionType(int)));

  connect(_widgetEnabledSubMenu, SIGNAL(activated(int)),
          this, SLOT(setWidgetEnabled(int)));

  connect(_deleteOpacityWidgetAction, SIGNAL(activated()), 
          this, SLOT(deleteOpacityWidget()));

  connect(_colorbarWidget, SIGNAL(mapChanged()), this, SLOT(updateMap()));

  connect(_colorbarWidget, SIGNAL(sendRgb(QRgb)), this, SIGNAL(sendRgb(QRgb)));
}

//----------------------------------------------------------------------------
// Remove all the opacity widgets
//----------------------------------------------------------------------------
void MappingFrame::deleteOpacityWidgets()
{
  makeCurrent();
  deselectWidgets();

  map<int, OpacityWidget*>::iterator iter;

  for (iter = _opacityWidgets.begin(); iter != _opacityWidgets.end(); iter++)
  {
    delete (*iter).second;
    (*iter).second = NULL;
  }

  _opacityWidgets.clear();
}

//-----------------------------------------------------------------------------
// Set up the OpenGL view port, matrix mode, etc.
//----------------------------------------------------------------------------
void MappingFrame::resizeGL(int width, int height)
{
  //
  // Update the size of the drawing rectangle
  //
  glViewport( 0, 0, (GLint)width, (GLint)height );

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);

  qglClearColor(QColor(0,0,0)); 

}
	
//----------------------------------------------------------------------------
// Draw the frame's contents
//----------------------------------------------------------------------------
void MappingFrame::paintGL()
{
  printOpenGLError();

  qglClearColor(backgroundColor());

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glColor3f(0.0, 0.0, 0.0);
  glBegin(GL_QUADS);
  {
    glVertex2f(_minX, _minY);
    glVertex2f(_minX, _maxY);
    glVertex2f(_maxX, _maxY);
    glVertex2f(_maxX, _minY); 
  }
  glEnd();

  //
  // Draw Histogram
  //
  if (_histogram)
  {
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _texid);

    if (_updateTexture)
    {
      updateTexture();
    }
	
    
    glColor3f(0.0, 0.784, 0.784);

	glBegin(GL_QUADS);
    {
      glTexCoord2f(0.0f, 0.0f);
      glVertex2f(xDataToWorld(_histogram->getMinData()), 0.0);
      
      glTexCoord2f(0.0f, 1.0f);
      glVertex2f(xDataToWorld(_histogram->getMinData()), 1.0);
      
      glTexCoord2f(1.0f, 1.0f);
      glVertex2f(xDataToWorld(_histogram->getMaxData()), 1.0);
      
      glTexCoord2f(1.0f, 0.0f); 
      glVertex2f(xDataToWorld(_histogram->getMaxData()), 0.0); 
    }
	glEnd();

    glDisable(GL_TEXTURE_2D);
  }

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  //
  // Draw Opacity Widgets
  //
  drawOpacityWidgets();

  //
  // Draw the opacity curve
  //
  drawOpacityCurve();

  //
  // Draw Domain Slider
  //
  drawDomainSlider();
  if(_isoSliderEnabled) drawIsoSlider();
  glDisable(GL_LIGHT0);
  glDisable(GL_LIGHTING);
	
  //
  // Draw the colorbar
  //
  drawColorbar();

  //
  // Draw axis region background
  //
  QColor color = parentWidget()->paletteBackgroundColor();
  glColor3f(color.red()/255.0, color.green()/255.0, color.blue()/255.0);

  float unitPerPixel = 1.0/(height()-totalFixedHeight());
  float maxY = _minY + (unitPerPixel * _axisRegionHeight);

  glBegin(GL_QUADS);
  {
    glVertex2f(_minX, _minY);
    glVertex2f(_minX, maxY);
    glVertex2f(_maxX, maxY);
    glVertex2f(_maxX, _minY); 
  }
  glEnd();

  //
  // Update Axis Labels
  //
  updateAxisLabels();

  //
  // If the MappingFrame widget is disabled, gray it out. 
  //
  if (!isEnabled() && !_isoSliderEnabled)
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.5, 0.5, 0.5, 0.35);
    
    glBegin(GL_QUADS);
    {
      glVertex3f(_minX, maxY, 0.9);
      glVertex3f(_minX, _maxY, 0.9);
      glVertex3f(_maxX, _maxY, 0.9);
      glVertex3f(_maxX, maxY, 0.9); 
    }
    glEnd();

    glDisable(GL_BLEND);
  }


  printOpenGLError();
}

//----------------------------------------------------------------------------
// Set up the OpenGL rendering state
//----------------------------------------------------------------------------
void MappingFrame::initializeGL()
{
  qglClearColor(QColor(0,0,0)); 

  glShadeModel( GL_SMOOTH );

  resize();

  glOrtho(_minX, _maxX, _minY, _maxY, -1.0, 1.0);

  //
  // Initialize the histogram texture
  //
  glGenTextures(1, &_texid);
  glBindTexture(GL_TEXTURE_2D, _texid);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  //
  // Enable lighting for opacity widget handles
  //
  static float ambient[]  = {0.2, 0.2, 0.2, 1.0};
  static float diffuse[]  = {1.0, 1.0, 1.0, 1.0};
  static float specular[] = {0.0, 0.0,  0.0, 0.0};
  static float lightpos[] = {0.0, 1.0, 5000000.0};

  glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
  glLightfv(GL_LIGHT0, GL_POSITION, lightpos);

  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 128);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glDisable(GL_CULL_FACE);

  if (_colorMappingEnabled)
  {
    _colorbarWidget->initializeGL();
  }
}

//----------------------------------------------------------------------------
// Draw the opacity curve
//----------------------------------------------------------------------------
void MappingFrame::drawOpacityCurve()
{
  if (_mapper && _opacityMappingEnabled)
  {
    float step = (_maxValue - _minValue)/(_NUM_BINS-1);
    
    glColor3f(0.0, 1.0, 0.0);
    
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    glBegin(GL_LINE_STRIP);
    {
      for (int i=0; i<_NUM_BINS; i++)
      {
        if (_minValue + i*step >= getMinDomainBound() &&
            _minValue + i*step <= getMaxDomainBound())
        {
          float opacity = getOpacity(_minValue + i*step);
          
          glVertex3f((float)i/(_NUM_BINS-1), opacity, 0.0);
        }
      }
    }
    glEnd();

    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
  }
}

//----------------------------------------------------------------------------
// Draw the opacity widgets
//----------------------------------------------------------------------------
void MappingFrame::drawOpacityWidgets()
{
  if (_opacityMappingEnabled)
  {
    map<int, OpacityWidget*>::iterator iter;
    
    glPushName(OPACITY_WIDGETS);
    
    for (iter = _opacityWidgets.begin(); iter != _opacityWidgets.end(); iter++)
    {
      (*iter).second->paintGL();
    }
    
    glPopName(); // OPACITY_WIDGETS
  }
}

//----------------------------------------------------------------------------
// Draw the domain slider
//----------------------------------------------------------------------------
void MappingFrame::drawDomainSlider()
{
  glPushName(DOMAIN_WIDGET);
  
  _domainSlider->paintGL();
  
  //
  // Draw Domain Variable Name
  //

  // TBD Note: RenderText
  
  //AN, 5/8/08: Note:  for some reason, moving the domain slider text
  //causes gl errors when the domain bounds are invalid.
  //See bug 1818812
  //As a workaround, the text is not moved. 
  //float x = (_domainSlider->minValue() + _domainSlider->maxValue()) / 2.0;
  //int wx  = (int)(xWorldToView(x) - _variableLabel->width())/2;
  int wx = (width() - _variableLabel->width())/2;
  _variableLabel->move(wx, _domainLabelHeight);
 
  glPopName();
}
//----------------------------------------------------------------------------
// Draw the iso slider
//----------------------------------------------------------------------------
void MappingFrame::drawIsoSlider()
{
	
	glPushName(ISO_WIDGET);
  
	_isoSlider->paintGL();
  
	glPopName();
}
//----------------------------------------------------------------------------
// Draw the colorbar
//----------------------------------------------------------------------------
void MappingFrame::drawColorbar()
{
  if (_colorMappingEnabled && _colorbarWidget)
  {
    glPushName(COLORBAR_WIDGET);

    _colorbarWidget->paintGL(); 

    glPopName();
  }
}

//----------------------------------------------------------------------------
// Rebuild the histogram texture
//----------------------------------------------------------------------------
void MappingFrame::updateTexture()
{
  if (_histogram && _mapper)
  {
	  float stretch;
	  if (_isoSliderEnabled)
		 stretch = ((ParamsIso*)(_mapper->getParams()))->GetIsoHistoStretch();
	  else 
		 stretch = _mapper->getParams()->GetHistoStretch();
	 


    for (int x=0; x<_NUM_BINS; x++)
    {
      float binValue = 0.0;

      //
      // Find the histogram value based on the current scaling
      // type.
      //
      switch (_histogramScale)
      {
        case LINEAR:
        {
          binValue = MIN(1.0, (stretch * _histogram->getBinSize(x) / 
                               _histogram->getMaxBinSize()));
          break;
        }

        case LOG:
        {
          binValue = logf(stretch * _histogram->getBinSize(x)) / 
            logf(_histogram->getMaxBinSize());
          break;
        }

        default: // BOOLEAN
        {
          binValue = _histogram->getBinSize(x) ? 1.0 : 0.0;
        }
      }

      int histoHeight = (int)(_NUM_BINS * binValue);

      for (int y=0; y<_NUM_BINS; y++)
      {
        int index = x + y*_NUM_BINS;
        
        if (y < histoHeight)
        {
          _texture[index] = 255;
        }
        else
        {
          _texture[index] = 0;
        }
      }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 256, 256, 0, GL_LUMINANCE, 
                 GL_UNSIGNED_BYTE, _texture);
    
    _updateTexture = false;
  }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void MappingFrame::updateAxisLabels()
{
  int x;
  int y = height()-10;

  list<float> ticks;

  _axisIter = _axisLabels.begin();

  OpacityWidget *opacWidget = dynamic_cast<OpacityWidget*>(_lastSelected);

  if (opacWidget)
  {
    list<float> points = opacWidget->selectedPoints();

    if (points.size())
    {
      list<float>::iterator iter;

      for (iter=points.begin(); iter!=points.end(); iter++)
      {
        x = (int)xWorldToView(xDataToWorld(*iter));
        addAxisLabel(x, y, QString("%1").arg(*iter));

        ticks.push_back(*iter);
      }
    }
    else
    {
      float mind = opacWidget->opacityMap()->minValue();
      float maxd = opacWidget->opacityMap()->maxValue();
      
      x = (int)xWorldToView(xDataToWorld(mind));
      addAxisLabel(x, y, QString("%1").arg(mind));

      x = (int)xWorldToView(xDataToWorld(maxd));
      addAxisLabel(x, y, QString("%1").arg(maxd));

      ticks.push_back(mind);
      ticks.push_back(maxd);
    }
  }

  else if (_colorbarWidget == _lastSelected && _lastSelected)
  {
    list<float> points = _colorbarWidget->selectedPoints();
    list<float>::iterator iter;

    for (iter=points.begin(); iter!=points.end(); iter++)
    {
      x = (int)xWorldToView(xDataToWorld(*iter));
      addAxisLabel(x, y, QString("%1").arg(*iter));

      ticks.push_back(*iter);
    }
  }

  else if (_domainSlider)
  {
    float mind = _domainSlider->minValue();
    float maxd = _domainSlider->maxValue();
    
    x = (int)xWorldToView(mind);
    addAxisLabel(x, y, QString("%1").arg(xWorldToData(mind)));
    
    x = (int)xWorldToView(maxd);
    addAxisLabel(x, y, QString("%1").arg(xWorldToData(maxd)));

    ticks.push_back(xWorldToData(mind));
    ticks.push_back(xWorldToData(maxd));
  }

  if (_isoSlider && _isoSliderEnabled) 
  {
	  float isoval = (_isoSlider->minValue()+_isoSlider->maxValue())*0.5;

	  x = int(xWorldToView(isoval));
	  addAxisLabel(x,y,QString("%1").arg(xWorldToData(isoval)));
  }

  for (; _axisIter != _axisLabels.end(); _axisIter++)
  {
    (*_axisIter)->hide();
  }

  //
  // Draw ticks
  //
  glColor4f(1.0, 1.0, 1.0, 1.0);

  float unitPerPixel = 1.0 / (float)(height()-totalFixedHeight());
  float y0 = _minY + unitPerPixel * _axisRegionHeight;
  float y1 = y0 + unitPerPixel * _bottomGap;
  list<float>::iterator iter;

  glBegin(GL_LINES);
  {
    for (iter=ticks.begin(); iter!=ticks.end(); iter++)
    {
      glVertex3f(xDataToWorld(*iter), y0, 0.05);
      glVertex3f(xDataToWorld(*iter), y1, 0.05);
    }
  }
  glEnd();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void MappingFrame::addAxisLabel(int x, int y, const QString &text)
{
  QLabel *label = NULL;

  if (_axisIter == _axisLabels.end())
  {
    label = new QLabel(parentWidget());
    label->setAlignment(Qt::AlignHCenter);
    _axisLabels.push_back(label);
  }
  else
  {
    label = *_axisIter;
    _axisIter++;
  }

  QPoint pos(x - label->width()/2, y-10);

  label->setText(text);
  label->move(mapToParent(pos));
  label->adjustSize();
  label->show();
}


//----------------------------------------------------------------------------
// Select the GLWidget(s) at the given position
//----------------------------------------------------------------------------
void MappingFrame::select(int x, int y, ButtonState state)
{
  const int length = 128;
  static GLuint selectionBuffer[length];

  GLint hits;
  GLint viewport[4];

  makeCurrent();

  glInitNames();
  glPushName(0);

  //
  // Setup selection buffer
  //
  glSelectBuffer(length, selectionBuffer);

  glGetIntegerv(GL_VIEWPORT, viewport);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glRenderMode(GL_SELECT);

  //
  // Establish new clipping volume as the unit cube around the 
  // mouse position.
  //
  glLoadIdentity();
  gluPickMatrix(x, viewport[3] - y, 4,4, viewport);
  glMatrixMode(GL_MODELVIEW);

  //
  // Render widgets
  //
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawOpacityWidgets();
  drawDomainSlider();
  if(_isoSliderEnabled) drawIsoSlider();
  drawColorbar();

  //
  // Restore projection matrix
  //
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  
  //
  // Collect hits
  //
  hits = glRenderMode(GL_RENDER);

  if (hits)
  {
    select(hits, selectionBuffer, state);
  }
  else
  {
    deselectWidgets();
  }
  glPopName();
}

//----------------------------------------------------------------------------
// Parse the GL selection buffer
//----------------------------------------------------------------------------
void MappingFrame::select(int hits, GLuint *selectionBuffer, ButtonState state)
{
  if (!(state & (Qt::ShiftButton|Qt::ControlButton)))
  {
    deselectWidgets();
  }

  int offset = 0;
  int hitOffset = 0;
  int maxCount = 0;

  //
  // Find the hit with the maximum count
  //
  for (int h=0; h < hits; h++)
  {
    int count = selectionBuffer[offset];

    if (count > maxCount)
    {
      maxCount = count;
      hitOffset = offset;
    }

    offset += count+3;  
  }

  //
  // Select the leaf object of the hit with the maximum count
  //
  if (selectionBuffer[hitOffset+3] == OPACITY_WIDGETS)
  {
    _lastSelected = _opacityWidgets[ selectionBuffer[hitOffset+4] ];
  }
  else if (selectionBuffer[hitOffset+3] == DOMAIN_WIDGET)
  {
    deselectWidgets();

    _lastSelected = _domainSlider;
  }
  else if (selectionBuffer[hitOffset+3] == ISO_WIDGET)
  {
    deselectWidgets();

    _lastSelected = _isoSlider;
  }
  else if (selectionBuffer[hitOffset+3] == COLORBAR_WIDGET)
  {
    _lastSelected = _colorbarWidget;
    
    if (maxCount < 2)
    {
      //
      // Clicked on the colorbar, but not on a control point. Deselect.
      //
      deselectWidgets();

      return;
    }
  }

  assert(_lastSelected);

  _lastSelected->select(selectionBuffer[hitOffset+maxCount+2], state);

  //
  // Depending on the state of _lastSelected and the button modifiers,
  // the GLWidget::select(...) call may have selected or deselected 
  // _lastSelected. We'll respond accordingly.
  //
  if (_lastSelected->selected())
  {
    _selectedWidgets.insert(_lastSelected);
  }
  else
  {
    _selectedWidgets.erase(_lastSelected);
    _lastSelected = NULL;
  }

  updateGL();

  emit canBindControlPoints(canBind());
}

//----------------------------------------------------------------------------
// Deselect all selected widgets
//----------------------------------------------------------------------------
void MappingFrame::deselectWidgets()
{
  if(_lastSelected)
  {
    _lastSelected->deselect();
    _lastSelected = NULL;
  }

  set<GLWidget*>::iterator iter;

  for (iter=_selectedWidgets.begin(); iter!=_selectedWidgets.end(); iter++)
  {
    (*iter)->deselect();
  }

  _selectedWidgets.clear();
}

//----------------------------------------------------------------------------
// Return the sum of all the fixed height areas of the mapping frame (i.e.,
// all the areas/widgets of the frame that fixed and do not scale relative
// to the size of the frame).
//----------------------------------------------------------------------------
int MappingFrame::totalFixedHeight()
{
  int total = _domainHeight + _axisRegionHeight + _bottomGap;

  if (_colorMappingEnabled)
  {
    total += _colorbarHeight;
  }

  if (_opacityMappingEnabled)
  {
    total += 2 * _opacityGap;
  }

  return total;
}


//----------------------------------------------------------------------------
// Handle resize events. This method adjusts the gl coordinates of the 
// glwidgets, so that, the domain widget and colorbar widget maintain
// a fixed size. 
//
//       _minX                           _maxX
//         |                               |
//         *-------------------------------* - _maxY (calculated)
//         *                               *
//         *        Domain Widget          *    fixed height
//         *                               *
//         * - - - - - - - - - - - - - - - * - 1.0
//         *                               *
//         *                               *
//         *                               *
//         *      Histogram/Opacity        *
//         *                               *    variabled height
//         *                               *
//         *                               *
//         *                               *
//         * - - - - - - - - - - - - - - - * - 0.0
//         *                               *
//         *       Colorbar Widget         *     fixed height
//         *                               *
//         * - - - - - - - - - - - - - - - * - (calculated)
//         *                               *
//         *       Annotation Space        *     fixed height
//         *                               *
//         *-------------------------------* - _maxY (calculated)
//
//
//
//----------------------------------------------------------------------------
void MappingFrame::resize()
{
  //
  // View to world coordinates factor
  //
  float unitPerPixel = 1.0 / (float)(height()-totalFixedHeight());

  //
  // Determine the new y extents
  //
  _minY = - unitPerPixel * (_axisRegionHeight + _bottomGap);

  if (_colorMappingEnabled)
  {
    _minY += - unitPerPixel * (_colorbarHeight);
  }

  if (_opacityMappingEnabled)
  {
    _minY += - unitPerPixel * (_opacityGap);
  }

  _maxY = 1.0 + unitPerPixel * (_domainHeight + _opacityGap);


  float domainWidth = unitPerPixel * _domainBarHeight;

  _domainSlider->setGeometry(_minX, _maxX, _maxY-domainWidth, _maxY);

  float bGap   = unitPerPixel * _bottomGap;

  if (_colorbarWidget)
  {
    float cbWidth = unitPerPixel * _colorbarHeight;

    if (!_opacityMappingEnabled)
    {
      bGap *= 0.5;
    }

    _colorbarWidget->setGeometry(_minX, _maxX, -(bGap+cbWidth), -bGap);
  }

  float ogap = unitPerPixel * _opacityGap;
  
  map<int, OpacityWidget*>::iterator iter;
  
  for (iter = _opacityWidgets.begin(); iter != _opacityWidgets.end(); iter++)
  {
    OpacityWidget *widget = (*iter).second;

    if (_colorMappingEnabled)
    {
      widget->setGeometry(_minX, _maxX, 0.0-ogap, 1.0+ogap);
    }
    else
    {
      widget->setGeometry(_minX, _maxX, 0.0-ogap-bGap, 1.0+ogap);
    }
  }       
}


//----------------------------------------------------------------------------
// Handle mouse press events
//----------------------------------------------------------------------------
void MappingFrame::mousePressEvent(QMouseEvent *event)
{
  select(event->x(), event->y(), event->state());

  _lastx = xViewToWorld(event->x());
  _lasty = yViewToWorld(height() - event->y());

  _clickedPos = event->pos();
  _minValueStart = _minValue;
  _maxValueStart = _maxValue;

  _button = event->button();

  if (_editMode && (_button == Qt::LeftButton || _button == Qt::MidButton))
  {
    if (_lastSelected)
    {
      if (_lastSelected != _domainSlider)
      {
        if (_lastSelected == _colorbarWidget)
        {
          emit startChange("Colormap edit");
        }
		else if (_lastSelected == _isoSlider) 
		{
		  emit startChange("Iso slider move");
		}
		else 
		{
          emit startChange("Opacity map edit");
        }
      }
    }
    
  } 
  else if (!_editMode && (_button == Qt::LeftButton))
	{
		emit startChange("Mapping window zoom/pan");
	}

  updateGL();
}

//----------------------------------------------------------------------------
// Handle mouse release events
//----------------------------------------------------------------------------
void MappingFrame::mouseReleaseEvent(QMouseEvent *event)
{
  _button = event->button();

  if (_editMode &&(_button == Qt::LeftButton || _button == Qt::MidButton))
  {
    if (_lastSelected == _domainSlider)
    {
      setDomain();
    }
	else if (_lastSelected == _isoSlider)
    {
	  setIsoSlider();
	}
    else if (_lastSelected)
    {
      emit endChange();
    }
  } 
  else if (!_editMode && _button == Qt::LeftButton) 
  {
    setMinEditBound(_minValue);
    setMaxEditBound(_maxValue);

    _domainSlider->setDomain(xDataToWorld(getMinDomainBound()), 
                             xDataToWorld(getMaxDomainBound()));

    updateGL();

	emit endChange();
  }
}

//----------------------------------------------------------------------------
// Handle mouse double-click events
//----------------------------------------------------------------------------
void MappingFrame::mouseDoubleClickEvent(QMouseEvent* /* event*/)
{
  editControlPoint();
}

//----------------------------------------------------------------------------
// Handle mouse movement events
//----------------------------------------------------------------------------
void MappingFrame::mouseMoveEvent(QMouseEvent* event)
{
  set<GLWidget*>::iterator iter;

  if (_selectedWidgets.size())
  {
    float x = xViewToWorld(event->x());
    float y = yViewToWorld(height() - event->y());

    float dx = x - _lastx;
    float dy = y - _lasty;

    _lastx = x;
    _lasty = y;

    if (_button == Qt::LeftButton && _editMode)
    {
      for (iter=_selectedWidgets.begin(); iter!=_selectedWidgets.end(); iter++)
      {
        (*iter)->drag(dx, dy);
      }
    }
    if (_button == Qt::MidButton)
    {
      for (iter=_selectedWidgets.begin(); iter!=_selectedWidgets.end(); iter++)
      {
        (*iter)->move(dx, dy);
      }
    }

    parentWidget()->repaint();
  }
  else if (_button == Qt::LeftButton && !_editMode)
  {
	float zoomRatio = pow(2.f, (float)(event->y()-_clickedPos.y())/height());

	//Determine the horizontal pan as a fraction of edit window width:
	float horizFraction = (float)(event->x()-_clickedPos.x())/(float)(width());

	//The zoom starts at the original drag start; i.e. that point won't move
	float startXMapped = 
      ((float)_clickedPos.x()/(float)(width())) * 
      (_maxValueStart-_minValueStart) + _minValueStart;

    float minv = startXMapped - (startXMapped - _minValueStart) * zoomRatio;
    float maxv= startXMapped + (_maxValueStart - startXMapped) * zoomRatio;

	_minValue = minv - horizFraction*(maxv - minv);
	_maxValue = maxv - horizFraction*(maxv - minv);
	if(_colorbarWidget) _colorbarWidget->setDirty();
  }

  updateGL();
}

//----------------------------------------------------------------------------
// Handle context menu events
//----------------------------------------------------------------------------
void MappingFrame::contextMenuEvent(QContextMenuEvent* /*event*/)
{
  if (_mapper == NULL)
  {
    return;
  }

  OpacityWidget *opacWidget = dynamic_cast<OpacityWidget*>(_lastSelected);

  _contextPoint = QCursor::pos();

  _contextMenu->clear();

  //
  // Opacity widget context
  //
  if (opacWidget)
  {
    if(opacWidget->controlPointSelected())
    {
      _editControlPointAction->addTo(_contextMenu);
      _deleteControlPointAction->addTo(_contextMenu);

      _contextMenu->insertSeparator();
    }

    _widgetEnabledSubMenu->setItemChecked(ENABLED, opacWidget->enabled());
    _widgetEnabledSubMenu->setItemChecked(DISABLED, !opacWidget->enabled());

    _contextMenu->insertItem("Opacity Contribution", _widgetEnabledSubMenu);
    _deleteOpacityWidgetAction->addTo(_contextMenu);

    _contextMenu->insertSeparator();

    if (_colorbarWidget)
    {
      _addColorControlPointAction->addTo(_contextMenu);
    }
  }
  
  //
  // Colobar context
  else if (_colorbarWidget && _lastSelected == _colorbarWidget)
  {
    if(_colorbarWidget->controlPointSelected())
    {
      _editControlPointAction->addTo(_contextMenu);
      _deleteControlPointAction->addTo(_contextMenu);
    }
    else
    {
      _addColorControlPointAction->addTo(_contextMenu);
    }    
  }

  //
  // General context
  //
  else
  {
    if (_opacityMappingEnabled)
    {
      _contextMenu->insertItem("New Opacity Widget", _addOpacityWidgetSubMenu);
    }

    _contextMenu->insertSeparator();

    if (_colorMappingEnabled)
    {
      _addColorControlPointAction->addTo(_contextMenu);
    }
  }

  //
  // Determine if the context point is within the bounds of a ControlPoint
  // widget. 
  //
  QPoint pos = mapFromGlobal(_contextPoint);
  float x = xWorldToData(xViewToWorld(pos.x()));

  map<int, OpacityWidget*>::iterator iter;
  
  for (iter = _opacityWidgets.begin(); iter != _opacityWidgets.end(); iter++)
  {
    OpacityMap *omap = (*iter).second->opacityMap();

    if (omap->type() == OpacityMap::CONTROL_POINT && 
        x >= omap->minValue() &&
        x <= omap->maxValue())
    {
      _addOpacityControlPointAction->addTo(_contextMenu);      
      break;
    } 
  }

  _contextMenu->insertSeparator();
  _contextMenu->insertItem("Histogram Scaling", _histogramScalingSubMenu);

  if (_mapper->getNumOpacityMaps() > 1)
  {
    _contextMenu->insertItem("Opacity Composition", _compTypeSubMenu);
  }

  _contextMenu->exec(_contextPoint);
}

//----------------------------------------------------------------------------
// Transform the x position in the data (model) space into the opengl world 
// space
//----------------------------------------------------------------------------
float MappingFrame::xDataToWorld(float x)
{
  float minVal = _minValue;
  float maxVal = _maxValue;

  if (maxVal == minVal) return(0.0);
  return (x - minVal) / (maxVal - minVal);
}

//----------------------------------------------------------------------------
// Transform the x position in the opengl world space into the data (model) 
// space
//----------------------------------------------------------------------------
float MappingFrame::xWorldToData(float x)
{
  float minVal = _minValue;
  float maxVal = _maxValue;

  return minVal + (x * (maxVal - minVal));
}

//----------------------------------------------------------------------------
// Transform the x position in view space to the x position in world space
//----------------------------------------------------------------------------
float MappingFrame::xViewToWorld(float x)
{
  if (_maxX <= _minX) return 0.f;
  return _minX + ((x / (float)width()) * (_maxX - _minX)); 
}

//----------------------------------------------------------------------------
// Transform the x position in world space to the x position in view space
//----------------------------------------------------------------------------
float MappingFrame::xWorldToView(float x)
{
  if (_maxX <= _minX) return 0.f;
  return width() * (x - _minX) / (_maxX - _minX);
}

//----------------------------------------------------------------------------
// Transform the y position in the data (model) space into the opengl world 
// space
//----------------------------------------------------------------------------
float MappingFrame::yDataToWorld(float y)
{
  return y;
}

//----------------------------------------------------------------------------
// Transform the y position in the opengl world space into the data (model) 
// space
//----------------------------------------------------------------------------
float MappingFrame::yWorldToData(float y)
{
  if (y < 0.0) return 0.0;
  if (y > 1.0) return 1.0;

  return y;
}

//----------------------------------------------------------------------------
// Transform the y position in view space to the y position in world space
//----------------------------------------------------------------------------
float MappingFrame::yViewToWorld(float y)
{
  assert(height() != 0);
  return _minY + ((y / (float)height()) * (_maxY - _minY)); 
}

//----------------------------------------------------------------------------
// Transform the y position in world space to the y position in view space
//----------------------------------------------------------------------------
float MappingFrame::yWorldToView(float y)
{
  if (_maxY <= _minY) return 0.f;
  return height() * (y - _minY) / (_maxY - _minY);
}

//----------------------------------------------------------------------------
// Return the minimum edit bound
//----------------------------------------------------------------------------
float MappingFrame::getMinEditBound()
{
  if (_mapper)
  {
    RenderParams *params = _mapper->getParams();
    assert(params);
	if (_isoSliderEnabled){
		return ((ParamsIso*)params)->getMinIsoEditBound();
	}
    else if (_opacityMappingEnabled)
    {
      return params->getMinOpacEditBound(_mapper->getOpacVarNum());
    }
    else if (_colorMappingEnabled)
    {
      return params->getMinColorEditBound(_mapper->getColorVarNum());
    }
  }

  return 0.0;
}

//----------------------------------------------------------------------------
// Return the maximum edit bound
//----------------------------------------------------------------------------
float MappingFrame::getMaxEditBound()
{
  if (_mapper)
  {
    RenderParams *params = _mapper->getParams();
    assert(params);
	if (_isoSliderEnabled){
		return ((ParamsIso*)params)->getMaxIsoEditBound();
	}
    else if (_opacityMappingEnabled)
    {
      return params->getMaxOpacEditBound(_mapper->getOpacVarNum());
    }
    else if (_colorMappingEnabled)
    {
      return params->getMaxColorEditBound(_mapper->getColorVarNum());
    }
  }

  return 1.0;
}

//----------------------------------------------------------------------------
// Set the minimum edit bound
//----------------------------------------------------------------------------
void MappingFrame::setMinEditBound(float val)
{
  if (_mapper)
  {
    RenderParams *params = _mapper->getParams();
    assert(params);

	if (_isoSliderEnabled){
		((ParamsIso*)params)->setMinIsoEditBound(val);
	}
    else if (_opacityMappingEnabled)
    {
      params->setMinOpacEditBound(val, _mapper->getOpacVarNum());
    }
    else if (_colorMappingEnabled)
    {
      params->setMinColorEditBound(val, _mapper->getColorVarNum());
    }
  }
}

//----------------------------------------------------------------------------
// Set the maximum edit bound
//----------------------------------------------------------------------------
void MappingFrame::setMaxEditBound(float val)
{
  if (_mapper)
  {
    RenderParams *params = _mapper->getParams();
    assert(params);
	if (_isoSliderEnabled){
		((ParamsIso*)params)->setMaxIsoEditBound(val);
	}
    else if (_opacityMappingEnabled)
    {
      params->setMaxOpacEditBound(val, _mapper->getOpacVarNum());
    }
    else if (_colorMappingEnabled)
    {
      params->setMaxColorEditBound(val, _mapper->getColorVarNum());
    }
  }
}

//----------------------------------------------------------------------------
// Return the minimum domain bound
//----------------------------------------------------------------------------
float MappingFrame::getMinDomainBound()
{
  if (_mapper)
  {
    assert(_mapper->getParams());

    if (_opacityMappingEnabled)
    {
      return _mapper->getMinOpacMapValue();
    }
    else if (_colorMappingEnabled)
    {
      return _mapper->getMinColorMapValue();
    }
  }

  return 0.0;
}


//----------------------------------------------------------------------------
// Return the maximum domain bound
//----------------------------------------------------------------------------
float MappingFrame::getMaxDomainBound()
{
  if (_mapper)
  {
    assert(_mapper->getParams());

    if (_opacityMappingEnabled)
    {
      return _mapper->getMaxOpacMapValue();
    }
    else if (_colorMappingEnabled)
    {
      return _mapper->getMaxColorMapValue();
    }
  }

  return 1.0;
}

//----------------------------------------------------------------------------
// Return the opacity value at the given position
//----------------------------------------------------------------------------
float MappingFrame::getOpacity(float value)
{
  if (_mapper)
  {
    return _mapper->opacityValue(value);
  }

  return 0.0;
}

//----------------------------------------------------------------------------
// Return the params
//----------------------------------------------------------------------------
RenderParams* MappingFrame::getParams()
{
  if (_mapper)
  {
    return _mapper->getParams();
  }

  return NULL;
}

//----------------------------------------------------------------------------
// Return the histogram
//----------------------------------------------------------------------------
Histo* MappingFrame::getHistogram()
{
  RenderParams *params = getParams();

  if (params)
  {
    return VizWinMgr::getEventRouter(params->getParamType())->
      getHistogram(params, params->isEnabled(),_isoSliderEnabled);
  }

  return NULL;
}

//----------------------------------------------------------------------------
// Add a new opacity widget
//----------------------------------------------------------------------------
void MappingFrame::addOpacityWidget(int menu)
{
  if (_mapper)
  {
    emit startChange("Opacity widget creation");

    OpacityMap::Type t    = (OpacityMap::Type)menu;
    OpacityWidget *widget = createOpacityWidget(_mapper->createOpacityMap(t));
    
    connect((QObject*)widget, SIGNAL(startChange(QString)),
            this, SIGNAL(startChange(QString)));
    connect((QObject*)widget, SIGNAL(endChange()),
            this, SIGNAL(endChange()));

    connect((QObject*)widget, SIGNAL(mapChanged()),
            this, SLOT(updateMap()));

    emit endChange();
  }
}

//----------------------------------------------------------------------------
// Add a new color control point
//----------------------------------------------------------------------------
void MappingFrame::addColorControlPoint()
{
  QPoint pos = mapFromGlobal(_contextPoint);
  GLfloat wx = pos.x();

  float x = xWorldToData(xViewToWorld(wx));

  if (_mapper)
  {
    Colormap *colormap = _mapper->getColormap();

    if (colormap && (x >= colormap->minValue() && x <= colormap->maxValue()))
    {
      emit startChange("Add color control point");
      
      colormap->addControlPointAt(x);
      
      emit endChange();
      
      updateGL();
    }
  }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void MappingFrame::addOpacityControlPoint()
{
  OpacityWidget *opacWidget = dynamic_cast<OpacityWidget*>(_lastSelected);
  QPoint pos = mapFromGlobal(_contextPoint);

  float x = xWorldToData(xViewToWorld(pos.x()));
  float y = yWorldToData(yViewToWorld(height() - pos.y()));

  emit startChange("Opacity control point addition");

  if (opacWidget)
  {
    opacWidget->opacityMap()->addControlPoint(x, y);
  }
  else
  {
    map<int, OpacityWidget*>::iterator iter;
    
    for (iter = _opacityWidgets.begin(); iter != _opacityWidgets.end(); iter++)
    {
      OpacityMap *omap = (*iter).second->opacityMap();
      
      if (omap->type() == OpacityMap::CONTROL_POINT && 
          x >= omap->minValue() &&
          x <= omap->maxValue())
      {
        omap->addControlPoint(x,y);
        break;
      } 
    }
  }

  emit endChange();

  updateGL();
}

//----------------------------------------------------------------------------
// Delete selected control points in either the opacity widget or colorbar
//----------------------------------------------------------------------------
void MappingFrame::deleteControlPoint()
{
  makeCurrent();

  OpacityWidget *opacWidget = dynamic_cast<OpacityWidget*>(_lastSelected);

  //
  // Opacity widget
  //
  if (opacWidget)
  {
    startChange("Opacity control point deletion");

    opacWidget->deleteSelectedControlPoint();

    emit endChange();
  }

  //
  // Colorbar widget
  //
  else if (_lastSelected == _colorbarWidget)
  {
    startChange("Color control point deletion");

    _colorbarWidget->deleteSelectedControlPoint();

    emit endChange();
  }    

  updateGL();
}

//----------------------------------------------------------------------------
// Edit the selected control point
//----------------------------------------------------------------------------
void MappingFrame::editControlPoint()
{
  OpacityWidget *opacWidget = dynamic_cast<OpacityWidget*>(_lastSelected);

  //
  // Opacity widget
  //
  if (opacWidget && opacWidget->controlPointSelected())
  {
    ControlPointEditor editor(this, 
                              opacWidget->opacityMap(), 
                              opacWidget->selectedControlPoint());
	emit startChange("Opacity Control Point Edit");
    editor.exec();
	emit endChange();
    updateMap();
    updateGL();
  }

  //
  // Colorbar
  //
  else if (_colorbarWidget && _lastSelected == _colorbarWidget && 
           _colorbarWidget->controlPointSelected())
  {
    ControlPointEditor editor(this, 
                              _colorbarWidget->colormap(), 
                              _colorbarWidget->selectedControlPoint());

    emit startChange("Color Control Point Edit");
    editor.exec();
	emit endChange();

    _colorbarWidget->setDirty();
    
    updateMap();
    updateGL();
  }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
OpacityWidget* MappingFrame::createOpacityWidget(OpacityMap *omap)
{
  OpacityWidget *widget = new OpacityWidget(this, omap);

  _opacityWidgets[widget->id()] = widget;

  float unitPerPixel = 1.0 / (float)(height()-totalFixedHeight());
  float gap = 5.0 * unitPerPixel;

  widget->setGeometry(_minX, _maxX, 0.0-gap, 1.0+gap);

  return widget;
}

//----------------------------------------------------------------------------
// Delete the selected opacity widget
//----------------------------------------------------------------------------
void MappingFrame::deleteOpacityWidget()
{
  makeCurrent();

  OpacityWidget *opacWidget = dynamic_cast<OpacityWidget*>(_lastSelected);

  if (opacWidget)
  {
    _selectedWidgets.erase(_lastSelected);
    _lastSelected = NULL;

    emit startChange("Opacity widget deletion");

    _opacityWidgets.erase(opacWidget->id());

    _mapper->deleteOpacityMap(opacWidget->opacityMap());

    delete _lastSelected;
    _lastSelected = NULL;

    emit endChange();
  }
}

//----------------------------------------------------------------------------
// Handle domain widget interactions.
//----------------------------------------------------------------------------
void MappingFrame::setDomain()
{
  float min = xWorldToData(_domainSlider->minValue());
  float max = xWorldToData(_domainSlider->maxValue());

  if (_mapper)
  {
    emit startChange("Mapping window boundary change");

    if (_opacityMappingEnabled)
    {
      _mapper->setMinOpacMapValue(min);
      _mapper->setMaxOpacMapValue(max);
    }

    if (_colorMappingEnabled)
    {
      _mapper->setMinColorMapValue(min);
      _mapper->setMaxColorMapValue(max);
    }

	
    emit endChange();

    RenderParams *params = _mapper->getParams();
    VizWinMgr::getEventRouter(params->getParamType())->updateMapBounds(params);
	
    updateGL();
  }
  else
  {
    _domainSlider->setDomain(xDataToWorld(_minValue),
                             xDataToWorld(_maxValue));
  }
}

//----------------------------------------------------------------------------
// Deal with iso slider movement
//----------------------------------------------------------------------------
void MappingFrame::setIsoSlider()
{
  if (!_mapper) return;

  float min = xWorldToData(_isoSlider->minValue());
  float max = xWorldToData(_isoSlider->maxValue());
  
  emit startChange("Slide Isovalue slider");
  RenderParams *params = _mapper->getParams();
  ParamsIso* iParams = (ParamsIso*)params;
  iParams->SetIsoValue((0.5*(max+min)));
  
  emit endChange();

  updateGL();
  
}
