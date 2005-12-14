//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		flowmapframe.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2005
//
//	Description:	Implements the FlowMapFrame class.  This provides
//		a frame in which the flow mapping editing occurs.
//		Principally involved in drawing and responding to mouse events.
//
#include "flowmapframe.h"
#include <qframe.h>
#include <qwidget.h>
#include <qimage.h>
#include <qpainter.h>
#include "flowmapeditor.h"
#include "tfelocationtip.h"
#include "messagereporter.h"
#include "params.h"
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qcursor.h>
#include <qlineedit.h>
#include "coloradjustdialog.h"
#include "opacadjustdialog.h"
#include "panelcommand.h"
//space below opacity window.  
//Include space for color slider bar
#define BELOWOPACITY (COLORBARWIDTH+SEPARATOR+COORDMARGIN+SLIDERWIDTH)

FlowMapFrame::FlowMapFrame( QWidget * parent, const char * name, WFlags f ) :
	QFrame(parent, name, f) {
	editor = 0;
	dirtyEditor = 0;
	needUpdate = true;
	locationTip = new TFELocationTip(this);
	setFocusPolicy(QWidget::StrongFocus);
	amDragging = false;
	//Create labels to show with editor
	//The first and last opacity label will be used for coords
	for (int i = 0; i<MAXNUMLABELS; i++){
		tfColorLabels[i] = new QLabel( parent, "tfColorLabel" );
		tfOpacLabels[i] = new QLabel( parent, "tfOpacLabel" );
		tfColorLabels[i] ->setEnabled(true);
		tfOpacLabels[i] ->setEnabled(true);
		tfColorLabels[i]->setGeometry( 0, 0, 50, 20 );
		tfColorLabels[i]->setBackgroundMode( QLabel::PaletteBackground );
		tfOpacLabels[i]->setGeometry( 0, 0, 50, 20 );
		tfOpacLabels[i]->setBackgroundMode( QLabel::PaletteBackground );
		tfColorLabels[i]->hide();
		tfOpacLabels[i]->hide();
		tfColorLabels[i]->setAlignment(Qt::AlignHCenter);
		tfOpacLabels[i]->setAlignment(Qt::AlignHCenter);
		QToolTip::add( tfColorLabels[i], "x-coord of selected color control point");
		QToolTip::add( tfOpacLabels[i], "x-coord of selected opacity control point");
	}

}
FlowMapFrame::~FlowMapFrame() {
	delete locationTip;
	//Will these be deleted by their parent????
	for (int i = 0; i<MAXNUMLABELS; i++){
		delete tfColorLabels[i];
		delete tfOpacLabels[i];
	}
}
	

