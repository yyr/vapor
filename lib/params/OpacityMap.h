//--OpacityMap.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// Various types of mappings from opacity to data value. 
// 
//----------------------------------------------------------------------------

#ifndef OpacityMap_H
#define OpacityMap_H

#include <vapor/tfinterpolator.h>
#include <vapor/OpacityMapBase.h>

#include <iostream>

namespace VAPoR {

class MapperFunctionBase;
class XmlNode;
class ParamNode;

class PARAMS_API OpacityMap : public OpacityMapBase 
{

public:

  OpacityMap(MapperFunctionBase *mapper, OpacityMap::Type type=CONTROL_POINT);
  OpacityMap(const OpacityMap &omap, MapperFunctionBase *mapper);

  virtual ~OpacityMap();

  const OpacityMap& operator=(const OpacityMap &cmap);

  virtual float minValue() ;      // Data Coordinates
  virtual void  minValue(float value); // Data Coordinates

  virtual float maxValue();      // Data Coordinates
  virtual void  maxValue(float value); // Data Coordinates

};
};

#endif // OpacityMap_H
