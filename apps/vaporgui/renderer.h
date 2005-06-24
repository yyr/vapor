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
	//Set colors to use in bound rendering:
	void setSubregionFrameColor(QColor& c);
	void setRegionFrameColor(QColor& c);
	//Draw the region bounds and frame it in full domain.
	//Arguments are in unit cube coordinates
	void renderDomainFrame(float* extents, float* minFull, float* maxFull);
	void renderRegionBounds(float* extents, int selectedFace, 
		float* cameraPos, float faceDisplacement);
	void drawSubregionBounds(float* extents);
	void drawAxes(float* extents);
	//One face of the region bounds can be highlighted if selected:
	int selectedFace;

	GLWindow* myGLWindow;
	VizWin* myVizWin;
	
	float regionFrameColor[3];
	float subregionFrameColor[3];

	//Helper functions for drawing region bounds:
	static float* cornerPoint(float* extents, int faceNum);
	// Faces of the cube are numbered 0..5 based on view from pos z axis:
	// back, front, bottom, top, left, right
	static bool faceIsVisible(float* extents, float* viewerCoords, int faceNum);
	void drawRegionFace(float* extents, int faceNum, bool isSelected);
};
};

#endif // RENDERER_H
