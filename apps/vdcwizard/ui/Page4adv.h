/********************************************************************************
** Form generated from reading UI file 'Page4adv.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE4ADV_H
#define PAGE4ADV_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>

QT_BEGIN_NAMESPACE

class Ui_Page4adv
{
public:
    QLabel *label_24;
    QLabel *label_25;
    QSpinBox *numThreadsSpinner;
    QSpinBox *refinementLevelSpinner;
    QSpinBox *compressionLevelSpinner;
    QLabel *label_23;
    QPushButton *acceptButton;
    QPushButton *cancelButton;
    QPushButton *restoreDefaultButton;

    void setupUi(QDialog *Page4adv)
    {
        if (Page4adv->objectName().isEmpty())
            Page4adv->setObjectName(QString::fromUtf8("Page4adv"));
        Page4adv->resize(301, 219);
        label_24 = new QLabel(Page4adv);
        label_24->setObjectName(QString::fromUtf8("label_24"));
        label_24->setGeometry(QRect(9, 29, 154, 40));
        label_24->setAlignment(Qt::AlignCenter);
        label_24->setWordWrap(true);
        label_25 = new QLabel(Page4adv);
        label_25->setObjectName(QString::fromUtf8("label_25"));
        label_25->setGeometry(QRect(18, 80, 136, 31));
        label_25->setAlignment(Qt::AlignCenter);
        label_25->setWordWrap(true);
        numThreadsSpinner = new QSpinBox(Page4adv);
        numThreadsSpinner->setObjectName(QString::fromUtf8("numThreadsSpinner"));
        numThreadsSpinner->setGeometry(QRect(170, 82, 82, 25));
        numThreadsSpinner->setMinimum(0);
        numThreadsSpinner->setValue(0);
        refinementLevelSpinner = new QSpinBox(Page4adv);
        refinementLevelSpinner->setObjectName(QString::fromUtf8("refinementLevelSpinner"));
        refinementLevelSpinner->setGeometry(QRect(170, 10, 82, 25));
        refinementLevelSpinner->setMinimum(-1);
        refinementLevelSpinner->setValue(-1);
        compressionLevelSpinner = new QSpinBox(Page4adv);
        compressionLevelSpinner->setObjectName(QString::fromUtf8("compressionLevelSpinner"));
        compressionLevelSpinner->setGeometry(QRect(170, 36, 82, 25));
        compressionLevelSpinner->setMinimum(-1);
        compressionLevelSpinner->setValue(-1);
        label_23 = new QLabel(Page4adv);
        label_23->setObjectName(QString::fromUtf8("label_23"));
        label_23->setGeometry(QRect(38, 14, 111, 16));
        label_23->setWordWrap(true);
        acceptButton = new QPushButton(Page4adv);
        acceptButton->setObjectName(QString::fromUtf8("acceptButton"));
        acceptButton->setGeometry(QRect(154, 170, 112, 20));
        cancelButton = new QPushButton(Page4adv);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));
        cancelButton->setGeometry(QRect(33, 170, 111, 20));
        restoreDefaultButton = new QPushButton(Page4adv);
        restoreDefaultButton->setObjectName(QString::fromUtf8("restoreDefaultButton"));
        restoreDefaultButton->setGeometry(QRect(33, 136, 233, 20));

        retranslateUi(Page4adv);

        QMetaObject::connectSlotsByName(Page4adv);
    } // setupUi

    void retranslateUi(QDialog *Page4adv)
    {
        Page4adv->setWindowTitle(QApplication::translate("Page4adv", "Populate VDF Data: Advanced Options", 0, QApplication::UnicodeUTF8));
        label_24->setText(QApplication::translate("Page4adv", "Compression Level", 0, QApplication::UnicodeUTF8));
        label_25->setText(QApplication::translate("Page4adv", "Number of     Threads", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        numThreadsSpinner->setToolTip(QApplication::translate("Page4adv", "Number of execution threads (0 => # processors).", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        refinementLevelSpinner->setToolTip(QApplication::translate("Page4adv", "Refinement levels saved. 0=>coarsest, 1=>next refinement, etc. -1=>finest.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        compressionLevelSpinner->setToolTip(QApplication::translate("Page4adv", "Compression levels saved. 0 => coarsest, 1 => next refinement, etc. -1  => all levels defined by the .vdf file. ", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_23->setText(QApplication::translate("Page4adv", "Refinement Level", 0, QApplication::UnicodeUTF8));
        acceptButton->setText(QApplication::translate("Page4adv", "Accept", 0, QApplication::UnicodeUTF8));
        cancelButton->setText(QApplication::translate("Page4adv", "Cancel", 0, QApplication::UnicodeUTF8));
        restoreDefaultButton->setText(QApplication::translate("Page4adv", "Restore Default Values", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page4adv: public Ui_Page4adv {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE4ADV_H