void FlowMapFrame::paintEvent(QPaintEvent* ){
	int i;
	if (!editor){ //just  bitblt a blank pixmap to the widget:
		pxMap.resize(size());
		pxMap.fill(QColor(0,0,0));
		bitBlt(this, QPoint(0,0),&pxMap);
		return;
	}
	if (editor == dirtyEditor || needUpdate){
		erase();
		editor->refreshImage();
		pxMap.convertFromImage(*editor->getImage(),0);
		
		needUpdate = false;
	} else {
		if (editor != dirtyEditor)
			MessageReporter::infoMsg("Editor change on TF frame update");
		return;
	}
	MapperFunction* tf = editor->getMapperFunction();
	
	QPainter painter(&pxMap);
	//Workaround for QT bug:  the clip rect is wrong, so
	//specify the whole window as the clip rect:
	painter.setClipRect(rect());
    //painter.setClipRect(e->rect());
	
	
	int x, y;
	QBrush drawBrush(SolidPattern);
	bool colorIsSelected = false;
	painter.setPen(CONTROLPOINTCOLOR);
	//First paint the unselected points, then the selected ones,
	//So we don't hide the selection.
	for (i = 0; i<editor->getNumColorControlPoints(); i++){
		if (editor->colorSelected(i)){
			colorIsSelected = true;
			continue;
		}
		editor->getColorControlPointPosition(i, &x);
		if (x >= -2 && x <= width()+2){
			drawTris(painter, x);
		}
	}
	if(colorIsSelected){
		painter.setPen(HIGHLIGHTCOLOR); 
		for (i = 0; i<editor->getNumColorControlPoints(); i++){
			if (!editor->colorSelected(i)) continue;
			editor->getColorControlPointPosition(i, &x);
			if (x >= -2 && x <= width()+2){
				drawTris(painter, x);
			}
		}
	}
	//Hide labels:
	for (i = 0; i<MAXNUMLABELS; i++){
		tfColorLabels[i]->hide();
		tfOpacLabels[i]->hide();
	}
	//Draw tic marks for selected control points, plus left and right
	//endpoints.  Opacity labels also used for left/right endpoints.
	//To avoid overlaps, labels are only shown if they have enough room.
	//
	painter.setPen(black);
	int recentTicPosition = 0;
	int numColorLabels = 0;
	int numOpacLabels = 1; //first one used for left endpoint.
	for (i = 0; i<editor->getNumColorControlPoints(); i++){
		editor->getColorControlPointPosition(i, &x);
		if (editor->colorSelected(i) && x >=0 && x < width()){
			painter.drawLine(x,height()-COORDMARGIN+3,x,height());
			//Possibly put a label here:
			if (numColorLabels < MAXNUMLABELS && x>recentTicPosition+40){
				tfColorLabels[numColorLabels]->move(pos()+QPoint(x-25, height()+15));
			
				tfColorLabels[numColorLabels]->setText(QString::number(editor->getMapperFunction()->
					colorCtrlPointPosition(i),'g',4));
				tfColorLabels[numColorLabels]->raise();
				tfColorLabels[numColorLabels++]->show();
				recentTicPosition = x;
			}
		}
	}
	//Do tic mark for left endpoint:
	float fx = editor->mapColorWin2Var(0);
	painter.drawLine(0,height()-COORDMARGIN+3,0,height());
	//Use the first color label to indicate left edit margin
	tfOpacLabels[0]->move(pos()+QPoint(-20, height()));
			
	tfOpacLabels[0]->setText(QString::number(fx,'g',4));
	tfOpacLabels[0]->raise();
	tfOpacLabels[0]->show();
	

	
	//Setup for opacity tic marks:
	recentTicPosition = 0;
	for (i = 0; i<editor->getNumOpacControlPoints(); i++){
		editor->getOpacControlPointPosition(i, &x, &y);
		if (editor->opacSelected(i) && x >0 && x < width()){
			painter.drawLine(x,height()-COORDMARGIN+3,x,height());
			//Possibly put a label here:
			if (numOpacLabels < MAXNUMLABELS-1 && x>recentTicPosition+40 && x < width()-40){
				tfOpacLabels[numOpacLabels]->move(pos()+QPoint(x-20, height()));
			
				tfOpacLabels[numOpacLabels]->setText(QString::number(editor->getMapperFunction()->
					opacCtrlPointPosition(i),'g',4));
				tfOpacLabels[numOpacLabels]->raise();
				tfOpacLabels[numOpacLabels++]->show();
				recentTicPosition = x;
			}
		}
	}
	fx = editor->mapColorWin2Var(width()-1);
	//Draw right edge tic mark:
	painter.drawLine(width()-1,height()-COORDMARGIN+3,width()-1,height());
	//Use the last opac label to indicate right edit margin
	tfOpacLabels[numOpacLabels]->move(pos()+QPoint(width()-20, height()));
			
	tfOpacLabels[numOpacLabels]->setText(QString::number(fx,'g',4));
	tfOpacLabels[numOpacLabels]->raise();
	tfOpacLabels[numOpacLabels++]->show();

	//Draw the opacity curve, 2 pixels wide
	
	QPen myPen(OPACITYCURVECOLOR, 2);
	painter.setPen(myPen);
	float prevX = editor->mapOpacWin2Var(0);
	float prevY = tf->opacityValue(prevX);
	int atEnd = 0;
	int prevIndex = tf->getLeftOpacityIndex(prevX);
	int prevIntX = 0;
	int prevIntY = (int)((1.f - prevY)*(height() -BELOWOPACITY + 0.5f));
	int nextIntX, nextIntY;
	int lastX = editor->mapOpacVar2Win(tf->getMaxOpacMapValue());
	//Draw a curve on the x-axis before domain start:
	if (prevX < tf->getMinOpacMapValue()){
		//Find out what xcoord maps to minMapValue:
		nextIntX = editor->mapOpacVar2Win(tf->getMinOpacMapValue(),false);
		painter.drawLine(prevIntX, height()-BELOWOPACITY, nextIntX, height()-BELOWOPACITY);
		prevIntX=nextIntX;
	}
	for (i = prevIndex + 1; i< editor->getNumOpacControlPoints(); i++) {
		if (atEnd) break;
		//Test if next point is still in interval
		editor->getOpacControlPointPosition(i, &nextIntX, &nextIntY);
		if (nextIntX > lastX){
			//draw horizontally to the right limit, then draw 0 to 
			//end of window.
			if(prevIntX < width()-1){
				painter.drawLine(prevIntX, prevIntY, lastX, prevIntY);
			}
			prevIntX = lastX;
			nextIntX = width()-1;
			nextIntY = height() -BELOWOPACITY;
			prevIntY = nextIntY;
			//Check if this is the "last" control point;
			//If so, draw horizontally to edge of edit region, then
			//draw zero to the right margin.
			/*
			if (i == editor->getNumOpacControlPoints()-1){
				if(prevIntX < width()-1){
					painter.drawLine(prevIntX, prevIntY, lastX, prevIntY);
				}
				nextIntX = width();
				nextIntY = height() -BELOWOPACITY;
				prevIntY = nextIntY;
			}
			//Otherwise interpolate to right edge.
			else {
				float nextX = editor->mapWin2Var(width()-1);
				float nextY = tf->opacityValue(nextX);
				nextIntX = width()-1;
				nextIntY = (int)((1.f - nextY)*(height() -BELOWOPACITY + 0.5f));
			}
			*/
			atEnd = 1;
		}
		painter.drawLine(prevIntX, prevIntY, nextIntX, nextIntY);
		prevIntX = nextIntX;
		prevIntY = nextIntY;
	}
	//Draw the control points
	for (i = 0; i<editor->getNumOpacControlPoints(); i++){
		if (editor->opacSelected(i))drawBrush.setColor(HIGHLIGHTCOLOR); else
			drawBrush.setColor(CONTROLPOINTCOLOR);
		editor->getOpacControlPointPosition(i, &x, &y);
		if (x >= -2 && x <= width()+2){
			painter.fillRect(x-3,y-3,6,6,drawBrush);
		}
	}
	//Draw left and right opacity limits (if on screen):
	int leftOpacLim = editor->mapOpacVar2Win(tf->getMinOpacMapValue(),false);
	int rightOpacLim = editor->mapOpacVar2Win(tf->getMaxOpacMapValue(),false);
	
	QPen vpen(ENDLINECOLOR, 3);
	drawBrush.setColor(ENDLINECOLOR);
	
	if (leftOpacLim > 0){
		if (editor->leftOpacDomainGrabbed()||editor->fullOpacDomainGrabbed()){
			vpen.setColor(HIGHLIGHTCOLOR);
			painter.setPen(vpen);
		} else {
			vpen.setColor(ENDLINECOLOR);
			painter.setPen(vpen);
		}
		painter.drawLine(leftOpacLim, height() -BELOWOPACITY, leftOpacLim, SLIDERWIDTH);
	}
	if (rightOpacLim < width()){
		if (editor->rightOpacDomainGrabbed()||editor->fullOpacDomainGrabbed()){
			vpen.setColor(HIGHLIGHTCOLOR);
			painter.setPen(vpen);
		} else {
			vpen.setColor(ENDLINECOLOR);
			painter.setPen(vpen);
		}
		painter.drawLine(rightOpacLim, height() -BELOWOPACITY, rightOpacLim, SLIDERWIDTH);
	}
	//Now bitblt the pixmap to the widget:
	bitBlt(this, QPoint(0,0),&pxMap);
}
/* 
	* When the left mouse is pressed, check if it's near a control point.
	* If so, "grab" that control point and select it.  
	* It will be repositioned with each
	* subsequent mouse move until the mouse is released.
	* If the mouse is not near a control point, create a (selected) control
	* point at that position.  
	* If the control key is pressed with the mouse button, then add this 
	* point to the already selected points; otherwise deselect other points.
	* If control is pressed and the picked point is already selected, 
	* then deselect the picked point.
	* If the shift button is pressed, remember that fact in the grab.  If this is
	* the start of a drag, the drag will be horizontally or vertically 
	* constrained.
	* This always grabs the point created or selected.
	* Whenever a color control point is selected (except when ctrl or shift
	* is pressed), that color is shown in the color picker.
	*/
