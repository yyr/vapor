//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		tfeditor.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the TFEditor class.  This is the class
//		that manages a transfer function editing panel
//
#include "tfeditor.h"
#include "transferfunction.h"
#include "tfframe.h"
#include <qimage.h>
#include "session.h"
#include "histo.h"
#include "dvrparams.h"
#include "vizwinmgr.h"
using namespace VAPoR;
//space below opacity window.  
#define BELOWOPACITY (COLORBARWIDTH+SEPARATOR+COORDMARGIN)

TFEditor::TFEditor(MapperFunction* tf,  TFFrame* frm): MapEditor(tf, frm){
	reset();
}
TFEditor::~TFEditor(){
	//Don't delete the image:  QT refcounts them
	//However, must notify the frame that I'm no longer here!
	//Also, be sure to delete the transfer function!
	
	if(((TFFrame*)myFrame)->getEditor() == this)
		((TFFrame*)myFrame)->setEditor(0);
}
//Reset to default state.  Should already have frame
//associated.
//
void TFEditor::
reset(){
	editImage = new QImage(myFrame->size(), 32);
	editImage->detach();
	
	height = myFrame->height();
	width = myFrame->width();
	for (int i = 0; i< MAXCONTROLPOINTS; i++){
		selectedColor[i] = false;
		selectedOpac[i] = false;
	}
	
	
	grabbedState = notGrabbed;
	leftMoveMax = -1;
	numColorSelect = 0;
	numOpacSelect = 0;
	histoMaxBin = -1;
}
// Use the data in the transfer function to construct the editing image
//
#define COLOR(i) qRgb(((int)(clut[4*i]*255.99)),((int)(clut[4*i+1]*255.99)),((int)(clut[4*i+2]*255.99)))

