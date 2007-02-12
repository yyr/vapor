//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		sessionparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements the SessionParams class.
//
#include "sessionparams.h"
#include "session.h"
#include "sessionparameters.h"
#include "mainform.h"
#include "messagereporter.h"
#include <qfiledialog.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qmessagebox.h>


using namespace VAPoR;
//Save current session parameters in this class
SessionParams::SessionParams(){
	Session* currentSession = Session::getInstance();
	MessageReporter* mReporter = MessageReporter::getInstance();
	jpegQuality = GLWindow::getJpegQuality();
	cacheSize = currentSession->getCacheMB();
	for (int i = 0; i<3; i++){
		MessageReporter::messagePriority mP = (MessageReporter::messagePriority) i;
		maxPopup[i] = mReporter->getMaxPopup(mP);
		maxLog[i] = mReporter->getMaxLog(mP);
	}
}
void SessionParams::launch(){
	
	sessionParamsDlg = new SessionParameters((QWidget*)MainForm::getInstance());
	QString str;
	sessionParamsDlg->cacheSizeEdit->setText(str.setNum(cacheSize));
	sessionParamsDlg->jpegQuality->setText(str.setNum(jpegQuality));
	sessionParamsDlg->cacheSizeEdit->setText(str.setNum(cacheSize));
	string logFileName = Session::getInstance()->getLogfileName();
	sessionParamsDlg->logFileName->setText(logFileName.c_str());
	
	connect(sessionParamsDlg->logFileButton,SIGNAL(pressed()),this, SLOT(logFileChoose()));
	sessionParamsDlg->maxErrorLog->setText(str.setNum(maxLog[2]));
	sessionParamsDlg->maxWarnLog->setText(str.setNum(maxLog[1]));
	sessionParamsDlg->maxInfoLog->setText(str.setNum(maxLog[0]));
	sessionParamsDlg->maxErrorPopup->setText(str.setNum(maxPopup[2]));
	sessionParamsDlg->maxWarnPopup->setText(str.setNum(maxPopup[1]));
	sessionParamsDlg->maxInfoPopup->setText(str.setNum(maxPopup[0]));
	int rc = sessionParamsDlg->exec();
	if (rc){
		//see if the memory size changed:
		Session* currentSession = Session::getInstance();
		MessageReporter* mReporter = MessageReporter::getInstance();
		int newVal = sessionParamsDlg->cacheSizeEdit->text().toInt();
		if (newVal > 10 && newVal != currentSession->getCacheMB()){
			currentSession->setCacheMB(newVal);
			MessageReporter::warningMsg("%s","Cache size will change at next metadata loading"); 
		}
		//Set the image quality:
		int newQual = sessionParamsDlg->jpegQuality->text().toInt();
		if (newQual > 0 && newQual <= 100) GLWindow::setJpegQuality(newQual);

		//set the log/popup numbers:
		maxPopup[0] = sessionParamsDlg->maxInfoPopup->text().toInt();
		maxPopup[1] = sessionParamsDlg->maxWarnPopup->text().toInt();
		maxPopup[2] = sessionParamsDlg->maxErrorPopup->text().toInt();
		maxLog[0] = sessionParamsDlg->maxInfoLog->text().toInt();
		maxLog[1] = sessionParamsDlg->maxWarnLog->text().toInt();
		maxLog[2] = sessionParamsDlg->maxErrorLog->text().toInt();
		for (int i = 0; i<3; i++){
			if (maxPopup[i] >= 0) mReporter->setMaxPopup((MessageReporter::messagePriority)i,maxPopup[i]);
			if (maxLog[i] >= 0) mReporter->setMaxLog((MessageReporter::messagePriority)i,maxLog[i]);
		}
		//see if the filename changed:
		if (logFileName != sessionParamsDlg->logFileName->text().ascii())
			mReporter->reset(sessionParamsDlg->logFileName->text().ascii());
	}
	delete sessionParamsDlg;
	sessionParamsDlg = 0;
}
//Slot to launch a file-chooser dialog.  Save its results 
void SessionParams::
logFileChoose(){
	QString s = QFileDialog::getSaveFileName (Session::getInstance()->getLogfileName().c_str(), "Text files (*.txt)", 0, 0, 
		"Select Log File Name" );
	if (s) {
		sessionParamsDlg->logFileName->setText(s);
		sessionParamsDlg->update();
	}
}
