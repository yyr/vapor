//--Colormap.h ---------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// A map from data value to/from color.
// 
//----------------------------------------------------------------------------

#ifndef Colormap_H
#define Colormap_H

#include <vapor/ExpatParseMgr.h>
#include <vapor/tfinterpolator.h>
#include <vapor/ColorMapBase.h>

namespace VAPoR {

class MapperFunctionBase;
class XmlNode;

class PARAMS_API VColormap : public ColorMapBase 
{

public:

  VColormap(MapperFunctionBase *mapper);
  VColormap(const VColormap &cmap, MapperFunctionBase *mapper);
  const VColormap& operator=(const VColormap &cmap);

  virtual ~VColormap();

  virtual float minValue() const;      // Data Coordinates
  virtual void  minValue(float value); // Data Coordinates

  virtual float maxValue() const;      // Data Coordinates
  virtual void  maxValue(float value); // Data Coordinates

protected:


private:

  MapperFunctionBase *_mapper;

};
};

#endif // Colormap_H
