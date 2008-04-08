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
#include "vapor/DataMgr.h"
#include "DVRBase.h"
#include "renderer.h"
#include "glwindow.h"
#include "VolumeRenderer.h"

namespace VAPoR {

  class RENDER_API IsoRenderer : public VolumeRenderer
  {
	
      
  public:
	
	virtual bool datarangeIsDirty() {return datarangeDirtyBit;}
	
	virtual void setDatarangeDirty(){datarangeDirtyBit = true;}
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

	void UpdateDriverRenderParamsSpec(RenderParams *rp);
    

  private:

	ParamNode::DirtyFlag _lightOnOffDF;
	ParamNode::DirtyFlag _isovalueDF;

  };
};

#endif //ISORENDERER_H
