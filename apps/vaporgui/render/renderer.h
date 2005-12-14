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
#include <qimage.h>
class QColor;

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
	virtual ~Renderer() {}
	//Following are called by the glwindow class attached to the vizwin
	//as needed for rendering
	//
    virtual void		initializeGL() = 0;
    virtual void		paintGL() = 0;
	void setSelectedFace(int faceNum) {selectedFace = faceNum;}
	
protected:
	
	void buildColorscaleImage();
	void renderColorscale(bool rebuild);
	//One face of the region bounds can be highlighted if selected:
	int selectedFace;

	//Dimensions of colormap image:
	int imgWidth, imgHeight;
	QImage glColorbarImage;
	GLWindow* myGLWindow;
	VizWin* myVizWin;
	
	float regionFrameColor[3];
	float subregionFrameColor[3];
	int savedNumXForms;
	
};
};

#endif // RENDERER_H
