//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		tfframe.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the TFFrame class.  This provides
//		a frame in which the transfer function editing occurs.
//		Principally involved in drawing and responding to mouse events.
//
#include "tfframe.h"
#include <qframe.h>
#include <qwidget.h>
#include <qimage.h>
#include <qpainter.h>
#include "tfeditor.h"
#include "tfelocationtip.h"
#include "dvrparams.h"
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qcursor.h>
#include <qlineedit.h>
#include "coloradjustdialog.h"
#include "opacadjustdialog.h"
#include "panelcommand.h"

TFFrame::TFFrame( QWidget * parent, const char * name, WFlags f ) :
	QFrame(parent, name, f) {
	//TFFrame is created by the dvr frame
	pxMap.resize(size());
	pxMap.fill(QColor(255,0,0));
	
	editor = 0;
	needUpdate = true;
	locationTip = new TFELocationTip(this);
	setFocusPolicy(QWidget::StrongFocus);
	amDragging = false;
	//Create labels to show with editor
	for (int i = 0; i<10; i++){
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
	TFFrame::~TFFrame() {
		delete locationTip;
		//Will these be deleted by their parent????
		for (int i = 0; i<10; i++){
			delete tfColorLabels[i];
			delete tfOpacLabels[i];
		}
	}
	

	void TFFrame::paintEvent(QPaintEvent* e){
		int i;
		if (!editor) return;
		if (editor->isDirty()|| needUpdate){
			editor->refreshImage();
			pxMap.convertFromImage(*editor->getImage(),0);
			
			needUpdate = false;
			
			
		}
		TransferFunction* tf = editor->getTransferFunction();
		//Debugging code to look at image column 100
		/*
		QImage* img0 = editor->getImage();
		QImage* img1  = editor->getOpacImage();
		int ra[200], ga[200], ba[200], alphaa[200];
		int ro[200], go[200], bo[200], alphao[200];
		QRgb rgbaa, rgbab;
		for (int j = 0; j< height()-BELOWOPACITY; j++){
			rgbaa = img0->pixel(100,j);
			rgbab = img1->pixel(100,j);
			ra[j] = qRed(rgbaa);
			ga[j] = qGreen(rgbaa);
			ba[j] = qBlue(rgbaa);

			alphaa[j]  = qAlpha(rgbab);
			ro[j] = qRed(rgbab);
			go[j] = qGreen(rgbab);
			bo[j] = qBlue(rgbab);
			alphao[j]  = qAlpha(rgbab);
		}
*/
		QPainter painter(&pxMap);
        painter.setClipRect(e->rect());
		
		
		int x, y;
		QBrush drawBrush(SolidPattern);
		for (i = 0; i<editor->getNumColorControlPoints(); i++){
			if (editor->colorSelected(i))painter.setPen(HIGHLIGHTCOLOR); else
				painter.setPen(CONTROLPOINTCOLOR);
			editor->getColorControlPointPosition(i, &x);
			if (x >= -2 && x <= width()+2){
				drawTris(painter, x);
			}
		}
		//Hide labels:
		for (i = 0; i<10; i++){
			tfColorLabels[i]->hide();
			tfOpacLabels[i]->hide();
		}
		//Draw tic marks for selected control points
		painter.setPen(black);
		int recentTicPosition = -40;
		int numColorLabels = 0;
		int numOpacLabels = 0;
		for (i = 0; i<editor->getNumColorControlPoints(); i++){
			editor->getColorControlPointPosition(i, &x);
			if (editor->colorSelected(i) && x >=0 && x < width()){
				painter.drawLine(x,height()-COORDMARGIN+3,x,height());
				//Possibly put a label here:
				if (numColorLabels < 10 && x>recentTicPosition+40){
					tfColorLabels[numColorLabels]->move(pos()+QPoint(x-25, height()+15));
				
					tfColorLabels[numColorLabels]->setText(QString::number(editor->getTransferFunction()->
						colorCtrlPointPosition(i),'g',4));
					tfColorLabels[numColorLabels]->raise();
					tfColorLabels[numColorLabels++]->show();
					recentTicPosition = x;
				}
			}
		}
		recentTicPosition = -40;
		for (i = 0; i<editor->getNumOpacControlPoints(); i++){
			editor->getOpacControlPointPosition(i, &x, &y);
			if (editor->opacSelected(i) && x >=0 && x < width()){
				painter.drawLine(x,height()-COORDMARGIN+3,x,height());
				//Possibly put a label here:
				if (numOpacLabels < 10 && x>recentTicPosition+40){
					tfOpacLabels[numOpacLabels]->move(pos()+QPoint(x-20, height()));
				
					tfOpacLabels[numOpacLabels]->setText(QString::number(editor->getTransferFunction()->
						opacCtrlPointPosition(i),'g',4));
					tfOpacLabels[numOpacLabels]->raise();
					tfOpacLabels[numOpacLabels++]->show();
					recentTicPosition = x;
				}
			}
		}
		//Draw the opacity curve, 2 pixels wide
		QPen myPen(OPACITYCURVECOLOR, 2);
		painter.setPen(myPen);
		float prevX = editor->mapWin2Var(0);
		float prevY = tf->opacityValue(prevX);
		int atEnd = 0;
		int prevIndex = tf->getLeftOpacityIndex(prevX);
		int prevIntX = 0;
		int prevIntY = (int)((1.f - prevY)*(height() -BELOWOPACITY + 0.5f));
		int nextIntX, nextIntY;
		int lastX = editor->mapVar2Win(tf->getMaxMapValue());
		//Draw a curve on the x-axis before domain start:
		if (prevX < tf->getMinMapValue()){
			//Find out what xcoord maps to minMapValue:
			nextIntX = editor->mapVar2Win(tf->getMinMapValue(),false);
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
		//Draw left and right limits (if on screen):
		int leftLim = editor->mapVar2Win(tf->getMinMapValue(),false);
		int rightLim = editor->mapVar2Win(tf->getMaxMapValue(),false);
		
		QPen vpen(ENDLINECOLOR, 3);
		drawBrush.setColor(ENDLINECOLOR);
		
		int halfHt = (height()-BELOWOPACITY)/2;
		if (leftLim > 0){
			if (editor->leftDomainGrabbed()){
				vpen.setColor(HIGHLIGHTCOLOR);
				painter.setPen(vpen);
			} else {
				vpen.setColor(ENDLINECOLOR);
				painter.setPen(vpen);
			}
			painter.drawLine(leftLim, height() -BELOWOPACITY, leftLim, 0);
			painter.fillRect(leftLim - 3, halfHt -3, 6, 6,drawBrush);
		}
		if (rightLim < width()){
			if (editor->rightDomainGrabbed()){
				vpen.setColor(HIGHLIGHTCOLOR);
				painter.setPen(vpen);
			} else {
				vpen.setColor(ENDLINECOLOR);
				painter.setPen(vpen);
			}
			painter.drawLine(rightLim, height() -BELOWOPACITY, rightLim, 0);
			painter.fillRect(rightLim - 3, halfHt -3, 6, 6,drawBrush);
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
	 * If the "b" key is pressed on a control point, and another control point
	 * (in the other (color/opacity) space is selected, then bind the previously
	 * selected point to this one (i.e. move it to the same x-coord).  The two
	 * will continue to move together if this is a drag.
	 * This always grabs the point created or selected.
	 * Whenever a color control point is selected (except when ctrl or shift
	 * is pressed), that color is shown in the color picker.
	 */
	void TFFrame::mousePressEvent( QMouseEvent * e){
		bool controlPressed=false;
		bool shiftPressed=false;
		mouseIsDown = true;
		if (e->button() == Qt::LeftButton){
			//Ignore mouse press over margin:
			if (e->y() >= (height() - COORDMARGIN)) return;
			//Notify the DVR that an editing change is starting:
			if (e->y() >= (height() - BARHEIGHT - COORDMARGIN - SEPARATOR/2)){
				startTFChange("transfer function color bar edit");
			} else {
				startTFChange("transfer function opacity edit");
			}
			editor->setDragStart(e->x(), e->y());
			amDragging = false;
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
			int index;
			int type = editor->closestControlPoint(e->x(), e->y(), &index);
			
			if (type == 0) { //New control point, create, select it:
				if (e->y() >= (height() - BARHEIGHT - COORDMARGIN -SEPARATOR/2)){
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
			} else if (type == -2){ //left domain grab
				editor->unSelectAll();
				editor->addLeftDomainGrab();
			} else if (type == 2) {
				editor->unSelectAll();
				editor->addRightDomainGrab();
			}
			//In any case, if shift is pressed, we fill selection interval, and
			//prepare for a constrained drag:
			if (shiftPressed && !editor->domainGrabbed()) {
				editor->selectInterval(type>0);
				editor->addConstrainedGrab();
			} 
			update();
			return;
		}
	}
	//When mouse is released, tell the panel that
	//there has been a change in the transfer function:
    void TFFrame::mouseReleaseEvent( QMouseEvent *e ){
		
		if (e->button() == Qt::LeftButton){
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
		
			amDragging = false;
			mouseIsDown = false;
			//If dragging bounds, ungrab, and notify dvrparams to update:
			if( editor->domainGrabbed()){
				editor->unGrabBounds();
				editor->getParams()->updateTFBounds();
			}
			//Check whether to enable the bind option:
			editor->getParams()->setBindButtons();
			//Notify the DVR that an editing change is starting:
			endTFChange();
			update();
		}
	}
	
	//When the mouse moves, display its new coordinates.  Move the "grabbed" 
	//control point
    void TFFrame::mouseMoveEvent( QMouseEvent * e){
		if (editor->isGrabbed()) {
			amDragging = true;
			if (editor->domainGrabbed()){
				editor->moveDomainBound(e->x());
			} else {
				editor->moveGrabbedControlPoints( e->x(), e->y());
			}
		} 
	}
	void TFFrame::resizeEvent( QResizeEvent *  ){
		needUpdate = true;
	}
	/* 
	 *delete all selected control points when delete key is pressed:
	 */
	void TFFrame::keyPressEvent(QKeyEvent* e){
		if (e->key() == Qt::Key_Delete){
			editor->deleteSelectedControlPoints();
			e->accept();
			update();
		}
	}
	//New HSV received from color picker.  Ignore it if the mouse is down.
	void TFFrame::newHsv(int h, int s, int v){
		if (mouseIsDown) return;
		editor->setHsv(h,s,v);
		
	}
	//Draw triangles at top and bottom of color bar
	void TFFrame::drawTris(QPainter& p, int x){
		for (int i = 0; i<BARHEIGHT/2; i++){
			int wid = (BARHEIGHT/2 -i)/2;
			p.drawLine(x -wid, height()-COORDMARGIN-i, x+wid, height()-COORDMARGIN-i);
			p.drawLine(x-wid, height()-COORDMARGIN-BARHEIGHT+i, x+wid, height() -COORDMARGIN-BARHEIGHT+i);
		}
	}


void TFFrame::contextMenuEvent( QContextMenuEvent *e )
{
	//Capture for undo/redo:
	editor->getParams()->confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(editor->getParams(), editor->getSession(),"TF editor right mouse action");
	
	
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
void TFFrame::
delColorPoint(int indx){
	editor->deleteControlPoint(indx, true);
}
void TFFrame::
delOpacPoint(int indx){
	editor->deleteControlPoint(indx, false);
}
void TFFrame::
newColor(int x){
	editor->insertColorControlPoint(x);
}
void TFFrame::
newOpac(int code){
	//decode the x and y from the int code:
	int y = code>>16;
	int x = (code <<16)>>16;
	editor->insertOpacControlPoint(x,y);
}

void TFFrame::
adjColor(int indx){
	ColorAdjustDialog* dlg = new ColorAdjustDialog(this, indx);
	if(dlg->exec()) update();
	delete dlg;
}

void TFFrame::
adjOpac(int indx){
	OpacAdjustDialog* dlg = new OpacAdjustDialog(this, indx);
	if(dlg->exec()) update();
	delete dlg;
}
