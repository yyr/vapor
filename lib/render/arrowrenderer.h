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


//! Virtual method to force rebuilding of any caches associated with this renderer.
//! The ArrowRender has no caches so this method does nothing.
    virtual void setAllDataDirty(){} //No need to do anything.

  protected:

//! Virtual method performs any OpenGL initialization, sets the initialized flag
    virtual int _initializeGL();

//! Virtual method issues all the OpenGL calls to draw the arrows in user coordinates. 
    virtual int _paintGL();

//! Protected method that gets the required data from the DataMgr, while determining valid extents.
//! \param[in] vector<string>& varnames Names of variables defining field
//! \param[out] RegularGrid* RegularGrids obtained from the DataMgr
//! \param[out] double validExts[6] Extents of valid data 
//! \param[out] size_t voxExts[6] Voxel extents of valid data
//! \return int refinement level of data, or -1 if unavailable
    int setupVariableData(
		vector<string>& varnames, RegularGrid *varData[4], double validExts[6], 
		size_t voxExts[6]
	);

//! Protected method that performs rendering of all arrows.
//! \param[in] int actualRefLevel refinement level to be rendered.
//! \param[in] float vectorScale Scale factor to be applied to arrows.
//! \param[in] float arrowRadius Radius of arrows in voxel diameters.
//! \param[in] RegularGrid*[4] RegularGrids used in rendering
	void performRendering(
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
