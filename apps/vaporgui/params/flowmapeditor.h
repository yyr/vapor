//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		FlowMapEditor.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Defines the FlowMapEditor class.  This is the class
//		that manages a mapper function editing panel for flow viz
//
#ifndef FLOWMAPEDITOR_H
#define FLOWMAPEDITOR_H


class QImage;
#include "flowparams.h"
#include "mapperfunction.h"
#include "mapeditor.h"
class FlowMapFrame;
namespace VAPoR {
class MapperFunction;
class TFInterpolator;
class Session;
class TFELocationTip;
class FlowParams;

class FlowMapEditor : public MapEditor {
public:
	FlowMapEditor(MapperFunction* TF, FlowMapFrame*);
	~FlowMapEditor();
	//Reset to default state, e.g. when loading new TF:
	//
	void reset();
	void setColorEditingRange(float minVal, float maxVal){
		setMinColorEditValue(minVal);
		setMaxColorEditValue(maxVal);
	}
	void setOpacEditingRange(float minVal, float maxVal){
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
	void moveColorDomainBound(int x);
	void moveOpacDomainBound(int x);
	
	int insertColorControlPoint(int xcoord);
	int insertOpacControlPoint(int xcoord, int ycoord = -1);
	//Find where on the image are two versions of a specific control point
	void getOpacControlPointPosition(int index, int* xpos, int* ypos);
	void getColorControlPointPosition(int index, int* xpos);
	int getNumColorControlPoints() {return myMapperFunction->getNumColorControlPoints();}
	int getNumOpacControlPoints() {return myMapperFunction->getNumOpacControlPoints();}
	
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
	
	//Map a screen horiz coordinate to float.  Results in a
	//value between minEditBound and maxEditBound.
	//float mapScreenXCoord(int x);
	//Map screen vert coord to Y.  Results in 0..1 for opacity coords,
	// -1 for color coords
	//float mapScreenYCoord(int y);
	
	
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
		dragMinStart = getMinOpacEditValue();
		dragMaxStart = getMaxOpacEditValue();
		mappedOpacDragStartX = mapOpacWin2Var(ix);
		leftMoveMax = -1;
	}
	void navigate(int x, int y);
	//Save the TF map bounds during a drag operation:
	void saveDomainBounds(){
		leftColorDomainSaved = getMapperFunction()->getMinColorMapValue();
		rightColorDomainSaved = getMapperFunction()->getMaxColorMapValue();
		leftOpacDomainSaved = getMapperFunction()->getMinOpacMapValue();
		rightOpacDomainSaved = getMapperFunction()->getMaxOpacMapValue();
	}
	FlowParams* getParams() {return (FlowParams*)myMapperFunction->getParams();}
	void moveDomainBound(int x);
	
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
	//They must lie within range of opacity mapping definition.
	
	float dragMinStart;
	float dragMaxStart;
	
	int dragStartX, dragStartY;
	float mappedColorDragStartX;
	float mappedOpacDragStartX;
	//floats to hold domain bounds during a full domain grab:
	//
	float leftColorDomainSaved, rightColorDomainSaved;
	float leftOpacDomainSaved, rightOpacDomainSaved;

	//Note that an Up move corresponds to decreasing y coords!
	//
	int leftMoveMax, rightMoveMax, upMoveMax, downMoveMax;
	
	
};
};


#endif
