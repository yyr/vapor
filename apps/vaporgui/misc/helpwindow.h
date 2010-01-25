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

#include <QTextBrowser>
#include <QMainWindow>
#include <qstringlist.h>
#include <qmap.h>
#include <qdir.h>
#include <QMenu>
//Added by qt3to4:

class QComboBox;
class QMenu;

class HelpWindow : public QMainWindow
{
    Q_OBJECT
public:
    HelpWindow( const QString& home_,  const QString& path, QWidget* parent = 0, const char *name=0 );
    ~HelpWindow();
	static void showHelp(const QString& filename);

private slots:
    void setBackwardAvailable( bool );
    void setForwardAvailable( bool );

    void sourceChanged( const QUrl& );
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
	QAction* newAct, *openAct, *printAct, *closeAct;
	QAction* backAct, *fwdAct, *aboutAct, *bookAct;
    QTextBrowser* browser;
    QComboBox *pathCombo;
    int backwardId, forwardId;
    QStringList history, bookmarks;
    QMap<int, QString> mHistory, mBookmarks;
    QMenu *hist, *bookm;

};





#endif

