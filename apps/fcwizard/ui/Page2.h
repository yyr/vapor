/********************************************************************************
** Form generated from reading UI file 'Page2.ui'
**
** Created: Tue Jul 30 14:13:47 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE2_H
#define PAGE2_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE

class Ui_Page2
{
public:
    QRadioButton *romsRadioButton;
    QListWidget *fileList;
    QLabel *label_8;
    QPushButton *addFileButton;
    QRadioButton *momRadioButton;
    QLabel *label_2;
    QPushButton *removeFileButton;
    QRadioButton *popRadioButton;

    void setupUi(QWizardPage *Page2)
    {
        if (Page2->objectName().isEmpty())
            Page2->setObjectName(QString::fromUtf8("Page2"));
        Page2->resize(475, 360);
        romsRadioButton = new QRadioButton(Page2);
        romsRadioButton->setObjectName(QString::fromUtf8("romsRadioButton"));
        romsRadioButton->setEnabled(true);
        romsRadioButton->setGeometry(QRect(320, 323, 61, 21));
        romsRadioButton->setLayoutDirection(Qt::LeftToRight);
        romsRadioButton->setChecked(false);
        fileList = new QListWidget(Page2);
        fileList->setObjectName(QString::fromUtf8("fileList"));
        fileList->setGeometry(QRect(20, 56, 436, 250));
        fileList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        fileList->setSelectionMode(QAbstractItemView::MultiSelection);
        label_8 = new QLabel(Page2);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(63, 317, 91, 31));
        label_8->setLayoutDirection(Qt::LeftToRight);
        label_8->setAlignment(Qt::AlignCenter);
        label_8->setWordWrap(true);
        addFileButton = new QPushButton(Page2);
        addFileButton->setObjectName(QString::fromUtf8("addFileButton"));
        addFileButton->setGeometry(QRect(210, 20, 114, 32));
        momRadioButton = new QRadioButton(Page2);
        momRadioButton->setObjectName(QString::fromUtf8("momRadioButton"));
        momRadioButton->setEnabled(true);
        momRadioButton->setGeometry(QRect(170, 323, 61, 20));
        momRadioButton->setLayoutDirection(Qt::LeftToRight);
        momRadioButton->setChecked(true);
        label_2 = new QLabel(Page2);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(30, 27, 101, 16));
        removeFileButton = new QPushButton(Page2);
        removeFileButton->setObjectName(QString::fromUtf8("removeFileButton"));
        removeFileButton->setGeometry(QRect(320, 20, 121, 32));
        popRadioButton = new QRadioButton(Page2);
        popRadioButton->setObjectName(QString::fromUtf8("popRadioButton"));
        popRadioButton->setEnabled(true);
        popRadioButton->setGeometry(QRect(250, 323, 61, 20));
        popRadioButton->setLayoutDirection(Qt::LeftToRight);

        retranslateUi(Page2);

        QMetaObject::connectSlotsByName(Page2);
    } // setupUi

    void retranslateUi(QWizardPage *Page2)
    {
        Page2->setWindowTitle(QApplication::translate("Page2", "WizardPage", 0, QApplication::UnicodeUTF8));
        romsRadioButton->setText(QApplication::translate("Page2", "ROMS", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("Page2", "Input Data Type", 0, QApplication::UnicodeUTF8));
        addFileButton->setText(QApplication::translate("Page2", "Add File(s)", 0, QApplication::UnicodeUTF8));
        momRadioButton->setText(QApplication::translate("Page2", "MOM", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Page2", "Basis data files:", 0, QApplication::UnicodeUTF8));
        removeFileButton->setText(QApplication::translate("Page2", "Remove File(s)", 0, QApplication::UnicodeUTF8));
        popRadioButton->setText(QApplication::translate("Page2", "POP", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page2: public Ui_Page2 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE2_H
