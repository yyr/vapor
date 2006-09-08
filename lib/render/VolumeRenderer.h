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
#include "glwindow.h"
#include "dvrparams.h"

namespace VAPoR {

  class RENDER_API VolumeRenderer : public Renderer 
  {
	
      
  public:
	  bool clutIsDirty() {return clutDirtyBit;}
	  bool datarangeIsDirty() {return datarangeDirtyBit;}
	  void setClutDirty(){clutDirtyBit = true;}
	  void setDatarangeDirty(){datarangeDirtyBit = true;}
    VolumeRenderer(GLWindow *w, DvrParams::DvrType type, RenderParams* rp);
    virtual ~VolumeRenderer();
    
	virtual void initializeGL();
    virtual void paintGL();

    bool hasLighting();

    static bool supported(DvrParams::DvrType type);
	
    //#ifdef BENCHMARKING
    void  resetTimer()     { _frames = 0; _seconds = 0; }
    float elapsedTime()    { return _seconds; }
    int   renderedFrames() { return _frames;  } 
    //#endif

  protected:
    bool datarangeDirtyBit;
	bool clutDirtyBit;

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
