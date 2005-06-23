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

//No more than 1 renderers in a window:
//Eventually this may be dynamic.
#define MAXNUMRENDERERS 1
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

	//bool newGuiCoords;
	//Following method is called when user has navigated
	void changeCoords(float *vpos, float* vdir, float* upvec);
	//Call this when the gui values need to update the visualizer:
	void setValuesFromGui(ViewpointParams* vparams);
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
	
	void setRegionDirty(bool isDirty){ regionDirty = isDirty;}
	void setClutDirty(bool isDirty){ clutDirty = isDirty;}
	void setDataRangeDirty(bool isDirty){ dataRangeDirty = isDirty;}
	bool regionIsDirty() {return regionDirty;}
	bool clutIsDirty() {return clutDirty;}
	bool dataRangeIsDirty() {return dataRangeDirty;}

	bool mouseIsDown() {return mouseDownHere;}
	int pointOverCube(RegionParams* rParams, float screenCoords[2]);
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

	//Access visualizer features
	void setBackgroundColor(QColor& c) {backgroundColor = c;}
	QColor& getBackgroundColor() {return backgroundColor;}
	QColor& getRegionFrameColor() {return regionFrameColor;}
	QColor& getSubregionFrameColor() {return subregionFrameColor;}
	bool axesAreEnabled() {return axesEnabled;}
	bool colorbarIsEnabled() {return colorbarEnabled;}
	bool regionFrameIsEnabled() {return regionFrameEnabled;}
	bool subregionFrameIsEnabled() {return subregionFrameEnabled;}
	float getAxisCoord(int i){return axisCoord[i];}
	int getColorbarCoord(int i) {return colorbarCoord[i];}

	void setRegionFrameColor(QColor& c) {regionFrameColor = c;}
	void setSubregionFrameColor(QColor& c) {subregionFrameColor = c;}
	void enableAxes(bool enable) {axesEnabled = enable;}
	void enableColorbar(bool enable) {colorbarEnabled = enable;}
	void enableRegionFrame(bool enable) {regionFrameEnabled = enable;}
	void enableSubregionFrame(bool enable) {subregionFrameEnabled = enable;}
	void setAxisCoord(int i, float val){axisCoord[i] = val;}
	void setColorbarCoord(int i, int crd) {colorbarCoord[i] = crd;}
	
protected:
	//Following flag is set whenever there is mouse navigation, so that we can use 
	//the new viewer position
	//at the next rendering
	bool newViewerCoords;
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
	bool dataRangeDirty;
	bool clutDirty;
	int capturing;
	int captureNum;
	//Flag to set indicating start of capture sequence.
	bool newCapture;
	QString captureName;

	//values in vizFeature
	QColor backgroundColor;
	QColor regionFrameColor;
	QColor subregionFrameColor;
	bool axesEnabled;
	bool regionFrameEnabled;
	bool subregionFrameEnabled;
	bool colorbarEnabled;
	float axisCoord[3];
	int colorbarCoord[2];
	
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

