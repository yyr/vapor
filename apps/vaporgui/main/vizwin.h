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
//	Description:	Defines the VizWin class
//		This is a QGL widget for OpenGL rendering
//		Supports mouse event reporting
//		The GL rendering calls are performed by the associated Visualizer class in the render lib
//

#ifndef VIZWIN_H
#define VIZWIN_H


#include <qvariant.h>
#include <qcolor.h>
#include <QCloseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QHideEvent>
#include <QGLWidget>

#include "vizwinmgr.h"

class QSpacerItem;
class QAction;
class QResizeEvent;
class QHideEvent;
class QRect;

#include "visualizer.h"

namespace VAPoR {

class MainForm;
class VizWinMgr;
class Visualizer;
class Renderer;
class FlowRenderer;
class Viewpoint;



class VizWin : public QGLWidget
{
    Q_OBJECT
   
public:
    VizWin(MainForm* parent ,  const QString& name, Qt::WFlags fl , VizWinMgr*  myMgr, QRect* location , int winNum);
    ~VizWin();
	
	void setWindowNum(int num) {
		myWindowNum = num;
	}
	int getWindowNum() {return myWindowNum;}
	bool isActiveWin(){
		return (myWindowNum == myWinMgr->getActiveViz());
	}

	//Following method is called when user has navigated
	void changeCoords(float *vpos, float* vdir, float* upvec);

	//Call this when the gui values need to update the visualizer:
	void setValuesFromGui(ViewpointParams* vparams);

	//Tell this visualizer to use global or local viewpoint:
	void setGlobalViewpoint(bool);
	
	Visualizer* getVisualizer() {return myVisualizer;}
	
	void setMouseDown(bool downUp) {
		mouseDownHere = downUp;
		myVisualizer->setMouseDown(downUp);
	}
	bool mouseIsDown() {return mouseDownHere;}

	static bool preRenderSetup(int viznum, bool newCoords){
		if (newCoords) {
			VizWinMgr::getInstance()->getVizWin(viznum)->changeViewerFrame();
		}
		//return AnimationController::getInstance()->beginRendering(viznum);
		return true;
	}
	static bool endRender(int viznum, bool isControlled){
		//if(isControlled) AnimationController::getInstance()->endRendering(viznum);
		return VizWinMgr::getInstance()->getVizWin(viznum)->mouseIsDown();
	}

    //Force an update in the gl window:
public slots:
	void updateGL();
		
protected:
	
    MainForm* myParent;
    int myWindowNum;
    VizWinMgr* myWinMgr;
	
	Visualizer* myVisualizer;

	//Method that gets the coord frame from GL, 
	//causes an update of the viewpoint params
	void	changeViewerFrame();
	
	//Indicate whether using global or local viewpoint:
	bool globalVP;
	
	
public slots:
    
    virtual void helpIndex();
    virtual void helpContents();
    virtual void helpAbout();
	virtual void setFocus();

protected:
	virtual QSize minimumSizeHint() const 
		{return QSize(400,400);}
	QString myName;
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
	//virtual void focusInEvent(QFocusEvent* e);
    virtual void closeEvent(QCloseEvent*);
    virtual void hideEvent(QHideEvent*);
    virtual void windowActivationChange(bool oldActive);
    virtual void resizeGL(int width, int height);
	virtual void initializeGL();
    bool isReallyMaximized();
	//Variables to control spin animation: 
	QTime* spinTimer;
	int elapsedTime; //Time since mouse press event in navigation mode
	int moveCount; //number of mouse move events since mouse press
	int moveCoords[2];  //position at last move event during rotation navigation
	int moveDist;  //Distance between last two mouse move events
	int latestMoveTime; //most recent time of move
	int olderMoveTime; //time of move before the latest
		
protected slots:
    virtual void languageChange();
    

};
};

#endif // VIZWIN_H