void TFEditor::refreshImage(){
	float clut[256*4];
	getTransferFunction()->makeLut(clut);

	QRgb color;
	int column = -1;
	int histoColumn = -1;
	width = myFrame->width();
	height = myFrame->height();
	if (width != editImage->width() || height != editImage->height()){
		//delete editImage;
		editImage = new QImage(myFrame->size(), 32);
		editImage->detach();
	}
	editImage->fill(0);
	//Determine relevant vizNum
	int viznum = getParams()->getVizNum();
	if (viznum < 0) {
		viznum = VizWinMgr::getInstance()->getActiveViz();
		assert(viznum >= 0);
	}
	Histo* histo = Histo::getHistogram(getParams()->getVarNum(),viznum);
	
	if (histo) {
		histoMaxBin = histo->getMaxBinSize();
		//If all the data is outside, the maxBin can be 0!
		if (histoMaxBin <= 0) histoMaxBin = 1;
	}
	int leftLim = mapVar2Win(getTransferFunction()->getMinMapValue(),false);
	int rightLim = mapVar2Win(getTransferFunction()->getMaxMapValue(),false);
	
	for (int x = 0; x<width; x++){
		int y;
		//Find what index corresponds to x
		//Perform a linear mapping to find point coord,
		//Then ask transfer function for discrete value
		//
		float xCoord = mapWin2Var(x);
		int newColumn = mapWin2Discrete(x,false);
		if (newColumn != column){
			if (newColumn < 0)
				color = COLOR(0);
			else if (newColumn > 255)
				color = COLOR(255);
			else {
				//if (xCoord < getTransferFunction()->getMinMapValue() ||
				//	xCoord > getTransferFunction()->getMaxMapValue())
				//	color = qRgb(0,0,0); 
				//else 
				color = COLOR(newColumn);
			}
			column = newColumn;
		}

		//Paint the domainslider region
		//
		if ((x < leftLim - SLIDERWIDTH)||
				(x> rightLim + SLIDERWIDTH)){
			//paint away from slider
			for (y = 0; y<SLIDERWIDTH-1; y++){
				editImage->setPixel(x,y,DEFAULTBACKGROUNDCOLOR);
			}
		} else if ((x >= leftLim) && (x <= rightLim)){
			// paint slider
			for (y = 0; y<SLIDERWIDTH-1; y++){
					editImage->setPixel(x,y,DOMAINSLIDERCOLOR);
			}
		} else if (x > rightLim) {
			// right pointer
			int top = (x-rightLim)/2;
			int bot = SLIDERWIDTH - top;
			for (y = 0; y<SLIDERWIDTH-1; y++){
				if (y < top || y >= bot)
					editImage->setPixel(x,y,DEFAULTBACKGROUNDCOLOR);
				else
					editImage->setPixel(x,y,DOMAINENDSLIDERCOLOR);
			}
		} else {
			assert(x<leftLim);
			// left pointer
			int top = (leftLim-x)/2;
			int bot = SLIDERWIDTH - top;
			for (y = 0; y<SLIDERWIDTH-1; y++){
				if (y < top || y >= bot)
					editImage->setPixel(x,y,DEFAULTBACKGROUNDCOLOR);
				else
					editImage->setPixel(x,y,DOMAINENDSLIDERCOLOR);
			}
		}
		//leave a thin margin below the domain slider
		editImage->setPixel(x,SLIDERWIDTH-1,DEFAULTBACKGROUNDCOLOR);


		//Only values between 0 and 1 are nonzero histograms (currently!)
		//The histogram will be displayed in solid color
		//
		//int newHistoColumn = (int)(xCoord*255.999f);
		float histoHeight;
		int histoInt;
		float histStretch = getParams()->getHistoStretch();
		if (histo){
			float histoVal = (xCoord - histo->getMinData())/(histo->getMaxData()-histo->getMinData());
			if (histoVal >= 0.f && histoVal <= 1.f){
				int newHistoColumn = (int)(histoVal*255.99);
				if (histoColumn != newHistoColumn){
					histoColumn = newHistoColumn;
					histoHeight = histStretch*(float)histo->getBinSize(histoColumn)/(float)histoMaxBin;
					if (histoHeight > 1.f) histoHeight = 1.f;
					assert(histoHeight >= 0.f);
				}
			}
			else histoHeight = 0.f;
		}
		else histoHeight = 0.f;
		
		//Convert histoHeight to a vertical pixel coord:
		//
		histoInt = BELOWOPACITY+(int)(histoHeight*(height-BELOWOPACITY-TOPMARGIN-SLIDERWIDTH));
		if (histoInt < 0){
			assert (histoInt >= 0);
		}
		
		//Solid  bars for histogram:
		//
		for (y = (height-histoInt); y<height- BELOWOPACITY; y++){
			editImage->setPixel(x,y,HISTOCOLOR/*blue-green??*/);
		}
		
		//Solid dark grey for separator
		//
		for (y = height - BELOWOPACITY; y< height-COLORBARWIDTH-COORDMARGIN; y++){
			editImage->setPixel(x,y, SEPARATORCOLOR/*darkGray*/);
		}
		
		//Color bar:
		//
		for (y = height - COLORBARWIDTH-COORDMARGIN; y< height-COORDMARGIN; y++){
			editImage->setPixel(x,y, color);
		}
		for (y = height - COORDMARGIN; y<height; y++){
			editImage->setPixel(x,y,DEFAULTBACKGROUNDCOLOR);
		}
	}
	
}
	
/*
 * Responses to mouse drags.  Drag control points.
 * Most of the effort is involved in establishing and maintaining the
 * constraints.  Drags are not allowed to move a control point past another, or
 * to push any control point above or below the opacity limits (0, 1)
 */

