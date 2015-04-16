//-- arrowrenderer.h ----------------------------------------------------------
//   
//                   Copyright (C)  2011
//     University Corporation for Atmospheric Research
//                   All Rights Reserved
//
//----------------------------------------------------------------------------
//
//      File:           arrowrenderer.h
//
//      Author:         Alan Norton
//
//
//      Description:  Definition of ArrowRenderer class
//
//
//
//----------------------------------------------------------------------------

#ifndef	ARROWRENDERER_H
#define	ARROWRENDERER_H

#include "vapor/RegularGrid.h"
#include "renderer.h"

namespace VAPoR {

//! \class ArrowRenderer
//! \brief Class that draws the barbs as specified by ArrowParams
//! \author Alan Norton
//! \version 3.0
//! \date February 2014

  class RENDER_API ArrowRenderer : public Renderer
  {
	
      
  public:
//! Constructor, must invoke Renderer constructor
//! \param[in] Visualizer* pointer to the visualizer where this will draw
//! \param[in] RenderParams* pointer to the ArrowParams describing this renderer
    ArrowRenderer(Visualizer *w, RenderParams* rp);

//! Destructor
    virtual ~ArrowRenderer();

//! CreateInstance:  Static method to create a renderer given the associated Params instance and visualizer
//! \param[in] Visualizer* pointer to the visualizer where this will draw
//! \param[in] RenderParams* pointer to the ArrowParams associated with this renderer
	static Renderer* CreateInstance(Visualizer* v, RenderParams* rp) {
		return new ArrowRenderer(v,rp);
	}


//! Virtual method to force rebuilding of any caches associated with this renderer.
//! The ArrowRender has no caches so this method does nothing.
    virtual void setAllDataDirty(){} //No need to do anything.

  protected:

//! Virtual method performs any OpenGL initialization, sets the initialized flag
    virtual int _initializeGL();

//! Virtual method issues all the OpenGL calls to draw the arrows in user coordinates.
//! \param[in] DataMgr* current DataMgr that owns the data being rendered.
//! \param[in] Params* Params* that is associated with this Renderer
    virtual int _paintGL(DataMgrV3_0*);


//! Protected method that performs rendering of all arrows.
//! \param[in] DataMgr* current DataMgr
//! \param[in] const RenderParams* associated RenderParams
//! \param[in] int actualRefLevel refinement level to be rendered.
//! \param[in] float vectorScale Scale factor to be applied to arrows.
//! \param[in] float arrowRadius Radius of arrows in voxel diameters.
//! \param[in] RegularGrid*[4] RegularGrids used in rendering
//! \retval int zero if successful
	int performRendering(DataMgrV3_0* dataMgr, const RenderParams* rParams,
		int actualRefLevel,float vectorScale, float arrowRadius, 
		RegularGrid *variableData[4]
	);

//! Protected method to draw one arrow (a hexagonal tube with a cone arrowhead)
//! \param[in] const float startPoint[3] beginning position of arrow
//! \param[in] const float endPoint[3] ending position of arrow
//! \param[in] float radius Radius of arrow in voxels
	void drawArrow(const float startPoint[3], const float endPoint[3], float radius);

  };
};

#endif //ARROWRENDERER_H