void FlowMapFrame::mousePressEvent( QMouseEvent * e){
	if (!editor) return;
	mouseIsDown = true;
	if (e->button() == Qt::LeftButton){
		//Ignore mouse press over margin:
		if (e->y() >= (height() - COORDMARGIN)) return;
		//Check for domain grabs:
		//Look for both Opacity and Color grabs:
		if (e->y() < SLIDERWIDTH){
			int leftLim = editor->mapOpacVar2Win(editor->getMapperFunction()->getMinOpacMapValue(),false);
			int rightLim = editor->mapOpacVar2Win(editor->getMapperFunction()->getMaxOpacMapValue(),false);
			if (e->x() < leftLim -SLIDERWIDTH ||
				e->x() > rightLim +SLIDERWIDTH ) return;
			//Notify the params that an editing change is starting:
			startTFChange("Mapper function opacity domain boundary move");
			editor->setOpacDragStart(e->x(), e->y());
			editor->saveDomainBounds();
			amDragging = false;
			editor->unSelectAll();
			if (e->x() <= leftLim){
				editor->addLeftOpacDomainGrab();
				return;
			}
			if (e->x() >= rightLim){
				editor->addRightOpacDomainGrab();
				return;
			}
			editor->addFullOpacDomainGrab();
			return;
		//check for color slider grab:
		} else if (((e->y()) >= height() - (COLORBARWIDTH+COORDMARGIN+SLIDERWIDTH)) 
				&& ((e->y()) < height() - (COLORBARWIDTH+COORDMARGIN) )){
			int leftLim = editor->mapColorVar2Win(editor->getMapperFunction()->getMinColorMapValue(),false);
			int rightLim = editor->mapColorVar2Win(editor->getMapperFunction()->getMaxColorMapValue(),false);
			if (e->x() < leftLim -SLIDERWIDTH ||
				e->x() > rightLim +SLIDERWIDTH ) return;
			//Notify the params that an editing change is starting:
			startTFChange("Mapper function color domain boundary move");
			editor->setColorDragStart(e->x(), e->y());
			editor->saveDomainBounds();
			amDragging = false;
			editor->unSelectAll();
			if (e->x() <= leftLim){
				editor->addLeftColorDomainGrab();
				return;
			}
			if (e->x() >= rightLim){
				editor->addRightColorDomainGrab();
				return;
			}
			editor->addFullColorDomainGrab();
			return;
		}
		//Mouse effect in window is dependent on mode:
		if (editor->getParams()->getEditMode())
			mouseEditStart(e);	
		else 
			mouseNavigateStart(e);
		update();
		return;
	}//end response to left-button down
}//End mousePressEvent

