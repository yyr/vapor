/********************************************************************************
** Form generated from reading UI file 'Page1.ui'
**
** Created: Wed Jul 31 13:32:06 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE1_H
#define PAGE1_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE

class Ui_Page1
{
public:
    QPushButton *populateDataButton;
    QLabel *label;
    QLabel *label_2;
    QPushButton *createVDFButton;
    QLabel *label_4;
    QLabel *label_3;
    QLabel *label_5;

    void setupUi(QWizardPage *Page1)
    {
        if (Page1->objectName().isEmpty())
            Page1->setObjectName(QString::fromUtf8("Page1"));
        Page1->resize(475, 360);
        populateDataButton = new QPushButton(Page1);
        populateDataButton->setObjectName(QString::fromUtf8("populateDataButton"));
        populateDataButton->setGeometry(QRect(270, 110, 141, 141));
        label = new QLabel(Page1);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(140, 20, 201, 16));
        label_2 = new QLabel(Page1);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(181, 50, 131, 16));
        createVDFButton = new QPushButton(Page1);
        createVDFButton->setObjectName(QString::fromUtf8("createVDFButton"));
        createVDFButton->setGeometry(QRect(70, 113, 141, 141));
        label_4 = new QLabel(Page1);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(291, 256, 91, 61));
        label_4->setAlignment(Qt::AlignCenter);
        label_4->setWordWrap(true);
        label_3 = new QLabel(Page1);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(90, 263, 101, 31));
        label_3->setAlignment(Qt::AlignCenter);
        label_3->setWordWrap(true);
        label_5 = new QLabel(Page1);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(225, 268, 21, 16));

        retranslateUi(Page1);

        QMetaObject::connectSlotsByName(Page1);
    } // setupUi

    void retranslateUi(QWizardPage *Page1)
    {
        Page1->setWindowTitle(QApplication::translate("Page1", "WizardPage", 0, QApplication::UnicodeUTF8));
        populateDataButton->setText(QString());
        label->setText(QApplication::translate("Page1", "VAPOR Data Conversion Wizard", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Page1", "Would you like to:", 0, QApplication::UnicodeUTF8));
        createVDFButton->setText(QString());
        label_4->setText(QApplication::translate("Page1", "Populate a VDC using an existing VDF metadata file", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("Page1", "Create a VDF metadata file", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("Page1", " or ", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page1: public Ui_Page1 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE1_H
