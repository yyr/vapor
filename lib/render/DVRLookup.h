//--DVRLookup.h -----------------------------------------------------
//
// Copyright (C) 2005 Kenny Gruchalla.  All rights reserved.
//
// A derived DVRTexture3d providing a pixel map for the color lookup.
//
//----------------------------------------------------------------------------

#ifndef DVRLookup_h
#define DVRLookup_h

#include <vapor/RegularGrid.h>
#include "DVRTexture3d.h"
#include "Vect3d.h"

#include <vector>

namespace VAPoR {

  class BBox;

class RENDER_API DVRLookup : public DVRTexture3d
{
 public:


  DVRLookup(GLenum type, int nthreads);
  virtual ~DVRLookup();

  virtual int GraphicsInit();
  
  virtual int SetRegion(const RegularGrid *rg, const float range[2], int num = 0);

  virtual void loadTexture(TextureBrick *brick);

  virtual int Render();

  virtual void SetCLUT(const float ctab[256][4]);
  virtual void SetOLUT(const float ftab[256][4], const int numRefinenements);

  static bool supported() { return true; }

protected:

  virtual void initColormap();

 
protected:

  float            *_colormap;
  GLint            _colormapsize;
};

};


#endif 
