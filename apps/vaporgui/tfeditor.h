//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		tfeditor.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Defines the TFEditor class.  This is the class
//		that manages a transfer function editing panel
//		This class provides Transfer Function editing capability.
//		One of these is created whenever the users requests to edit a
//		Transfer function.  One is associated with each DVR panel.
//		This also manages the editing session, responds to mouse events
//		Contains a number of definitions associated with the editor window,
//		controlling layout and colors.
//
#ifndef TFEDITOR_H
#define TFEDITOR_H
// distance for being "close" to a pixel
#define CLOSE_DISTANCE 4
//Layout parameters for bar
#define BARHEIGHT 15
//Separator is between color bar and opacity graph
#define SEPARATOR 4
//CoordMargin is at bottom 
#define COORDMARGIN 10
//Topmargin is between curve and top of image
#define TOPMARGIN 3
//space below opacity window
#define BELOWOPACITY (BARHEIGHT+SEPARATOR+COORDMARGIN)
//Factor for getting wider than specified window:
#define HORIZOVERLAP 0.01f
//Float constants to describe vertical regions:
#define ABOVEWINDOW 2.f
#define ONSEPARATOR -1.f
#define ONCOLORBAR -2.f
#define BELOWCOLORBAR -3.f
#define BELOWWINDOW -4.f

//COLORS:
#define SEPARATORCOLOR qRgb(64,64,64)
#define DEFAULTBACKGROUNDCOLOR qRgb(233,236,216)
#define HISTOCOLOR qRgb(0,200,200)
#define HIGHLIGHTCOLOR yellow
#define OPACITYCURVECOLOR white
#define CONTROLPOINTCOLOR darkGray
#define ENDLINECOLOR lightGray


class QImage;

#include "transferfunction.h"
class TFFrame;
namespace VAPoR {
class TransferFunction;
class TFInterpolator;
class Session;

class Histo;
class TFELocationTip;
class DVRParams;

class TFEditor{
public:
	TFEditor(DvrParams*, TransferFunction* TF, TFFrame*, Session* );
	~TFEditor();
	void setEditingRange(float minVal, float maxVal){
		minEditValue = minVal;
		maxEditValue = maxVal;
		dirty = true;
	}
	void setHistoStretch(float factor){histoStretchFactor = factor;}
	float getHistoStretch(){return histoStretchFactor;}
	void refreshImage();
	QImage* getImage(){
		return editImage;
	}
	
	bool isDirty(){ return dirty;}
	void setDirty(bool val) {dirty = val;}
	//Responses to mouse events:
	void moveGrabbedControlPoints(int x, int y);
	void moveDomainBound(int x);
	
	int insertColorControlPoint(int xcoord);
	int insertOpacControlPoint(int xcoord, int ycoord = -1);
	//Find where on the image are two versions of a specific control point
	void getOpacControlPointPosition(int index, int* xpos, int* ypos);
	void getColorControlPointPosition(int index, int* xpos);
	int getNumColorControlPoints() {return myTransferFunction->getNumColorControlPoints();}
	int getNumOpacControlPoints() {return myTransferFunction->getNumOpacControlPoints();}
	QRgb getControlPointColor(int index){
		return myTransferFunction->getControlPointRGB(index);
	}
	int getBarHeight(){ return BARHEIGHT;}
	int closestControlPoint(int x, int y, int* index);
	void unSelectAll();
	void selectInterval(bool colorPoint);
	bool colorSelected(int i) {return selectedColor[i];}
	bool opacSelected(int i) {return selectedOpac[i];}
	void selectColor(int index) {
		if (!selectedColor[index]) {
			selectedColor[index] = true;
			numColorSelect++;
		}
	}
	void unselectColor(int index) {
		if (selectedColor[index]) {
			selectedColor[index] = false;
			numColorSelect--;
		}
	}
	void selectOpac(int index) {
		if (!selectedOpac[index]) {
			selectedOpac[index] = true;
			numOpacSelect++;
		}
	}
	void unselectOpac(int index) {
		if (selectedOpac[index]) {
			selectedOpac[index] = false;
			numOpacSelect--;
		}
	}
	void deleteControlPoint(int pointNum, bool colorPoint);
	void deleteSelectedControlPoints();
	//Set all selected color control points to an HSV:
	void setHsv(int h, int s, int v);
	//Coordinate mapping functions:
	//map TF variable to window coords.  
	// if classify, Left goes to -1, right goes to width.
	int mapVar2Win(float x, bool classify = false);
	//map window to variable, can go to any value
	float mapWin2Var(int x);
	//map window to discrete mapping value (0..2**nbits - 1)
	//if truncate, limits are mapped to ending discrete value
	int  mapWin2Discrete(int x, bool truncate = false);
	//map variable to discrete, optionally limits to end values
	int mapVar2Discrete(float x);
	