// Mark the start of a mouse edit
//
void FlowMapFrame::
mouseEditStart(QMouseEvent* e){
	if (!editor) return;
	bool controlPressed=false;
	bool shiftPressed=false;
	//See if we can classify where the mouse is:
	int index;
	int type = editor->closestControlPoint(e->x(), e->y(), &index);

	//Notify the params that an editing change is starting:
	if (e->y() >= (height() - COLORBARWIDTH - COORDMARGIN - SLIDERWIDTH - SEPARATOR/2)){			
		startTFChange("Mapper function color bar edit");
		editor->setColorDragStart(e->x(), e->y());
	} else {
		startTFChange("Mapper function opacity edit");
		editor->setOpacDragStart(e->x(), e->y());
	}
	
	amDragging = false;
	dragType = 0;
	unSelectColorIndex = -1;
	unSelectOpacIndex = -1;
	
	//If shift pressed, ignore control-pressed.
	if (e->state() & Qt::ShiftButton) shiftPressed = true;
	else if (e->state() & Qt::ControlButton) controlPressed = true;
	else {//neither key pressed:
		editor->unSelectAll();
		editor->unGrab();
	}
	//Always reset constrained grabs:
	editor->removeConstrainedGrab();
		
	//Make changes in selection:
	//Turn off left-mouse creation:

	if (type == 0) { 
		return;
		/*
		//New control point, create, select it:
		if (e->y() >= (height() - COLORBARWIDTH - COORDMARGIN -SEPARATOR/2)){
			//new color control point, modify index
			index = editor->insertColorControlPoint(e->x());
			if (index >=0){
				emit (sendRgb(editor->getControlPointColor(index)));
				editor->addColorGrab();
				type = 1;
			}
		}
		else{
			//new opacity control point
			index = editor->insertOpacControlPoint(e->x(), e->y());
			if (index>=0) {
				editor->addOpacGrab();
				type = -1;
			}
		}
		*/
	}
	//Possibly prepare for unselecting a selected point.
	//If the user ctrl-clicks without dragging a selected
	//point then we will unselect it as soon as the mouse
	//button is released.
	else if (controlPressed && type == 1 && editor->colorSelected(index)){
		unSelectColorIndex = index;
		
	}
	else if (controlPressed && type == -1 && editor->opacSelected(index)){
		unSelectOpacIndex = index;
		
	}
	else if (type == 1){
		//select color control point.  
		editor->selectColor(index);
		emit (sendRgb(editor->getControlPointColor(index)));
		editor->addColorGrab();
	} else if (type == -1) { //type == -1 is existing opacity select
		editor->selectOpac(index);
		editor->addOpacGrab();
	} 
	//In any case, if shift is pressed, we fill selection interval, and
	//prepare for a constrained drag:
	if (shiftPressed) {
		editor->selectInterval(type>0);
		editor->addConstrainedGrab();
	} 
	update();
	return;
}