void TFEditor::
moveGrabbedControlPoints(int newX, int newY){
	//Following static arrays remember position where the
	//mouse is clicked
	//
	static float startingOpacPosition[MAXCONTROLPOINTS];
	static float startingColorPosition[MAXCONTROLPOINTS];
	static float startingOpac[MAXCONTROLPOINTS];
	static float lastXMove;
	int i, j;
	//First, see if this is an undetermined constrained drag.  
	// Determine if horiz or vert
	//
	if (grabbedState & constrainedGrab) {
		int horizChange = abs(newX - dragStartX);
		int vertChange = abs(newY - dragStartY);
		if (vertChange> horizChange) grabbedState |= verticalGrab;
		else grabbedState |= horizontalGrab;
		//Turn off constrained-grab, so won't test direction
		//again.
		//
		grabbedState &= ~constrainedGrab;
	}
	//Determine drag limits, if not already known.
	//Recognize the first move after a mouse down event
	//by checking the value of leftMoveMax.
	//The limits are set so that none of the selected control
	//points can be dragged above or below the window, and 
	//the horizontal drag can't be to the same transfer function
	//quantization value as any unselected point.
	//These limits are only determined at the start of a drag!
	//
	if (leftMoveMax == -1){
		lastXMove = 0.f;
		
		//Start with extreme limits for quantization error:
		leftMoveMax = 10000000;
		rightMoveMax = 10000000;
		upMoveMax = 10000000;
		downMoveMax = 10000000;
		int otherX, otherY, testX, testY;
		//For each selected point, find the first unselected control point to the left and right:
		//
		if (grabbedState & colorGrab) {
			assert(getNumColorControlPoints()>0);
			for (i = 0; i< getNumColorControlPoints(); i++){
				if (selectedColor[i]){
					getColorControlPointPosition(i, &testX);
					//Save the coords of starting selected points:
					//
					startingColorPosition[i] = getTransferFunction()->colorCtrlPointPosition(i);
					//check left:
					for (j = i-1; j>0; j--){
						if (!selectedColor[j]){
							getColorControlPointPosition(j, &otherX);
							if((testX - otherX)< leftMoveMax)
								leftMoveMax = testX -otherX;
							break;
						}
					}
					//Check right
					for (j = i+1; j<getNumColorControlPoints(); j++){
						if (!selectedColor[j]){
							getColorControlPointPosition(j, &otherX);
							if((otherX-testX)< rightMoveMax)
								rightMoveMax = otherX-testX;
							break;
						}
					}
				}
			}
		}
		//do same for opacity:
		if (grabbedState & opacGrab) {
			assert(getNumOpacControlPoints()>0);
			for (i = 0; i< getNumOpacControlPoints(); i++){
				if (selectedOpac[i]){
					getOpacControlPointPosition(i, &testX, &testY);
					startingOpacPosition[i] = getTransferFunction()->opacCtrlPointPosition(i);
					startingOpac[i] = getTransferFunction()->opacityValue(i);
					//check left:
					for (j = i-1; j>0; j--){
						if (!selectedOpac[j]){
							getOpacControlPointPosition(j, &otherX, &otherY);
							if((testX - otherX)< leftMoveMax)
								leftMoveMax = testX -otherX;
							break;
						}
					}
					//Check right
					for (j = i+1; j<getNumOpacControlPoints(); j++){
						if (!selectedOpac[j]){
							getOpacControlPointPosition(j, &otherX, &otherY);
							if((otherX-testX)< rightMoveMax)
								rightMoveMax = otherX-testX;
							break;
						}
					}
				}
			}
			//With opacity grab, must find vertical constraints 
			//Can't go above or below window, so find the least
			//up or down distance that any of the selected points
			//can move to leave the window.
			//Note that "up" corresponds to decreasing Y coord.
			//
			for (i = 0; i< getNumOpacControlPoints(); i++){
				if (selectedOpac[i]){
					getOpacControlPointPosition(i, &testX, &testY);
					//check up and down:
					if (testY < upMoveMax) upMoveMax = testY;
					if (height-BELOWOPACITY - testY < downMoveMax) downMoveMax = height-testY-BELOWOPACITY;
				}
			}
		}
	    //Adjust the horiz limits.  
		//We only allow a drag to 1 less than the closest unselected
		//control point.  It IS allowed to push past the window
		//left/right boundaries.
		//
		leftMoveMax --;
		rightMoveMax--;
		if (leftMoveMax < 0) leftMoveMax = 0;
		if (rightMoveMax < 0) rightMoveMax = 0;
		
	}
	//Now deal with the current move. 
	int xmove = newX - dragStartX;
	int ymove = newY - dragStartY;
	if (xmove  > rightMoveMax) xmove = rightMoveMax;
	if (xmove  < -leftMoveMax) xmove = -leftMoveMax;
	//Downward is increasing y:
	if (ymove >  downMoveMax) ymove = downMoveMax;
	if (ymove < -upMoveMax) ymove = -upMoveMax;
	
	//Deal with constrained move:
	if (grabbedState & verticalGrab) xmove = 0; 
	if (grabbedState & horizontalGrab) ymove = 0;
	
	//Convert relative move to a float value:
	float xMoveFloat = mapWin2Var(dragStartX+xmove) -
		mapWin2Var(dragStartX);
	float yMoveFloat = mapWin2Opac(dragStartY+ymove) -
		mapWin2Opac(dragStartY);
	
	//Now, move all control points, subject to any horizontal or vertical constraint.
	//If there is a vertical grab, don't move color control points.
	//
	if (numColorSelect > 0 && !(grabbedState&verticalGrab)){
		for (i = 0; i< getNumColorControlPoints(); i++) {
			if (selectedColor[i]){
				 
				getTransferFunction()->moveColorControlPoint(i, startingColorPosition[i]+xMoveFloat);
				//assert(newIndex == i);
			}
		}
	}
	//Choose the order of access to control points so that
	//they don't leapfrog each other:
	//
	if (numOpacSelect > 0) {
		if (xMoveFloat < lastXMove) {//moving to left
			for (i = 0; i< getNumOpacControlPoints(); i++){
				if (selectedOpac[i]){
					if (startingOpac[i] + yMoveFloat >= 1.f){
						yMoveFloat = 1.f-startingOpac[i];
					}
					//int newIndex = 
					getTransferFunction()->moveOpacControlPoint(i, startingOpacPosition[i]+xMoveFloat,
						startingOpac[i]+yMoveFloat);
					//assert(newIndex == i);
				}
			}
		} else { //moving to right, access in reverse order
			for (i = getNumOpacControlPoints()-1; i> 0; i--){
				if (selectedOpac[i]){
					if (startingOpac[i] + yMoveFloat >= 1.f){
						yMoveFloat = 1.f-startingOpac[i];
					}
					//int newIndex = 
					getTransferFunction()->moveOpacControlPoint(i, startingOpacPosition[i]+xMoveFloat,
						startingOpac[i]+yMoveFloat);
					//assert(newIndex == i);
				}
			}
		}
		lastXMove = xMoveFloat;
	}
	
	setDirty();
	myFrame->update();
}
// Set the zoom and pan, based on the new position of the mouse during a drag
void TFEditor::
navigate (int x, int y){
	//Determine zoom amount as a fraction of edit window height. 
	float zoomAmount = (float)(y-dragStartY)/(float)(height);
	float zoomRatio = pow(2.f, zoomAmount);
	//Determine the horizontal pan as a fraction of edit window width:
	float horizFraction = (float)(x-dragStartX)/(float)(width);
	//The zoom starts at the original drag start; i.e. that point won't move
	float startXMapped = ((float)dragStartX/(float)(width))*(dragMaxStart-dragMinStart) + dragMinStart;
	//stretch the part to the left and the part to the right by zoom ratio
	//
	float minval = startXMapped - (startXMapped - dragMinStart)*zoomRatio;
	float maxval = startXMapped + (dragMaxStart - startXMapped)*zoomRatio;
	//Then apply the pan to the zoomed window:
	setMinEditValue(minval - horizFraction*(maxval - minval));
	setMaxEditValue(maxval - horizFraction*(maxval - minval));
	setDirty();
	myFrame->update();

}

