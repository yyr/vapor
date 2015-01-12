//--ColorMapBase.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// A map from data value to/from color.
// 
//----------------------------------------------------------------------------

#ifndef ColorMapBase_H
#define ColorMapBase_H

#include <vapor/ParamsBase.h>
#include <vapor/tfinterpolator.h>

namespace VAPoR {

class ParamNode;
class MapperFunctionBase;

class PARAMS_API ColorMapBase : public ParamsBase
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


  ColorMapBase();
 
  virtual ~ColorMapBase();

  const ColorMapBase& operator=(const ColorMapBase &cmap);

  void  clear();

  //The base class has Get/Set of min and max which are relative to 0,1
  double GetMinValue()  {return GetValueDouble(_minTag);}     // Data Coordinates
  double GetMaxValue()  {return GetValueDouble(_maxTag);}     // Data Coordinates

  void SetMinValue(double val); 
  void SetMaxValue(double val);

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

  static string xmlTag() { return _tag; }

 
  void interpType(TFInterpolator::type t){_interpType = t;}
  TFInterpolator::type interpType() {return _interpType;}
  void setMapper(MapperFunctionBase* m) {_mapper = m;}
	
protected:
  MapperFunctionBase *_mapper;
  int leftIndex(float val);
  TFInterpolator::type _interpType;

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
	
  private:
	
    float _value;
    Color _color;
    
  };

  static bool sortCriterion(ControlPoint *p1, ControlPoint *p2);

  


private:

  
  vector<ControlPoint*> _controlPoints;

  static const string _tag;
  static const string _minTag;
  static const string _maxTag;
  static const string _controlPointTag;  
  static const string _cpHSVTag;
  static const string _cpRGBTag;
  static const string _cpValueTag;
  static const string _discreteColorAttr;
};
class PARAMS_API ARGB{
public:
	ARGB(int r, int g, int b){
		argbvalue = ((r&255)<<16) | ((g&255)<<8) | (b&255);
	}
private:
	unsigned int argbvalue;

};
};

#endif // ColorMapBase_H