// Mark the start of a mouse zoom action
//

// Mark the start of a mouse navigate action
//
void FlowMapFrame::
mouseNavigateStart(QMouseEvent* e){
	if (!editor) return;
	startTFChange("Mapper function editor navigate");
	//Decide if we are navigating in color or opacity:
	if (e->y() >= (height() - COLORBARWIDTH - COORDMARGIN - SLIDERWIDTH - SEPARATOR/2)){			
		editor->setColorDragStart(e->x(), e->y());
		editor->setColorNavigateGrab();
	} else {
		editor->setOpacDragStart(e->x(), e->y());
		editor->setOpacNavigateGrab();
	}
	
	amDragging = false;
	dragType = 1;
	return;
}
//When mouse is released, if we were editing, tell the panel that
//there has been a change in the Mapper function.  May need to 
//update visualizer and/or tf display
void FlowMapFrame::mouseReleaseEvent( QMouseEvent *e ){
	if (!editor) return;
	if (e->button() == Qt::LeftButton){
		//If dragging bounds, ungrab, and notify params to update:
		if( editor->anyDomainGrabbed()){
			editor->unGrabBounds();
			editor->getParams()->updateMapBounds();
		} else { //otherwise, depends on mode 
			//
			if (dragType == 0) { //Edit mode:
				//If a ctrl-click occurred on a selected point, and
				//there was no drag, then unselect it.
				if (e->state() & Qt::ControlButton) {
					if (!amDragging){
						if (unSelectColorIndex >= 0){
							editor->unselectColor(unSelectColorIndex);
							if (editor->numColorSelected() == 0){
								editor->removeColorGrab();
							}
							
						}
						if (unSelectOpacIndex >= 0){
							editor->unselectOpac(unSelectOpacIndex);
							if (editor->numOpacSelected() == 0){
								editor->removeOpacGrab();
							}
							
						}
					}
				}
			}
		}
		//In any case, clean up, and TFE gui:
		amDragging = false;
		mouseIsDown = false;
		
		//Notify the params that an editing change is finishing:
		endTFChange();
		update();
	}
}
	
//When the mouse moves, display its new coordinates.  Move the "grabbed" 
//control point, or zoom/pan the display
//
void FlowMapFrame::mouseMoveEvent( QMouseEvent * e){
	if (!editor) return;
	if (editor->isGrabbed()) {
		amDragging = true;
		if (editor->opacDomainGrabbed()){
			editor->moveOpacDomainBound(e->x());
		} else if (editor->colorDomainGrabbed()){
			editor->moveColorDomainBound(e->x());
		} 
		else if (dragType == 0){
			editor->moveGrabbedControlPoints( e->x(), e->y());
		} else {
			assert(dragType == 1);
			editor->navigate(e->x(), e->y());
		} 
	} 
}
void FlowMapFrame::resizeEvent( QResizeEvent *  ){
	needUpdate = true;
}
/* 
	*delete all selected control points when delete key is pressed:
	*/
