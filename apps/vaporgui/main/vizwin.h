//************************************************************************
//											*
//		     Copyright (C)  2004						*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved						*
//											*
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
#include <qcolor.h>
#include <QCloseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QHideEvent>
#include "vizwinmgr.h"
#include "trackball.h"


class QSpacerItem;
class QAction;
class QResizeEvent;
class QHideEvent;
class QRect;

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

class VizWin : public QWidget
{
    Q_OBJECT

            
public:
    VizWin(MainForm* parent ,  const QString& name, Qt::WFlags fl , VizWinMgr*  myMgr, QRect* location , int winNum);
    ~VizWin();

	
	void setDirtyBit(DirtyBitType t, bool val){
		if(myGLWindow) myGLWindow->setDirtyBit(t,val);
	}
		
	bool vizIsDirty(DirtyBitType t){return myGLWindow->vizIsDirty(t);}
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
	
	GLWindow* getGLWindow() {return myGLWindow;}
	
	void setRegionDirty(bool isDirty){ setDirtyBit(RegionBit,isDirty);}
	void setAnimationDirty(bool isDirty){ setDirtyBit(AnimationBit,isDirty);}
	void setRegionNavigating(bool isDirty){ setDirtyBit(NavigatingBit,isDirty);}
	
	bool regionIsDirty() {return vizIsDirty(RegionBit);}

	
	bool regionIsNavigating() {return vizIsDirty(NavigatingBit);}
	
	void setMouseDown(bool downUp) {
		mouseDownHere = downUp;
		myGLWindow->setMouseDown(downUp);
	}
	bool mouseIsDown() {return mouseDownHere;}
	

	//Access visualizer features
	const QColor getBackgroundColor() {return DataStatus::getInstance()->getBackgroundColor();}
	const QColor getRegionFrameColor() {return DataStatus::getInstance()->getRegionFrameColor();}
	const QColor getSubregionFrameColor() {return DataStatus::getInstance()->getSubregionFrameColor();}
	QColor& getColorbarBackgroundColor() {return myGLWindow->getColorbarBackgroundColor();}
	const vector<bool> getColorbarEnabled() {return myGLWindow->getColorbarEnabled();}
	const vector<string> getColorbarTitles() {return myGLWindow->getColorbarTitles();}
	
	//Pass get/set onto glwindow:
	bool axisArrowsAreEnabled() {return myGLWindow->axisArrowsAreEnabled();}
	bool axisAnnotationIsEnabled() {return myGLWindow->axisAnnotationIsEnabled();}
	
	bool regionFrameIsEnabled() {return myGLWindow->regionFrameIsEnabled();}
	bool subregionFrameIsEnabled() {return myGLWindow->subregionFrameIsEnabled();}
	float getAxisArrowCoord(int i){return myGLWindow->getAxisArrowCoord(i);}
	double getAxisOriginCoord(int i){return myGLWindow->getAxisOriginCoord(i);}
	double getMinTic(int i){return myGLWindow->getMinTic(i);}
	double getMaxTic(int i){return myGLWindow->getMaxTic(i);}
	double getTicLength(int i){return myGLWindow->getTicLength(i);}
	int getNumTics(int i){return myGLWindow->getNumTics(i);}
	int getTicDir(int i){return myGLWindow->getTicDir(i);}
	int getLabelHeight(){return myGLWindow->getLabelHeight();}
	int getLabelDigits(){return myGLWindow->getLabelDigits();}
	float getTicWidth(){return myGLWindow->getTicWidth();}
	QColor& getAxisColor(){return myGLWindow->getAxisColor();}

	const vector<float> getColorbarLLX() {return myGLWindow->getColorbarLLX();}
	const vector<float> getColorbarLLY() {return myGLWindow->getColorbarLLY();}
	float getColorbarSize(int i) {return myGLWindow->getColorbarSize(i);}
	int getColorbarNumTics() {return myGLWindow->getColorbarNumTics();}
	int getColorbarDigits() {return myGLWindow->getColorbarDigits();}
	void setColorbarDigits(int ndigs) {myGLWindow->setColorbarDigits(ndigs);}
	int getColorbarFontsize() {return myGLWindow->getColorbarFontsize();}
	void setColorbarFontsize(int sz) {myGLWindow->setColorbarFontsize(sz);}


	void setBackgroundColor(QColor& c) {myGLWindow->setBackgroundColor(c);}
	void setColorbarBackgroundColor(QColor& c) {myGLWindow->setColorbarBackgroundColor( c);}
	void setRegionFrameColor(QColor& c) {myGLWindow->setRegionFrameColor(c);}
	void setSubregionFrameColor(QColor& c) {myGLWindow->setSubregionFrameColor(c);}
	void enableAxisArrows(bool enable) {myGLWindow->enableAxisArrows(enable);}
	void enableAxisAnnotation(bool enable) {myGLWindow->enableAxisAnnotation(enable);}
	
