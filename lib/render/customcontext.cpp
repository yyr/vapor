//************************************************************************
//																		*
//		     Copyright (C)  2009										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		customContext.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		January 2009
//
//	Description:	Implementation of the customContext class
//		This extents the QGLContext, so that we can request
//		the context to have an Aux Buffer (not present in Mac OS10.5.6)
//
#include "customcontext.h"
using namespace VAPoR;
#ifdef WIN32
int CustomContext::choosePixelFormat(void*p, HDC pdc){
	//Nothing special for windows.

	return QGLContext::choosePixelFormat(p,pdc);
}
#elif defined (Q_WS_X11)
void *CustomContext::chooseVisual() {
	GLint attribs[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_AUX_BUFFERS, 1, None};
	XVisualInfo *vis = glXChooseVisual(device()->x11Display(), device()->x11Screen(), attribs);
	
	if (vis) {
		GLint numBufs = 0; 
		//Check if we got it:
		glXGetConfig(device()->x11Display(), vis, GLX_AUX_BUFFERS, &numBufs);
		if (numBufs < 1) {
			qWarning("no Aux buffers");
			assert(0);
		}
		return vis;
	}
	qWarning( "Null returned from glXChooseVisual");
	assert(0); //Didn't work!!
	return QGLContext::chooseVisual();
}
#endif
#if defined(Q_WS_MAC)
void *CustomContext::chooseMacVisual(GDHandle gdev) {
	GLint attribs[] = {AGL_ALL_RENDERERS, AGL_RGBA, AGL_AUX_BUFFERS, 1, AGL_NONE};
	AGLPixelFormat fmt = aglChoosePixelFormat(NULL, 0, attribs);
	if (fmt) {
		GLint numBufs = 0; 
		//Check if we got it:
		aglDescribePixelFormat(fmt, AGL_AUX_BUFFERS, &numBufs);
		QString txt(" Number of aux buffers: ");
		txt += QString::number(numBufs);
		QMessageBox::information(0,"Number of Aux buffers",txt, QMessageBox::Ok);
		return fmt;
	}
	assert(0); //Didn't work!!
	return QGLContext::chooseMacVisual(gdev);
}
#endif
