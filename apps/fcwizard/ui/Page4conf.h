/********************************************************************************
** Form generated from reading UI file 'Page4conf.ui'
**
** Created: Tue Aug 20 12:10:01 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE4CONF_H
#define PAGE4CONF_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>

QT_BEGIN_NAMESPACE

class Ui_Page4conf
{
public:
    QDialogButtonBox *buttonBox;
    QListWidget *timeConflictList;
    QLabel *label;

    void setupUi(QDialog *Page4conf)
    {
        if (Page4conf->objectName().isEmpty())
            Page4conf->setObjectName(QString::fromUtf8("Page4conf"));
        Page4conf->resize(400, 300);
        buttonBox = new QDialogButtonBox(Page4conf);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(30, 260, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        timeConflictList = new QListWidget(Page4conf);
        timeConflictList->setObjectName(QString::fromUtf8("timeConflictList"));
        timeConflictList->setGeometry(QRect(30, 40, 331, 211));
        label = new QLabel(Page4conf);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(30, 10, 201, 16));

        retranslateUi(Page4conf);
        QObject::connect(buttonBox, SIGNAL(accepted()), Page4conf, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), Page4conf, SLOT(reject()));

        QMetaObject::connectSlotsByName(Page4conf);
    } // setupUi

    void retranslateUi(QDialog *Page4conf)
    {
        Page4conf->setWindowTitle(QApplication::translate("Page4conf", "Dialog", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("Page4conf", "Time Conflicts for Variable \"U\"", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page4conf: public Ui_Page4conf {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE4CONF_H
