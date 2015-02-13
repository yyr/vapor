/********************************************************************************
** Form generated from reading UI file 'CmdLine.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef CMDLINE_H
#define CMDLINE_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSpacerItem>
#include <QtGui/QTextEdit>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CommandLine
{
public:
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QSpacerItem *verticalSpacer_2;
    QTextEdit *commandLineText;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CommandLine)
    {
        if (CommandLine->objectName().isEmpty())
            CommandLine->setObjectName(QString::fromUtf8("CommandLine"));
        CommandLine->resize(631, 346);
        widget = new QWidget(CommandLine);
        widget->setObjectName(QString::fromUtf8("widget"));
        widget->setGeometry(QRect(40, 10, 576, 320));
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(widget);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout->addWidget(label);

        verticalSpacer_2 = new QSpacerItem(18, 17, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);

        commandLineText = new QTextEdit(widget);
        commandLineText->setObjectName(QString::fromUtf8("commandLineText"));
        commandLineText->setEnabled(true);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(commandLineText->sizePolicy().hasHeightForWidth());
        commandLineText->setSizePolicy(sizePolicy);
        commandLineText->setReadOnly(true);

        verticalLayout->addWidget(commandLineText);

        verticalSpacer = new QSpacerItem(20, 18, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(widget);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setLayoutDirection(Qt::LeftToRight);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Ok);
        buttonBox->setCenterButtons(true);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(CommandLine);
        QObject::connect(buttonBox, SIGNAL(accepted()), CommandLine, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), CommandLine, SLOT(reject()));

        QMetaObject::connectSlotsByName(CommandLine);
    } // setupUi

    void retranslateUi(QDialog *CommandLine)
    {
        CommandLine->setWindowTitle(QApplication::translate("CommandLine", "Dialog", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("CommandLine", "The following terminal command will convert your data according to the current settings:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class CommandLine: public Ui_CommandLine {};
} // namespace Ui

QT_END_NAMESPACE

#endif // CMDLINE_H
