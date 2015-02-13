/********************************************************************************
** Form generated from reading UI file 'Page2.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PAGE2_H
#define PAGE2_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>
#include <QtGui/QWizardPage>

QT_BEGIN_NAMESPACE

class Ui_Page2
{
public:
    QListWidget *fileList;
    QLabel *label_8;
    QPushButton *addFileButton;
    QLabel *label_2;
    QPushButton *removeFileButton;
    QTextEdit *outputVDFtext;
    QLabel *vdfLabel;
    QPushButton *browseOutputVdfFile;
    QLabel *Title;
    QLabel *selectFilePixmap;
    QComboBox *dataTypeComboBox;
    QLabel *processIndicator;

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
        fileList = new QListWidget(Page2);
        fileList->setObjectName(QString::fromUtf8("fileList"));
        fileList->setGeometry(QRect(20, 182, 436, 131));
        fileList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        fileList->setSelectionMode(QAbstractItemView::ContiguousSelection);
        label_8 = new QLabel(Page2);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(11, 319, 141, 41));
        label_8->setLayoutDirection(Qt::LeftToRight);
        label_8->setAlignment(Qt::AlignCenter);
        label_8->setWordWrap(true);
        addFileButton = new QPushButton(Page2);
        addFileButton->setObjectName(QString::fromUtf8("addFileButton"));
        addFileButton->setGeometry(QRect(190, 157, 131, 20));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(addFileButton->sizePolicy().hasHeightForWidth());
        addFileButton->setSizePolicy(sizePolicy1);
        label_2 = new QLabel(Page2);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(30, 158, 171, 16));
        removeFileButton = new QPushButton(Page2);
        removeFileButton->setObjectName(QString::fromUtf8("removeFileButton"));
        removeFileButton->setGeometry(QRect(322, 157, 131, 20));
        outputVDFtext = new QTextEdit(Page2);
        outputVDFtext->setObjectName(QString::fromUtf8("outputVDFtext"));
        outputVDFtext->setGeometry(QRect(20, 114, 436, 22));
        outputVDFtext->setReadOnly(false);
        outputVDFtext->setTextInteractionFlags(Qt::TextEditorInteraction);
        vdfLabel = new QLabel(Page2);
        vdfLabel->setObjectName(QString::fromUtf8("vdfLabel"));
        vdfLabel->setGeometry(QRect(30, 88, 221, 16));
        browseOutputVdfFile = new QPushButton(Page2);
        browseOutputVdfFile->setObjectName(QString::fromUtf8("browseOutputVdfFile"));
        browseOutputVdfFile->setGeometry(QRect(343, 87, 95, 20));
        Title = new QLabel(Page2);
        Title->setObjectName(QString::fromUtf8("Title"));
        Title->setGeometry(QRect(100, 13, 311, 41));
        QFont font;
        font.setPointSize(19);
        Title->setFont(font);
        selectFilePixmap = new QLabel(Page2);
        selectFilePixmap->setObjectName(QString::fromUtf8("selectFilePixmap"));
        selectFilePixmap->setGeometry(QRect(22, 11, 59, 56));
        sizePolicy.setHeightForWidth(selectFilePixmap->sizePolicy().hasHeightForWidth());
        selectFilePixmap->setSizePolicy(sizePolicy);
        dataTypeComboBox = new QComboBox(Page2);
        dataTypeComboBox->setObjectName(QString::fromUtf8("dataTypeComboBox"));
        dataTypeComboBox->setEnabled(false);
        dataTypeComboBox->setGeometry(QRect(180, 323, 111, 30));
        processIndicator = new QLabel(Page2);
        processIndicator->setObjectName(QString::fromUtf8("processIndicator"));
        processIndicator->setGeometry(QRect(310, 40, 141, 41));
        processIndicator->setLayoutDirection(Qt::LeftToRight);
        processIndicator->setAlignment(Qt::AlignCenter);
        processIndicator->setWordWrap(true);

        retranslateUi(Page2);

        QMetaObject::connectSlotsByName(Page2);
    } // setupUi

    void retranslateUi(QWizardPage *Page2)
    {
        Page2->setWindowTitle(QApplication::translate("Page2", "WizardPage", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("Page2", "Input Data Type:", 0, QApplication::UnicodeUTF8));
        addFileButton->setText(QApplication::translate("Page2", "Browse File(s)", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("Page2", "Basis data files:", 0, QApplication::UnicodeUTF8));
        removeFileButton->setText(QApplication::translate("Page2", "Remove File(s)", 0, QApplication::UnicodeUTF8));
        vdfLabel->setText(QApplication::translate("Page2", "Output VDF File:", 0, QApplication::UnicodeUTF8));
        browseOutputVdfFile->setText(QApplication::translate("Page2", "Browse", 0, QApplication::UnicodeUTF8));
        Title->setText(QApplication::translate("Page2", "Create VDF", 0, QApplication::UnicodeUTF8));
        selectFilePixmap->setText(QString());
        dataTypeComboBox->clear();
        dataTypeComboBox->insertItems(0, QStringList()
         << QString()
         << QApplication::translate("Page2", "MOM", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Page2", "POP", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Page2", "ROMS", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Page2", "WRF", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Page2", "CAM", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("Page2", "GRIMs", 0, QApplication::UnicodeUTF8)
        );
        processIndicator->setText(QApplication::translate("Page2", "Analyzing Files...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class Page2: public Ui_Page2 {};
} // namespace Ui

QT_END_NAMESPACE

#endif // PAGE2_H
