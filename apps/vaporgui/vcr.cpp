//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vcr.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:	Implements the Vcr class
//		Eventually this will be used for animation control
//		Copied from QT sample code
//
#include "vcr.h"
#include <qpushbutton.h>
#include <qlayout.h>
#include <qslider.h>
using namespace VAPoR;

static const char * rewind_xpm[] = {
"16 16 3 1",
" 	c None",
".	c #FFFFFF",
"+	c #000000",
"................",
".++..........++.",
".++........++++.",
".++......++++++.",
".++....++++++++.",
".++..++++++++++.",
".++++++++++++++.",
".++++++++++++++.",
".++++++++++++++.",
".++++++++++++++.",
".++..++++++++++.",
".++....++++++++.",
".++......++++++.",
".++........++++.",
".++.........+++.",
"................"};

static const char * play_xpm[] = {
"16 16 3 1",
" 	c None",
".	c #FFFFFF",
"+	c #000000",
"................",
".++.............",
".++++...........",
".++++++.........",
".++++++++.......",
".++++++++++.....",
".++++++++++++...",
".+++++++++++++..",
".+++++++++++++..",
".++++++++++++...",
".++++++++++.....",
".++++++++.......",
".++++++.........",
".++++...........",
".+++............",
"................"};

static const char * next_xpm[] = {
"16 16 3 1",
" 	c None",
".	c #FFFFFF",
"+	c #000000",
"................",
".++.....+.......",
".+++....++......",
".++++...+++.....",
".+++++..++++....",
".++++++.+++++...",
".+++++++++++++..",
".++++++++++++++.",
".++++++++++++++.",
".+++++++++++++..",
".++++++.+++++...",
".+++++..++++....",
".++++...+++.....",
".+++....++......",
".++.....+.......",
"................"};

static const char * stop_xpm[] = {
"16 16 3 1",
" 	c None",
".	c #FFFFFF",
"+	c #000000",
"................",
".++++++++++++++.",
".++++++++++++++.",
".++++++++++++++.",
".+++........+++.",
".+++........+++.",
".+++........+++.",
".+++........+++.",
".+++........+++.",
".+++........+++.",
".+++........+++.",
".+++........+++.",
".++++++++++++++.",
".++++++++++++++.",
".++++++++++++++.",
"................"};


Vcr::Vcr( QWidget *parent, const char *name )
    : QWidget( parent, name )
{
	vlayout = new QVBoxLayout(this);
	vlayout->setSpacing(0);
	vlayout->setMargin(0);
	QSlider* timeSlider = new QSlider(Qt::Horizontal, this, "Timeline");
	vlayout->addWidget(timeSlider);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin( 0 );

    QPushButton *rewind = new QPushButton( QPixmap( rewind_xpm ), 0, this, "vcr_rewind" );
    layout->addWidget( rewind );
    connect( rewind, SIGNAL(clicked()), SIGNAL(rewind()) ); 

    QPushButton *play = new QPushButton( QPixmap( play_xpm ), 0, this, "vcr_play" );
    layout->addWidget( play );
    connect( play, SIGNAL(clicked()), SIGNAL(play()) ); 

    QPushButton *next = new QPushButton( QPixmap( next_xpm ), 0, this, "vcr_next" );
    layout->addWidget( next );
    connect( next, SIGNAL(clicked()), SIGNAL(next()) ); 

    QPushButton *stop = new QPushButton( QPixmap( stop_xpm ), 0, this, "vcr_stop" );
    layout->addWidget( stop );
    connect( stop, SIGNAL(clicked()), SIGNAL(stop()) ); 
	vlayout->addLayout(layout);
	

}


