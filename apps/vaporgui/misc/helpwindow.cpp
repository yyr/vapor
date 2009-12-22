/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/
#include "../images/back.xpm"
#include "../images/forward.xpm"
#include "../images/home2.xpm"
#include "helpwindow.h"
#include "GetAppPath.h"
#include <qstatusbar.h>
#include <QFileDialog>
#include <qpixmap.h>
#include <qimage.h>
#include <QToolBar>
#include <qmenubar.h>
#include <qtoolbutton.h>
#include <qicon.h>
#include <qfile.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <qcombobox.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qobject.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qdatastream.h>
#include <qprinter.h>
#include <qpainter.h>

#include <ctype.h>
#include <stdlib.h>


using namespace VetsUtil;

HelpWindow* HelpWindow::theHelpWindow = 0;

HelpWindow::HelpWindow( const QString& home_, const QString& _path,
			QWidget* parent, const char *name )
    : QMainWindow( parent, name, Qt::WDestructiveClose ),
      pathCombo( 0 )
{
    readHistory();
    readBookmarks();

    browser = new QTextBrowser( this );

    browser->setSource(_path);
    connect( browser, SIGNAL( sourceChanged(const QUrl& ) ),
	     this, SLOT( sourceChanged( const QUrl&) ) );

    setCentralWidget( browser );

    if ( !home_.isEmpty() )
	browser->setSource( home_ );

    connect( browser, SIGNAL( highlighted( const QUrl&) ),
	     statusBar(), SLOT( showMessage( const QUrl&)) );

    resize( 900,700 );
	QMenu* file = menuBar()->addMenu(tr("File"));
    file->insertItem( tr("&New Window"), this, SLOT( newWindow() ), Qt::CTRL+Qt::Key_N );
    file->insertItem( tr("&Open File"), this, SLOT( openFile() ), Qt::CTRL+Qt::Key_O );
    file->insertItem( tr("&Print"), this, SLOT( print() ), Qt::CTRL+Qt::Key_P );
    file->insertSeparator();
    file->insertItem( tr("&Close"), this, SLOT( close() ), Qt::CTRL+Qt::Key_Q );
    file->insertItem( tr("E&xit"), qApp, SLOT( closeAllWindows() ), Qt::CTRL+Qt::Key_X );
    // The same three icons are used twice each.
	QPixmap* backPix = new QPixmap(back);
	QPixmap* forwardPix = new QPixmap(forward);
	QPixmap* homePix = new QPixmap(home2);
   
	QIcon icon_back(*backPix);
    QIcon icon_forward( *forwardPix );
    QIcon icon_home( *homePix);

    QMenu* go = menuBar()->addMenu(tr("Go")); 
    backwardId = go->insertItem( icon_back,
				 tr("&Backward"), browser, SLOT( backward() ),
				 Qt::CTRL+Qt::Key_Left );
    forwardId = go->insertItem( icon_forward,
				tr("&Forward"), browser, SLOT( forward() ),
				Qt::CTRL+Qt::Key_Right );
    go->insertItem( icon_home, tr("&Home"), browser, SLOT( home() ) );

    QMenu* help = menuBar()->addMenu(tr("Help"));
    help->insertItem( tr("&About"), this, SLOT( about() ) );

    hist = menuBar()->addMenu(tr("History"));
    QStringList::Iterator it = history.begin();
    for ( ; it != history.end(); ++it )
	mHistory[ hist->insertItem( *it ) ] = *it;
    connect( hist, SIGNAL( activated( int ) ),
	     this, SLOT( histChosen( int ) ) );

    QMenu* bookm = menuBar()->addMenu(tr("Bookmarks")); 
    bookm->insertItem( tr( "Add Bookmark" ), this, SLOT( addBookmark() ) );
    bookm->insertSeparator();

    QStringList::Iterator it2 = bookmarks.begin();
    for ( ; it2 != bookmarks.end(); ++it2 )
	mBookmarks[ bookm->insertItem( *it2 ) ] = *it2;
    connect( bookm, SIGNAL( activated( int ) ),
	     this, SLOT( bookmChosen( int ) ) );

    menuBar()->setItemEnabled( forwardId, FALSE);
    menuBar()->setItemEnabled( backwardId, FALSE);
    connect( browser, SIGNAL( backwardAvailable( bool ) ),
	     this, SLOT( setBackwardAvailable( bool ) ) );
    connect( browser, SIGNAL( forwardAvailable( bool ) ),
	     this, SLOT( setForwardAvailable( bool ) ) );


    QToolBar* toolbar = new QToolBar( this );
    addToolBar( toolbar);
    QToolButton* button;

    button = new QToolButton( icon_back, tr("Backward"), "", browser, SLOT(backward()), toolbar );
    connect( browser, SIGNAL( backwardAvailable(bool) ), button, SLOT( setEnabled(bool) ) );
    button->setEnabled( FALSE );
    button = new QToolButton( icon_forward, tr("Forward"), "", browser, SLOT(forward()), toolbar );
    connect( browser, SIGNAL( forwardAvailable(bool) ), button, SLOT( setEnabled(bool) ) );
    button->setEnabled( FALSE );
    button = new QToolButton( icon_home, tr("Home"), "", browser, SLOT(home()), toolbar );

    toolbar->addSeparator();

    pathCombo = new QComboBox( TRUE, toolbar );
    connect( pathCombo, SIGNAL( activated( const QString & ) ),
	     this, SLOT( pathSelected( const QString & ) ) );
    //toolbar->setStretchableWidget( pathCombo );
    //setRightJustification( TRUE );
    //setDockEnabled( Qt::DockLeft, FALSE );
    //setDockEnabled( Qt::DockRight, FALSE );

    pathCombo->insertItem( home_ );
    browser->setFocus();

}


