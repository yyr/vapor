/********************************************************************************
** Form generated from reading UI file 'Page2badfile.ui'
**
** Created: Mon Sep 23 12:30:53 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE2BADFILE_H
#define PAGE2BADFILE_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>

QT_BEGIN_NAMESPACE

class Ui_Page2badfile
{
public:
    QDialogButtonBox *buttonBox;
    QLabel *label;
    QLabel *label_2;

    void setupUi(QDialog *Page2badfile)
    {
        if (Page2badfile->objectName().isEmpty())
            Page2badfile->setObjectName(QString::fromUtf8("Page2badfile"));
        Page2badfile->resize(351, 146);
        buttonBox = new QDialogButtonBox(Page2badfile);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(140, 109, 81, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        label = new QLabel(Page2badfile);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(29, 18, 62, 16));
        label_2 = new QLabel(Page2badfile);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(50, 30, 271, 81));
        label_2->setWordWrap(true);

        retranslateUi(Page2badfile);
        QObject::connect(buttonBox, SIGNAL(accepted()), Page2badfile, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), Page2badfile, SLOT(reject()));

        QMetaObject::connectSlotsByName(Page2badfile);
    } // setupUi

    void retranslateUi(QDialog *Page2badfile)
    {
        Page2badfile->setWindowTitle(QApplication::translate("Page2badfile", "Dialog", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Page2badfile", "Warning:", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Page2badfile", "The file you've selected does not have the standard .vdf file extension.  If the specified file already exists, its will be corrupted if you proceed.", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page2badfile: public Ui_Page2badfile {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE2BADFILE_H
