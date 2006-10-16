//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		proberenderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2006
//
//	Description:	Definition of the ProbeRenderer class
//
#ifndef PROBERENDERER_H
#define PROBERENDERER_H

#include "assert.h"
#include "renderer.h"
#include "probeparams.h"
namespace VAPoR {

class RENDER_API ProbeRenderer : public Renderer
{

public:

    ProbeRenderer( GLWindow* , ProbeParams* );
    ~ProbeRenderer();
	
	virtual void		initializeGL();
    virtual void		paintGL();


protected:


};
};

#endif // PROBERENDERER_H
