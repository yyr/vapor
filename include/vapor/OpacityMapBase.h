//--OpacityMapBase.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// Various types of mappings from opacity to data value. 
// 
//----------------------------------------------------------------------------

#ifndef OpacityMapBase_H
#define OpacityMapBase_H


#include <iostream>
#include <vapor/ParamsBase.h>
#include <vapor/tfinterpolator.h>

namespace VAPoR {

class XmlNode;
class ParamNode;
class MapperFunctionBase;

class PARAMS_API OpacityMapBase : public ParamsBase
{
 

public:

  enum Type
  {
    CONTROL_POINT,
    GAUSSIAN,
    INVERTED_GAUSSIAN,
    SINE
  };

  OpacityMapBase(OpacityMapBase::Type type=CONTROL_POINT);

  virtual ~OpacityMapBase();

  void clear();
  const OpacityMapBase& operator=(const OpacityMapBase &cmap);

  float opacity(float value);
  bool  bounds(float value);

  void SetType(OpacityMapBase::Type type);
  OpacityMapBase::Type GetType()          {
	  return (OpacityMapBase::Type) GetValueLong(_typeTag);
  }

  //The base class has Get/Set of min and max which are relative to 0,1
  double GetMinValue()  {return GetValueDouble(_minTag);}     // Data Coordinates
  double GetMaxValue()  {return GetValueDouble(_maxTag);}     // Data Coordinates

  void SetMinValue(double val); 
  void SetMaxValue(double val);

  virtual float minValue() ;      // Data Coordinates
  virtual void  minValue(float value); // Data Coordinates
  virtual float maxValue();      // Data Coordinates
  virtual void  maxValue(float value); // Data Coordinates

  
  bool IsEnabled() {
	  return (GetValueLong(_enabledTag) != 0 ? true : false);
  }
  void SetEnabled(bool enabled);
  double GetMean(){ return GetValueDouble(_meanTag);}
  void SetMean(double mean);
  double GetSSQ(){return GetValueDouble(_ssqTag);}
  void SetSSQ(double ssq);
  double GetFreq(){ return GetValueDouble(_freqTag);}
  void SetFreq(double freq);
  double GetPhase(){ return GetValueDouble(_phaseTag);}
  void SetPhase(double phase);

  int numControlPoints()      { return (int)GetControlPoints().size()/2; }

  void  addNormControlPoint(float normv, float opacity); // Normalized Coords
  void  addControlPoint(float value, float opacity);     // Data Coordinates
  void  deleteControlPoint(int index);
  void  moveControlPoint(int index, float dx, float dy); // Data Coordinates

  float controlPointOpacity(int index);
  void  controlPointOpacity(int index, float opacity);

  float controlPointValue(int index);               // Data Coordinates
  void  controlPointValue(int index, float value);  // Data Coordinates
  /*
  void   mean(double mean);                      // Normalized 
  double mean() const          { return _mean; } // Normalized 

  void   sigmaSq(double ssq);                             // Normalized
  double sigmaSq() const       { return normSSq(_ssq); }   // Normalized

  void   sineFreq(double freq);                                  // Normalized
  double sineFreq() const      { return normSineFreq(_freq); }   // Normalized

  void   sinePhase(double p);                                  // Normalized
  double sinePhase() const   { return normSinePhase(_phase); } // Normalized
  */
  void setOpaque();
  bool isOpaque();

  static string xmlTag() { return _tag; }

  void SetInterpType(TFInterpolator::type t);
	 
  TFInterpolator::type GetInterpType() {
	  return (TFInterpolator::type) GetValueLong(_interpTypeTag);
  }
  vector<double> GetControlPoints(){
	  return GetValueDoubleVec(_controlPointsTag);
  }
  void SetControlPoints(vector<double> opacityControlPoints);
  void setMapper(MapperFunctionBase* m) {_mapper = m;}

protected: 
  MapperFunctionBase *_mapper;
  
  int leftControlIndex(float val);

  double normSSq(double ssq);
  double denormSSq(double ssq);

  double normSineFreq(double freq);
  double denormSineFreq(double freq);

  double normSinePhase(double phase);
  double denormSinePhase(double phase);
   
private:

  const double _minSSq;
  const double _maxSSq;
  const double _minFreq;
  const double _maxFreq;
  const double _minPhase;
  const double _maxPhase;

  static const string _tag;
  static const string _minTag;
  static const string _maxTag;
  static const string _enabledTag;
  static const string _meanTag;
  static const string _ssqTag;
  static const string _freqTag;
  static const string _phaseTag;
  static const string _typeTag;
  static const string _controlPointsTag;
  static const string _interpTypeTag;
};
};

#endif // OpacityMapBase_H
