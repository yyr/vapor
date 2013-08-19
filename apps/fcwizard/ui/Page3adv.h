/********************************************************************************
** Form generated from reading UI file 'Page3adv.ui'
**
** Created: Fri Aug 16 11:50:12 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE3ADV_H
#define PAGE3ADV_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QTextEdit>

QT_BEGIN_NAMESPACE

class Ui_Page3adv
{
public:
    QLabel *label_5;
    QSpinBox *bsxSpinner;
    QSpinBox *bszSpinner;
    QCheckBox *periodicxButton;
    QLabel *label_4;
    QLabel *label_7;
    QTextEdit *compressionRatioBox;
    QCheckBox *periodicyButton;
    QSpinBox *bsySpinner;
    QCheckBox *periodiczButton;
    QPushButton *acceptButton;
    QPushButton *cancelButton;
    QPushButton *restoreDefaultButton;

    void setupUi(QDialog *Page3adv)
    {
        if (Page3adv->objectName().isEmpty())
            Page3adv->setObjectName(QString::fromUtf8("Page3adv"));
        Page3adv->resize(361, 217);
        label_5 = new QLabel(Page3adv);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(50, 67, 81, 31));
        label_5->setAlignment(Qt::AlignCenter);
        label_5->setWordWrap(true);
        bsxSpinner = new QSpinBox(Page3adv);
        bsxSpinner->setObjectName(QString::fromUtf8("bsxSpinner"));
        bsxSpinner->setGeometry(QRect(170, 70, 51, 25));
        bsxSpinner->setMinimum(1);
        bsxSpinner->setValue(64);
        bszSpinner = new QSpinBox(Page3adv);
        bszSpinner->setObjectName(QString::fromUtf8("bszSpinner"));
        bszSpinner->setGeometry(QRect(270, 70, 51, 25));
        bszSpinner->setMinimum(1);
        bszSpinner->setValue(64);
        periodicxButton = new QCheckBox(Page3adv);
        periodicxButton->setObjectName(QString::fromUtf8("periodicxButton"));
        periodicxButton->setGeometry(QRect(170, 110, 31, 20));
        label_4 = new QLabel(Page3adv);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(39, 110, 101, 16));
        label_7 = new QLabel(Page3adv);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(40, 24, 101, 31));
        label_7->setAlignment(Qt::AlignCenter);
        label_7->setWordWrap(true);
        compressionRatioBox = new QTextEdit(Page3adv);
        compressionRatioBox->setObjectName(QString::fromUtf8("compressionRatioBox"));
        compressionRatioBox->setGeometry(QRect(170, 26, 141, 21));
        periodicyButton = new QCheckBox(Page3adv);
        periodicyButton->setObjectName(QString::fromUtf8("periodicyButton"));
        periodicyButton->setGeometry(QRect(210, 110, 31, 20));
        bsySpinner = new QSpinBox(Page3adv);
        bsySpinner->setObjectName(QString::fromUtf8("bsySpinner"));
        bsySpinner->setGeometry(QRect(220, 70, 51, 25));
        bsySpinner->setMinimum(1);
        bsySpinner->setValue(64);
        periodiczButton = new QCheckBox(Page3adv);
        periodiczButton->setObjectName(QString::fromUtf8("periodiczButton"));
        periodiczButton->setGeometry(QRect(250, 110, 31, 20));
        acceptButton = new QPushButton(Page3adv);
        acceptButton->setObjectName(QString::fromUtf8("acceptButton"));
        acceptButton->setGeometry(QRect(180, 172, 114, 32));
        cancelButton = new QPushButton(Page3adv);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));
        cancelButton->setGeometry(QRect(50, 172, 114, 32));
        restoreDefaultButton = new QPushButton(Page3adv);
        restoreDefaultButton->setObjectName(QString::fromUtf8("restoreDefaultButton"));
        restoreDefaultButton->setGeometry(QRect(89, 138, 171, 32));

        retranslateUi(Page3adv);

        QMetaObject::connectSlotsByName(Page3adv);
    } // setupUi

    void retranslateUi(QDialog *Page3adv)
    {
        Page3adv->setWindowTitle(QApplication::translate("Page3adv", "Create VDF Metadata File: Advanced Options", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("Page3adv", "Storage Block Factor", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        bsxSpinner->setToolTip(QApplication::translate("Page3adv", "Internal storage blocking factor expressed in grid points (NXxNYxNZ).  Default: 64x64x64.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        bszSpinner->setToolTip(QApplication::translate("Page3adv", "Internal storage blocking factor expressed in grid points (NXxNYxNZ).  Default: 64x64x64.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        periodicxButton->setToolTip(QApplication::translate("Page3adv", "Axis periodicity specifies whether data will \"loop back\" to its start or end point, after exceeding the bounds of an axis.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        periodicxButton->setText(QApplication::translate("Page3adv", "X", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("Page3adv", "Axis Periodicity", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("Page3adv", "Compression Ratio List", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        compressionRatioBox->setToolTip(QApplication::translate("Page3adv", "Colon delimited list compression ratios. The default is 1:10:100:500.  The maximum compression ratio is wavelet and block size dependent. ", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        compressionRatioBox->setHtml(QApplication::translate("Page3adv", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Lucida Grande'; font-size:13pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">1:10:100:500</p></body></html>", 0, QApplication::UnicodeUTF8));
        periodicyButton->setText(QApplication::translate("Page3adv", "Y", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        bsySpinner->setToolTip(QApplication::translate("Page3adv", "Internal storage blocking factor expressed in grid points (NXxNYxNZ).  Default: 64x64x64.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        periodiczButton->setText(QApplication::translate("Page3adv", "Z", 0, QApplication::UnicodeUTF8));
        acceptButton->setText(QApplication::translate("Page3adv", "Accept", 0, QApplication::UnicodeUTF8));
        cancelButton->setText(QApplication::translate("Page3adv", "Cancel", 0, QApplication::UnicodeUTF8));
        restoreDefaultButton->setText(QApplication::translate("Page3adv", "Restore Default Values", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page3adv: public Ui_Page3adv {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE3ADV_H