void TFEditor::
moveDomainBound(int x){
	float newX = mapWin2Var(x);
	//fullDomainGrab?
	if (grabbedState & fullDomainGrab){
		getParams()->setMinMapBound(leftDomainSaved + newX - mappedDragStartX);
		getParams()->setMaxMapBound(rightDomainSaved + newX - mappedDragStartX);
	} else {
		float mappedX = mapWin2Var(x);
		//Check that the user has not moved one bound past the other:
		if (grabbedState&leftDomainGrab){
			if(mappedX < getParams()->getMaxMapBound()){
				getParams()->setMinMapBound(mappedX);
			}
		}
		else if (grabbedState&rightDomainGrab){
			if(mappedX > getParams()->getMinMapBound()){
				getParams()->setMaxMapBound(mappedX);
			}
		} else assert(0);
	}
	setDirty();
	myFrame->update();
}

//Find where on the image is a specific control point
void TFEditor::
getColorControlPointPosition(int index, int* xpos){
	float xvar = getTransferFunction()->colorCtrlPointPosition(index);
	*xpos = mapVar2Win(xvar);
	//*xpos = getTransferFunction()->mapColorControlPoint(index, minEditValue, maxEditValue, width);
}
//Find where on the image is a specific control point
//Can return values outside of window, but only about 1000000 each way
//
void TFEditor::
getOpacControlPointPosition(int index, int* xpos, int* ypos){
	float xvar = getTransferFunction()->opacCtrlPointPosition(index);
	float opac = getTransferFunction()->controlPointOpacity(index);
	*xpos = mapVar2Win(xvar);
	*ypos = mapOpac2Win(opac);
	//*xpos = getTransferFunction()->mapOpacControlPoint(index, minEditValue, maxEditValue, width);
	//*ypos = (int)((1.f-getTransferFunction()->opacityValue(index))*(height - BELOWOPACITY));
}
/* 
 * Determine if the specified point is close to a control point.  Return the
 * index of the color control point, and also specify whether its a color or opacity control point
 * returns 1 for color, -1 for opacity, +2 for Right domain-edge, 
 * -2 for left domain-edge, 0 for none
 */

