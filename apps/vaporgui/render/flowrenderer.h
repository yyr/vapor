//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		flowrenderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2005
//
//	Description:	Definition of the FlowRenderer class
//
#ifndef FLOWRENDERER_H
#define FLOWRENDERER_H

#include <qgl.h>

#include "renderer.h"
namespace VAPoR {
class VizWin;
class FlowRenderer : public Renderer
{
    Q_OBJECT

public:

    FlowRenderer( VizWin*  );
    ~FlowRenderer();
	
	virtual void		initializeGL();
    virtual void		paintGL();

protected:

	void buildFlowGeometry();


};
};

#endif // FLOWRENDERER_H
