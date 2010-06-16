//************************************************************************
//																		*
//		     Copyright (C)  2010										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		pythonedit.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2010
//
//	Description:	Window for editing python scripts
//					
//

#ifndef PYTHONEDIT_H
#define PYTHONEDIT_H

#include <QDialog>
#include <QMap>
#include <QPointer>

class QComboBox;
class QPushButton;
class QTextEdit;
class QMenu;


namespace VAPoR {
class PythonEdit : public QDialog
{
    Q_OBJECT

public:
   
	PythonEdit(QWidget *parent, QString varname = "");



private:
   
    bool checkScript();
	void addInputVar2();
	void addOutputVar2();
	void delInputVar2();
	void delOutputVar2();
	void addInputVar3();
	void addOutputVar3();
	void delInputVar3();
	void delOutputVar3();
	QComboBox* inputVars2;
	QComboBox* outputVars2;
	QComboBox* inputVars3;
	QComboBox* outputVars3;

private slots:
	
	void inputVarsActive2(int);
	void outputVarsActive2(int);
	void inputVarsActive3(int);
	void outputVarsActive3(int);
	void testScript();
	void applyScript();
	void quit();
   

private:
   
   

    QAction *actionSave,
        *actionTextBold,
        *actionTextUnderline,
        *actionTextItalic,
        *actionTextColor,
        *actionAlignLeft,
        *actionAlignCenter,
        *actionAlignRight,
        *actionAlignJustify,
        *actionUndo,
        *actionRedo,
        *actionCut,
        *actionCopy,
        *actionPaste;

    

   
    QString fileName;
    QTextEdit *pythonEdit;
};
};
#endif