void HelpWindow::showHelp(const QString& filename){
	

	QString filePath =  GetAppPath("vapor", "share").c_str();

	filePath += "/doc/" + filename;


#ifdef WIN32
	filePath.replace('/','\\');
#endif
	if (!theHelpWindow){
		theHelpWindow = new HelpWindow(filePath, filePath);
	} 
	theHelpWindow->browser->setSource(filePath);
	theHelpWindow->show();
}

void HelpWindow::setBackwardAvailable( bool b)
{
    menuBar()->setItemEnabled( backwardId, b);
}

void HelpWindow::setForwardAvailable( bool b)
{
    menuBar()->setItemEnabled( forwardId, b);
}


void HelpWindow::sourceChanged( const QUrl& url )
{
    if ( browser->documentTitle().isNull() )
	setCaption( "VAPOR Help Viewer - " + url );
    else
	setCaption( "VAPOR Help Viewer - " + browser->documentTitle() ) ;

    if ( !url.isEmpty() && pathCombo ) {
	bool exists = FALSE;
	int i;
	for ( i = 0; i < pathCombo->count(); ++i ) {
	    if ( pathCombo->text( i ) == url ) {
		exists = TRUE;
		break;
	    }
	}
	if ( !exists ) {
	    pathCombo->insertItem( url, 0 );
	    pathCombo->setCurrentItem( 0 );
	    mHistory[ hist->insertItem( url ) ] = url;
	} else
	    pathCombo->setCurrentItem( i );
    }
}

HelpWindow::~HelpWindow()
{
    history =  mHistory.values();

    QFile f( QDir::currentDirPath() + "/.history" );
    f.open( QIODevice::WriteOnly );
    QDataStream s( &f );
    s << history;
    f.close();

    bookmarks = mBookmarks.values();

    QFile f2( QDir::currentDirPath() + "/.bookmarks" );
    f2.open( QIODevice::WriteOnly );
    QDataStream s2( &f2 );
    s2 << bookmarks;
    f2.close();
	theHelpWindow = 0;
}

void HelpWindow::about()
{
    QMessageBox::about( this, "Help Viewer for VAPOR",
			"<p>This displays context-sensitive help</p>");
		
}

void HelpWindow::openFile()
{
#ifndef QT_NO_FILEDIALOG
    QString fn = QFileDialog::getOpenFileName(this, QString::null, QString::null );

    if ( !fn.isEmpty() )
	browser->setSource( fn );
#endif
}

void HelpWindow::newWindow()
{
    ( new HelpWindow(browser->source(), "qbrowser") )->show();
}

void HelpWindow::print()
{
#ifndef QT_NO_PRINTER
    QPrinter printer( QPrinter::HighResolution );
    printer.setFullPage(TRUE);
    if ( printer.setup( this ) ) {
	browser->print(&printer);
    }
#endif
}

void HelpWindow::pathSelected( const QString &_path )
{
    browser->setSource( _path );
    if ( mHistory.values().contains(_path) )
	mHistory[ hist->insertItem( _path ) ] = _path;
}

void HelpWindow::readHistory()
{
    if ( QFile::exists( QDir::currentDirPath() + "/.history" ) ) {
	QFile f( QDir::currentDirPath() + "/.history" );
	f.open( QIODevice::ReadOnly );
	QDataStream s( &f );
	s >> history;
	f.close();
	while ( history.count() > 20 )
	    history.remove( history.begin() );
    }
}

void HelpWindow::readBookmarks()
{
    if ( QFile::exists( QDir::currentDirPath() + "/.bookmarks" ) ) {
	QFile f( QDir::currentDirPath() + "/.bookmarks" );
	f.open( QIODevice::ReadOnly );
	QDataStream s( &f );
	s >> bookmarks;
	f.close();
    }
}

void HelpWindow::histChosen( int i )
{
    if ( mHistory.contains( i ) )
	browser->setSource( mHistory[ i ] );
}

void HelpWindow::bookmChosen( int i )
{
    if ( mBookmarks.contains( i ) )
	browser->setSource( mBookmarks[ i ] );
}

void HelpWindow::addBookmark()
{
    mBookmarks[ bookm->insertItem( caption() ) ] = browser->source();
}
