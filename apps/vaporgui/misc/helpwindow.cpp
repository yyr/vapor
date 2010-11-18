/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/
#include <QtCore/QLibraryInfo>
#include <QtGui/QApplication>
#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QMessageBox>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QFileDialog>

#include "helpwindow.h"
#include "GetAppPath.h"
#include <vector>
#include <string>

using namespace VetsUtil;

QString HelpWindow::lastDir;

HelpWindow::HelpWindow()
{
   
    textViewer = new TextEdit;
    setCentralWidget(textViewer);
    createActions();
    createMenus();
    setWindowTitle(tr("VAPOR Help Viewer"));
    resize(750, 400);

}


void HelpWindow::showHelp(const char* filename){
	std::vector<std::string>paths;
	paths.push_back("doc");
	lastDir = GetAppPath("VAPOR","share",paths).c_str();
	paths.push_back(string(filename));
	QString filePath = GetAppPath("VAPOR","share",paths).c_str();
	HelpWindow* theHelpWindow = new HelpWindow();
	theHelpWindow->textViewer->setContents(filePath);
	theHelpWindow->show();

}

void HelpWindow::about()
{
    QMessageBox::about(this, tr("About VAPOR Help Viewer"),
                       tr("This viewer displays html text\n"
                          "such as is used to provide Flow setup help."));
}



void HelpWindow::open()
{
	QString s = QFileDialog::getOpenFileName(this,"Select HTML File to Open",lastDir, "*.html");
	if (s.isNull()) return;
	textViewer->setContents(s.toLatin1());
	lastDir = s;
}


void HelpWindow::createActions()
{

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    clearAct = new QAction(tr("&Clear"), this);
    clearAct->setShortcut(tr("Ctrl+C"));
    connect(clearAct, SIGNAL(triggered()), textViewer, SLOT(clear()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    aboutAct = new QAction(tr("&About"), this);
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}


void HelpWindow::createMenus()
{
    fileMenu = new QMenu(tr("&File"), this);
    fileMenu->addAction(openAct);
    fileMenu->addAction(clearAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    helpMenu = new QMenu(tr("&Help"), this);
    helpMenu->addSeparator();
    helpMenu->addAction(aboutAct);
    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(helpMenu);
}



TextEdit::TextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setReadOnly(true);
}

void TextEdit::setContents(const QString &fileName)
{
    QFileInfo fi(fileName);
    srcUrl = QUrl::fromLocalFile(fi.absoluteFilePath());
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QString data(file.readAll());
        if (fileName.endsWith(".html"))
            setHtml(data);
        else
            setPlainText(data);
    }
}

QVariant TextEdit::loadResource(int type, const QUrl &name)
{
    if (type == QTextDocument::ImageResource) {
        QFile file(srcUrl.resolved(name).toLocalFile());
        if (file.open(QIODevice::ReadOnly))
            return file.readAll();
    }
    return QTextEdit::loadResource(type, name);
}
