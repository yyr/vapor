/********************************************************************************
** Form generated from reading UI file 'Page2.ui'
**
** Created: Thu Sep 26 11:52:54 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE2_H
#define PAGE2_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QTextEdit>
#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE

class Ui_Page2
{
public:
    QRadioButton *romsRadioButton;
    QListWidget *fileList;
    QLabel *label_8;
    QPushButton *addFileButton;
    QRadioButton *momRadioButton;
    QLabel *label_2;
    QPushButton *removeFileButton;
    QRadioButton *popRadioButton;
    QTextEdit *outputVDFtext;
    QLabel *vdfLabel;
    QPushButton *browseOutputVdfFile;
    QLabel *Title;
    QLabel *selectFilePixmap;

    void setupUi(QWizardPage *Page2)
    {
        if (Page2->objectName().isEmpty())
            Page2->setObjectName(QString::fromUtf8("Page2"));
        Page2->resize(475, 360);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(Page2->sizePolicy().hasHeightForWidth());
        Page2->setSizePolicy(sizePolicy);
        romsRadioButton = new QRadioButton(Page2);
        romsRadioButton->setObjectName(QString::fromUtf8("romsRadioButton"));
        romsRadioButton->setEnabled(false);
        romsRadioButton->setGeometry(QRect(322, 330, 61, 21));
        romsRadioButton->setLayoutDirection(Qt::LeftToRight);
        romsRadioButton->setChecked(false);
        fileList = new QListWidget(Page2);
        fileList->setObjectName(QString::fromUtf8("fileList"));
        fileList->setGeometry(QRect(20, 182, 436, 131));
        fileList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        fileList->setSelectionMode(QAbstractItemView::MultiSelection);
        label_8 = new QLabel(Page2);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(65, 324, 91, 31));
        label_8->setLayoutDirection(Qt::LeftToRight);
        label_8->setAlignment(Qt::AlignCenter);
        label_8->setWordWrap(true);
        addFileButton = new QPushButton(Page2);
        addFileButton->setObjectName(QString::fromUtf8("addFileButton"));
        addFileButton->setGeometry(QRect(220, 157, 110, 20));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(addFileButton->sizePolicy().hasHeightForWidth());
        addFileButton->setSizePolicy(sizePolicy1);
        momRadioButton = new QRadioButton(Page2);
        momRadioButton->setObjectName(QString::fromUtf8("momRadioButton"));
        momRadioButton->setEnabled(false);
        momRadioButton->setGeometry(QRect(172, 330, 61, 20));
        momRadioButton->setLayoutDirection(Qt::LeftToRight);
        momRadioButton->setChecked(false);
        label_2 = new QLabel(Page2);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(30, 158, 101, 16));
        removeFileButton = new QPushButton(Page2);
        removeFileButton->setObjectName(QString::fromUtf8("removeFileButton"));
        removeFileButton->setGeometry(QRect(332, 157, 121, 20));
        popRadioButton = new QRadioButton(Page2);
        popRadioButton->setObjectName(QString::fromUtf8("popRadioButton"));
        popRadioButton->setEnabled(false);
        popRadioButton->setGeometry(QRect(252, 330, 61, 20));
        popRadioButton->setLayoutDirection(Qt::LeftToRight);
        outputVDFtext = new QTextEdit(Page2);
        outputVDFtext->setObjectName(QString::fromUtf8("outputVDFtext"));
        outputVDFtext->setGeometry(QRect(20, 114, 436, 22));
        vdfLabel = new QLabel(Page2);
        vdfLabel->setObjectName(QString::fromUtf8("vdfLabel"));
        vdfLabel->setGeometry(QRect(30, 88, 221, 16));
        browseOutputVdfFile = new QPushButton(Page2);
        browseOutputVdfFile->setObjectName(QString::fromUtf8("browseOutputVdfFile"));
        browseOutputVdfFile->setGeometry(QRect(343, 87, 95, 20));
        Title = new QLabel(Page2);
        Title->setObjectName(QString::fromUtf8("Title"));
        Title->setGeometry(QRect(100, 13, 271, 41));
        QFont font;
        font.setPointSize(19);
        Title->setFont(font);
        selectFilePixmap = new QLabel(Page2);
        selectFilePixmap->setObjectName(QString::fromUtf8("selectFilePixmap"));
        selectFilePixmap->setGeometry(QRect(22, 11, 59, 56));
        sizePolicy.setHeightForWidth(selectFilePixmap->sizePolicy().hasHeightForWidth());
        selectFilePixmap->setSizePolicy(sizePolicy);

        retranslateUi(Page2);

        QMetaObject::connectSlotsByName(Page2);
    } // setupUi

    void retranslateUi(QWizardPage *Page2)
    {
        Page2->setWindowTitle(QApplication::translate("Page2", "WizardPage", 0, QApplication::UnicodeUTF8));
        romsRadioButton->setText(QApplication::translate("Page2", "ROMS", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("Page2", "Input Data Type", 0, QApplication::UnicodeUTF8));
        addFileButton->setText(QApplication::translate("Page2", "Browse File(s)", 0, QApplication::UnicodeUTF8));
        momRadioButton->setText(QApplication::translate("Page2", "MOM", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Page2", "Basis data files:", 0, QApplication::UnicodeUTF8));
        removeFileButton->setText(QApplication::translate("Page2", "Remove File(s)", 0, QApplication::UnicodeUTF8));
        popRadioButton->setText(QApplication::translate("Page2", "POP", 0, QApplication::UnicodeUTF8));
        vdfLabel->setText(QApplication::translate("Page2", "Output VDF File:", 0, QApplication::UnicodeUTF8));
        browseOutputVdfFile->setText(QApplication::translate("Page2", "Browse", 0, QApplication::UnicodeUTF8));
        Title->setText(QApplication::translate("Page2", "Create VDF", 0, QApplication::UnicodeUTF8));
        selectFilePixmap->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class Page2: public Ui_Page2 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE2_H
