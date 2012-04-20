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
#include <vapor/DataMgr.h>
#include "DVRBase.h"
#include "renderer.h"
#include "glwindow.h"
#include "dvrparams.h"

namespace VAPoR {

  class RENDER_API VolumeRenderer : public Renderer 
  {
	
      
  public:

	virtual void setAllDataDirty() {}
	virtual void setDatarangeDirty() {}
	
    VolumeRenderer(GLWindow *w, DvrParams::DvrType type, RenderParams* rp, string name = "VolumeRenderer");
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

    virtual DVRBase* create_driver(DvrParams::DvrType type, int nthreads);

    DVRBase* _driver;
    DvrParams::DvrType _type;

    virtual void DrawVoxelScene(unsigned fast);
    virtual void DrawVoxelWindow(unsigned fast);

	virtual int _updateRegion(
		DataMgr *dataMgr, RenderParams *rp, RegionParams *regp,
		size_t ts, string varname, int reflevel, int lod,
		const size_t min[3], const size_t max[3]
	);

	virtual void _updateDriverRenderParamsSpec(RenderParams *rp);

    int    _frames;
    double _seconds;

  private:
	size_t _timeStep;
	double _extents[6];
	float _range[6];
	int	_varNum;
	int _lod;
	int _reflevel;
	bool _userTextureSizeIsSet;
	int _userTextureSize;
	  
  };
};

#endif //VOLUMERRENDERER_H
