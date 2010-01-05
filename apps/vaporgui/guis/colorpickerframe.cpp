//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		colorpicker.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the colorpicker class.  
//


#include <qwidget.h>
#include <q3frame.h>
#include <qimage.h>
#include <qpainter.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qapplication.h>
#include <q3dragobject.h>

#include <qlayout.h>
#include <qpushbutton.h>
#include <qpixmap.h>
#include <qdrawutil.h>
#include <qvalidator.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3PointArray>
#include <QDropEvent>
#include <QDragLeaveEvent>
#include <QPaintEvent>
#include <Q3GridLayout>
#include <QMouseEvent>
#include <Q3VBoxLayout>
#include <QDragEnterEvent>
//#include "qgridview.h"
//#include "qstyle.h"
//#include "qsettings.h"
//#include "qpopupmenu.h"

#include "colorpickerframe.h"
//Color picker dimensions:
static int pWidth = 140;
static int pHeight = 120;
QPoint ColorPicker::colPt()
{ return QPoint( (360-hue)*(pWidth-1)/360, (255-sat)*(pHeight-1)/255 ); }
int ColorPicker::huePt( const QPoint &pt )
{ return 360 - pt.x()*360/(pWidth-1); }
int ColorPicker::satPt( const QPoint &pt )
{ return 255 - pt.y()*255/(pHeight-1) ; }
void ColorPicker::setCol( const QPoint &pt )
{ setCol( huePt(pt), satPt(pt) ); }

ColorPicker::ColorPicker(QWidget* parent, const char* name )
    : Q3Frame( parent, name )
{
    hue = 0; sat = 0;
    setCol( 150, 255 );

    QImage img( pWidth, pHeight, 32 );
    int x,y;
    for ( y = 0; y < pHeight; y++ )
	for ( x = 0; x < pWidth; x++ ) {
	    QPoint p( x, y );
	    img.setPixel( x, y, QColor(huePt(p), satPt(p),
				       200, QColor::Hsv).rgb() );
	}
    pix = new QPixmap;
    pix->convertFromImage(img,0);
    setBackgroundMode( Qt::NoBackground );
    setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed )  );
	setEnabled(false);
}

ColorPicker::~ColorPicker()
{
    delete pix;
}

QSize ColorPicker::sizeHint() const
{
    return QSize( pWidth + 2*frameWidth(), pHeight + 2*frameWidth() );
}

void ColorPicker::setCol( int h, int s )
{
    int nhue = QMIN( QMAX(0,h), 360 );
    int nsat = QMIN( QMAX(0,s), 255);
    if ( nhue == hue && nsat == sat )
	return;
    QRect r( colPt(), QSize(20,20) );
    hue = nhue; sat = nsat;
    r = r.unite( QRect( colPt(), QSize(20,20) ) );
    r.moveBy( contentsRect().x()-9, contentsRect().y()-9 );
    //    update( r );
    repaint( r, FALSE );
}

void ColorPicker::mouseMoveEvent( QMouseEvent *m )
{
    QPoint p = m->pos() - contentsRect().topLeft();
    setCol( p );
    emit newCol( hue, sat );
}

void ColorPicker::mousePressEvent( QMouseEvent *m )
{
	emit mouseDown();
    QPoint p = m->pos() - contentsRect().topLeft();
    setCol( p );
	
    emit newCol( hue, sat );
}

void ColorPicker::drawContents(QPainter* p)
{
    QRect r = contentsRect();

    p->drawPixmap( r.topLeft(), *pix );
    QPoint pt = colPt() + r.topLeft();
    p->setPen( QPen(Qt::black) );

    p->fillRect( pt.x()-9, pt.y(), 20, 2, Qt::black );
    p->fillRect( pt.x(), pt.y()-9, 2, 20, Qt::black );

}



void ColorShowLabel::drawContents( QPainter *p )
{
    p->fillRect( contentsRect(), col );
}


void ColorShowLabel::mousePressEvent( QMouseEvent *e )
{
    mousePressed = TRUE;
    pressPos = e->pos();
}

void ColorShowLabel::mouseMoveEvent( QMouseEvent *e )
{
#ifndef QT_NO_DRAGANDDROP
    if ( !mousePressed )
	return;
    if ( ( pressPos - e->pos() ).manhattanLength() > QApplication::startDragDistance() ) {
	Q3ColorDrag *drg = new Q3ColorDrag( col, this );
	QPixmap pix( 30, 20 );
	pix.fill( col );
	QPainter p( &pix );
	p.drawRect( 0, 0, pix.width(), pix.height() );
	p.end();
	drg->setPixmap( pix );
	mousePressed = FALSE;
	drg->dragCopy();
    }
#endif
}

