//-- VolumeRender.h ----------------------------------------------------------
//   
//                   Copyright (C)  2005
//     University Corporation for Atmospheric Research
//                   All Rights Reserved
//
//----------------------------------------------------------------------------
//
//      File:           VolumeRenderer.h
//
//      Author:         Kenny Gruchalla
//
//      Date:           December 2005
//
//      Description:  Definition of VolumeRenderer class
//
//
//
//----------------------------------------------------------------------------

#ifndef	VolumeRenderer_H
#define	VolumeRenderer_H

#include <stdio.h>
#include "vapor/DataMgr.h"
#include "DVRBase.h"
#include "renderer.h"
#include "vizwin.h"

namespace VAPoR {

  class DvrParams;

  class VolumeRenderer : public Renderer 
  {
	//Q_OBJECT  ?WHY?
      
  public:

    VolumeRenderer(VizWin *w);
    ~VolumeRenderer();
    
	virtual void initializeGL();
    virtual void paintGL();

  protected:
    
    DVRBase* create_driver(const char *name, int nthreads);

  private:
  
    DVRBase* driver;

    void DrawVoxelScene(unsigned fast);
    void DrawVoxelWindow(unsigned fast);

  };
};

#endif //VOLUMERRENDERER_H