	//Map verticalWin to opacity, optionally  special constants
	float mapWin2Opac(int y, bool classify = false);
	//map opacity to window position, optionally truncate to valid
	int mapOpac2Win(float op, bool truncate = false);

	//Map a screen horiz coordinate to float.  Results in a
	//value between minEditValue and maxEditValue.
	//float mapScreenXCoord(int x);
	//Map screen vert coord to Y.  Results in 0..1 for opacity coords,
	// -1 for color coords
	//float mapScreenYCoord(int y);
	float getMinEditValue(){return minEditValue;}
	float getMaxEditValue(){return maxEditValue;}
	void setMinEditValue(float val) {
		minEditValue = val;
	}
	void setMaxEditValue(float val) {
		maxEditValue = val;
	}
	TransferFunction* getTransferFunction() {return myTransferFunction;}
	void setTransferFunction(TransferFunction* tf) {myTransferFunction = tf;}
	//Currently histogram is assumed to be limited to interval [0,1]:
	int getHistoValue(float point);

	//Public methods for controlling grabbedState:
	void unGrab() {grabbedState = notGrabbed;}
	void unGrabBounds() {grabbedState &= ~(leftDomainGrab|rightDomainGrab);}
	void removeConstrainedGrab() {grabbedState &= ~allConstrainedGrabs;}
	void addConstrainedGrab(){grabbedState |= constrainedGrab;}
	void addOpacGrab(){grabbedState |= opacGrab;}
	void addColorGrab() {grabbedState |= colorGrab;}
	void removeColorGrab() {grabbedState &= ~colorGrab;}
	void removeOpacGrab(){grabbedState &= ~opacGrab;}
	void addLeftDomainGrab() {grabbedState = leftDomainGrab;}
	void addRightDomainGrab() {grabbedState = rightDomainGrab;}
	int numColorSelected() { return numColorSelect;}
	int numOpacSelected() {return numOpacSelect;}
	bool isGrabbed() {return (grabbedState != notGrabbed);}
	bool domainGrabbed() {return grabbedState&(leftDomainGrab|rightDomainGrab);}
	bool leftDomainGrabbed() {return grabbedState & leftDomainGrab;}
	bool rightDomainGrabbed() {return grabbedState & rightDomainGrab;}
	bool canBind() {return (numColorSelected()==1 && numOpacSelected()==1);}
	void bindOpacToColor(); 
	void bindColorToOpac();
	//Mouse-down event starts with this:
	void setDragStart(int ix, int iy){
		dragStartX = ix;
		dragStartY = iy;
		leftMoveMax = -1;
	}
	DvrParams* getParams(){ return myParams;}
	void setParams(DvrParams* p) {myParams = p;}
	Session* getSession() {return session;}

	
protected:
	//grabbedState is "or" of some of following masks.
	//Whenever a new mouse-down event happens, it must reset
	//allConstrainedGrabs.  When mouse down occurs without
	//ctrl or shift keys, it resets opacGrab and colorGrab.
	//left and right domain grabs remove all other grabs
	enum grabType {
		notGrabbed = 0,
		colorGrab = 1,
		opacGrab = 2,
		constrainedGrab = 4,
		verticalGrab = 8,
		horizontalGrab = 16,
		allConstrainedGrabs = 28,
		leftDomainGrab = 32,
		rightDomainGrab = 64
	};
	unsigned int grabbedState;
	//Booleans to indicate what is currently selected;
	//Not really part of transfer function definition
	bool selectedColor[MAXCONTROLPOINTS];
	bool selectedOpac[MAXCONTROLPOINTS];
	int numColorSelect;
	int numOpacSelect;
	QImage* editImage;
	//QImage* opacImage;
	//The min and max values are established in the parent dialog.
	//They must lie within range of transfer function definition.
	float minEditValue;
	float maxEditValue;
	int height;
	int width;
	TransferFunction* myTransferFunction;
	bool dirty;
	TFFrame* myFrame;
	DvrParams* myParams;
	Session* session;
	float histoStretchFactor;
	int dragStartX, dragStartY;
	//Bounds for current drag.  dragMaxX = -1000000.f if not set:
	//float dragMinX, dragMaxX, dragMinY, dragMaxY;
	//Bounds expressed in terms of relative screen coords
	//leftMoveMax = -1 if not set (prior to mouse move)
	//Note that an Up move corresponds to decreasing y coords!
	int leftMoveMax, rightMoveMax, upMoveMax, downMoveMax;
	int histoMaxBin;
	
};
};


#endif
