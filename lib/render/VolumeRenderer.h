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
	
	virtual bool datarangeIsDirty() {return datarangeDirtyBit;}
	
	virtual void setDatarangeDirty(){datarangeDirtyBit = true;}
    VolumeRenderer(GLWindow *w, DvrParams::DvrType type, RenderParams* rp);
    virtual ~VolumeRenderer();
    
	virtual void initializeGL();
    virtual void paintGL();

    virtual bool hasLighting();
    virtual bool hasPreintegration();

    static bool supported(DvrParams::DvrType type);
	
    //#ifdef BENCHMARKING
    virtual void  resetTimer()     { _frames = 0; _seconds = 0; }
    virtual float elapsedTime()    { return _seconds; }
    virtual int   renderedFrames() { return _frames;  } 
    //#endif

  protected:
    bool datarangeDirtyBit;

    virtual DVRBase* create_driver(DvrParams::DvrType type, int nthreads);

    DVRBase* _driver;
    DvrParams::DvrType _type;

    virtual void DrawVoxelScene(unsigned fast);
    virtual void DrawVoxelWindow(unsigned fast);

	virtual void UpdateDriverRenderParamsSpec(RenderParams *rp);

    int    _frames;
    double _seconds;

	int _userTextureSize;
	bool _userTextureSizeIsSet;
    GLenum _voxelType;

  };
};

#endif //VOLUMERRENDERER_H
