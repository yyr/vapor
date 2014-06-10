//************************************************************************
//																		*
//		     Copyright (C)  2013										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vizwin.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2013
//

//

#ifndef VIZWIN_H
#define VIZWIN_H

#include <QGLWidget>
#include "vizwinmgr.h"
#include "visualizer.h"
#include <QWheelEvent>

class QCloseEvent;
class QRect;
class QMouseEvent;
class QFocusEvent;

namespace VAPoR {
//!
//! \defgroup Public VAPOR Developer API
//!
class MainForm;
class VizWinMgr;
class Visualizer;
class Viewpoint;

//! \class VizWin
//! \ingroup Public
//! \brief A QGLWidget that supports display based on GL methods invoked in a Visualizer
//! \author Alan Norton
//! \version 3.0
//! \date    October 2013
//!	The VizWin class is a QGLWidget that supports the rendering by the VAPOR Visualizer class.
//! The standard rendering methods (resize, initialize, paint) are passed to the Visualizer.
//! In addition this is the class that responds to mouse events, resulting in scene navigation 
//! or manipulator changes.
//! 
class VizWin : public QGLWidget
{
    Q_OBJECT
   
public:

	//! Specify the index of this window, which is also the Visualizer index as produced by ControlExec::NewVisualizer().
	//! \param[in] visIndex visualizer index.
	void setWindowNum(int visIndex) {
		myWindowNum = visIndex;
	}
	//! Identify the visualizer index
	//! \retval visualizer index.
	int getWindowNum() {return myWindowNum;}
	
	//! Obtain the visualizer instance that corresponds to this VizWin
	//! \retval Visualizer instance that is embedded in this VizWin
	Visualizer* getVisualizer() {return myVisualizer;}
	
	//! Force the window to update, even if nothing has changed.
	void reallyUpdate();
//! @name Internal
//! Internal methods not intended for general use
//!
///@{
	VizWin(MainForm* parent ,  const QString& name, Qt::WFlags fl , VizWinMgr*  myMgr, QRect* location , int winNum);
    ~VizWin();
///@}
#ifndef DOXYGEN_SKIP_THIS
public slots:
	virtual void setFocus();

private:
	virtual QSize minimumSizeHint() const 
		{return QSize(400,400);}
	
	QPoint mouseDownPosition;
	bool mouseDownHere;
    //Event handling
	//Virtual overrides:
	virtual void wheelEvent(QWheelEvent* e){
		e->accept();
	}
	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseReleaseEvent(QMouseEvent*);
	virtual void mouseMoveEvent(QMouseEvent*);
	virtual void focusInEvent(QFocusEvent* e);
    virtual void closeEvent(QCloseEvent*);
   
    virtual void resizeGL(int width, int height);
	virtual void initializeGL();
	void paintGL();
	void setMouseDown(bool downUp) {
		mouseDownHere = downUp;
		myVisualizer->setMouseDown(downUp);
	}
	bool mouseIsDown() {return mouseDownHere;}

	
    int myWindowNum;
    VizWinMgr* myWinMgr;
	Visualizer* myVisualizer;
	//Variables to control spin animation: 
	QTime* spinTimer;
	int elapsedTime; //Time since mouse press event in navigation mode
	int moveCount; //number of mouse move events since mouse press
	int moveCoords[2];  //position at last move event during rotation navigation
	int moveDist;  //Distance between last two mouse move events
	int latestMoveTime; //most recent time of move
	int olderMoveTime; //time of move before the latest
#endif //DOXYGEN_SKIP_THIS
};
};

#endif // VIZWIN_H

