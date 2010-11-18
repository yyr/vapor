/****************************************************************************
** QT example code, modified for use in VAPOR by A. Norton 1/10/07
** Modified again in 2010 based on TextEdit example in QT 4.6
** $Id$
**
** Copyright (C) 1992-2010 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include <QtGui/QMainWindow>
#include <QtCore/QUrl>
#include <QtGui/QTextEdit>
class TextEdit;

class HelpWindow : public QMainWindow
{
    Q_OBJECT

public:
    HelpWindow();
	static void showHelp(const char*);
	TextEdit *textViewer;

private slots:
    void about();
    void open();


private:
	static QString lastDir;
    void createActions();
    void createMenus();

    QMenu *fileMenu;
    QMenu *helpMenu;

    QAction *clearAct;
    QAction *openAct;
    QAction *exitAct;
    QAction *aboutAct;
};

class TextEdit : public QTextEdit
{
    Q_OBJECT

public:
    TextEdit(QWidget *parent = 0);
    void setContents(const QString &fileName);

private:
    QVariant loadResource(int type, const QUrl &name);
    QUrl srcUrl;
};
#endif
