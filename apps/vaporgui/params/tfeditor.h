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


class QImage;
#include "dvrparams.h"
#include "transferfunction.h"
#include "mapeditor.h"
class TFFrame;
namespace VAPoR {
class TransferFunction;
class TFInterpolator;
class Session;

class Histo;
class TFELocationTip;
class DVRParams;

class TFEditor : public MapEditor {
public:
	TFEditor(MapperFunction* TF, TFFrame*);
	~TFEditor();
	//Reset to default state, e.g. when loading new TF:
	//
	virtual void reset();
	void setEditingRange(float minVal, float maxVal){
		setMinColorEditValue(minVal);
		setMaxColorEditValue(maxVal);
		setMinOpacEditValue(minVal);
		setMaxOpacEditValue(maxVal);
	}
	
	void refreshImage();
	QImage* getImage(){
		return editImage;
	}
	
	
	virtual void setDirty();
	//Responses to mouse events:
	void moveGrabbedControlPoints(int x, int y);
	void moveDomainBound(int x);
	
	int insertColorControlPoint(int xcoord);
	int insertOpacControlPoint(int xcoord, int ycoord = -1);
	//Find where on the image are two versions of a specific control point
	virtual void getOpacControlPointPosition(int index, int* xpos, int* ypos);
	virtual void getColorControlPointPosition(int index, int* xpos);
	int getNumColorControlPoints() {return myMapperFunction->getNumColorControlPoints();}
	int getNumOpacControlPoints() {return myMapperFunction->getNumOpacControlPoints();}
	
	int getBarHeight(){ return COLORBARWIDTH;}
	
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
	int mapVar2Win(float x, bool classify = false) 
	{return mapOpacVar2Win(x,classify);}
	//map window to variable, can go to any value
	float mapWin2Var(int x)
	{return mapOpacWin2Var(x);}
	//map window to discrete mapping value (0..2**nbits - 1)
	//if truncate, limits are mapped to ending discrete value
	int  mapWin2Discrete(int x, bool truncate = false)
	{return mapOpacWin2Discrete(x, truncate);}
	//map variable to discrete, optionally limits to end values
	int mapVar2Discrete(float x)
	{return mapOpacVar2Discrete(x);}
	
	//Map verticalWin to opacity, optionally  special constants
	virtual float mapWin2Opac(int y, bool classify = false);
	//map opacity to window position, optionally truncate to valid
	virtual int mapOpac2Win(float op, bool truncate = false);

	//Map a screen horiz coordinate to float.  Results in a
	//value between minEditBound and maxEditBound.
	//float mapScreenXCoord(int x);
	//Map screen vert coord to Y.  Results in 0..1 for opacity coords,
	// -1 for color coords
	//float mapScreenYCoord(int y);
	float getMinEditValue(){
		return (getMinColorEditValue());
	}
	float getMaxEditValue(){
		return (getMaxColorEditValue());
	}
	void setMinEditValue(float val) {
		setMinColorEditValue(val);
		setMinOpacEditValue(val);
	}
	void setMaxEditValue(float val) {
		setMaxColorEditValue(val);
		setMaxOpacEditValue(val);
	}
	TransferFunction* getTransferFunction() {return (TransferFunction*)myMapperFunction;}
	void setTransferFunction(TransferFunction* tf) {myMapperFunction = tf;}
	//Currently histogram is assumed to be limited to interval [0,1]:
	int getHistoValue(float point);

	//Public methods for controlling grabbedState:
	void unGrab() {grabbedState = notGrabbed;}
	void unGrabBounds() {grabbedState &= ~(domainGrab);}
	void removeConstrainedGrab() {grabbedState &= ~allConstrainedGrabs;}
	void addConstrainedGrab(){grabbedState |= constrainedGrab;}
	void addOpacGrab(){grabbedState |= opacGrab;}
	void addColorGrab() {grabbedState |= colorGrab;}
	void removeColorGrab() {grabbedState &= ~colorGrab;}
	void removeOpacGrab(){grabbedState &= ~opacGrab;}
	void addLeftDomainGrab() {grabbedState = leftDomainGrab;}
	void addRightDomainGrab() {grabbedState = rightDomainGrab;}
	void addFullDomainGrab() {grabbedState = fullDomainGrab;}
	void setNavigateGrab() { grabbedState = navigateGrab;}
	

	int numColorSelected() { return numColorSelect;}
	int numOpacSelected() {return numOpacSelect;}
	bool isGrabbed() {return (grabbedState != notGrabbed);}
	bool domainGrabbed() {return grabbedState&(domainGrab);}
	bool leftDomainGrabbed() {return grabbedState & leftDomainGrab;}
	bool rightDomainGrabbed() {return grabbedState & rightDomainGrab;}
	bool fullDomainGrabbed() {return grabbedState & fullDomainGrab;}
	bool canBind() {return (numColorSelected()==1 && numOpacSelected()==1);}
	void bindOpacToColor(); 
	void bindColorToOpac();
	//Mouse-down event starts with this:
	void setDragStart(int ix, int iy){
		dragStartX = ix;
		dragStartY = iy;
		dragMinStart = getMinEditValue();
		dragMaxStart = getMaxEditValue();
		mappedDragStartX = mapWin2Var(ix);
		leftMoveMax = -1;
	}
	void navigate(int x, int y);
	//Save the TF map bounds during a drag operation:
	void saveDomainBounds(){
		leftDomainSaved = getTransferFunction()->getMinMapValue();
		rightDomainSaved = getTransferFunction()->getMaxMapValue();
	}
	Params* getParams() {return myMapperFunction->getParams();}
	void setVarNum(int varnum){ setColorVarNum(varnum); setOpacVarNum(varnum);}
	int getVarNum() {return colorVarNum;}

	
protected:
	//grabbedState is "or" of some of following masks.
	//Whenever a new mouse-down event happens, it must reset
	//allConstrainedGrabs.  When mouse down occurs without
	//ctrl or shift keys, it resets opacGrab and colorGrab.
	//domain grabs remove all other grabs
	enum grabType {
		notGrabbed = 0,
		colorGrab = 1,
		opacGrab = 2,
		constrainedGrab = 4,
		verticalGrab = 8,
		horizontalGrab = 16,
		allConstrainedGrabs = 28,
		leftDomainGrab = 32,
		rightDomainGrab = 64,
		fullDomainGrab = 128,
		domainGrab = 224, //or of all domain grab types
		navigateGrab = 256
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
	
	float dragMinStart;
	float dragMaxStart;
	
	
	int dragStartX, dragStartY;
	float mappedDragStartX;
	//floats to hold domain bounds during a full domain grab:
	//
	float leftDomainSaved, rightDomainSaved;

	//Note that an Up move corresponds to decreasing y coords!
	//
	int leftMoveMax, rightMoveMax, upMoveMax, downMoveMax;
	int histoMaxBin;
	
};
};


#endif