#ifndef QT_NO_DRAGANDDROP
void ColorShowLabel::dragEnterEvent( QDragEnterEvent *e )
{
    if ( Q3ColorDrag::canDecode( e ) )
	e->accept();
    else
	e->ignore();
}

void ColorShowLabel::dragLeaveEvent( QDragLeaveEvent * )
{
}

void ColorShowLabel::dropEvent( QDropEvent *e )
{
    if ( Q3ColorDrag::canDecode( e ) ) {
	Q3ColorDrag::decode( e, col );
	repaint( FALSE );
	emit colorDropped( col.rgb() );
	e->accept();
    } else {
	e->ignore();
    }
}
#endif // QT_NO_DRAGANDDROP

void ColorShowLabel::mouseReleaseEvent( QMouseEvent * )
{
    if ( !mousePressed )
	return;
    mousePressed = FALSE;
}

ColorShower::ColorShower( QWidget *parent, const char *name )
    :QWidget( parent, name)
{
    curCol = qRgb( -1, -1, -1 );
    ColIntValidator *val256 = new ColIntValidator( 0, 255, this );
    ColIntValidator *val360 = new ColIntValidator( 0, 360, this );

    Q3GridLayout *gl = new Q3GridLayout( this, 1, 1, 6 );
	/*
    lab = new ColorShowLabel( this );
    lab->setMinimumWidth( 60 ); //###
    gl->addMultiCellWidget(lab, 0,-1,0,0);
    connect( lab, SIGNAL( colorDropped(QRgb) ),
	     this, SIGNAL( newCol(QRgb) ) );
    connect( lab, SIGNAL( colorDropped(QRgb) ),
	     this, SLOT( setRgb(QRgb) ) );
*/
    hEd = new ColNumLineEdit( this, "qt_hue_edit" );
    hEd->setValidator( val360 );
    QLabel *l = new QLabel( hEd, "Hu&e:", this, "qt_hue_lbl" );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 0, 1 );
    gl->addWidget( hEd, 0, 2 );

    sEd = new ColNumLineEdit( this, "qt_sat_edit" );
    sEd->setValidator( val256 );
    l = new QLabel( sEd, "&Sat:", this, "qt_sat_lbl" );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 1, 1 );
    gl->addWidget( sEd, 1, 2 );

    vEd = new ColNumLineEdit( this, "qt_val_edit" );
    vEd->setValidator( val256 );
    l = new QLabel( vEd, "&Val:", this, "qt_val_lbl" );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 2, 1 );
    gl->addWidget( vEd, 2, 2 );

    rEd = new ColNumLineEdit( this, "qt_red_edit" );
    rEd->setValidator( val256 );
    l = new QLabel( rEd, "&Red:", this, "qt_red_lbl" );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 0, 3 );
    gl->addWidget( rEd, 0, 4 );

    gEd = new ColNumLineEdit( this, "qt_grn_edit" );
    gEd->setValidator( val256 );
    l = new QLabel( gEd, "&Green:", this, "qt_grn_lbl" );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 1, 3 );
    gl->addWidget( gEd, 1, 4 );

    bEd = new ColNumLineEdit( this, "qt_blue_edit" );
    bEd->setValidator( val256 );
    l = new QLabel( bEd, "Bl&ue:", this, "qt_blue_lbl" );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 2, 3 );
    gl->addWidget( bEd, 2, 4 );

	//AN 2/26/07
	//Modified following so typed-in colors are only used after
	//<enter> is pressed in a color text box.
	//Makes it consistent with rest of VAPOR app, and easier to
	//specify a color by its 3 components
    //connect( hEd, SIGNAL(textChanged(const QString&)), this, SLOT(hsvEd()) );
    //connect( sEd, SIGNAL(textChanged(const QString&)), this, SLOT(hsvEd()) );
    //connect( vEd, SIGNAL(textChanged(const QString&)), this, SLOT(hsvEd()) );
	connect(hEd,SIGNAL(returnPressed()),this, SLOT(hsvEd()));
	connect(sEd,SIGNAL(returnPressed()),this, SLOT(hsvEd()));
	connect(vEd,SIGNAL(returnPressed()),this, SLOT(hsvEd()));

    //connect( rEd, SIGNAL(textChanged(const QString&)), this, SLOT(rgbEd()) );
    //connect( gEd, SIGNAL(textChanged(const QString&)), this, SLOT(rgbEd()) );
    //connect( bEd, SIGNAL(textChanged(const QString&)), this, SLOT(rgbEd()) );

	connect( rEd, SIGNAL(returnPressed()), this, SLOT(rgbEd()) );
    connect( gEd, SIGNAL(returnPressed()), this, SLOT(rgbEd()) );
    connect( bEd, SIGNAL(returnPressed()), this, SLOT(rgbEd()) );

	lab = new ColorShowLabel( this );
    lab->setMinimumWidth( 60 ); //###
	lab->setMinimumHeight(30);
    gl->addMultiCellWidget(lab, 3,3,0,4);
    connect( lab, SIGNAL( colorDropped(QRgb) ),
	     this, SIGNAL( newCol(QRgb) ) );
    connect( lab, SIGNAL( colorDropped(QRgb) ),
	     this, SLOT( setRgb(QRgb) ) );
    
}

