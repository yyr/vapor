/********************************************************************************
** Form generated from reading UI file 'Page3addnewvar.ui'
**
** Created: Thu Sep 26 11:52:54 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE3ADDNEWVAR_H
#define PAGE3ADDNEWVAR_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPlainTextEdit>

QT_BEGIN_NAMESPACE

class Ui_Page3addnewvar
{
public:
    QDialogButtonBox *buttonBox;
    QPlainTextEdit *addVar;
    QLabel *label;

    void setupUi(QDialog *Page3addnewvar)
    {
        if (Page3addnewvar->objectName().isEmpty())
            Page3addnewvar->setObjectName(QString::fromUtf8("Page3addnewvar"));
        Page3addnewvar->resize(287, 112);
        buttonBox = new QDialogButtonBox(Page3addnewvar);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(50, 80, 171, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        addVar = new QPlainTextEdit(Page3addnewvar);
        addVar->setObjectName(QString::fromUtf8("addVar"));
        addVar->setGeometry(QRect(20, 40, 241, 31));
        label = new QLabel(Page3addnewvar);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(23, 13, 191, 16));

        retranslateUi(Page3addnewvar);
        QObject::connect(buttonBox, SIGNAL(accepted()), Page3addnewvar, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), Page3addnewvar, SLOT(reject()));

        QMetaObject::connectSlotsByName(Page3addnewvar);
    } // setupUi

    void retranslateUi(QDialog *Page3addnewvar)
    {
        Page3addnewvar->setWindowTitle(QApplication::translate("Page3addnewvar", "Dialog", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Page3addnewvar", "Define new variable name:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page3addnewvar: public Ui_Page3addnewvar {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE3ADDNEWVAR_H
