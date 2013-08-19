/********************************************************************************
** Form generated from reading UI file 'Page4.ui'
**
** Created: Fri Aug 16 11:50:14 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE4_H
#define PAGE4_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QTableWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE

class Ui_Page4
{
public:
    QLabel *label;
    QLabel *label_29;
    QSpinBox *startTimeSpinner;
    QLabel *label_26;
    QLabel *label_12;
    QTextEdit *inputVdfFileEdit;
    QSpinBox *numtsSpinner;
    QLabel *label_28;
    QPushButton *browseOutputVdfFile;
    QProgressBar *progressBar;
    QPushButton *goButton;
    QPushButton *advancedOptionsButton;
    QPushButton *pushButton_3;
    QPushButton *scanVDF;
    QPushButton *pushButton_2;
    QTableWidget *variableList;
    QPushButton *pushButton_4;

    void setupUi(QWizardPage *Page4)
    {
        if (Page4->objectName().isEmpty())
            Page4->setObjectName(QString::fromUtf8("Page4"));
        Page4->resize(475, 360);
        label = new QLabel(Page4);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(22, 14, 161, 21));
        QFont font;
        font.setPointSize(17);
        label->setFont(font);
        label_29 = new QLabel(Page4);
        label_29->setObjectName(QString::fromUtf8("label_29"));
        label_29->setGeometry(QRect(20, 40, 151, 21));
        QFont font1;
        font1.setUnderline(false);
        label_29->setFont(font1);
        startTimeSpinner = new QSpinBox(Page4);
        startTimeSpinner->setObjectName(QString::fromUtf8("startTimeSpinner"));
        startTimeSpinner->setGeometry(QRect(368, 63, 91, 25));
        label_26 = new QLabel(Page4);
        label_26->setObjectName(QString::fromUtf8("label_26"));
        label_26->setGeometry(QRect(261, 90, 101, 20));
        label_26->setAlignment(Qt::AlignCenter);
        label_26->setWordWrap(true);
        label_12 = new QLabel(Page4);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setGeometry(QRect(260, 13, 101, 19));
        inputVdfFileEdit = new QTextEdit(Page4);
        inputVdfFileEdit->setObjectName(QString::fromUtf8("inputVdfFileEdit"));
        inputVdfFileEdit->setGeometry(QRect(260, 35, 199, 22));
        numtsSpinner = new QSpinBox(Page4);
        numtsSpinner->setObjectName(QString::fromUtf8("numtsSpinner"));
        numtsSpinner->setGeometry(QRect(368, 90, 91, 25));
        numtsSpinner->setMinimum(-1);
        numtsSpinner->setMaximum(10000);
        numtsSpinner->setValue(-1);
        label_28 = new QLabel(Page4);
        label_28->setObjectName(QString::fromUtf8("label_28"));
        label_28->setGeometry(QRect(258, 63, 101, 20));
        label_28->setAlignment(Qt::AlignCenter);
        label_28->setWordWrap(true);
        browseOutputVdfFile = new QPushButton(Page4);
        browseOutputVdfFile->setObjectName(QString::fromUtf8("browseOutputVdfFile"));
        browseOutputVdfFile->setGeometry(QRect(383, 8, 82, 23));
        progressBar = new QProgressBar(Page4);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setGeometry(QRect(320, 335, 131, 23));
        progressBar->setValue(24);
        goButton = new QPushButton(Page4);
        goButton->setObjectName(QString::fromUtf8("goButton"));
        goButton->setGeometry(QRect(245, 329, 71, 32));
        advancedOptionsButton = new QPushButton(Page4);
        advancedOptionsButton->setObjectName(QString::fromUtf8("advancedOptionsButton"));
        advancedOptionsButton->setGeometry(QRect(38, 329, 161, 32));
        pushButton_3 = new QPushButton(Page4);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
        pushButton_3->setGeometry(QRect(125, 94, 91, 21));
        scanVDF = new QPushButton(Page4);
        scanVDF->setObjectName(QString::fromUtf8("scanVDF"));
        scanVDF->setEnabled(true);
        scanVDF->setGeometry(QRect(20, 70, 101, 21));
        pushButton_2 = new QPushButton(Page4);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        pushButton_2->setGeometry(QRect(20, 95, 101, 21));
        variableList = new QTableWidget(Page4);
        variableList->setObjectName(QString::fromUtf8("variableList"));
        variableList->setGeometry(QRect(20, 121, 431, 200));
        pushButton_4 = new QPushButton(Page4);
        pushButton_4->setObjectName(QString::fromUtf8("pushButton_4"));
        pushButton_4->setEnabled(false);
        pushButton_4->setGeometry(QRect(125, 70, 91, 21));

        retranslateUi(Page4);

        QMetaObject::connectSlotsByName(Page4);
    } // setupUi

    void retranslateUi(QWizardPage *Page4)
    {
        Page4->setWindowTitle(QApplication::translate("Page4", "WizardPage", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Page4", "Populate Data", 0, QApplication::UnicodeUTF8));
        label_29->setText(QApplication::translate("Page4", "Variables to be written:", 0, QApplication::UnicodeUTF8));
        label_26->setText(QApplication::translate("Page4", "# of Timesteps", 0, QApplication::UnicodeUTF8));
        label_12->setText(QApplication::translate("Page4", "Input VDF File:", 0, QApplication::UnicodeUTF8));
        label_28->setText(QApplication::translate("Page4", "Start Time Step", 0, QApplication::UnicodeUTF8));
        browseOutputVdfFile->setText(QApplication::translate("Page4", "Browse", 0, QApplication::UnicodeUTF8));
        goButton->setText(QApplication::translate("Page4", "Go!", 0, QApplication::UnicodeUTF8));
        advancedOptionsButton->setText(QApplication::translate("Page4", "Advanced Options", 0, QApplication::UnicodeUTF8));
        pushButton_3->setText(QApplication::translate("Page4", "Clear All", 0, QApplication::UnicodeUTF8));
        scanVDF->setText(QApplication::translate("Page4", "Scan VDF", 0, QApplication::UnicodeUTF8));
        pushButton_2->setText(QApplication::translate("Page4", "Select All", 0, QApplication::UnicodeUTF8));
        pushButton_4->setText(QApplication::translate("Page4", "Scan Data", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page4: public Ui_Page4 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE4_H