void ColorShower::showCurrentColor()
{
    lab->setColor( currentColor() );
    lab->repaint(FALSE); //###
}

void ColorShower::rgbEd()
{
    rgbOriginal = TRUE;
    
	curCol = qRgb( rEd->val(), gEd->val(), bEd->val() );

    rgb2hsv(currentColor(), hue, sat, val );

    hEd->setNum( hue );
    sEd->setNum( sat );
    vEd->setNum( val );

    showCurrentColor();
    emit newCol( currentColor() );
}

void ColorShower::hsvEd()
{
    rgbOriginal = FALSE;
    hue = hEd->val();
    sat = sEd->val();
    val = vEd->val();

    curCol = QColor( hue, sat, val, QColor::Hsv ).rgb();

    rEd->setNum( qRed(currentColor()) );
    gEd->setNum( qGreen(currentColor()) );
    bEd->setNum( qBlue(currentColor()) );

    showCurrentColor();
    emit newCol( currentColor() );
}

void ColorShower::setRgb( QRgb rgb )
{
    rgbOriginal = TRUE;
    curCol = rgb;

    rgb2hsv( currentColor(), hue, sat, val );

    hEd->setNum( hue );
    sEd->setNum( sat );
    vEd->setNum( val );

    rEd->setNum( qRed(currentColor()) );
    gEd->setNum( qGreen(currentColor()) );
    bEd->setNum( qBlue(currentColor()) );

    showCurrentColor();
}

void ColorShower::setHsv( int h, int s, int v )
{
    rgbOriginal = FALSE;
    hue = h; val = v; sat = s; //Range check###
    curCol = QColor( hue, sat, val, QColor::Hsv ).rgb();

    hEd->setNum( hue );
    sEd->setNum( sat );
    vEd->setNum( val );

    rEd->setNum( qRed(currentColor()) );
    gEd->setNum( qGreen(currentColor()) );
    bEd->setNum( qBlue(currentColor()) );

    showCurrentColor();
}


int ColorLuminancePicker::y2val( int y )
{
    int d = height() - 2*coff - 1;
    return 255 - (y - coff)*255/d;
}

int ColorLuminancePicker::val2y( int v )
{
    int d = height() - 2*coff - 1;
    return coff + (255-v)*d/255;
}

ColorLuminancePicker::ColorLuminancePicker(QWidget* parent,
						  const char* name)
    :QWidget( parent, name )
{
    hue = 100; val = 100; sat = 100;
    pix = 0;
    //    setBackgroundMode( NoBackground );
}

ColorLuminancePicker::~ColorLuminancePicker()
{
    delete pix;
}

void ColorLuminancePicker::mouseMoveEvent( QMouseEvent *m )
{
    setVal( y2val(m->y()) );
}
void ColorLuminancePicker::mousePressEvent( QMouseEvent *m )
{
	emit mouseDown();
    setVal( y2val(m->y()) );
}

void ColorLuminancePicker::setVal( int v )
{
    if ( val == v )
	return;
    val = QMAX( 0, QMIN(v,255));
    delete pix; pix=0;
    repaint( FALSE ); //###
    emit newHsv( hue, sat, val );
}

//receives from a hue,sat chooser and relays.
void ColorLuminancePicker::setCol( int h, int s )
{
    setCol( h, s, val );
    emit newHsv( h, s, val );
}

void ColorLuminancePicker::paintEvent( QPaintEvent * )
{
	return;//QT4, paint here causes crash on windows.
    int w = width() - 5;

    QRect r( 0, foff, w, height() - 2*foff );
    int wi = r.width() - 2;
    int hi = r.height() - 2;
    if ( !pix || pix->height() != hi || pix->width() != wi ) {
	delete pix;
	QImage img( wi, hi, 32 );
	int y;
	for ( y = 0; y < hi; y++ ) {
	    QColor c( hue, sat, y2val(y+coff), QColor::Hsv );
	    QRgb r = c.rgb();
	    int x;
	    for ( x = 0; x < wi; x++ )
		img.setPixel( x, y, r );
	}
	pix = new QPixmap;
	pix->convertFromImage(img, Qt::AvoidDither|Qt::ThresholdDither);
    }
    QPainter p(this);
    p.drawPixmap( 1, coff, *pix );
    const QColorGroup &g = colorGroup();
    qDrawShadePanel( &p, r, g, TRUE );
    p.setPen( g.foreground() );
    p.setBrush( g.foreground() );
    Q3PointArray a;
    int y = val2y(val);
    a.setPoints( 3, w, y, w+5, y+5, w+5, y-5 );
    erase( w, 0, 5, height() );
    p.drawPolygon( a );
}

