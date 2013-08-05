/********************************************************************************
** Form generated from reading UI file 'Page3.ui'
**
** Created: Fri Aug 2 14:54:52 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE3_H
#define PAGE3_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QTableWidget>
#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE

class Ui_Page3
{
public:
    QLabel *label;
    QLabel *label_40;
    QListWidget *outputVdfFileList;
    QLabel *label_11;
    QLabel *label_3;
    QSpinBox *numtsSpinner;
    QSpinBox *startTimeSpinner;
    QLabel *label_14;
    QPushButton *browseOutputVdfFile;
    QProgressBar *progressBar;
    QPushButton *goButton;
    QPushButton *advanceOptionButton;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QPushButton *pushButton_3;
    QTableWidget *listWidget;
    QPushButton *vdfCommentButton;

    void setupUi(QWizardPage *Page3)
    {
        if (Page3->objectName().isEmpty())
            Page3->setObjectName(QString::fromUtf8("Page3"));
        Page3->resize(475, 360);
        label = new QLabel(Page3);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(22, 14, 101, 21));
        QFont font;
        font.setPointSize(17);
        label->setFont(font);
        label_40 = new QLabel(Page3);
        label_40->setObjectName(QString::fromUtf8("label_40"));
        label_40->setGeometry(QRect(20, 71, 151, 21));
        QFont font1;
        font1.setUnderline(false);
        label_40->setFont(font1);
        outputVdfFileList = new QListWidget(Page3);
        outputVdfFileList->setObjectName(QString::fromUtf8("outputVdfFileList"));
        outputVdfFileList->setGeometry(QRect(260, 36, 200, 21));
        label_11 = new QLabel(Page3);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setGeometry(QRect(281, 63, 71, 20));
        label_3 = new QLabel(Page3);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(261, 90, 101, 20));
        numtsSpinner = new QSpinBox(Page3);
        numtsSpinner->setObjectName(QString::fromUtf8("numtsSpinner"));
        numtsSpinner->setGeometry(QRect(368, 90, 91, 25));
        numtsSpinner->setMinimum(1);
        numtsSpinner->setMaximum(999999999);
        numtsSpinner->setValue(1);
        startTimeSpinner = new QSpinBox(Page3);
        startTimeSpinner->setObjectName(QString::fromUtf8("startTimeSpinner"));
        startTimeSpinner->setGeometry(QRect(368, 63, 91, 25));
        startTimeSpinner->setMinimum(1);
        startTimeSpinner->setMaximum(999999);
        startTimeSpinner->setValue(1);
        label_14 = new QLabel(Page3);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        label_14->setGeometry(QRect(260, 13, 101, 16));
        browseOutputVdfFile = new QPushButton(Page3);
        browseOutputVdfFile->setObjectName(QString::fromUtf8("browseOutputVdfFile"));
        browseOutputVdfFile->setGeometry(QRect(383, 8, 82, 23));
        progressBar = new QProgressBar(Page3);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setGeometry(QRect(320, 335, 131, 23));
        progressBar->setValue(24);
        goButton = new QPushButton(Page3);
        goButton->setObjectName(QString::fromUtf8("goButton"));
        goButton->setGeometry(QRect(245, 329, 71, 32));
        advanceOptionButton = new QPushButton(Page3);
        advanceOptionButton->setObjectName(QString::fromUtf8("advanceOptionButton"));
        advanceOptionButton->setGeometry(QRect(38, 329, 161, 32));
        pushButton = new QPushButton(Page3);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(20, 93, 71, 21));
        pushButton_2 = new QPushButton(Page3);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));
        pushButton_2->setGeometry(QRect(176, 93, 71, 21));
        pushButton_3 = new QPushButton(Page3);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
        pushButton_3->setGeometry(QRect(98, 93, 71, 21));
        listWidget = new QTableWidget(Page3);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));
        listWidget->setGeometry(QRect(20, 121, 439, 200));
        vdfCommentButton = new QPushButton(Page3);
        vdfCommentButton->setObjectName(QString::fromUtf8("vdfCommentButton"));
        vdfCommentButton->setGeometry(QRect(15, 37, 183, 32));

        retranslateUi(Page3);

        QMetaObject::connectSlotsByName(Page3);
    } // setupUi

    void retranslateUi(QWizardPage *Page3)
    {
        Page3->setWindowTitle(QApplication::translate("Page3", "WizardPage", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Page3", "Create VDF", 0, QApplication::UnicodeUTF8));
        label_40->setText(QApplication::translate("Page3", "Variables to be Copied:", 0, QApplication::UnicodeUTF8));
        label_11->setText(QApplication::translate("Page3", "Start Time", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("Page3", "# of Timesteps", 0, QApplication::UnicodeUTF8));
        label_14->setText(QApplication::translate("Page3", "Output VDF File:", 0, QApplication::UnicodeUTF8));
        browseOutputVdfFile->setText(QApplication::translate("Page3", "Browse", 0, QApplication::UnicodeUTF8));
        goButton->setText(QApplication::translate("Page3", "Go!", 0, QApplication::UnicodeUTF8));
        advanceOptionButton->setText(QApplication::translate("Page3", " Advanced Options", 0, QApplication::UnicodeUTF8));
        pushButton->setText(QApplication::translate("Page3", "Select All", 0, QApplication::UnicodeUTF8));
        pushButton_2->setText(QApplication::translate("Page3", "Add New", 0, QApplication::UnicodeUTF8));
        pushButton_3->setText(QApplication::translate("Page3", "Clear All", 0, QApplication::UnicodeUTF8));
        vdfCommentButton->setText(QApplication::translate("Page3", "Add VDF Comment", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page3: public Ui_Page3 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE3_H
