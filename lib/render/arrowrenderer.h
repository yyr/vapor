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

  class RENDER_API ArrowRenderer : public Renderer
  {
	
      
  public:
	
    ArrowRenderer(GLWindow *w, RenderParams* rp);
    virtual ~ArrowRenderer();
	virtual void initializeGL();
	virtual void paintGL();
	virtual void setAllDataDirty(){} //No need to do anything.

  protected:
	//Method that gets the required data from the DataMgr, while determining valid extents.
	//Returns the available refinement level, or -1 if needed data not available.
    int setupVariableData(
		vector<string>& varnames, RegularGrid *varData[4], double validExts[6], 
		size_t voxExts[6]
	);

	//Perform the OpenGL rendering
	void performRendering(
		int actualRefLevel,float vectorScale, float arrowRadius, 
		RegularGrid *variableData[4]
	);

	//Draw one arrow
	void drawArrow(const float startPoint[3], const float endPoint[3], float radius);

  private:


  };
};

#endif //ARROWRENDERER_H