void FlowMapFrame::keyPressEvent(QKeyEvent* e){
	if (!editor) return;
	if (e->key() == Qt::Key_Delete){
		editor->deleteSelectedControlPoints();
		e->accept();
		update();
	}
}
//New HSV received from color picker.  Ignore it if the mouse is down.
void FlowMapFrame::newHsv(int h, int s, int v){
	if (!editor) return;
	if (mouseIsDown) return;
	editor->setHsv(h,s,v);
	
}
//Draw triangles at top and bottom of color bar
void FlowMapFrame::drawTris(QPainter& p, int x){
	for (int i = 0; i<COLORBARWIDTH/2; i++){
		int wid = (COLORBARWIDTH/2 -i)/2;
		p.drawLine(x -wid, height()-COORDMARGIN-i, x+wid, height()-COORDMARGIN-i);
		p.drawLine(x-wid, height()-COORDMARGIN-COLORBARWIDTH+i, x+wid, height() -COORDMARGIN-COLORBARWIDTH+i);
	}
}


void FlowMapFrame::contextMenuEvent( QContextMenuEvent *e )
{
	if (!editor) return;
	//Capture for undo/redo:
	editor->getParams()->confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(editor->getParams(), "TF editor right mouse action");
	
	
	//Is this near a control point?
	
	int index;
	int type = editor->closestControlPoint(e->x(), e->y(), &index);
	//type is 1 for color, -1 for opacity 0 for neither
    QPopupMenu* contextMenu = new QPopupMenu( this );
    QLabel *caption = new QLabel( "<font color=black><b>"
        "Edit Control Points</b></u></font>", this );
    caption->setAlignment( Qt::AlignCenter );
    contextMenu->insertItem( caption );
    int itemNum;
	if(type == 1) {
		itemNum = contextMenu->insertItem( "&Adjust color control point", this, SLOT(adjColor(int)), CTRL+Key_A );
		contextMenu->setItemParameter(itemNum, index);
		itemNum = contextMenu->insertItem("&Delete color control point", this, SLOT(delColorPoint(int)), CTRL+Key_D);
		contextMenu->setItemParameter(itemNum, index);
	}
	if(type == -1) {
		itemNum = contextMenu->insertItem( "&Adjust opacity control point", this, SLOT(adjOpac(int)), CTRL+Key_A );
		contextMenu->setItemParameter(itemNum, index);
		itemNum = contextMenu->insertItem("&Delete opacity control point", this, SLOT(delOpacPoint(int)), CTRL+Key_D);
		contextMenu->setItemParameter(itemNum, index);
	}
	if (type == 0){
		itemNum = contextMenu->insertItem( "New &Opacity control point",  this, SLOT(newOpac(int)), CTRL+Key_O );
		//encode x and y as one int to send in QT message
		//We certainly don't need more than 16 bits per coord
		int code = e->x() | (e->y())<<16;
		contextMenu->setItemParameter(itemNum, code);
		itemNum = contextMenu->insertItem( "New &Color control point", this, SLOT(newColor(int)), CTRL+Key_C);
		contextMenu->setItemParameter(itemNum, e->x());
	}
	contextMenu->exec( QCursor::pos() );
    delete contextMenu;
	PanelCommand::captureEnd(cmd, editor->getParams());
}
/* SLOTS:
*/
void FlowMapFrame::
delColorPoint(int indx){
	editor->deleteControlPoint(indx, true);
}
void FlowMapFrame::
delOpacPoint(int indx){
	editor->deleteControlPoint(indx, false);
}
void FlowMapFrame::
newColor(int x){
	editor->insertColorControlPoint(x);
}
void FlowMapFrame::
newOpac(int code){
	//decode the x and y from the int code:
	int y = code>>16;
	int x = (code <<16)>>16;
	editor->insertOpacControlPoint(x,y);
}

void FlowMapFrame::
adjColor(int indx){
	ColorAdjustDialog* dlg = new ColorAdjustDialog(this, editor, indx);
	if(dlg->exec()) update();
	delete dlg;
}

void FlowMapFrame::
adjOpac(int indx){
	OpacAdjustDialog* dlg = new OpacAdjustDialog(this, editor, indx);
	if(dlg->exec()) update();
	delete dlg;
}
