//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		volumizerrenderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:  Definition of VolumizerRenderer class	
//
// Code was extracted from mdb, setup, vox in order to do simple
// volume rendering with volumizer
//

/* Original Author:
 *	Ken Chin-Purcell (ken@ahpcrc.umn.edu)
 *	Army High Performance Computing Research Center (AHPCRC)
 *	University of Minnesota
 *
 */
#ifdef VOLUMIZER
#ifndef	VOLUMIZERRENDERER_H
#define	VOLUMIZERRENDERER_H

#include <stdio.h>
#include "vapor/DataMgr.h"
#include "DVRBase.h"
#include "renderer.h"

namespace VAPoR {
class DvrParams;
class VolumizerRenderer : public Renderer {
	Q_OBJECT

public:

    VolumizerRenderer(VizWin*);
    
	virtual void		initializeGL();
    virtual void		paintGL();


protected:
	DVRBase* create_driver(
		const char	*name,int	nthreads);
	/*
	void renderRegionBounds(float* extents, int selectedFace, 
		float* cameraPos, float faceDisplacement);
	*/

private:
	DVRBase* driver;
	void DrawVoxelScene(unsigned fast);
	void DrawVoxelWindow(unsigned fast);
	

	
	DvrParams* myDVRParams;
   

};
};
#endif	//	VOLUMIZERRENDERER_H
#endif //VOLUMIZER