	void enableRegionFrame(bool enable) {myGLWindow->enableRegionFrame( enable);}
	void enableSubregionFrame(bool enable) {myGLWindow->enableSubregionFrame( enable);}
	void setAxisArrowCoord(int i, float val){myGLWindow->setAxisArrowCoord( i,  val);}
	void setAxisOriginCoord(int i, float val){myGLWindow->setAxisOriginCoord( i,  val);}
	void setMinTic(int i, float val){myGLWindow->setMinTic( i,  val);}
	void setMaxTic(int i, float val){myGLWindow->setMaxTic( i,  val);}
	void setTicLength(int i, float val){myGLWindow->setTicLength( i,  val);}
	void setNumTics(int i, int val){myGLWindow->setNumTics( i,  val);}
	void setTicDir(int i, int val){myGLWindow->setTicDir( i,  val);}
	void setTicWidth(float val) {myGLWindow->setTicWidth(val);}
	void setLabelHeight(int val) {myGLWindow->setLabelHeight(val);}
	void setLabelDigits(int val) {myGLWindow->setLabelDigits(val);}
	void setAxisColor(QColor& c) {myGLWindow->setAxisColor(c);}

	void setAxisExtents(const float* extents){
		for (int i = 0; i<3; i++){
			setMinTic(i, extents[i]);
			setMaxTic(i, extents[i+3]);
		}
		setTicLength(0, 0.05*(extents[4]-extents[1]));
		setTicLength(1, 0.05*(extents[3]-extents[0]));
		setTicLength(2, 0.05*(extents[3]-extents[0]));
	}
	void getAxisExtents(float* extents){
		for (int i = 0; i<3; i++){
			extents[i] = getMinTic(i);
			extents[i+3] = getMaxTic(i);
		}
	}
	bool useLatLonAnnotation() {return myGLWindow->useLatLonAnnotation();}
	void setLatLonAnnotation(bool val) {myGLWindow->setLatLonAnnotation(val);}
	
	void setColorbarEnabled(const vector<bool> vb) {myGLWindow->setColorbarEnabled(vb);}
	void setColorbarEnabled(int colorbarIndex, bool val){
		vector<bool> enabled = getColorbarEnabled();
		enabled[colorbarIndex] = val;
		setColorbarEnabled(enabled);
	}
	void setColorbarTitle(int colorbarIndex, string title){
		vector<string> titles = getColorbarTitles();
		titles[colorbarIndex] = title;
		setColorbarTitles(titles);
	}
	void setColorbarTitles(vector<string>titles){myGLWindow->setColorbarTitles(titles);}
	void setColorbarLLX(const vector<float> vf) {myGLWindow->setColorbarLLX( vf);}
	void setColorbarLLY(const vector<float> vf) {myGLWindow->setColorbarLLY( vf);}
	void setColorbarLLXY(int colorbarIndex, float llx, float lly){
		vector<float> LLX = getColorbarLLX();
		vector<float> LLY = getColorbarLLY();
		LLX[colorbarIndex] = llx;
		LLY[colorbarIndex] = lly;
		setColorbarLLX(LLX);
		setColorbarLLY(LLY);
	}
	void setColorbarSize(int i, float crd) {myGLWindow->setColorbarSize( i,  crd);}
	void setColorbarNumTics(int i) {myGLWindow->setColorbarNumTics( i);}

		
	void setTimeAnnotCoords(float crds[2]){myGLWindow->setTimeAnnotCoords(crds);}
	float getTimeAnnotCoord(int j) {return myGLWindow->getTimeAnnotCoord(j);}
	void setTimeAnnotColor(QColor c) {myGLWindow->setTimeAnnotColor(c);}
	QColor getTimeAnnotColor() {return myGLWindow->getTimeAnnotColor();}
	void setTimeAnnotType(int t) {myGLWindow->setTimeAnnotType(t);}
	int getTimeAnnotType(){ return myGLWindow->getTimeAnnotType();}
	void setTimeAnnotTextSize(int size){myGLWindow->setTimeAnnotTextSize(size);}
	int getTimeAnnotTextSize(){return myGLWindow->getTimeAnnotTextSize();}
	void setTimeAnnotDirty(){myGLWindow->setTimeAnnotDirty(true);}
	void setAxisLabelsDirty(){myGLWindow->setAxisLabelsDirty(true);}
	void setTextRenderersDirty(){myGLWindow->setTextRenderersDirty(true);}
	
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

	void endSpin();
public slots:
    //Force an update in the gl window:
	void updateGL() { 
		if (myGLWindow && !GLWindow::isRendering()) myGLWindow->update();
	}

	
protected:
	
    MainForm* myParent;
    int myWindowNum;
    	VizWinMgr* myWinMgr;
	Trackball* myTrackball;
	Trackball* localTrackball;
	//OpenGL widget for graphics:
	GLWindow* myGLWindow;

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
	virtual void focusInEvent(QFocusEvent* e);
    virtual void closeEvent(QCloseEvent*);
    virtual void hideEvent(QHideEvent*);
    virtual void windowActivationChange(bool oldActive);
    virtual void resizeEvent(QResizeEvent*);
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

