//************************************************************************
//																		*
//		     Copyright (C)  2009										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		customContext.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		January 2009
//
//	Description:	Definition of the customContext class
//
#ifndef CUSTOMCONTEXT_H
#define CUSTOMCONTEXT_H
#include <qapplication.h>
#include <qgl.h>
#include <vapor/common.h>
#if defined(Q_WS_X11)  //for X11 platforms...
#include <GL/glx.h> 
#elif defined(Q_WS_MAC)
#include <agl.h>
#endif
namespace VAPoR {
	class RENDER_API CustomContext : public QGLContext 
	{
	public:
		CustomContext( const QGLFormat &fmt)
			: QGLContext(fmt) {}
	protected:
#ifdef WIN32
		int choosePixelFormat(void* p, HDC hdc);
#elif defined (Q_WS_X11)// X11
		void *chooseVisual();
#elif defined (Q_WS_MAC)
		void *chooseMacVisual(GDHandle gdev);
#else
		assert(0); //unsupported platform
#endif
	};

}; //end VAPoR namespace
#endif //CUSTOMCONTEXT_H
