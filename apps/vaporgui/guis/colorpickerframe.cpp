//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		colorpickerframe.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the ColorPickerFrame class.  
//


#include <qwidget.h>

#include <QFrame>
#include <qimage.h>
#include <qpainter.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qapplication.h>

#include <qlayout.h>
#include <qpushbutton.h>
#include <qpixmap.h>
#include <qdrawutil.h>
#include <qvalidator.h>

#include <QHBoxLayout>
#include <QPaintEvent>
#include <QGridLayout>
#include <QMouseEvent>
#include <QVBoxLayout>

#include "colorpickerframe.h"
//Color picker dimensions:
static int pWidth = 140;
static int pHeight = 120;
QPoint ColorPicker::colPt()
{ return QPoint( (360-hue)*(pWidth-1)/360, (255-sat)*(pHeight-1)/255 ); }
int ColorPicker::huePt( const QPoint &pt )
{ 
	int h =  360 - pt.x()*360/(pWidth-1);
	return (h < 360 ? h : 0 );
}
int ColorPicker::satPt( const QPoint &pt )
{ return 255 - pt.y()*255/(pHeight-1) ; }
void ColorPicker::setCol( const QPoint &pt )
{ setCol( huePt(pt), satPt(pt) ); }

ColorPicker::ColorPicker(QWidget* parent )
    : QFrame( parent )
{
    hue = 0; sat = 0;
    setCol( 150, 255 );

	QImage img( pWidth, pHeight, QImage::Format_RGB32 );
    int x,y;
    for ( y = 0; y < pHeight; y++ )
	for ( x = 0; x < pWidth; x++ ) {
	    QPoint p( x, y );
		img.setPixel( x, y, QColor::fromHsv(huePt(p), satPt(p),200).rgb());
	}
	QPixmap px = QPixmap::fromImage(img,0);
	pix = &px;
    
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
    int nhue = qMin( qMax(0,h), 360 );
    int nsat = qMin( qMax(0,s), 255);
    if ( nhue == hue && nsat == sat )
	return;
    QRect r( colPt(), QSize(20,20) );
    hue = nhue; sat = nsat;
    r = r.unite( QRect( colPt(), QSize(20,20) ) );
    r.translate( contentsRect().x()-9, contentsRect().y()-9 );
    //    update( r );
    repaint( r);
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
void ColorPicker::paintEvent(QPaintEvent* ){
	QPainter p(this);
    drawFrame(&p);
    QRect r = contentsRect();
	if (pix->isNull()) return;
    p.drawPixmap(r.topLeft(), *pix);
    QPoint pt = colPt() + r.topLeft();
    p.setPen(Qt::black);

    p.fillRect(pt.x()-9, pt.y(), 20, 2, Qt::black);
    p.fillRect(pt.x(), pt.y()-9, 2, 20, Qt::black);
}



void ColorShowLabel::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    drawFrame(&p);
    p.fillRect(contentsRect()&e->rect(), col);
}



void ColorShowLabel::mousePressEvent( QMouseEvent *e )
{
    mousePressed = TRUE;
    pressPos = e->pos();
}

void ColorShowLabel::mouseMoveEvent( QMouseEvent * )
{

}



void ColorShowLabel::mouseReleaseEvent( QMouseEvent * )
{
    if ( !mousePressed )
	return;
    mousePressed = FALSE;
}

