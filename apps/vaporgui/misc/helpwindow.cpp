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
#include <QPrintDialog>
#include <qpixmap.h>
#include <qimage.h>
#include <QToolBar>
#include <QAction>
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
			QWidget* parent, const char * )
    : QMainWindow( parent ),
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
	//Create actions for each menu item, add to file menu:
	newAct = new QAction(tr("&New Window"), this);
	newAct->setShortcut(Qt::CTRL+Qt::Key_N);
	connect(newAct,SIGNAL(triggered()),this, SLOT(newWindow()));
	file->addAction(newAct);

   
	openAct = new QAction(tr("&Open File"), this);
	openAct->setShortcut(Qt::CTRL+Qt::Key_O);
	connect(openAct,SIGNAL(triggered()),this, SLOT(openFile()));
	file->addAction(openAct);
   
	printAct = new QAction(tr("&Print"), this);
	printAct->setShortcut(Qt::CTRL+Qt::Key_P);
	connect(printAct,SIGNAL(triggered()),this, SLOT(print()));
	file->addAction(printAct);

    file->addSeparator();
   
	closeAct = new QAction(tr("&Close"), this);
	closeAct->setShortcut(Qt::CTRL+Qt::Key_Q);
	connect(closeAct,SIGNAL(triggered()),this, SLOT(close()));
	file->addAction(closeAct);

    // The same three icons are used twice each.
	QPixmap* backPix = new QPixmap(back);
	QPixmap* forwardPix = new QPixmap(forward);
	QPixmap* homePix = new QPixmap(home2);
   
	QIcon icon_back(*backPix);
    QIcon icon_forward( *forwardPix );
    QIcon icon_home( *homePix);

    QMenu* go = menuBar()->addMenu(tr("Go")); 

	backAct = new QAction(icon_back,tr("&Backward"), this);
	backAct->setShortcut(Qt::CTRL+Qt::Key_Left);
	connect(backAct,SIGNAL(triggered()),this, SLOT(backward()));
	go->addAction(openAct);

	fwdAct = new QAction(icon_forward,tr("&Forward"), this);
	fwdAct->setShortcut(Qt::CTRL+Qt::Key_Right);
	connect(fwdAct,SIGNAL(triggered()),this, SLOT(forward()));
	go->addAction(fwdAct);

    /*backwardId = go->insertItem( icon_back,
				 tr("&Backward"), browser, SLOT( backward() ),
				 Qt::CTRL+Qt::Key_Left );
    forwardId = go->insertItem( icon_forward,
				tr("&Forward"), browser, SLOT( forward() ),
				Qt::CTRL+Qt::Key_Right );
    go->insertItem( icon_home, tr("&Home"), browser, SLOT( home() ) );
	*/

    QMenu* help = menuBar()->addMenu(tr("Help"));
	aboutAct = new QAction(tr("&About"),this);
	connect(aboutAct,SIGNAL(triggered()),this, SLOT(about()));
	help->addAction(fwdAct);
    //help->insertItem( tr("&About"), this, SLOT( about() ) );

    hist = menuBar()->addMenu(tr("History"));
    QStringList::Iterator it = history.begin();
	int histIndex = 0;
	for ( ; it != history.end(); ++it ){
		hist->addAction( *it );
		mHistory[histIndex++ ] = *it;
		connect( hist, SIGNAL( activated( int ) ),
			this, SLOT( histChosen( int ) ) );
	}

    QMenu* bookm = menuBar()->addMenu(tr("Bookmarks")); 
	bookAct = new QAction(tr("Bookmarks"),this);
	connect(bookAct,SIGNAL(activated()), this, SLOT( addBookmark()));
    
    bookm->addSeparator();

    QStringList::Iterator it2 = bookmarks.begin();
	int bookIndex = 0;
	for ( ; it2 != bookmarks.end(); ++it2 ){
		bookm->addAction(*it2);
		mBookmarks[ bookIndex++ ] = *it2;
		connect( bookm, SIGNAL( activated( int ) ),
			this, SLOT( bookmChosen( int ) ) );
	}

    fwdAct->setEnabled(FALSE);
	backAct->setEnabled(FALSE);
	
    connect( browser, SIGNAL( backwardAvailable( bool ) ),
	     this, SLOT( setBackwardAvailable( bool ) ) );
    connect( browser, SIGNAL( forwardAvailable( bool ) ),
	     this, SLOT( setForwardAvailable( bool ) ) );


    QToolBar* toolbar = new QToolBar( this );
    addToolBar( toolbar);
    QToolButton* button;

	button = new QToolButton(toolbar);
	button->setIcon(icon_back);
	button->setText(tr("Backward"));
	connect(button, SIGNAL(triggered()), browser, SLOT(backward()));
    //button = new QToolButton( icon_back, tr("Backward"), "", browser, SLOT(backward()), toolbar );
    connect( browser, SIGNAL( backwardAvailable(bool) ), button, SLOT( setEnabled(bool) ) );
    button->setEnabled( FALSE );

	button = new QToolButton(toolbar);
	button->setIcon(icon_forward);
	button->setText(tr("Forward"));
	connect(button, SIGNAL(triggered()), browser, SLOT(forward()));
   
    connect( browser, SIGNAL( forwardAvailable(bool) ), button, SLOT( setEnabled(bool) ) );
    button->setEnabled( FALSE );

   // button = new QToolButton( icon_forward, tr("Forward"), "", browser, SLOT(forward()), toolbar );
    //connect( browser, SIGNAL( forwardAvailable(bool) ), button, SLOT( setEnabled(bool) ) );
    //button->setEnabled( FALSE );
    //button = new QToolButton( icon_home, tr("Home"), "", browser, SLOT(home()), toolbar );

    toolbar->addSeparator();

    pathCombo = new QComboBox(toolbar );
    connect( pathCombo, SIGNAL( activated( const QString & ) ),
	     this, SLOT( pathSelected( const QString & ) ) );
    //toolbar->setStretchableWidget( pathCombo );
    //setRightJustification( TRUE );
    //setDockEnabled( Qt::DockLeft, FALSE );
    //setDockEnabled( Qt::DockRight, FALSE );

    pathCombo->insertItem(0, home_ );
    browser->setFocus();

}


