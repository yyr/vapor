/********************************************************************************
** Form generated from reading UI file 'ErrMsg.ui'
**
** Created: Thu Sep 26 11:52:54 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ERRMSG_H
#define ERRMSG_H

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

class Ui_ErrMsg
{
public:
    QDialogButtonBox *buttonBox;
    QLabel *label;
    QTextEdit *errorList;

    void setupUi(QDialog *ErrMsg)
    {
        if (ErrMsg->objectName().isEmpty())
            ErrMsg->setObjectName(QString::fromUtf8("ErrMsg"));
        ErrMsg->resize(359, 304);
        buttonBox = new QDialogButtonBox(ErrMsg);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(140, 260, 81, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        label = new QLabel(ErrMsg);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(25, 13, 251, 16));
        errorList = new QTextEdit(ErrMsg);
        errorList->setObjectName(QString::fromUtf8("errorList"));
        errorList->setEnabled(true);
        errorList->setGeometry(QRect(30, 40, 301, 211));
        errorList->setReadOnly(true);

        retranslateUi(ErrMsg);
        QObject::connect(buttonBox, SIGNAL(accepted()), ErrMsg, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), ErrMsg, SLOT(reject()));

        QMetaObject::connectSlotsByName(ErrMsg);
    } // setupUi

    void retranslateUi(QDialog *ErrMsg)
    {
        ErrMsg->setWindowTitle(QApplication::translate("ErrMsg", "Dialog", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("ErrMsg", "The following errors occurred:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class ErrMsg: public Ui_ErrMsg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ERRMSG_H
