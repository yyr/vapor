/********************************************************************************
** Form generated from reading UI file 'Page3cmt.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE3CMT_H
#define PAGE3CMT_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QTextEdit>

QT_BEGIN_NAMESPACE

class Ui_Page3cmt
{
public:
    QDialogButtonBox *buttonBox;
    QTextEdit *vdfComment;
    QLabel *label;

    void setupUi(QDialog *Page3cmt)
    {
        if (Page3cmt->objectName().isEmpty())
            Page3cmt->setObjectName(QString::fromUtf8("Page3cmt"));
        Page3cmt->resize(357, 248);
        buttonBox = new QDialogButtonBox(Page3cmt);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(170, 210, 171, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        vdfComment = new QTextEdit(Page3cmt);
        vdfComment->setObjectName(QString::fromUtf8("vdfComment"));
        vdfComment->setGeometry(QRect(20, 40, 311, 161));
        label = new QLabel(Page3cmt);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(23, 13, 191, 16));

        retranslateUi(Page3cmt);
        QObject::connect(buttonBox, SIGNAL(accepted()), Page3cmt, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), Page3cmt, SLOT(reject()));

        QMetaObject::connectSlotsByName(Page3cmt);
    } // setupUi

    void retranslateUi(QDialog *Page3cmt)
    {
        Page3cmt->setWindowTitle(QApplication::translate("Page3cmt", "Dialog", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Page3cmt", "Add VDF Top Level Comment:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page3cmt: public Ui_Page3cmt {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE3CMT_H
