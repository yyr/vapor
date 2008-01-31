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

class PARAMS_API Colormap : public ColorMapBase 
{

public:

  Colormap(MapperFunctionBase *mapper);
  Colormap(const Colormap &cmap, MapperFunctionBase *mapper);
  const Colormap& operator=(const Colormap &cmap);

  virtual ~Colormap();

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