int TFEditor::
closestControlPoint(int x, int y, int* index) {
	int xc, yc;
	int i;
	
	//Color selected?
	//
	if (y >= (height - COORDMARGIN - COLORBARWIDTH -SEPARATOR/2) ){
		for (i = 0; i< getNumColorControlPoints(); i++){
			getColorControlPointPosition(i, &xc);
			if (abs(xc-x) > CLOSE_DISTANCE) continue;
			*index = i;
			return 1;
		}
	} else {
		for (i = 0; i< getNumOpacControlPoints(); i++){
			getOpacControlPointPosition(i, &xc, &yc);
			if (abs(xc-x) > CLOSE_DISTANCE) continue;
			if(abs(yc-y) <= CLOSE_DISTANCE) {
				*index = i;
				return -1;
			}
		}
	}
	return 0;
}
/*
 * insert a new control point, also select it.
 * return the index of the new point, or -1 if unsuccessful.
 */
int TFEditor::
insertOpacControlPoint(int x, int y){
	float opacity = mapWin2Opac(y);
	if (opacity < 0.f) opacity = 0.f;
	if (opacity > 1.f) opacity = 1.f;

	float point = mapWin2Var(x);
	/* replaced:
	if (y == -1) opacity = -1.f;
	else {
		if (y > height - BELOWOPACITY){ y = height-BELOWOPACITY;}
		opacity = 1.f - (float)y/(float)(height-BELOWOPACITY);
	}
	*/
	int ptNum = getTransferFunction()->insertOpacControlPoint(point, opacity);
	if (ptNum < 0) return -1;
	//Must move selection of existing control points:
	for (int i = getNumOpacControlPoints()-1; i>ptNum;  i--)
		selectedOpac[i] = selectedOpac[i-1];
	selectedOpac[ptNum] = true;
	numOpacSelect++;
	setDirty();
	return ptNum;
}
int TFEditor::
insertColorControlPoint(int x){
	float point = mapWin2Var(x);
	int ptNum = getTransferFunction()->insertColorControlPoint(point);
	if (ptNum < 0) return -1;
	//Must also move selection of existing control points:
	//
	for (int i = getNumColorControlPoints()-1; i>ptNum;  i--)
		selectedColor[i] = selectedColor[i-1];
	selectedColor[ptNum] = true;
	numColorSelect++;
	setDirty();
	return ptNum;
}
void TFEditor::
unSelectAll(){
	int i;
	for (i = 0; i< getTransferFunction()->getNumColorControlPoints(); i++) {
		selectedColor[i] = false;
	}
	for (i = 0; i< getTransferFunction()->getNumOpacControlPoints(); i++) {
		selectedOpac[i] = false;
	}
	numColorSelect = 0;
	numOpacSelect = 0;
	setDirty();
}


