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
#include "vapor/MyBase.h"
#include "common.h"
#include "params.h"
using namespace VetsUtil;

namespace VAPoR {
class GLWindow;
class DataMgr;
class Metadata;

class RENDER_API Renderer: public QObject, public MyBase
{
     Q_OBJECT
	
	
public:
	//Constructor is called when the renderer is enabled.  Does any
	//setup of renderer state
	//
	Renderer(GLWindow* vw, RenderParams* rp);
	virtual ~Renderer();
	//Following are called by the glwindow class attached to the vizwin
	//as needed for rendering
	//
    virtual void	initializeGL() = 0;
    virtual void		paintGL() = 0;
	
	//Whenever the params associated with the renderer is changed, must call this:
	virtual void setRenderParams(RenderParams* rp) {currentRenderParams = rp;}
	RenderParams* getRenderParams(){return currentRenderParams;}
	bool isInitialized() {return initialized;}
	
signals:

    void statusMessage(const QString&);

protected:
	
	RenderParams* currentRenderParams;
	void buildColorscaleImage();
	void renderColorscale(bool rebuild);
	

	//Dimensions of colormap image:
	int imgWidth, imgHeight;
	QImage glColorbarImage;
	GLWindow* myGLWindow;
	
	float regionFrameColor[3];
	float subregionFrameColor[3];
	int savedNumXForms;
	bool initialized;
};
};

#endif // RENDERER_H
