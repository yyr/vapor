/********************************************************************************
** Form generated from reading UI file 'Page3.ui'
**
** Created: Wed Aug 7 16:53:43 2013
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
    QLabel *label_11;
    QLabel *label_3;
    QSpinBox *numtsSpinner;
    QSpinBox *startTimeSpinner;
    QPushButton *goButton;
    QPushButton *advanceOptionButton;
    QPushButton *selectAllButton;
    QPushButton *addNewButton;
    QPushButton *clearAllButton;
    QTableWidget *tableWidget;
    QPushButton *vdfCommentButton;
    QLabel *createVdfLabel;

    void setupUi(QWizardPage *Page3)
    {
        if (Page3->objectName().isEmpty())
            Page3->setObjectName(QString::fromUtf8("Page3"));
        Page3->resize(475, 360);
        label = new QLabel(Page3);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(80, 10, 121, 41));
        QFont font;
        font.setPointSize(19);
        label->setFont(font);
        label_40 = new QLabel(Page3);
        label_40->setObjectName(QString::fromUtf8("label_40"));
        label_40->setGeometry(QRect(20, 67, 151, 21));
        QFont font1;
        font1.setUnderline(false);
        label_40->setFont(font1);
        label_11 = new QLabel(Page3);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setGeometry(QRect(261, 63, 96, 20));
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
        startTimeSpinner->setGeometry(QRect(367, 63, 91, 25));
        startTimeSpinner->setMinimum(1);
        startTimeSpinner->setMaximum(999999);
        startTimeSpinner->setValue(1);
        goButton = new QPushButton(Page3);
        goButton->setObjectName(QString::fromUtf8("goButton"));
        goButton->setGeometry(QRect(390, 330, 71, 32));
        advanceOptionButton = new QPushButton(Page3);
        advanceOptionButton->setObjectName(QString::fromUtf8("advanceOptionButton"));
        advanceOptionButton->setGeometry(QRect(30, 330, 161, 32));
        selectAllButton = new QPushButton(Page3);
        selectAllButton->setObjectName(QString::fromUtf8("selectAllButton"));
        selectAllButton->setGeometry(QRect(20, 89, 71, 21));
        addNewButton = new QPushButton(Page3);
        addNewButton->setObjectName(QString::fromUtf8("addNewButton"));
        addNewButton->setGeometry(QRect(176, 89, 71, 21));
        clearAllButton = new QPushButton(Page3);
        clearAllButton->setObjectName(QString::fromUtf8("clearAllButton"));
        clearAllButton->setGeometry(QRect(98, 89, 71, 21));
        tableWidget = new QTableWidget(Page3);
        tableWidget->setObjectName(QString::fromUtf8("tableWidget"));
        tableWidget->setGeometry(QRect(20, 121, 439, 200));
        vdfCommentButton = new QPushButton(Page3);
        vdfCommentButton->setObjectName(QString::fromUtf8("vdfCommentButton"));
        vdfCommentButton->setGeometry(QRect(200, 330, 183, 32));
        createVdfLabel = new QLabel(Page3);
        createVdfLabel->setObjectName(QString::fromUtf8("createVdfLabel"));
        createVdfLabel->setGeometry(QRect(20, 8, 55, 58));

        retranslateUi(Page3);

        QMetaObject::connectSlotsByName(Page3);
    } // setupUi

    void retranslateUi(QWizardPage *Page3)
    {
        Page3->setWindowTitle(QApplication::translate("Page3", "WizardPage", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Page3", "Create VDF", 0, QApplication::UnicodeUTF8));
        label_40->setText(QApplication::translate("Page3", "Variables to include:", 0, QApplication::UnicodeUTF8));
        label_11->setText(QApplication::translate("Page3", "Start Time Step", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("Page3", "# of Timesteps", 0, QApplication::UnicodeUTF8));
        goButton->setText(QApplication::translate("Page3", "Go!", 0, QApplication::UnicodeUTF8));
        advanceOptionButton->setText(QApplication::translate("Page3", " Advanced Options", 0, QApplication::UnicodeUTF8));
        selectAllButton->setText(QApplication::translate("Page3", "Select All", 0, QApplication::UnicodeUTF8));
        addNewButton->setText(QApplication::translate("Page3", "Add New", 0, QApplication::UnicodeUTF8));
        clearAllButton->setText(QApplication::translate("Page3", "Clear All", 0, QApplication::UnicodeUTF8));
        vdfCommentButton->setText(QApplication::translate("Page3", "Add VDF Comment", 0, QApplication::UnicodeUTF8));
        createVdfLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class Page3: public Ui_Page3 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE3_H
