//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vizwin.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Sept 2004
//
//	Description:	Defines the VizWin class
//		This is the widget that contains the visualizers
//		Supports mouse event reporting
//		The actual GL rendering window is inserted as a frame into this
//

#ifndef VIZWIN_H
#define VIZWIN_H

//No more than 10 renderers in a window:
//Eventually this may be dynamic.
#define MAXNUMRENDERERS 10
#include <qvariant.h>
#include <qmainwindow.h>
#include "vizwinmgr.h"
#include "trackball.h"


class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QAction;
class QActionGroup;
class QToolBar;
class QPopupMenu;
class QResizeEvent;
class QHideEvent;
class QRect;
class QWorkspace;
class QHBoxLayout;

#include "glwindow.h"
namespace VAPoR {

class MainForm;
class VizWinMgr;
class GLWindow;
class Renderer;
class Viewpoint;

class VizWin : public QMainWindow
{
    Q_OBJECT

            
public:
    VizWin( QWorkspace* parent , const char* name , WFlags fl , VizWinMgr*  myMgr, QRect* location , int winNum);
    ~VizWin();

	void setWindowNum(int num) {
		myWindowNum = num;
	}
	int getWindowNum() {return myWindowNum;}
	bool isActiveWin(){
		return (myWindowNum == myWinMgr->getActiveViz());
	}
    //QToolBar *vizToolBar;
    
    //QAction* helpContentsAction;
    //QAction* helpIndexAction;
    //QAction* helpAboutAction;
	//QAction* homeAction;
	//QAction* sethomeAction;

	//Following flag is set whenever there is mouse navigation, so that we can get the viewer position
	//at the next rendering
	bool newViewerCoords;
	//bool newGuiCoords;
	//Following method is called when user has navigated
	void changeCoords(float *vpos, float* vdir, float* upvec);
	//Call this when the gui values need to update the visualizer:
	void setValuesFromGui(Viewpoint* vp);
	//Tell this visualizer to use global or local viewpoint:
	void setGlobalViewpoint(bool);
    //Force an update in the gl window:
	void updateGL() { if (myGLWindow) myGLWindow->update();}
	int getNumRenderers() { return numRenderers;}
	Renderer* renderer[MAXNUMRENDERERS];
	void setNumRenderers(int num) {numRenderers = num;}
	void addRenderer(Renderer* ren);
	void removeRenderer(const char* rendererName);
	GLWindow* getGLWindow() {return myGLWindow;}
	VizWinMgr* getWinMgr() {return myWinMgr;}
	void setRegionDirty(bool isDirty){ regionDirty = isDirty;}
	bool regionIsDirty() {return regionDirty;}
	bool mouseIsDown() {return mouseDownHere;}
	
	
protected:
	
	int numRenderers;
    QWorkspace* myParent;
    int myWindowNum;
    VizWinMgr* myWinMgr;
	Trackball* myTrackball;
	Trackball* localTrackball;
	//OpenGL widget for graphics:
	GLWindow* myGLWindow;
	
	//Indicate whether using global or local viewpoint:
	bool globalVP;
	//Indicate whether the region has been changed:
	bool regionDirty;
	

	
	

public slots:
    
    virtual void helpIndex();
    virtual void helpContents();
    virtual void helpAbout();
	virtual void setFocus();

	

protected:
	
	QPoint mouseDownPosition;
	bool mouseDownHere;


    //Event handling
	//Virtual overrides:
	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseReleaseEvent(QMouseEvent*);
	virtual void mouseMoveEvent(QMouseEvent*);
	virtual void focusInEvent(QFocusEvent* e);
    virtual void closeEvent(QCloseEvent*);
    virtual void hideEvent(QHideEvent*);
    virtual void windowActivationChange(bool oldActive);
    virtual void resizeEvent(QResizeEvent*);
    bool isReallyMaximized();
	


	
protected slots:
    virtual void languageChange();
    

};
};

#endif // VIZWIN_H

