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
#include "dvreventrouter.h"

namespace VAPoR {

  class VolumeRenderer : public Renderer 
  {
	//Q_OBJECT  ?WHY?
      
  public:

    VolumeRenderer(VizWin *w, DvrParams::DvrType type);
    virtual ~VolumeRenderer();
    
	virtual void initializeGL();
    virtual void paintGL();

    static bool supported(DvrParams::DvrType type);
	
    //#ifdef BENCHMARKING
    void  resetTimer()     { _frames = 0; _seconds = 0; }
    float elapsedTime()    { return _seconds; }
    int   renderedFrames() { return _frames;  } 
    //#endif

  protected:
    
    DVRBase* create_driver(DvrParams::DvrType type, int nthreads);

  private:
  
    DVRBase* driver;
    DvrParams::DvrType _type;

    void DrawVoxelScene(unsigned fast);
    void DrawVoxelWindow(unsigned fast);

    int    _frames;
    double _seconds;

  };
};

#endif //VOLUMERRENDERER_H