void TFEditor::
selectInterval(bool colorPoint){
	int first = 0, last = 0;
	if (colorPoint){ //color interval
		for (int i = 1; i < getNumColorControlPoints()-1; i++){
			if (selectedColor[i]){
				if(!first) first = i; 
				last = i;
			}
		}
		if (first!=last) {
			for (int j = first; j<= last; j++){
				if (!selectedColor[j]){
					selectedColor[j] = true;
					numColorSelect++;
				}
			}
		}
	} else { //opacity interval
		for (int i = 1; i < getNumOpacControlPoints()-1; i++){
			if (selectedOpac[i]){
				if(!first) first = i; 
				last = i;
			}
		}
		if (first!=last) {
			for (int j = first; j<= last; j++){
				if (!selectedOpac[j]){
					selectedOpac[j] = true;
					numOpacSelect++;
				}
			}
		}
	} 
	
	setDirty();
}
/* 
 * When a control point is deleted, selection must also be modified.
 */
void TFEditor::
deleteControlPoint(int pointNum, bool colorPoint){
	if (colorPoint){
		if (selectedColor[pointNum]) numColorSelect--;
		for (int i = pointNum; i< getTransferFunction()->getNumColorControlPoints(); i++){
			selectedColor[i] = selectedColor[i+1];
		}
		getTransferFunction()->deleteColorControlPoint(pointNum);
	} else {
		if (selectedOpac[pointNum]) numOpacSelect--;
		for (int i = pointNum; i< getTransferFunction()->getNumOpacControlPoints(); i++){
			selectedOpac[i] = selectedOpac[i+1];
		}
		getTransferFunction()->deleteOpacControlPoint(pointNum);
	}
	
	setDirty();

}
/*  
 *  Must delete multiple control points in backwards order so that the numbering
 *  does not get corrupted.
 */
void TFEditor::
deleteSelectedControlPoints(){
	int i;
	for (i = getTransferFunction()->getNumColorControlPoints() -2; i> 0; i--){
		if (selectedColor[i]) deleteControlPoint(i, true);
	}
	for (i = getTransferFunction()->getNumOpacControlPoints() -2; i> 0; i--){
		if (selectedOpac[i]) deleteControlPoint(i, false);
	}
	assert(numColorSelect == 0);
	assert(numOpacSelect == 0);

}
/* 
 * Set all the selected color control points to the specified hsv.  
 * h is 0..360; s and v are 0..255
 */
void TFEditor::setHsv(int h, int s, int v){
	float newHue = (float)h/360.f;
	float newSat = (float)s/255.f;
	float newVal = (float)v/255.f;
	bool change = false;
	for (int i = 0; i<getNumColorControlPoints(); i++){
		if (selectedColor[i]){
			getTransferFunction()->setControlPointHSV(i, newHue, newSat, newVal);
			change = true;
		}
	}
	if (change) {
		setDirty();
		myFrame->update();
	}
}


