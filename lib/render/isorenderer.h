//-- isorenderer.h ----------------------------------------------------------
//   
//                   Copyright (C)  2007
//     University Corporation for Atmospheric Research
//                   All Rights Reserved
//
//----------------------------------------------------------------------------
//
//      File:           isorenderer.h
//
//      Author:         Alan Norton
//
//      Date:           August 2007
//
//      Description:  Definition of IsoRenderer class
//
//
//
//----------------------------------------------------------------------------

#ifndef	ISORENDERER_H
#define	ISORENDERER_H

#include <stdio.h>
#include <vapor/DataMgr.h>
#include "DVRBase.h"
#include "renderer.h"
#include "glwindow.h"
#include "VolumeRenderer.h"
#include "ParamsIso.h"

namespace VAPoR {

  class RENDER_API IsoRenderer : public VolumeRenderer
  {
	
      
  public:
	
	bool datarangeIsDirty() { return(_dataDF.Test()); }

	void setDatarangeDirty() { _dataDF.Set(); }

	void clearDatarangeDirty() { _dataDF.Clear(); }

	bool clutIsDirty() { return(_colorMapDF.Test()); }

	void setClutDirty() { _colorMapDF.Set(); }

	void clearClutDirty() { _colorMapDF.Clear(); }

    IsoRenderer(GLWindow *w, DvrParams::DvrType type, RenderParams* rp);
    virtual ~IsoRenderer();
    
    static bool supported(DvrParams::DvrType type);
	
    //#ifdef BENCHMARKING
    virtual void  resetTimer()     { _frames = 0; _seconds = 0; }
    virtual float elapsedTime()    { return _seconds; }
    virtual int   renderedFrames() { return _frames;  } 
    //#endif

	virtual void setRenderParams(RenderParams* rp);


  protected:

 void *_getRegion(
	DataMgr *data_mgr, RenderParams *rp, RegionParams *reg_params,
	size_t ts, const char *varname, int numxforms, int lod,
	const size_t min[3], const size_t max[3]
 ); 

 void _updateDriverRenderParamsSpec(RenderParams *rp);
    

  private:

	ParamNode::DirtyFlag _lightOnOffDF;
	ParamNode::DirtyFlag _isovalueDF;
	ParamNode::DirtyFlag _colorMapDF;
	ParamNode::DirtyFlag _dataDF;

	ParamsIso *_myParamsIso;
	void _unregisterDirtyFlags();

  };
};

#endif //ISORENDERER_H
