
//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		colorpickerframe.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the colorpicker class.  This
//		was adapted from the QColorDialog, includes various
//		widgets from that class.  Unlike the QColorDialog, this
//		is not modal, and it supports connections to set and get
//		the color (in rgb).  This also omits display of the users
//		palette of previously used colors.  This is intended to be 
//		imbedded in the DVR and other tab panels
//
#ifndef COLORPICKER_H
#define COLORPICKER_H
#include <QFrame>
#include <qlineedit.h>
#include <qvalidator.h>
#include <QPaintEvent>
#include <QPixmap>
#include <QLabel>
#include <QMouseEvent>

class QPixmap;
class QLabel;

class ColorPicker : public QFrame
{
    Q_OBJECT
public:
    ColorPicker(QWidget* parent=0);
    ~ColorPicker();

public slots:
    void setCol( int h, int s );

signals:
    void newCol( int h, int s );
	void mouseUp();
	void mouseDown();

protected:
    QSize sizeHint() const;
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent( QMouseEvent * );
    void mousePressEvent( QMouseEvent * );
	void mouseReleaseEvent(QMouseEvent *){
		emit(mouseUp());}

private:
    int hue;
    int sat;

    QPoint colPt();
    int huePt( const QPoint &pt );
    int satPt( const QPoint &pt );
    void setCol( const QPoint &pt );

    QPixmap *pix;
};


class ColorShowLabel : public QFrame
{
    Q_OBJECT

public:
    ColorShowLabel( QWidget *parent ) : QFrame( parent) {
	setFrameStyle( QFrame::Panel|QFrame::Sunken );
	setAcceptDrops( TRUE );
	mousePressed = FALSE;
    }
    void setColor( QColor c ) { col = c; }

signals:
    void colorDropped( QRgb );

protected:
	void paintEvent(QPaintEvent *);
    
    void mousePressEvent( QMouseEvent *e );
    void mouseMoveEvent( QMouseEvent *e );
    void mouseReleaseEvent( QMouseEvent *e );


private:
    QColor col;
    bool mousePressed;
    QPoint pressPos;

};
class ColNumLineEdit : public QLineEdit
{
public:
    ColNumLineEdit( QWidget *parent )
	: QLineEdit( parent ) { setMaxLength( 3 );}
    QSize sizeHint() const {
		int margL,margT,margR,margB;
		getContentsMargins(&margL,&margT,&margR, &margB);
		int frameWidth = frameGeometry().width()-width();
		return QSize( fontMetrics().width( "999" ) + 2 * ( margL + frameWidth ),
		      QLineEdit::sizeHint().height() ); 
	}
    void setNum( int i ) {
	QString s;
	s.setNum(i);
	bool block = signalsBlocked();
	blockSignals(TRUE);
	setText( s );
	blockSignals(block);
    }
    int val() const { return text().toInt(); }
};
class ColorShower : public QWidget
{
    Q_OBJECT
public:
    ColorShower( QWidget *parent );

    //things that don't emit signals
    void setHsv( int h, int s, int v );

    int currentAlpha() const { return alphaEd->val(); }
    void setCurrentAlpha( int a ) { alphaEd->setNum( a ); }
    void showAlpha( bool b );


    QRgb currentColor() const { return curCol; }

public slots:
    void setRgb( QRgb rgb );

signals:
    void newCol( QRgb rgb );
private slots:
    void rgbEd();
    void hsvEd();
private:
    void showCurrentColor();
    int hue, sat, val;
    QRgb curCol;
    ColNumLineEdit *hEd;
    ColNumLineEdit *sEd;
    ColNumLineEdit *vEd;
    ColNumLineEdit *rEd;
    ColNumLineEdit *gEd;
    ColNumLineEdit *bEd;
    ColNumLineEdit *alphaEd;
    QLabel *alphaLab;
    ColorShowLabel *lab;
    bool rgbOriginal;
};
class ColorLuminancePicker : public QWidget
{
    Q_OBJECT
public:
    ColorLuminancePicker(QWidget* parent=0);
    ~ColorLuminancePicker();

public slots:
    void setCol( int h, int s, int v );
    void setCol( int h, int s );
	

signals:
    void newHsv( int h, int s, int v );
	void mouseUp();
	void mouseDown();

protected:
    void paintEvent( QPaintEvent*);
    void mouseMoveEvent( QMouseEvent * );
    void mousePressEvent( QMouseEvent * );
	void mouseReleaseEvent(QMouseEvent *){
		emit(mouseUp());}

private:
    enum { foff = 3, coff = 4 }; //frame and contents offset
    int val;
    int hue;
    int sat;

    int y2val( int y );
    int val2y( int val );
    void setVal( int v );

    QPixmap *pix;
};
class ColIntValidator: public QIntValidator
{
public:
    ColIntValidator( int bottom, int top,
		   QWidget * parent)
	:QIntValidator( bottom, top, parent ) {}

    QValidator::State validate( QString &, int & ) const;
};
static inline void rgb2hsv( QRgb rgb, int&h, int&s, int&v )
{
    QColor c;
    c.setRgb( rgb );
    c.getHsv(&h,&s,&v);
}
class ColorPickerFrame : public QFrame
{
Q_OBJECT
public:
    ColorPickerFrame( QWidget*);
    QRgb currentColor() const { return cs->currentColor(); }
    void setCurrentColor( QRgb rgb );

public slots:
    void newHsv( int h, int s, int v );
    void newColorTypedIn( QRgb rgb );
	void mouseDownRelay();
	void mouseUpRelay();
   
public:
    ColorPicker *cp;
    ColorLuminancePicker *lp;
    ColorShower *cs;

signals:
    void hsvOut( int h, int s, int v );
	void startColorChange();
	void endColorChange();
	void mouseUp();
	void mouseDown();
};

#endif

