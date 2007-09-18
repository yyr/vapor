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

class RenderParams;
class XmlNode;

class PARAMS_API Colormap : public ColorMapBase 
{

public:

  Colormap(RenderParams *params);
  Colormap(const Colormap &cmap);
  const Colormap& operator=(const Colormap &cmap);

  virtual ~Colormap();

  virtual float minValue() const;      // Data Coordinates
  virtual void  minValue(float value); // Data Coordinates

  virtual float maxValue() const;      // Data Coordinates
  virtual void  maxValue(float value); // Data Coordinates

   void setParams(RenderParams *p) { _params = p; }

protected:


private:

  RenderParams *_params;

};
};

#endif // Colormap_H
