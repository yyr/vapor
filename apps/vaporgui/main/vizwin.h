//************************************************************************
//																		*
//		     Copyright (C)  2004										*
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
//	Date:		Sept 2004
//
//	Description:	Defines the VizWin class
//		This is the widget that contains the visualizers
//		Supports mouse event reporting
//		The actual GL rendering window is inserted as a frame into this
//

#ifndef VIZWIN_H
#define VIZWIN_H


#include <qvariant.h>
#include <qmainwindow.h>
#include <qcolor.h>
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
#include "animationcontroller.h"
namespace VAPoR {

class MainForm;
class VizWinMgr;
class GLWindow;
class Renderer;
class FlowRenderer;
class Viewpoint;

class TranslateStretchManip;
class TranslateRotateManip;

class VizWin : public QMainWindow
{
    Q_OBJECT

            
public:
    VizWin( QWorkspace* parent , const char* name , WFlags fl , VizWinMgr*  myMgr, QRect* location , int winNum);
    ~VizWin();

	
	void setDirtyBit(DirtyBitType t, bool val){
		myGLWindow->setDirtyBit(t,val);
	}
		
	bool vizIsDirty(DirtyBitType t){return myGLWindow->vizIsDirty(t);}
	void setWindowNum(int num) {
		myWindowNum = num;
	}
	int getWindowNum() {return myWindowNum;}
	bool isActiveWin(){
		return (myWindowNum == myWinMgr->getActiveViz());
	}

	//bool newGuiCoords;
	//Following method is called when user has navigated
	void changeCoords(float *vpos, float* vdir, float* upvec);
	//Call this when the gui values need to update the visualizer:
	void setValuesFromGui(ViewpointParams* vparams);
	//Tell this visualizer to use global or local viewpoint:
	void setGlobalViewpoint(bool);
	
	
	GLWindow* getGLWindow() {return myGLWindow;}
	
	
	void setRegionDirty(bool isDirty){ setDirtyBit(RegionBit,isDirty);}
	void setAnimationDirty(bool isDirty){ setDirtyBit(AnimationBit,isDirty);}
	//void setDvrClutDirty(bool isDirty){ setDirtyBit(Params::DvrParamsType,DvrClutBit, isDirty);}
	
	//void setDvrDatarangeDirty(bool isDirty){ setDirtyBit(Params::DvrParamsType,DvrDatarangeBit,isDirty);}
	void setRegionNavigating(bool isDirty){ setDirtyBit(DvrRegionBit,isDirty);}
	//void setFlowDataDirty(bool isDirty) {setDirtyBit(Params::FlowParamsType,FlowDataBit,isDirty);}
	//void setFlowGraphicsDirty(bool isDirty) {setDirtyBit(Params::FlowParamsType,FlowGraphicsBit,isDirty);}
	
	bool regionIsDirty() {return vizIsDirty(RegionBit);}

	//bool dvrClutIsDirty(DvrParams* ) {return vizIsDirty(DvrClutBit);}
	//bool dvrDatarangeIsDirty(DvrParams* ) {return vizIsDirty(DvrDatarangeBit);}
	//bool flowDataIsDirty() {return vizIsDirty(FlowDataBit);}
	//bool flowGraphicsIsDirty() {return vizIsDirty(FlowGraphicsBit);}

	bool regionIsNavigating() {return vizIsDirty(DvrRegionBit);}
	
	void setMouseDown(bool downUp) {
		mouseDownHere = downUp;
		myGLWindow->setMouseDown(downUp);
	}
	bool mouseIsDown() {return mouseDownHere;}
	int pointOverCube(RegionParams* rParams, float screenCoords[2]);
	int pointOverCube(FlowParams* rParams, float screenCoords[2]);
/* Following all moved to GLWindow:
	//Determine the approximate size of a pixel in terms of viewer coordinates.
	float getPixelSize();
	bool viewerCoordsChanged() {return newViewerCoords;}
	void setViewerCoordsChanged(bool isNew) {newViewerCoords = isNew;}
	bool isCapturing() {return (capturing != 0);}
	bool isSingleCapturing() {return (capturing == 1);}
	void startCapture(QString& name, int startNum) {
		capturing = 2;
		captureNum = startNum;
		captureName = name;
		newCapture = true;
		updateGL();
	}
	void singleCapture(QString& name){
		capturing = 1;
		captureName = name;
		newCapture = true;
		updateGL();
	}
	bool captureIsNew() { return newCapture;}
	void setCaptureNew(bool isNew){ newCapture = isNew;}
	void stopCapture() {capturing = 0;}
	//Routine is called at the end of rendering.  If capture is 1 or 2, it converts image
	//to jpeg and saves file.  If it ever encounters an error, it turns off capture.
	//If capture is 1 (single frame capture) it turns off capture.
	void doFrameCapture();
*/
	//Access visualizer features
	