void ColorLuminancePicker::setCol( int h, int s , int v )
{
    val = v;
    hue = h;
    sat = s;
    delete pix; pix=0;
    repaint( FALSE );//####
}
QValidator::State ColIntValidator::validate( QString &s, int &pos ) const
{
    State state = QIntValidator::validate(s,pos);
    if ( state == QValidator::Intermediate ) {
	long int val = s.toLong();
	// This is not a general solution, assumes that top() > 0 and
	// bottom >= 0
	if ( val < 0 ) {
	    s = "0";
	    pos = 1;
	} else if ( val > top() ) {
	    s.setNum( top() );
	    pos = s.length();
	}
    }
    return state;
}
ColorPickerFrame::ColorPickerFrame(QWidget* parent, const char* name  ) :
    Q3Frame(parent, name)
{
   
    const int lumSpace = 3;
    int border = 12;
   
    Q3HBoxLayout *topLay = new Q3HBoxLayout( this, border, 6 );
    Q3VBoxLayout *leftLay = new Q3VBoxLayout( topLay );

    Q3VBoxLayout *rightLay = new Q3VBoxLayout( topLay );

    Q3HBoxLayout *pickLay = new Q3HBoxLayout( rightLay );


    Q3VBoxLayout *cLay = new Q3VBoxLayout( pickLay );
    cp = new ColorPicker( this, "qt_colorpicker" );
    cp->setFrameStyle( Q3Frame::Panel + Q3Frame::Sunken );
    cLay->addSpacing( lumSpace );
    cLay->addWidget( cp );
    cLay->addSpacing( lumSpace );

    lp = new ColorLuminancePicker( this, "qt_luminance_picker" );
    lp->setFixedWidth( 20 ); //###
    pickLay->addWidget( lp );

    connect( cp, SIGNAL(newCol(int,int)), lp, SLOT(setCol(int,int)) );
    connect( lp, SIGNAL(newHsv(int,int,int)), this, SLOT(newHsv(int,int,int)) );
	//Relay mouse up/down events for redo/undo
	connect (cp, SIGNAL(mouseDown()), this, SLOT(mouseDownRelay()));
	connect (lp, SIGNAL(mouseDown()), this, SLOT(mouseDownRelay()));
	connect (cp, SIGNAL(mouseUp()), this, SLOT(mouseUpRelay()));
	connect (lp, SIGNAL(mouseUp()), this, SLOT(mouseUpRelay()));

    rightLay->addStretch();

    cs = new ColorShower( this, "qt_colorshower" );
    connect( cs, SIGNAL(newCol(QRgb)), this, SLOT(newColorTypedIn(QRgb)));
    leftLay->addWidget( cs );

    //QHBoxLayout *buttons;
    
	/*buttons = new QHBoxLayout( leftLay );

    QPushButton *ok, *cancel;
    ok = new QPushButton( "OK", this, "qt_ok_btn" );
    connect( ok, SIGNAL(clicked()), parent, SLOT(accept()) );
    ok->setDefault(TRUE);
    cancel = new QPushButton( "Cancel", this, "qt_cancel_btn" );
    connect( cancel, SIGNAL(clicked()), parent, SLOT(reject()) );
    buttons->addWidget( ok );
    buttons->addWidget( cancel );
    buttons->addStretch();*/

    
}
//sets all widgets except cs to display rgb
void ColorPickerFrame::newColorTypedIn( QRgb rgb )
{
    int h, s, v;
    rgb2hsv(rgb, h, s, v );
    //cp->setCol( h, s );
    //lp->setCol( h, s, v);
	emit(startColorChange());
	newHsv(h,s,v);
	emit(endColorChange());
}

//sets all widgets to display h,s,v
void ColorPickerFrame::newHsv( int h, int s, int v )
{
    cs->setHsv( h, s, v );
    cp->setCol( h, s );
    lp->setCol( h, s, v );
	emit(hsvOut(h,s,v));
}
//send mouse up/down events to params panel
void ColorPickerFrame::mouseDownRelay()
{
    emit(startColorChange());
}
//send mouse up/down events to params panel
void ColorPickerFrame::mouseUpRelay()
{
    emit(endColorChange());
}