ColorShower::ColorShower( QWidget *parent )
    :QWidget( parent)
{
    curCol = qRgb( -1, -1, -1 );
    ColIntValidator *val256 = new ColIntValidator( 0, 255, this );
    ColIntValidator *val360 = new ColIntValidator( 0, 360, this );

    QGridLayout *gl = new QGridLayout( this);
	
	/*
    lab = new ColorShowLabel( this );
    lab->setMinimumWidth( 60 ); //###
    gl->addMultiCellWidget(lab, 0,-1,0,0);
    connect( lab, SIGNAL( colorDropped(QRgb) ),
	     this, SIGNAL( newCol(QRgb) ) );
    connect( lab, SIGNAL( colorDropped(QRgb) ),
	     this, SLOT( setRgb(QRgb) ) );
*/
    hEd = new ColNumLineEdit( this);
    hEd->setValidator( val360 );
    QLabel *l = new QLabel("Hu&e:", this );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 0, 1 );
    gl->addWidget( hEd, 0, 2 );

    sEd = new ColNumLineEdit( this );
    sEd->setValidator( val256 );
    l = new QLabel( "&Sat:", this );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 1, 1 );
    gl->addWidget( sEd, 1, 2 );

    vEd = new ColNumLineEdit( this);
    vEd->setValidator( val256 );
    l = new QLabel( "&Val:", this );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 2, 1 );
    gl->addWidget( vEd, 2, 2 );

    rEd = new ColNumLineEdit( this );
    rEd->setValidator( val256 );
    l = new QLabel( "&Red:", this );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 0, 3 );
    gl->addWidget( rEd, 0, 4 );

    gEd = new ColNumLineEdit( this);
    gEd->setValidator( val256 );
    l = new QLabel( "&Green:", this );
    l->setAlignment( Qt::AlignRight|Qt::AlignVCenter );
    gl->addWidget( l, 1, 3 );
    gl->addWidget( gEd, 1, 4 );

    bEd = new ColNumLineEdit( this );
    bEd->setValidator( val256 );
    l = new QLabel( "Bl&ue:", this );
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
	gl->addWidget(lab,3,0,1,5);
    //gl->addMultiCellWidget(lab, 3,3,0,4);
    connect( lab, SIGNAL( colorDropped(QRgb) ),
	     this, SIGNAL( newCol(QRgb) ) );
    connect( lab, SIGNAL( colorDropped(QRgb) ),
	     this, SLOT( setRgb(QRgb) ) );
    
}

void ColorShower::showCurrentColor()
{
    lab->setColor( currentColor() );
    lab->repaint(); //###
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

	curCol = QColor::fromHsv(hue, sat, val).rgb();

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
	curCol = QColor::fromHsv( hue, sat, val).rgb();

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

ColorLuminancePicker::ColorLuminancePicker(QWidget* parent)
    : QWidget( parent)
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
    val = qMax( 0, qMin(v,255));
    delete pix; pix=0;
    repaint( ); //###
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
	 int w = width() - 5;

    QRect r(0, foff, w, height() - 2*foff);
    int wi = r.width() - 2;
    int hi = r.height() - 2;
    if (!pix || pix->height() != hi || pix->width() != wi) {
        delete pix;
        QImage img(wi, hi, QImage::Format_RGB32);
        int y;
        uint *pixel = (uint *) img.scanLine(0);
        for (y = 0; y < hi; y++) {
            const uint *end = pixel + wi;
            while (pixel < end) {
                QColor c;
                c.setHsv(hue, sat, y2val(y+coff));
                *pixel = c.rgb();
                ++pixel;
            }
        }
        pix = new QPixmap(QPixmap::fromImage(img));
    }
    QPainter p(this);
    p.drawPixmap(1, coff, *pix);
    const QPalette &g = palette();
    qDrawShadePanel(&p, r, g, true);
    p.setPen(g.foreground().color());
    p.setBrush(g.foreground());
    QPolygon a;
    int y = val2y(val);
    a.setPoints(3, w, y, w+5, y+5, w+5, y-5);
    p.eraseRect(w, 0, 5, height());
    p.drawPolygon(a);
}
	
void ColorLuminancePicker::setCol( int h, int s , int v )
{
    val = v;
    hue = h;
    sat = s;
    delete pix; pix=0;
    repaint();//####
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
ColorPickerFrame::ColorPickerFrame(QWidget* parent ) :
    QFrame(parent)
{
   
    const int lumSpace = 3;
    //int border = 12;
   
    QHBoxLayout *topLay = new QHBoxLayout( this);
	topLay->setSpacing(6);
    QVBoxLayout *leftLay = new QVBoxLayout();
	topLay->insertLayout(0,leftLay);


    QVBoxLayout *rightLay = new QVBoxLayout();
	topLay->insertLayout(1,rightLay);

    QHBoxLayout *pickLay = new QHBoxLayout();
	rightLay->insertLayout(0,pickLay);


    QVBoxLayout *cLay = new QVBoxLayout();
	pickLay->insertLayout(0,cLay);
    cp = new ColorPicker( this );
    cp->setFrameStyle( QFrame::Panel + QFrame::Sunken );
    cLay->addSpacing( lumSpace );
    cLay->addWidget( cp );
    cLay->addSpacing( lumSpace );

    lp = new ColorLuminancePicker( this);
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

    cs = new ColorShower( this);
    connect( cs, SIGNAL(newCol(QRgb)), this, SLOT(newColorTypedIn(QRgb)));
    leftLay->addWidget( cs );

    

    
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