	QColor& getBackgroundColor() {return myGLWindow->getBackgroundColor();}
	QColor& getRegionFrameColor() {return myGLWindow->getRegionFrameColor();}
	QColor& getSubregionFrameColor() {return myGLWindow->getSubregionFrameColor();}
	QColor& getColorbarBackgroundColor() {return myGLWindow->getColorbarBackgroundColor();}
	
	//Pass get/set onto glwindow:
	bool axesAreEnabled() {return myGLWindow->axesAreEnabled();}
	bool colorbarIsEnabled() {return myGLWindow->colorbarIsEnabled();}
	bool regionFrameIsEnabled() {return myGLWindow->regionFrameIsEnabled();}
	bool subregionFrameIsEnabled() {return myGLWindow->subregionFrameIsEnabled();}
	float getAxisCoord(int i){return myGLWindow->getAxisCoord(i);}
	float getColorbarLLCoord(int i) {return myGLWindow->getColorbarLLCoord(i);}
	float getColorbarURCoord(int i) {return myGLWindow->getColorbarURCoord(i);}
	float getColorbarNumTics() {return myGLWindow->getColorbarNumTics();}

	void setBackgroundColor(QColor& c) {myGLWindow->setBackgroundColor(c);}
	void setColorbarBackgroundColor(QColor& c) {myGLWindow->setColorbarBackgroundColor( c);}
	void setRegionFrameColor(QColor& c) {myGLWindow->setRegionFrameColor(c);}
	void setSubregionFrameColor(QColor& c) {myGLWindow->setSubregionFrameColor(c);}
	void enableAxes(bool enable) {myGLWindow->enableAxes(enable);}
	void enableColorbar(bool enable) {myGLWindow->enableColorbar( enable) ;}
	void enableRegionFrame(bool enable) {myGLWindow->enableRegionFrame( enable);}
	void enableSubregionFrame(bool enable) {myGLWindow->enableSubregionFrame( enable);}
	void setAxisCoord(int i, float val){myGLWindow->setAxisCoord( i,  val);}
	void setColorbarLLCoord(int i, float crd) {myGLWindow->setColorbarLLCoord( i,  crd);;}
	void setColorbarURCoord(int i, float crd) {myGLWindow->setColorbarURCoord( i,  crd);}
	void setColorbarNumTics(int i) {myGLWindow->setColorbarNumTics( i);}
	
	bool colorbarIsDirty() {return myGLWindow->colorbarIsDirty();}
	void setColorbarDirty(bool val){myGLWindow->setColorbarDirty(val);}
	
	static bool preRenderSetup(int viznum, bool newCoords){
		if (newCoords) {
			VizWinMgr::getInstance()->getVizWin(viznum)->changeViewerFrame();
		}
		return AnimationController::getInstance()->beginRendering(viznum);
		
	}
	static bool endRender(int viznum, bool isControlled){
		if(isControlled) AnimationController::getInstance()->endRendering(viznum);
		return VizWinMgr::getInstance()->getVizWin(viznum)->mouseIsDown();
	}

public slots:
    //Force an update in the gl window:
	void updateGL() { if (myGLWindow) myGLWindow->update();}

	
protected:
	
    QWorkspace* myParent;
    int myWindowNum;
    VizWinMgr* myWinMgr;
	Trackball* myTrackball;
	Trackball* localTrackball;
	//OpenGL widget for graphics:
	GLWindow* myGLWindow;

	//Method that gets the coord frame from GL, 
	//causes an update of the viewpoint params
	//
	void	changeViewerFrame();
	
	//Indicate whether using global or local viewpoint:
	bool globalVP;
	/*
	int capturing;
	int captureNum;
	//Flag to set indicating start of capture sequence.
	bool newCapture;
	QString captureName;

	*/
	
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

