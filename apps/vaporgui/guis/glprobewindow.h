//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		GLProbeWindow.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Definition of GLProbeWindow class: 
//	A GLProbeWindow object is embedded in probeframe.  
//  It paints a texture in the window.

#ifndef GLPROBEWINDOW_H
#define GLPROBEWINDOW_H

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <qgl.h>
#define CURSOR_COLOR 1.f,1.f,1.f
#define CURSOR_SIZE 0.05f
class ProbeFrame;

namespace VAPoR {



class GLProbeWindow : public QGLWidget
{

public:

    GLProbeWindow( const QGLFormat& fmt, QWidget* parent, const char* name, ProbeFrame*  );
    ~GLProbeWindow();
	
	void setTextureSize(float horiz, float vert);
	void mapPixelToProbeCoords(int ix, int iy, float* x, float* y);
	void setAnimatingTexture(bool val){animatingTexture = val; animatingFrameNum = 0;animationStarting=true;}
	void advanceAnimatingFrame(){animatingFrameNum++;}
	void setCaptureName(QString& name){captureName = name;}
	void setCapturing(bool doCapture){capturing = doCapture;}
	void setCaptureNum(int num) {captureNum = num;}

	


protected:

	
	//These methods cannot be overridden
	//
    void		initializeGL();
    void		paintGL();
    void		resizeGL( int w, int h );

	bool getPixelData(int minx, int miny, int sizex, int sizey,unsigned char* pixData);
	void doFrameCapture();

	//Size of probe in world coords.
	//This rectange will do its best to fill the probe space.
	float horizTexSize, vertTexSize;
	float rectLeft, rectTop;
	ProbeFrame* probeFrame;
	bool animatingTexture;
	int animatingFrameNum;
	bool animationStarting;
	int patternListNum;
	int currentAnimationTimestep;
	int captureNum;
	QString captureName;
	bool capturing;
	

};
};

#endif // GLWINDOW_H
