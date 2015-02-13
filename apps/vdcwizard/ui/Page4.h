/********************************************************************************
** Form generated from reading UI file 'Page4.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
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
#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE

class Ui_Page4
{
public:
    QLabel *label;
    QLabel *label_29;
    QSpinBox *startTimeSpinner;
    QLabel *label_26;
    QSpinBox *numtsSpinner;
    QLabel *label_28;
    QPushButton *advancedOptionsButton;
    QPushButton *clearAllButton;
    QPushButton *selectAllButton;
    QTableWidget *tableWidget;
    QLabel *populateDataLabel;
    QProgressBar *progressBar;
    QLabel *percentCompleteLabel;
    QPushButton *cancelButton;
    QPushButton *showCommandButton;

    void setupUi(QWizardPage *Page4)
    {
        if (Page4->objectName().isEmpty())
            Page4->setObjectName(QString::fromUtf8("Page4"));
        Page4->resize(475, 360);
        label = new QLabel(Page4);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(100, 13, 341, 41));
        QFont font;
        font.setPointSize(19);
        label->setFont(font);
        label_29 = new QLabel(Page4);
        label_29->setObjectName(QString::fromUtf8("label_29"));
        label_29->setGeometry(QRect(20, 72, 191, 21));
        QFont font1;
        font1.setUnderline(false);
        label_29->setFont(font1);
        startTimeSpinner = new QSpinBox(Page4);
        startTimeSpinner->setObjectName(QString::fromUtf8("startTimeSpinner"));
        startTimeSpinner->setGeometry(QRect(368, 63, 91, 25));
        startTimeSpinner->setMaximum(1000);
        startTimeSpinner->setValue(0);
        label_26 = new QLabel(Page4);
        label_26->setObjectName(QString::fromUtf8("label_26"));
        label_26->setGeometry(QRect(241, 91, 121, 20));
        label_26->setAlignment(Qt::AlignCenter);
        label_26->setWordWrap(true);
        numtsSpinner = new QSpinBox(Page4);
        numtsSpinner->setObjectName(QString::fromUtf8("numtsSpinner"));
        numtsSpinner->setGeometry(QRect(368, 90, 91, 25));
        numtsSpinner->setMinimum(0);
        numtsSpinner->setMaximum(1000);
        numtsSpinner->setValue(1);
        label_28 = new QLabel(Page4);
        label_28->setObjectName(QString::fromUtf8("label_28"));
        label_28->setGeometry(QRect(242, 63, 120, 20));
        label_28->setAlignment(Qt::AlignCenter);
        label_28->setWordWrap(true);
        advancedOptionsButton = new QPushButton(Page4);
        advancedOptionsButton->setObjectName(QString::fromUtf8("advancedOptionsButton"));
        advancedOptionsButton->setGeometry(QRect(28, 328, 151, 23));
        clearAllButton = new QPushButton(Page4);
        clearAllButton->setObjectName(QString::fromUtf8("clearAllButton"));
        clearAllButton->setGeometry(QRect(127, 94, 91, 21));
        selectAllButton = new QPushButton(Page4);
        selectAllButton->setObjectName(QString::fromUtf8("selectAllButton"));
        selectAllButton->setGeometry(QRect(20, 94, 101, 21));
        tableWidget = new QTableWidget(Page4);
        tableWidget->setObjectName(QString::fromUtf8("tableWidget"));
        tableWidget->setGeometry(QRect(20, 121, 439, 200));
        tableWidget->verticalHeader()->setVisible(false);
        populateDataLabel = new QLabel(Page4);
        populateDataLabel->setObjectName(QString::fromUtf8("populateDataLabel"));
        populateDataLabel->setGeometry(QRect(22, 11, 59, 56));
        progressBar = new QProgressBar(Page4);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setGeometry(QRect(302, 34, 118, 23));
        progressBar->setValue(24);
        progressBar->setTextVisible(false);
        percentCompleteLabel = new QLabel(Page4);
        percentCompleteLabel->setObjectName(QString::fromUtf8("percentCompleteLabel"));
        percentCompleteLabel->setGeometry(QRect(286, 13, 151, 20));
        percentCompleteLabel->setAlignment(Qt::AlignCenter);
        cancelButton = new QPushButton(Page4);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));
        cancelButton->setGeometry(QRect(350, 328, 101, 23));
        showCommandButton = new QPushButton(Page4);
        showCommandButton->setObjectName(QString::fromUtf8("showCommandButton"));
        showCommandButton->setGeometry(QRect(183, 328, 163, 23));

        retranslateUi(Page4);

        QMetaObject::connectSlotsByName(Page4);
    } // setupUi

    void retranslateUi(QWizardPage *Page4)
    {
        Page4->setWindowTitle(QApplication::translate("Page4", "WizardPage", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Page4", "Populate Data", 0, QApplication::UnicodeUTF8));
        label_29->setText(QApplication::translate("Page4", "Variables to be written:", 0, QApplication::UnicodeUTF8));
        label_26->setText(QApplication::translate("Page4", "# of Timesteps", 0, QApplication::UnicodeUTF8));
        label_28->setText(QApplication::translate("Page4", "Start Time Step", 0, QApplication::UnicodeUTF8));
        advancedOptionsButton->setText(QApplication::translate("Page4", "Advanced", 0, QApplication::UnicodeUTF8));
        clearAllButton->setText(QApplication::translate("Page4", "Clear All", 0, QApplication::UnicodeUTF8));
        selectAllButton->setText(QApplication::translate("Page4", "Select All", 0, QApplication::UnicodeUTF8));
        populateDataLabel->setText(QString());
        percentCompleteLabel->setText(QString());
        cancelButton->setText(QApplication::translate("Page4", "Cancel", 0, QApplication::UnicodeUTF8));
        showCommandButton->setText(QApplication::translate("Page4", "Show Command", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page4: public Ui_Page4 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE4_H
