//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		tfframe.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Defines the TFFrame class.  This provides
//		a frame in which the transfer function editing occurs.
//		Principally involved in drawing and responding to mouse events.
//		This class is used as a custom dialog for Qt Designer, so that
//		various QT dialogs can easily embed a tf editor.
//
#ifndef TFFRAME_H
#define TFFRAME_H

#include <qframe.h>
#include <qwidget.h>
#include <qimage.h>
#include "tfelocationtip.h"
class QLabel;
class QWidget;

using namespace VAPoR;

class VAPoR::TFEditor;

class TFFrame : public QFrame {
	Q_OBJECT
public:
	TFFrame( QWidget * parent = 0, const char * name = 0, WFlags f = 0 );
	~TFFrame();
	void setEditor(VAPoR::TFEditor* e) {editor = e; locationTip->setEditor(e);}
	void startTFChange(char* s){editor->getTransferFunction()->startChange(s);}
	void endTFChange(){editor->getTransferFunction()->endChange();}
	VAPoR::TFEditor* getEditor(){return editor;}
	void notifyColorSelector(QRgb clr){
		emit(sendRgb(clr));
	}
		
public slots:
	//Receive new color from color picker frame:
	//
    void newHsv( int h, int s, int v );
	//Catch color changes
	//
	void startColorChange(){editor->getTransferFunction()->startChange("select TF color");}
	void endColorChange() {editor->getTransferFunction()->endChange();}
	void delColorPoint(int indx);
	void delOpacPoint(int intx);
	void newColor(int x);
	void newOpac(int x);
	void adjColor(int indx);
	void adjOpac(int indx);
signals:
	//send a new color out
	void sendRgb(QRgb rgb);
protected:
	
	//Virtual, Reimplemented here:
	void paintEvent(QPaintEvent* event);
	void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );
	void contextMenuEvent( QContextMenuEvent *e );
	
	void resizeEvent( QResizeEvent * );
	void keyPressEvent( QKeyEvent *);
	void drawTris(QPainter& p, int x);
	QPixmap pxMap;
	
	VAPoR::TFEditor* editor;
	VAPoR::TFELocationTip *locationTip;
	bool needUpdate;
	bool amDragging;
	bool mouseIsDown;
	int unSelectColorIndex, unSelectOpacIndex;
	//Array of QLabels
	QLabel* tfColorLabels[10];
	QLabel* tfOpacLabels[10]; 
};


#endif