//Find value of histogram.  Just used by tfelocationtip
//
int TFEditor::
getHistoValue(float point){
	//Determine relevant vizNum
	int viznum = getParams()->getVizNum();
	if (viznum < 0) {
		viznum = VizWinMgr::getInstance()->getActiveViz();
		assert(viznum >= 0);
	}
	Histo* hist = Histo::getHistogram(getParams()->getVarNum(),viznum);
	if (!hist) return -1;
	float ind = (point - hist->getMinData())/(hist->getMaxData()-hist->getMinData());
	if (ind < 0.f || ind >= 1.f) return 0;
	int index = (int)(ind*255.999f);
	return hist->getBinSize(index);
}
void TFEditor::
bindOpacToColor(){
	assert(canBind());
	int colorNum=-1, opacNum=-1;
	int i;
	//Find the selected color and opacity
	//
	for (i = 0; i<getNumColorControlPoints();i++){
		if (selectedColor[i]){
			colorNum = i;
			break;
		}
	}
	for (i = 0; i<getNumOpacControlPoints(); i++){
		if (selectedOpac[i]){
			opacNum = i;
			break;
		}
	}
	//Don't allow the binding if an existing opac control point is already
	//in the proposed place:
	//
	int existingPt = getTransferFunction()->getLeftOpacityIndex(
		getTransferFunction()->colorCtrlPointPosition(colorNum));
	if (getTransferFunction()->opacCtrlPointPosition(existingPt) ==
		getTransferFunction()->colorCtrlPointPosition(colorNum)) return;

	int newOpacIndex = getTransferFunction()->moveOpacControlPoint(opacNum,
		getTransferFunction()->colorCtrlPointPosition(colorNum),
		getTransferFunction()->opacityValue(opacNum));
	if (newOpacIndex!= opacNum){
		//Swap the selections:
		
		selectedOpac[newOpacIndex] = true;
		selectedOpac[opacNum] = false;
		
	}
	
	setDirty();
	myFrame->update();
}
void TFEditor::
bindColorToOpac(){
	assert(canBind());
	int colorNum=-1, opacNum=-1;
	int i;
	//Find the selected color and opacity
	//
	for (i = 0; i<getNumColorControlPoints();i++){
		if (selectedColor[i]){
			colorNum = i;
			break;
		}
	}
	for (i = 0; i<getNumOpacControlPoints(); i++){
		if (selectedOpac[i]){
			opacNum = i;
			break;
		}
	}
	//Don't allow the binding if an existing color control point is already
	//in the proposed place:
	//
	int existingPt = getTransferFunction()->getLeftColorIndex(
		getTransferFunction()->opacCtrlPointPosition(opacNum));
	if (getTransferFunction()->colorCtrlPointPosition(existingPt) ==
		getTransferFunction()->opacCtrlPointPosition(opacNum)) return;

	int newColorIndex = getTransferFunction()->moveColorControlPoint(colorNum,
		getTransferFunction()->opacCtrlPointPosition(opacNum));
	//Must update selection if the index changed:
	//
	if (newColorIndex!= colorNum){
		//Swap the selections:
		//
		selectedColor[newColorIndex] = true;
		selectedColor[colorNum] = false;
	}
	setDirty();
	myFrame->update();
}

	

/*
 * map vertical window to opacity. special constants for
 * outside window, if classify is true.
 * So far no one is using "classify = true"...
 */
float TFEditor::mapWin2Opac(int y, bool classify){
	if(classify){
		if (y < SLIDERWIDTH) return ONOPACITYSLIDER;
		if (y < (TOPMARGIN+SLIDERWIDTH)) return ABOVEWINDOW;
		if (y >= height - BELOWOPACITY) {
			if (y >= height) return BELOWWINDOW;
			if (y >= height - COORDMARGIN) return BELOWCOLORBAR;
			if (y >= height - COORDMARGIN - COLORBARWIDTH)
				return ONCOLORBAR;
			return ONSEPARATOR;
		}
	}
	return ((float)(height - BELOWOPACITY -1 - y)/
		(float)(height - BELOWOPACITY - TOPMARGIN - SLIDERWIDTH - 1));
}

/*
 *  map opacity to window position.  possibly truncate
 */
int TFEditor::mapOpac2Win(float opac, bool truncate){
	if (truncate){
		if (opac > 1.f) return 0;
		if (opac < 0.f) return (height - BELOWOPACITY -1);
	}
	return TOPMARGIN +SLIDERWIDTH + (int)((1.-opac)*(float)(height - BELOWOPACITY -TOPMARGIN -SLIDERWIDTH-1));
}

void TFEditor::setDirty(){
	((TFFrame*)myFrame)->setDirtyEditor(this);
}
