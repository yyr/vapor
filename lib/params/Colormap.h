//--Colormap.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// A map from data value to/from color.
// 
//----------------------------------------------------------------------------

#ifndef Colormap_H
#define Colormap_H

#include "tfinterpolator.h"
#include "vapor/ExpatParseMgr.h"

namespace VAPoR {

class RenderParams;
class XmlNode;

class PARAMS_API Colormap : public ParsedXml 
{

public:

  class PARAMS_API Color
  {

  public:
   
    Color();
    Color(float h, float s, float v);
    Color(const Color &color);

    void toRGB(float *rgb);

    void  hue(float h) { _hue = h; }
    float hue()        { return _hue; }

    void  sat(float s) { _sat = s; }
    float sat()        { return _sat; }

    void  val(float v) { _val = v; }
    float val()        { return _val; }

  private:

    float _hue;
    float _sat;
    float _val;
  };


  Colormap(RenderParams *params);
  Colormap(const Colormap &cmap);

  virtual ~Colormap();

  const Colormap& operator=(const Colormap &cmap);

  XmlNode* buildNode();

  void  clear();

  float minValue() const;      // Data Coordinates
  void  minValue(float value); // Data Coordinates

  float maxValue() const;      // Data Coordinates
  void  maxValue(float value); // Data Coordinates

  int numControlPoints()                { return (int)_controlPoints.size(); }

  Color controlPointColor(int index);
  void  controlPointColor(int index, Color color);

  float controlPointValue(int index);               // Data Coordinates
  void  controlPointValue(int index, float value);  // Data Coordinates

  void addControlPointAt(float value);
  void addControlPointAt(float value, Color color);
  void addNormControlPoint(float normValue, Color color);
  void deleteControlPoint(int index);

  void move(int index, float delta);
  
  Color color(float value);

  void setParams(RenderParams *p) { _params = p; }

  static string xmlTag() { return _tag; }

  virtual bool elementStartHandler(ExpatParseMgr*, int, std::string&, 
                                   const char **attribs);
  virtual bool elementEndHandler(ExpatParseMgr*, int depth, std::string &tag);
 

protected:

  int leftIndex(float val);

  class ControlPoint
  {

  public:

    ControlPoint();
    ControlPoint(Color c, float v);
    ControlPoint(const ControlPoint &cp);

    void  color(Color color) { _color = color; }
    Color color()            { return _color; }

    void  value(float val) { _value = val; }
    float value()          { return _value; }

    void                 type(TFInterpolator::type t) { _type = t; }
    TFInterpolator::type type()                       { return _type; }

    void  select()   { _selected = true; }
    void  deselect() { _selected = false; }
    bool  selected() { return _selected; }

  private:

    TFInterpolator::type _type;

    float _value;
    Color _color;
    
    bool  _selected;
  };

  static bool sortCriterion(ControlPoint *p1, ControlPoint *p2);


private:

  RenderParams *_params;

  float _minValue;
  float _maxValue;

  vector<ControlPoint*> _controlPoints;

  static const string _tag;
  static const string _minTag;
  static const string _maxTag;
  static const string _controlPointTag;  
  static const string _cpHSVTag;
  static const string _cpValueTag;
};
};

#endif // Colormap_H