void HelpWindow::showHelp(const QString& filename){
	

	vector <string> paths;
	paths.push_back("doc");
	paths.push_back(filename.toStdString());
	QString filePath =  GetAppPath("VAPOR", "share", paths).c_str();

	if (!theHelpWindow){
		theHelpWindow = new HelpWindow(filePath, filePath);
	} 
	theHelpWindow->browser->setSource(filePath);
	theHelpWindow->show();
}

void HelpWindow::setBackwardAvailable( bool b)
{
    backAct->setEnabled(b);
}

void HelpWindow::setForwardAvailable( bool b)
{
    fwdAct->setEnabled(b);
}


void HelpWindow::sourceChanged( const QUrl& url )
{
    if ( browser->documentTitle().isNull() )
	setWindowTitle( "VAPOR Help Viewer - " + url.toString() );
    else
	setWindowTitle( "VAPOR Help Viewer - " + browser->documentTitle() ) ;

    if ( !url.isEmpty() && pathCombo ) {
	bool exists = FALSE;
	int i;
	for ( i = 0; i < pathCombo->count(); ++i ) {
	    if ( pathCombo->itemText( i ) == url.toString() ) {
		exists = TRUE;
		break;
	    }
	}
	if ( !exists ) {
	    pathCombo->insertItem(0, url.toString() );
	    pathCombo->setCurrentIndex( 0 );
		hist->addAction(url.toString());
	    mHistory[ hist->actions().count()-1 ] = url.toString();
	} else
	    pathCombo->setCurrentIndex( i );
    }
}

HelpWindow::~HelpWindow()
{
    history =  mHistory.values();

    QFile f( QDir::currentPath() + "/.history" );
    f.open( QIODevice::WriteOnly );
    QDataStream s( &f );
    s << history;
    f.close();

    bookmarks = mBookmarks.values();

    QFile f2( QDir::currentPath() + "/.bookmarks" );
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
    ( new HelpWindow(browser->source().toString(), "qbrowser") )->show();
}

void HelpWindow::print()
{
#ifndef QT_NO_PRINTER
    QPrinter printer( QPrinter::HighResolution );
    printer.setFullPage(TRUE);
	QPrintDialog dialog(&printer, this);
    if ( dialog.exec() ) {
		browser->print(&printer);
    }
#endif
}

void HelpWindow::pathSelected( const QString &_path )
{
    browser->setSource( _path );
	if ( mHistory.values().contains(_path) ){
		hist->addAction(_path);
		mHistory[ hist->actions().count()-1 ] = _path;
	}
}

void HelpWindow::readHistory()
{
    if ( QFile::exists( QDir::currentPath() + "/.history" ) ) {
	QFile f( QDir::currentPath() + "/.history" );
	f.open( QIODevice::ReadOnly );
	QDataStream s( &f );
	s >> history;
	f.close();
	while ( history.count() > 20 )
	    history.erase( 0 );
    }
}

void HelpWindow::readBookmarks()
{
    if ( QFile::exists( QDir::currentPath() + "/.bookmarks" ) ) {
	QFile f( QDir::currentPath() + "/.bookmarks" );
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
	bookm->addAction(windowTitle());
    mBookmarks[ bookm->actions().count()-1 ] = browser->source().toString();
}
