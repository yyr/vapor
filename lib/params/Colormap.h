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
class ParamNode;

class PARAMS_API VColormap : public ColorMapBase 
{

public:

  VColormap(MapperFunctionBase *mapper);
  VColormap(const VColormap &cmap, MapperFunctionBase *mapper);
  const VColormap& operator=(const VColormap &cmap);

  virtual ~VColormap();


};
};

#endif // Colormap_H
