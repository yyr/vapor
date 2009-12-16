/****************************************************************************
** QT example code, modified for use in VAPOR by A. Norton 1/10/07
** $Id$
**
** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include <q3mainwindow.h>
#include <q3textbrowser.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qdir.h>
//Added by qt3to4:
#include <Q3PopupMenu>

class QComboBox;
class Q3PopupMenu;

class HelpWindow : public Q3MainWindow
{
    Q_OBJECT
public:
    HelpWindow( const QString& home_,  const QString& path, QWidget* parent = 0, const char *name=0 );
    ~HelpWindow();
	static void showHelp(const QString& filename);

private slots:
    void setBackwardAvailable( bool );
    void setForwardAvailable( bool );

    void sourceChanged( const QString& );
    void about();
   
    void openFile();
    void newWindow();
    void print();

    void pathSelected( const QString & );
    void histChosen( int );
    void bookmChosen( int );
    void addBookmark();

private:
	static HelpWindow* theHelpWindow;
    void readHistory();
    void readBookmarks();

    Q3TextBrowser* browser;
    QComboBox *pathCombo;
    int backwardId, forwardId;
    QStringList history, bookmarks;
    QMap<int, QString> mHistory, mBookmarks;
    Q3PopupMenu *hist, *bookm;

};





#endif

