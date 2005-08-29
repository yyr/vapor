//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		flowmapframe.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2005
//
//	Description:	Defines the FlowMapFrame class.  This provides
//		a frame in which the mapper function editing occurs.
//		Principally involved in drawing and responding to mouse events.
//		This class is used as a custom dialog for Qt Designer, so that
//		various QT dialogs can easily embed a tf editor.
//
#ifndef FLOWMAPFRAME_H
#define FLOWMAPFRAME_H
#define MAXNUMLABELS 10
#include <qframe.h>
#include <qwidget.h>
#include <qimage.h>
#include "tfelocationtip.h"
#include "flowmapeditor.h"
class QLabel;
class QWidget;

using namespace VAPoR;
class FlowMapFrame : public QFrame {
	Q_OBJECT
public:
	FlowMapFrame( QWidget * parent = 0, const char * name = 0, WFlags f = 0 );
	~FlowMapFrame();
	void setEditor(VAPoR::FlowMapEditor* e) {editor = e; locationTip->setEditor(e);}
	void startTFChange(char* s){editor->getMapperFunction()->startChange(s);}
	void endTFChange(){editor->getMapperFunction()->endChange();}
	VAPoR::FlowMapEditor* getEditor(){return editor;}
	void notifyColorSelector(QRgb clr){
		emit(sendRgb(clr));
	}
	void setDirtyEditor(VAPoR::FlowMapEditor* ed) {dirtyEditor = ed;}
	
		
public slots:
	//Receive new color from color picker frame:
	//
    void newHsv( int h, int s, int v );
	//Catch color changes
	//
	void startColorChange(){if(editor)editor->getMapperFunction()->startChange("select TF color");}
	void endColorChange() {if(editor)editor->getMapperFunction()->endChange();}
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
	VAPoR::FlowMapEditor* dirtyEditor;
	//Virtual, Reimplemented here:
	void paintEvent(QPaintEvent* event);
	void mousePressEvent( QMouseEvent * );
    void mouseReleaseEvent( QMouseEvent * );
    void mouseMoveEvent( QMouseEvent * );
	void contextMenuEvent( QContextMenuEvent *e );
	
	void resizeEvent( QResizeEvent * );
	void keyPressEvent( QKeyEvent *);
	void drawTris(QPainter& p, int x);

	void mouseEditStart(QMouseEvent*);
	void mouseNavigateStart(QMouseEvent*);

	QPixmap pxMap;
	
	VAPoR::FlowMapEditor* editor;
	VAPoR::TFELocationTip *locationTip;
	bool needUpdate;
	bool amDragging;
	int dragType; //0 for edit, 1 for navigate
	bool mouseIsDown;
	int unSelectColorIndex, unSelectOpacIndex;
	//Array of QLabels
	QLabel* tfColorLabels[MAXNUMLABELS];
	QLabel* tfOpacLabels[MAXNUMLABELS]; 
};


#endif

