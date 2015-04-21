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
class QListWidgetItem;
class QListWidget;



namespace VAPoR {
class ScriptCommand;
class PythonEdit : public QDialog
{
    Q_OBJECT

public:
   
	PythonEdit(QWidget *parent, QString varname = "");
	//Load the user startup script (if it exists) from default location
	//Performed right after preferences are loaded.
	static bool loadUserStartupScript();

private:
   
    bool checkScript();
	
	QListWidget* inputVars2;
	QListWidget* inputVars3;
	QListWidget* outputVars2;
	QListWidget* outputVars3;
	QPushButton* add3DVar;
	QPushButton* add2DVar;
	QPushButton* rem3DVar;
	QPushButton* rem2DVar;
	static const QString varDefWhatsThisText;
	static const QString startupWhatsThisText;

private slots:
	void addOutputVar2();
	void delOutputVar2();
	void addOutputVar3();
	void delOutputVar3();
	void inputVarsActive2(QListWidgetItem*);
	void inputVarsActive3(QListWidgetItem*);
	void testScript();
	void applyScript();
	void saveScript();
	void loadScript();
	void getHelp();
	void quit();
	void textChanged();
	void deleteScript();
   

private:
	ScriptCommand* currentCommand;
   bool changeFlag;
   bool startUp;
   QString variableName;
	int currentScriptId;
   

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
