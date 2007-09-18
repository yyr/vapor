//--OpacityMap.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// Various types of mappings from opacity to data value. 
// 
//----------------------------------------------------------------------------

#ifndef OpacityMap_H
#define OpacityMap_H

#include <vapor/ExpatParseMgr.h>
#include <vapor/tfinterpolator.h>
#include <vapor/OpacityMapBase.h>

#include <iostream>

namespace VAPoR {

class RenderParams;
class XmlNode;

class PARAMS_API OpacityMap : public OpacityMapBase 
{

public:

  OpacityMap(RenderParams *params, OpacityMap::Type type=CONTROL_POINT);
  OpacityMap(const OpacityMap &omap);

  virtual ~OpacityMap();

  const OpacityMap& operator=(const OpacityMap &cmap);

  virtual float minValue() const;      // Data Coordinates
  virtual void  minValue(float value); // Data Coordinates

  virtual float maxValue() const;      // Data Coordinates
  virtual void  maxValue(float value); // Data Coordinates

  void setParams(RenderParams *p) { _params = p; }

private:

  RenderParams *_params;

};
};

#endif // OpacityMap_H
