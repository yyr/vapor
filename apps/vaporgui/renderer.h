//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************///
//	File:		renderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Defines the Renderer class.
//		A pure virtual class that is implemented for each renderer.
//		Methods are called by the glwindow class as needed.
//

#ifndef RENDERER_H
#define RENDERER_H
#include <qobject.h>

namespace VAPoR {
class VizWin;
class GLWindow;
class DataMgr;
class RegionParams;
class ViewpointParams;
class Metadata;

class Renderer: public QObject
{
     Q_OBJECT
	
public:
	//Constructor is called when the renderer is enabled.  Does any
	//setup of renderer state
	//
	Renderer(VizWin* vw);
	//Following are called by the glwindow class attached to the vizwin
	//as needed for rendering
	//
    virtual void		initializeGL() = 0;
    virtual void		paintGL() = 0;
	
protected:
	GLWindow* myGLWindow;
	VizWin* myVizWin;
	//There can exist only one DataMgr during the life of this renderer:
	//
	DataMgr* myDataMgr;
	Metadata* myMetadata;
	//Rendering-related parameters (from Bob_App) come from region etc.
	//These should be reset whenever the region parameters are dirtied.
	//
	RegionParams* myRegionParams;
	ViewpointParams* myViewpointParams;
};
};

#endif // RENDERER_H
