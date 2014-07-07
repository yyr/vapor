/********************************************************************************
** Form generated from reading UI file 'isolinetab.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ISOLINETAB_H
#define ISOLINETAB_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "../../guis/instancetable.h"
#include "../../guis/qtthumbwheel.h"
#include "../isolineframe.h"
#include "instancetable.h"
#include "isolineframe.h"
#include "mappingframe.h"
#include "qtthumbwheel.h"

QT_BEGIN_NAMESPACE

class Ui_IsolineTab
{
public:
    QHBoxLayout *horizontalLayout_10;
    QVBoxLayout *verticalLayout_7;
    QLabel *textLabel1;
    QHBoxLayout *hboxLayout;
    InstanceTable *instanceTable;
    QSpacerItem *spacer32;
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout1;
    QPushButton *newInstanceButton;
    QSpacerItem *spacer36;
    QPushButton *deleteInstanceButton;
    QHBoxLayout *hboxLayout2;
    QSpacerItem *spacer37;
    QComboBox *copyCombo;
    QSpacerItem *spacer152;
    QSpacerItem *spacer26_2;
    QSpacerItem *verticalSpacer_4;
    QFrame *frame_2;
    QHBoxLayout *horizontalLayout_4;
    QVBoxLayout *verticalLayout_2;
    QLabel *label_2;
    QVBoxLayout *verticalLayout_13;
    QHBoxLayout *fidelityLayout;
    QSpacerItem *horizontalSpacer_16;
    QPushButton *fidelityDefaultButton;
    QSpacerItem *horizontalSpacer_9;
    QGroupBox *fidelityBox;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QLabel *textLabel1_13;
    QComboBox *lodCombo;
    QSpacerItem *spacer111;
    QLabel *textLabel1_2;
    QComboBox *refinementCombo;
    QSpacerItem *spacer110;
    QVBoxLayout *verticalLayout_9;
    QLabel *textLabel1_3;
    QHBoxLayout *horizontalLayout_5;
    QSpacerItem *horizontalSpacer_15;
    QLabel *label_9;
    QComboBox *dimensionCombo;
    QSpacerItem *horizontalSpacer_17;
    QLabel *label_10;
    QComboBox *variableCombo;
    QSpacerItem *horizontalSpacer_18;
    QHBoxLayout *horizontalLayout_15;
    QSpacerItem *horizontalSpacer_19;
    QLabel *label_11;
    QLineEdit *minIsoEdit;
    QLabel *label_12;
    QLineEdit *maxIsoEdit;
    QLabel *label_13;
    QLineEdit *countIsoEdit;
    QHBoxLayout *_13;
    QPushButton *editButton;
    QPushButton *navigateButton;
    QPushButton *alignButton;
    QPushButton *newHistoButton;
    QFrame *isoSelectFrame;
    QVBoxLayout *_14;
    MappingFrame *isoSelectionFrame;
    QHBoxLayout *_15;
    QLineEdit *leftHistoEdit;
    QSpacerItem *spacer34;
    QLabel *textLabel2_4;
    QSpacerItem *spacer35;
    QLineEdit *rightHistoEdit;
    QHBoxLayout *_16;
    QLabel *minDataBound;
    QLabel *textLabel2_5;
    QLabel *maxDataBound;
    QHBoxLayout *_19;
    QSpacerItem *horizontalSpacer_8;
    QPushButton *fitDataButton;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *editIsovaluesButton;
    QSpacerItem *horizontalSpacer_20;
    QPushButton *fitIsovalsButton;
    QSpacerItem *spacer29_4;
    QHBoxLayout *_17;
    QSpacerItem *spacer45;
    QPushButton *loadButton;
    QSpacerItem *spacer160;
    QPushButton *loadInstalledButton;
    QSpacerItem *spacer30;
    QPushButton *saveButton;
    QSpacerItem *spacer46;
    QHBoxLayout *_20;
    QSpacerItem *horizontalSpacer_21;
    QPushButton *copyToProbeButton;
    QSpacerItem *spacer28_5;
    QLabel *textLabel1_17;
    QLineEdit *histoScaleEdit;
    QSpacerItem *spacer29_5;
    QSpacerItem *verticalSpacer_3;
    QFrame *frame;
    QHBoxLayout *horizontalLayout_8;
    QVBoxLayout *verticalLayout_6;
    QPushButton *showHideLayoutButton;
    QFrame *layoutFrame;
    QHBoxLayout *horizontalLayout_7;
    QVBoxLayout *vboxLayout1;
    QHBoxLayout *hboxLayout3;
    QVBoxLayout *vboxLayout2;
    QLabel *textLabel2_2_2;
    QHBoxLayout *hboxLayout4;
    QLabel *textLabel3_12_2;
    QSlider *xCenterSlider;
    QLineEdit *xCenterEdit;
    QHBoxLayout *hboxLayout5;
    QLabel *textLabel3_12_3_3;
    QSlider *yCenterSlider;
    QLineEdit *yCenterEdit;
    QHBoxLayout *hboxLayout6;
    QLabel *textLabel3_12_3_2_2;
    QSlider *zCenterSlider;
    QLineEdit *zCenterEdit;
    QVBoxLayout *vboxLayout3;
    QLabel *textLabel2_2;
    QHBoxLayout *hboxLayout7;
    QLabel *textLabel3_12;
    QSlider *xSizeSlider;
    QLineEdit *xSizeEdit;
    QHBoxLayout *hboxLayout8;
    QLabel *textLabel3_12_3;
    QSlider *ySizeSlider;
    QLineEdit *ySizeEdit;
    QHBoxLayout *hboxLayout9;
    QSpacerItem *spacer54_2;
    QHBoxLayout *hboxLayout10;
    QSpacerItem *spacer48_2;
    QLabel *textLabel1_11;
    QSpacerItem *spacer49_2;
    QLabel *textLabel1_11_4;
    QHBoxLayout *hboxLayout11;
    QLabel *textLabel9;
    QLabel *minUserXLabel;
    QLabel *minUserYLabel;
    QLabel *minUserZLabel;
    QSpacerItem *spacer53_3;
    QLabel *minGridXLabel;
    QLabel *minGridYLabel;
    QLabel *minGridZLabel;
    QHBoxLayout *hboxLayout12;
    QLabel *textLabel9_2;
    QLabel *maxUserXLabel;
    QLabel *maxUserYLabel;
    QLabel *maxUserZLabel;
    QSpacerItem *spacer52_2;
    QLabel *maxGridXLabel;
    QLabel *maxGridYLabel;
    QLabel *maxGridZLabel;
    QFrame *minMaxLonLatFrame;
    QHBoxLayout *hboxLayout13;
    QVBoxLayout *vboxLayout4;
    QHBoxLayout *hboxLayout14;
    QLabel *textLabel9_2_2;
    QLabel *maxLonLabel;
    QLabel *maxLatLabel;
    QHBoxLayout *hboxLayout15;
    QLabel *textLabel9_2_2_2;
    QLabel *minLonLabel;
    QLabel *minLatLabel;
    QSpacerItem *spacer51;
    QFrame *orientationFrame;
    QVBoxLayout *verticalLayout_4;
    QHBoxLayout *orientationLayout_2;
    QVBoxLayout *_2;
    QLabel *textLabel3_3;
    QGridLayout *_3;
    QLineEdit *thetaEdit;
    QLineEdit *psiEdit;
    QLabel *textLabel12_3;
    QLineEdit *phiEdit;
    QLabel *textLabel1_7;
    QLabel *textLabel12_4;
    QHBoxLayout *_4;
    QComboBox *axisAlignCombo;
    QComboBox *rotate90Combo;
    QVBoxLayout *_9;
    QLabel *textLabel2_3;
    QHBoxLayout *_10;
    QLabel *textLabel1_8;
    QtThumbWheel *xThumbWheel;
    QHBoxLayout *_11;
    QLabel *textLabel1_5_3;
    QtThumbWheel *yThumbWheel;
    QHBoxLayout *_12;
    QLabel *textLabel1_5_2_3;
    QtThumbWheel *zThumbWheel;
    QSpacerItem *spacer57_2;
    QHBoxLayout *hboxLayout16;
    QSpacerItem *spacer55_2;
    QPushButton *fitRegionButton;
    QPushButton *cropRegionButton;
    QPushButton *fitDomainButton;
    QPushButton *cropDomainButton;
    QSpacerItem *spacer56;
    QFrame *frame_4;
    QHBoxLayout *horizontalLayout_9;
    QVBoxLayout *verticalLayout_5;
    QPushButton *showHideImageButton;
    QFrame *imageFrame;
    QHBoxLayout *horizontalLayout_2;
    QVBoxLayout *verticalLayout;
    QFrame *isolineFrameHolder;
    QHBoxLayout *hboxLayout17;
    QVBoxLayout *vboxLayout5;
    QLabel *textLabel3;
    IsolineFrame *isolineImageFrame;
    QSpacerItem *spacer49;
    QFrame *frame8;
    QHBoxLayout *hboxLayout18;
    QVBoxLayout *vboxLayout6;
    QLabel *textLabel2_6;
    QHBoxLayout *hboxLayout19;
    QSpacerItem *spacer149;
    QLabel *textLabel5;
    QLabel *selectedXLabel;
    QLabel *selectedYLabel;
    QLabel *selectedZLabel;
    QSpacerItem *spacer150;
    QFrame *latLonFrame;
    QHBoxLayout *hboxLayout20;
    QHBoxLayout *hboxLayout21;
    QSpacerItem *spacer149_2;
    QLabel *textLabel5_2;
    QLabel *selectedLonLabel;
    QLabel *selectedLatLabel;
    QSpacerItem *spacer150_2;
    QHBoxLayout *hboxLayout22;
    QSpacerItem *spacer151;
    QLabel *variableLabel;
    QLabel *valueMagLabel;
    QSpacerItem *spacer152_2;
    QSpacerItem *spacer148;
    QHBoxLayout *hboxLayout23;
    QCheckBox *attachSeedCheckbox;
    QPushButton *addSeedButton;
    QSpacerItem *spacer144;
    QLabel *textLabel11;
    QHBoxLayout *hboxLayout24;
    QPushButton *regionCenterButton;
    QPushButton *viewCenterButton;
    QPushButton *rakeCenterButton;
    QPushButton *probeCenterButton;
    QPushButton *showHideAppearanceButton;
    QFrame *appearanceFrame;
    QVBoxLayout *verticalLayout_3;
    QVBoxLayout *verticalLayout_8;
    QLabel *label_6;
    QHBoxLayout *_5;
    QSpacerItem *horizontalSpacer_7;
    QLabel *textLabel1_12;
    QSpacerItem *spacer75_4;
    QLineEdit *isolineWidthEdit;
    QHBoxLayout *_6;
    QSpacerItem *spacer75_5;
    QLabel *textLabel1_14;
    QSpacerItem *horizontalSpacer_10;
    QLineEdit *textSizeEdit;
    QLabel *label_7;
    QHBoxLayout *_7;
    QSpacerItem *horizontalSpacer_6;
    QSpacerItem *horizontalSpacer_14;
    QLabel *textLabel1_15;
    QSpacerItem *spacer75_6;
    QSpacerItem *horizontalSpacer_11;
    QSpacerItem *horizontalSpacer_4;
    QPushButton *isolinePanelBackgroundColorButton;
    QLineEdit *isolinePanelBackgroundColorEdit;
    QSpacerItem *horizontalSpacer_12;
    QLineEdit *panelLineWidthEdit;
    QHBoxLayout *_8;
    QSpacerItem *spacer75_7;
    QLabel *textLabel1_16;
    QSpacerItem *horizontalSpacer_13;
    QLineEdit *panelTextSizeEdit;
    QFrame *frame_6;
    QFrame *frame_5;

    void setupUi(QWidget *IsolineTab)
    {
        if (IsolineTab->objectName().isEmpty())
            IsolineTab->setObjectName(QString::fromUtf8("IsolineTab"));
        IsolineTab->resize(450, 2195);
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(IsolineTab->sizePolicy().hasHeightForWidth());
        IsolineTab->setSizePolicy(sizePolicy);
        IsolineTab->setMinimumSize(QSize(0, 400));
        IsolineTab->setMaximumSize(QSize(450, 32767));
        horizontalLayout_10 = new QHBoxLayout(IsolineTab);
        horizontalLayout_10->setSpacing(6);
        horizontalLayout_10->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setSpacing(6);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        textLabel1 = new QLabel(IsolineTab);
        textLabel1->setObjectName(QString::fromUtf8("textLabel1"));
        textLabel1->setAlignment(Qt::AlignCenter);
        textLabel1->setWordWrap(false);

        verticalLayout_7->addWidget(textLabel1);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        instanceTable = new InstanceTable(IsolineTab);
        instanceTable->setObjectName(QString::fromUtf8("instanceTable"));
        instanceTable->setMinimumSize(QSize(150, 100));

        hboxLayout->addWidget(instanceTable);

        spacer32 = new QSpacerItem(35, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer32);

        vboxLayout = new QVBoxLayout();
        vboxLayout->setSpacing(6);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        newInstanceButton = new QPushButton(IsolineTab);
        newInstanceButton->setObjectName(QString::fromUtf8("newInstanceButton"));

        hboxLayout1->addWidget(newInstanceButton);

        spacer36 = new QSpacerItem(23, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacer36);

        deleteInstanceButton = new QPushButton(IsolineTab);
        deleteInstanceButton->setObjectName(QString::fromUtf8("deleteInstanceButton"));

        hboxLayout1->addWidget(deleteInstanceButton);


        vboxLayout->addLayout(hboxLayout1);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setSpacing(6);
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        spacer37 = new QSpacerItem(23, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacer37);

        copyCombo = new QComboBox(IsolineTab);
        copyCombo->setObjectName(QString::fromUtf8("copyCombo"));
        copyCombo->setMinimumSize(QSize(120, 0));
        copyCombo->setFocusPolicy(Qt::ClickFocus);

        hboxLayout2->addWidget(copyCombo);

        spacer152 = new QSpacerItem(21, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacer152);


        vboxLayout->addLayout(hboxLayout2);


        hboxLayout->addLayout(vboxLayout);

        spacer26_2 = new QSpacerItem(34, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer26_2);


        verticalLayout_7->addLayout(hboxLayout);

        verticalSpacer_4 = new QSpacerItem(20, 13, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_7->addItem(verticalSpacer_4);

        frame_2 = new QFrame(IsolineTab);
        frame_2->setObjectName(QString::fromUtf8("frame_2"));
        frame_2->setMaximumSize(QSize(422, 16777215));
        frame_2->setFrameShape(QFrame::StyledPanel);
        frame_2->setFrameShadow(QFrame::Plain);
        frame_2->setLineWidth(0);
        horizontalLayout_4 = new QHBoxLayout(frame_2);
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        label_2 = new QLabel(frame_2);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setAlignment(Qt::AlignCenter);

        verticalLayout_2->addWidget(label_2);

        verticalLayout_13 = new QVBoxLayout();
        verticalLayout_13->setSpacing(6);
        verticalLayout_13->setObjectName(QString::fromUtf8("verticalLayout_13"));
        fidelityLayout = new QHBoxLayout();
        fidelityLayout->setSpacing(6);
        fidelityLayout->setObjectName(QString::fromUtf8("fidelityLayout"));
        horizontalSpacer_16 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        fidelityLayout->addItem(horizontalSpacer_16);

        fidelityDefaultButton = new QPushButton(frame_2);
        fidelityDefaultButton->setObjectName(QString::fromUtf8("fidelityDefaultButton"));

        fidelityLayout->addWidget(fidelityDefaultButton);

        horizontalSpacer_9 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        fidelityLayout->addItem(horizontalSpacer_9);

        fidelityBox = new QGroupBox(frame_2);
        fidelityBox->setObjectName(QString::fromUtf8("fidelityBox"));

        fidelityLayout->addWidget(fidelityBox);


        verticalLayout_13->addLayout(fidelityLayout);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        textLabel1_13 = new QLabel(frame_2);
        textLabel1_13->setObjectName(QString::fromUtf8("textLabel1_13"));
        textLabel1_13->setAlignment(Qt::AlignCenter);
        textLabel1_13->setWordWrap(false);

        horizontalLayout->addWidget(textLabel1_13);

        lodCombo = new QComboBox(frame_2);
        lodCombo->setObjectName(QString::fromUtf8("lodCombo"));
        QFont font;
        font.setPointSize(10);
        lodCombo->setFont(font);
        lodCombo->setFocusPolicy(Qt::ClickFocus);
        lodCombo->setMaxCount(5);

        horizontalLayout->addWidget(lodCombo);

        spacer111 = new QSpacerItem(30, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(spacer111);

        textLabel1_2 = new QLabel(frame_2);
        textLabel1_2->setObjectName(QString::fromUtf8("textLabel1_2"));
        textLabel1_2->setAlignment(Qt::AlignCenter);
        textLabel1_2->setWordWrap(false);

        horizontalLayout->addWidget(textLabel1_2);

        refinementCombo = new QComboBox(frame_2);
        refinementCombo->setObjectName(QString::fromUtf8("refinementCombo"));
        refinementCombo->setFont(font);
        refinementCombo->setFocusPolicy(Qt::ClickFocus);
        refinementCombo->setMaxCount(5);

        horizontalLayout->addWidget(refinementCombo);

        spacer110 = new QSpacerItem(13, 38, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(spacer110);


        verticalLayout_13->addLayout(horizontalLayout);


        verticalLayout_2->addLayout(verticalLayout_13);

        verticalLayout_9 = new QVBoxLayout();
        verticalLayout_9->setSpacing(6);
        verticalLayout_9->setObjectName(QString::fromUtf8("verticalLayout_9"));
        textLabel1_3 = new QLabel(frame_2);
        textLabel1_3->setObjectName(QString::fromUtf8("textLabel1_3"));
        textLabel1_3->setMinimumSize(QSize(300, 0));
        textLabel1_3->setAlignment(Qt::AlignCenter);
        textLabel1_3->setWordWrap(false);

        verticalLayout_9->addWidget(textLabel1_3);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalSpacer_15 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_15);

        label_9 = new QLabel(frame_2);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        horizontalLayout_5->addWidget(label_9);

        dimensionCombo = new QComboBox(frame_2);
        dimensionCombo->setObjectName(QString::fromUtf8("dimensionCombo"));
        dimensionCombo->setMinimumSize(QSize(50, 0));
        dimensionCombo->setMaximumSize(QSize(40, 16777215));
        dimensionCombo->setFont(font);

        horizontalLayout_5->addWidget(dimensionCombo);

        horizontalSpacer_17 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_17);

        label_10 = new QLabel(frame_2);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setAlignment(Qt::AlignCenter);

        horizontalLayout_5->addWidget(label_10);

        variableCombo = new QComboBox(frame_2);
        variableCombo->setObjectName(QString::fromUtf8("variableCombo"));
        QSizePolicy sizePolicy1(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(variableCombo->sizePolicy().hasHeightForWidth());
        variableCombo->setSizePolicy(sizePolicy1);
        variableCombo->setMinimumSize(QSize(100, 0));
        QFont font1;
        font1.setFamily(QString::fromUtf8("MS Shell Dlg 2"));
        font1.setPointSize(10);
        variableCombo->setFont(font1);
        variableCombo->setFocusPolicy(Qt::ClickFocus);

        horizontalLayout_5->addWidget(variableCombo);

        horizontalSpacer_18 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_18);


        verticalLayout_9->addLayout(horizontalLayout_5);

        horizontalLayout_15 = new QHBoxLayout();
        horizontalLayout_15->setSpacing(6);
        horizontalLayout_15->setObjectName(QString::fromUtf8("horizontalLayout_15"));
        horizontalSpacer_19 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_15->addItem(horizontalSpacer_19);

        label_11 = new QLabel(frame_2);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(label_11);

        minIsoEdit = new QLineEdit(frame_2);
        minIsoEdit->setObjectName(QString::fromUtf8("minIsoEdit"));
        minIsoEdit->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(minIsoEdit);

        label_12 = new QLabel(frame_2);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(label_12);

        maxIsoEdit = new QLineEdit(frame_2);
        maxIsoEdit->setObjectName(QString::fromUtf8("maxIsoEdit"));
        maxIsoEdit->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(maxIsoEdit);

        label_13 = new QLabel(frame_2);
        label_13->setObjectName(QString::fromUtf8("label_13"));
        label_13->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(label_13);

        countIsoEdit = new QLineEdit(frame_2);
        countIsoEdit->setObjectName(QString::fromUtf8("countIsoEdit"));
        countIsoEdit->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(countIsoEdit);


        verticalLayout_9->addLayout(horizontalLayout_15);

        _13 = new QHBoxLayout();
        _13->setSpacing(6);
        _13->setObjectName(QString::fromUtf8("_13"));
        editButton = new QPushButton(frame_2);
        editButton->setObjectName(QString::fromUtf8("editButton"));
        editButton->setCheckable(true);
        editButton->setChecked(true);

        _13->addWidget(editButton);

        navigateButton = new QPushButton(frame_2);
        navigateButton->setObjectName(QString::fromUtf8("navigateButton"));
        navigateButton->setCheckable(true);

        _13->addWidget(navigateButton);

        alignButton = new QPushButton(frame_2);
        alignButton->setObjectName(QString::fromUtf8("alignButton"));

        _13->addWidget(alignButton);

        newHistoButton = new QPushButton(frame_2);
        newHistoButton->setObjectName(QString::fromUtf8("newHistoButton"));

        _13->addWidget(newHistoButton);


        verticalLayout_9->addLayout(_13);

        isoSelectFrame = new QFrame(frame_2);
        isoSelectFrame->setObjectName(QString::fromUtf8("isoSelectFrame"));
        isoSelectFrame->setMinimumSize(QSize(0, 190));
        isoSelectFrame->setFrameShape(QFrame::NoFrame);
        isoSelectFrame->setFrameShadow(QFrame::Raised);
        _14 = new QVBoxLayout(isoSelectFrame);
        _14->setSpacing(0);
        _14->setContentsMargins(0, 0, 0, 0);
        _14->setObjectName(QString::fromUtf8("_14"));
        isoSelectionFrame = new MappingFrame(isoSelectFrame);
        isoSelectionFrame->setObjectName(QString::fromUtf8("isoSelectionFrame"));
        isoSelectionFrame->setMinimumSize(QSize(300, 120));
        isoSelectionFrame->setMaximumSize(QSize(16777215, 170));
        isoSelectionFrame->setProperty("colorMapping", QVariant(true));
        isoSelectionFrame->setProperty("opacityMapping", QVariant(true));

        _14->addWidget(isoSelectionFrame);


        verticalLayout_9->addWidget(isoSelectFrame);

        _15 = new QHBoxLayout();
        _15->setSpacing(6);
        _15->setObjectName(QString::fromUtf8("_15"));
        leftHistoEdit = new QLineEdit(frame_2);
        leftHistoEdit->setObjectName(QString::fromUtf8("leftHistoEdit"));

        _15->addWidget(leftHistoEdit);

        spacer34 = new QSpacerItem(48, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _15->addItem(spacer34);

        textLabel2_4 = new QLabel(frame_2);
        textLabel2_4->setObjectName(QString::fromUtf8("textLabel2_4"));
        textLabel2_4->setAlignment(Qt::AlignCenter);
        textLabel2_4->setWordWrap(false);

        _15->addWidget(textLabel2_4);

        spacer35 = new QSpacerItem(67, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _15->addItem(spacer35);

        rightHistoEdit = new QLineEdit(frame_2);
        rightHistoEdit->setObjectName(QString::fromUtf8("rightHistoEdit"));
        rightHistoEdit->setAlignment(Qt::AlignRight);

        _15->addWidget(rightHistoEdit);


        verticalLayout_9->addLayout(_15);

        _16 = new QHBoxLayout();
        _16->setSpacing(6);
        _16->setObjectName(QString::fromUtf8("_16"));
        minDataBound = new QLabel(frame_2);
        minDataBound->setObjectName(QString::fromUtf8("minDataBound"));
        QFont font2;
        minDataBound->setFont(font2);
        minDataBound->setWordWrap(false);

        _16->addWidget(minDataBound);

        textLabel2_5 = new QLabel(frame_2);
        textLabel2_5->setObjectName(QString::fromUtf8("textLabel2_5"));
        textLabel2_5->setAlignment(Qt::AlignCenter);
        textLabel2_5->setWordWrap(false);

        _16->addWidget(textLabel2_5);

        maxDataBound = new QLabel(frame_2);
        maxDataBound->setObjectName(QString::fromUtf8("maxDataBound"));
        maxDataBound->setFont(font2);
        maxDataBound->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        maxDataBound->setWordWrap(false);

        _16->addWidget(maxDataBound);


        verticalLayout_9->addLayout(_16);

        _19 = new QHBoxLayout();
        _19->setSpacing(6);
        _19->setObjectName(QString::fromUtf8("_19"));
        horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _19->addItem(horizontalSpacer_8);

        fitDataButton = new QPushButton(frame_2);
        fitDataButton->setObjectName(QString::fromUtf8("fitDataButton"));

        _19->addWidget(fitDataButton);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _19->addItem(horizontalSpacer_2);

        editIsovaluesButton = new QPushButton(frame_2);
        editIsovaluesButton->setObjectName(QString::fromUtf8("editIsovaluesButton"));
        editIsovaluesButton->setMinimumSize(QSize(100, 0));

        _19->addWidget(editIsovaluesButton);

        horizontalSpacer_20 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _19->addItem(horizontalSpacer_20);

        fitIsovalsButton = new QPushButton(frame_2);
        fitIsovalsButton->setObjectName(QString::fromUtf8("fitIsovalsButton"));

        _19->addWidget(fitIsovalsButton);

        spacer29_4 = new QSpacerItem(31, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _19->addItem(spacer29_4);


        verticalLayout_9->addLayout(_19);

        _17 = new QHBoxLayout();
        _17->setSpacing(6);
        _17->setObjectName(QString::fromUtf8("_17"));
        spacer45 = new QSpacerItem(16, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _17->addItem(spacer45);

        loadButton = new QPushButton(frame_2);
        loadButton->setObjectName(QString::fromUtf8("loadButton"));
        QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(loadButton->sizePolicy().hasHeightForWidth());
        loadButton->setSizePolicy(sizePolicy2);
        loadButton->setMaximumSize(QSize(200, 32767));

        _17->addWidget(loadButton);

        spacer160 = new QSpacerItem(31, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _17->addItem(spacer160);

        loadInstalledButton = new QPushButton(frame_2);
        loadInstalledButton->setObjectName(QString::fromUtf8("loadInstalledButton"));
        loadInstalledButton->setMinimumSize(QSize(150, 0));

        _17->addWidget(loadInstalledButton);

        spacer30 = new QSpacerItem(16, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _17->addItem(spacer30);

        saveButton = new QPushButton(frame_2);
        saveButton->setObjectName(QString::fromUtf8("saveButton"));
        saveButton->setMaximumSize(QSize(200, 32767));

        _17->addWidget(saveButton);

        spacer46 = new QSpacerItem(23, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _17->addItem(spacer46);


        verticalLayout_9->addLayout(_17);

        _20 = new QHBoxLayout();
        _20->setSpacing(6);
        _20->setObjectName(QString::fromUtf8("_20"));
        horizontalSpacer_21 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _20->addItem(horizontalSpacer_21);

        copyToProbeButton = new QPushButton(frame_2);
        copyToProbeButton->setObjectName(QString::fromUtf8("copyToProbeButton"));
        copyToProbeButton->setMinimumSize(QSize(98, 0));

        _20->addWidget(copyToProbeButton);

        spacer28_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _20->addItem(spacer28_5);

        textLabel1_17 = new QLabel(frame_2);
        textLabel1_17->setObjectName(QString::fromUtf8("textLabel1_17"));
        textLabel1_17->setWordWrap(false);

        _20->addWidget(textLabel1_17);

        histoScaleEdit = new QLineEdit(frame_2);
        histoScaleEdit->setObjectName(QString::fromUtf8("histoScaleEdit"));
        histoScaleEdit->setMaximumSize(QSize(50, 32767));
        histoScaleEdit->setAlignment(Qt::AlignHCenter);

        _20->addWidget(histoScaleEdit);

        spacer29_5 = new QSpacerItem(31, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _20->addItem(spacer29_5);


        verticalLayout_9->addLayout(_20);


        verticalLayout_2->addLayout(verticalLayout_9);


        horizontalLayout_4->addLayout(verticalLayout_2);


        verticalLayout_7->addWidget(frame_2);

        verticalSpacer_3 = new QSpacerItem(20, 13, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_7->addItem(verticalSpacer_3);

        frame = new QFrame(IsolineTab);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setMaximumSize(QSize(422, 16777215));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Plain);
        frame->setLineWidth(0);
        horizontalLayout_8 = new QHBoxLayout(frame);
        horizontalLayout_8->setSpacing(6);
        horizontalLayout_8->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setSpacing(6);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        showHideLayoutButton = new QPushButton(frame);
        showHideLayoutButton->setObjectName(QString::fromUtf8("showHideLayoutButton"));

        verticalLayout_6->addWidget(showHideLayoutButton);

        layoutFrame = new QFrame(frame);
        layoutFrame->setObjectName(QString::fromUtf8("layoutFrame"));
        layoutFrame->setFrameShape(QFrame::NoFrame);
        layoutFrame->setFrameShadow(QFrame::Plain);
        layoutFrame->setLineWidth(0);
        horizontalLayout_7 = new QHBoxLayout(layoutFrame);
        horizontalLayout_7->setSpacing(6);
        horizontalLayout_7->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setSpacing(6);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setSpacing(6);
        hboxLayout3->setObjectName(QString::fromUtf8("hboxLayout3"));
        vboxLayout2 = new QVBoxLayout();
        vboxLayout2->setSpacing(6);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        textLabel2_2_2 = new QLabel(layoutFrame);
        textLabel2_2_2->setObjectName(QString::fromUtf8("textLabel2_2_2"));
        QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(textLabel2_2_2->sizePolicy().hasHeightForWidth());
        textLabel2_2_2->setSizePolicy(sizePolicy3);
        textLabel2_2_2->setAlignment(Qt::AlignCenter);
        textLabel2_2_2->setWordWrap(true);

        vboxLayout2->addWidget(textLabel2_2_2);

        hboxLayout4 = new QHBoxLayout();
        hboxLayout4->setSpacing(6);
        hboxLayout4->setObjectName(QString::fromUtf8("hboxLayout4"));
        textLabel3_12_2 = new QLabel(layoutFrame);
        textLabel3_12_2->setObjectName(QString::fromUtf8("textLabel3_12_2"));
        QSizePolicy sizePolicy4(QSizePolicy::Minimum, QSizePolicy::Expanding);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(textLabel3_12_2->sizePolicy().hasHeightForWidth());
        textLabel3_12_2->setSizePolicy(sizePolicy4);
        textLabel3_12_2->setWordWrap(false);

        hboxLayout4->addWidget(textLabel3_12_2);

        xCenterSlider = new QSlider(layoutFrame);
        xCenterSlider->setObjectName(QString::fromUtf8("xCenterSlider"));
        QSizePolicy sizePolicy5(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(xCenterSlider->sizePolicy().hasHeightForWidth());
        xCenterSlider->setSizePolicy(sizePolicy5);
        xCenterSlider->setMinimumSize(QSize(80, 0));
        xCenterSlider->setMinimum(0);
        xCenterSlider->setMaximum(256);
        xCenterSlider->setPageStep(1);
        xCenterSlider->setValue(128);
        xCenterSlider->setOrientation(Qt::Horizontal);

        hboxLayout4->addWidget(xCenterSlider);

        xCenterEdit = new QLineEdit(layoutFrame);
        xCenterEdit->setObjectName(QString::fromUtf8("xCenterEdit"));
        QSizePolicy sizePolicy6(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(0);
        sizePolicy6.setHeightForWidth(xCenterEdit->sizePolicy().hasHeightForWidth());
        xCenterEdit->setSizePolicy(sizePolicy6);
        xCenterEdit->setMaximumSize(QSize(100, 25));
        xCenterEdit->setMaxLength(200);
        xCenterEdit->setAlignment(Qt::AlignHCenter);

        hboxLayout4->addWidget(xCenterEdit);


        vboxLayout2->addLayout(hboxLayout4);

        hboxLayout5 = new QHBoxLayout();
        hboxLayout5->setSpacing(6);
        hboxLayout5->setObjectName(QString::fromUtf8("hboxLayout5"));
        textLabel3_12_3_3 = new QLabel(layoutFrame);
        textLabel3_12_3_3->setObjectName(QString::fromUtf8("textLabel3_12_3_3"));
        sizePolicy4.setHeightForWidth(textLabel3_12_3_3->sizePolicy().hasHeightForWidth());
        textLabel3_12_3_3->setSizePolicy(sizePolicy4);
        textLabel3_12_3_3->setWordWrap(false);

        hboxLayout5->addWidget(textLabel3_12_3_3);

        yCenterSlider = new QSlider(layoutFrame);
        yCenterSlider->setObjectName(QString::fromUtf8("yCenterSlider"));
        sizePolicy5.setHeightForWidth(yCenterSlider->sizePolicy().hasHeightForWidth());
        yCenterSlider->setSizePolicy(sizePolicy5);
        yCenterSlider->setMinimumSize(QSize(80, 0));
        yCenterSlider->setMinimum(0);
        yCenterSlider->setMaximum(256);
        yCenterSlider->setPageStep(1);
        yCenterSlider->setValue(100);
        yCenterSlider->setOrientation(Qt::Horizontal);

        hboxLayout5->addWidget(yCenterSlider);

        yCenterEdit = new QLineEdit(layoutFrame);
        yCenterEdit->setObjectName(QString::fromUtf8("yCenterEdit"));
        sizePolicy6.setHeightForWidth(yCenterEdit->sizePolicy().hasHeightForWidth());
        yCenterEdit->setSizePolicy(sizePolicy6);
        yCenterEdit->setMaximumSize(QSize(100, 25));
        yCenterEdit->setMaxLength(200);
        yCenterEdit->setAlignment(Qt::AlignHCenter);

        hboxLayout5->addWidget(yCenterEdit);


        vboxLayout2->addLayout(hboxLayout5);

        hboxLayout6 = new QHBoxLayout();
        hboxLayout6->setSpacing(6);
        hboxLayout6->setObjectName(QString::fromUtf8("hboxLayout6"));
        textLabel3_12_3_2_2 = new QLabel(layoutFrame);
        textLabel3_12_3_2_2->setObjectName(QString::fromUtf8("textLabel3_12_3_2_2"));
        sizePolicy4.setHeightForWidth(textLabel3_12_3_2_2->sizePolicy().hasHeightForWidth());
        textLabel3_12_3_2_2->setSizePolicy(sizePolicy4);
        textLabel3_12_3_2_2->setWordWrap(false);

        hboxLayout6->addWidget(textLabel3_12_3_2_2);

        zCenterSlider = new QSlider(layoutFrame);
        zCenterSlider->setObjectName(QString::fromUtf8("zCenterSlider"));
        sizePolicy5.setHeightForWidth(zCenterSlider->sizePolicy().hasHeightForWidth());
        zCenterSlider->setSizePolicy(sizePolicy5);
        zCenterSlider->setMinimumSize(QSize(80, 0));
        zCenterSlider->setMinimum(0);
        zCenterSlider->setMaximum(256);
        zCenterSlider->setPageStep(1);
        zCenterSlider->setValue(100);
        zCenterSlider->setOrientation(Qt::Horizontal);

        hboxLayout6->addWidget(zCenterSlider);

        zCenterEdit = new QLineEdit(layoutFrame);
        zCenterEdit->setObjectName(QString::fromUtf8("zCenterEdit"));
        sizePolicy6.setHeightForWidth(zCenterEdit->sizePolicy().hasHeightForWidth());
        zCenterEdit->setSizePolicy(sizePolicy6);
        zCenterEdit->setMaximumSize(QSize(100, 25));
        zCenterEdit->setMaxLength(200);
        zCenterEdit->setAlignment(Qt::AlignHCenter);

        hboxLayout6->addWidget(zCenterEdit);


        vboxLayout2->addLayout(hboxLayout6);


        hboxLayout3->addLayout(vboxLayout2);

        vboxLayout3 = new QVBoxLayout();
        vboxLayout3->setSpacing(6);
        vboxLayout3->setObjectName(QString::fromUtf8("vboxLayout3"));
        textLabel2_2 = new QLabel(layoutFrame);
        textLabel2_2->setObjectName(QString::fromUtf8("textLabel2_2"));
        sizePolicy3.setHeightForWidth(textLabel2_2->sizePolicy().hasHeightForWidth());
        textLabel2_2->setSizePolicy(sizePolicy3);
        textLabel2_2->setAlignment(Qt::AlignCenter);
        textLabel2_2->setWordWrap(true);

        vboxLayout3->addWidget(textLabel2_2);

        hboxLayout7 = new QHBoxLayout();
        hboxLayout7->setSpacing(6);
        hboxLayout7->setObjectName(QString::fromUtf8("hboxLayout7"));
        textLabel3_12 = new QLabel(layoutFrame);
        textLabel3_12->setObjectName(QString::fromUtf8("textLabel3_12"));
        sizePolicy4.setHeightForWidth(textLabel3_12->sizePolicy().hasHeightForWidth());
        textLabel3_12->setSizePolicy(sizePolicy4);
        textLabel3_12->setWordWrap(false);

        hboxLayout7->addWidget(textLabel3_12);

        xSizeSlider = new QSlider(layoutFrame);
        xSizeSlider->setObjectName(QString::fromUtf8("xSizeSlider"));
        sizePolicy5.setHeightForWidth(xSizeSlider->sizePolicy().hasHeightForWidth());
        xSizeSlider->setSizePolicy(sizePolicy5);
        xSizeSlider->setMinimumSize(QSize(80, 0));
        xSizeSlider->setMinimum(0);
        xSizeSlider->setMaximum(256);
        xSizeSlider->setPageStep(1);
        xSizeSlider->setValue(128);
        xSizeSlider->setOrientation(Qt::Horizontal);

        hboxLayout7->addWidget(xSizeSlider);

        xSizeEdit = new QLineEdit(layoutFrame);
        xSizeEdit->setObjectName(QString::fromUtf8("xSizeEdit"));
        sizePolicy6.setHeightForWidth(xSizeEdit->sizePolicy().hasHeightForWidth());
        xSizeEdit->setSizePolicy(sizePolicy6);
        xSizeEdit->setMaximumSize(QSize(100, 25));
        xSizeEdit->setMaxLength(200);
        xSizeEdit->setAlignment(Qt::AlignHCenter);

        hboxLayout7->addWidget(xSizeEdit);


        vboxLayout3->addLayout(hboxLayout7);

        hboxLayout8 = new QHBoxLayout();
        hboxLayout8->setSpacing(6);
        hboxLayout8->setObjectName(QString::fromUtf8("hboxLayout8"));
        textLabel3_12_3 = new QLabel(layoutFrame);
        textLabel3_12_3->setObjectName(QString::fromUtf8("textLabel3_12_3"));
        sizePolicy4.setHeightForWidth(textLabel3_12_3->sizePolicy().hasHeightForWidth());
        textLabel3_12_3->setSizePolicy(sizePolicy4);
        textLabel3_12_3->setWordWrap(false);

        hboxLayout8->addWidget(textLabel3_12_3);

        ySizeSlider = new QSlider(layoutFrame);
        ySizeSlider->setObjectName(QString::fromUtf8("ySizeSlider"));
        sizePolicy5.setHeightForWidth(ySizeSlider->sizePolicy().hasHeightForWidth());
        ySizeSlider->setSizePolicy(sizePolicy5);
        ySizeSlider->setMinimumSize(QSize(80, 0));
        ySizeSlider->setMinimum(0);
        ySizeSlider->setMaximum(256);
        ySizeSlider->setPageStep(1);
        ySizeSlider->setValue(100);
        ySizeSlider->setOrientation(Qt::Horizontal);

        hboxLayout8->addWidget(ySizeSlider);

        ySizeEdit = new QLineEdit(layoutFrame);
        ySizeEdit->setObjectName(QString::fromUtf8("ySizeEdit"));
        sizePolicy6.setHeightForWidth(ySizeEdit->sizePolicy().hasHeightForWidth());
        ySizeEdit->setSizePolicy(sizePolicy6);
        ySizeEdit->setMaximumSize(QSize(100, 25));
        ySizeEdit->setMaxLength(200);
        ySizeEdit->setAlignment(Qt::AlignHCenter);

        hboxLayout8->addWidget(ySizeEdit);


        vboxLayout3->addLayout(hboxLayout8);

        hboxLayout9 = new QHBoxLayout();
        hboxLayout9->setSpacing(6);
        hboxLayout9->setObjectName(QString::fromUtf8("hboxLayout9"));

        vboxLayout3->addLayout(hboxLayout9);


        hboxLayout3->addLayout(vboxLayout3);


        vboxLayout1->addLayout(hboxLayout3);

        spacer54_2 = new QSpacerItem(20, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacer54_2);

        hboxLayout10 = new QHBoxLayout();
        hboxLayout10->setSpacing(6);
        hboxLayout10->setObjectName(QString::fromUtf8("hboxLayout10"));
        spacer48_2 = new QSpacerItem(51, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout10->addItem(spacer48_2);

        textLabel1_11 = new QLabel(layoutFrame);
        textLabel1_11->setObjectName(QString::fromUtf8("textLabel1_11"));
        textLabel1_11->setAlignment(Qt::AlignCenter);
        textLabel1_11->setWordWrap(false);

        hboxLayout10->addWidget(textLabel1_11);

        spacer49_2 = new QSpacerItem(91, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout10->addItem(spacer49_2);

        textLabel1_11_4 = new QLabel(layoutFrame);
        textLabel1_11_4->setObjectName(QString::fromUtf8("textLabel1_11_4"));
        textLabel1_11_4->setAlignment(Qt::AlignCenter);
        textLabel1_11_4->setWordWrap(false);

        hboxLayout10->addWidget(textLabel1_11_4);


        vboxLayout1->addLayout(hboxLayout10);

        hboxLayout11 = new QHBoxLayout();
        hboxLayout11->setSpacing(6);
        hboxLayout11->setObjectName(QString::fromUtf8("hboxLayout11"));
        textLabel9 = new QLabel(layoutFrame);
        textLabel9->setObjectName(QString::fromUtf8("textLabel9"));
        textLabel9->setAlignment(Qt::AlignCenter);
        textLabel9->setWordWrap(false);

        hboxLayout11->addWidget(textLabel9);

        minUserXLabel = new QLabel(layoutFrame);
        minUserXLabel->setObjectName(QString::fromUtf8("minUserXLabel"));
        minUserXLabel->setMinimumSize(QSize(70, 0));
        minUserXLabel->setAlignment(Qt::AlignCenter);
        minUserXLabel->setWordWrap(false);

        hboxLayout11->addWidget(minUserXLabel);

        minUserYLabel = new QLabel(layoutFrame);
        minUserYLabel->setObjectName(QString::fromUtf8("minUserYLabel"));
        minUserYLabel->setMinimumSize(QSize(70, 0));
        minUserYLabel->setAlignment(Qt::AlignCenter);
        minUserYLabel->setWordWrap(false);

        hboxLayout11->addWidget(minUserYLabel);

        minUserZLabel = new QLabel(layoutFrame);
        minUserZLabel->setObjectName(QString::fromUtf8("minUserZLabel"));
        minUserZLabel->setMinimumSize(QSize(70, 0));
        minUserZLabel->setAlignment(Qt::AlignCenter);
        minUserZLabel->setWordWrap(false);

        hboxLayout11->addWidget(minUserZLabel);

        spacer53_3 = new QSpacerItem(3, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);

        hboxLayout11->addItem(spacer53_3);

        minGridXLabel = new QLabel(layoutFrame);
        minGridXLabel->setObjectName(QString::fromUtf8("minGridXLabel"));
        minGridXLabel->setAlignment(Qt::AlignCenter);
        minGridXLabel->setWordWrap(false);

        hboxLayout11->addWidget(minGridXLabel);

        minGridYLabel = new QLabel(layoutFrame);
        minGridYLabel->setObjectName(QString::fromUtf8("minGridYLabel"));
        minGridYLabel->setAlignment(Qt::AlignCenter);
        minGridYLabel->setWordWrap(false);

        hboxLayout11->addWidget(minGridYLabel);

        minGridZLabel = new QLabel(layoutFrame);
        minGridZLabel->setObjectName(QString::fromUtf8("minGridZLabel"));
        minGridZLabel->setAlignment(Qt::AlignCenter);
        minGridZLabel->setWordWrap(false);

        hboxLayout11->addWidget(minGridZLabel);


        vboxLayout1->addLayout(hboxLayout11);

        hboxLayout12 = new QHBoxLayout();
        hboxLayout12->setSpacing(6);
        hboxLayout12->setObjectName(QString::fromUtf8("hboxLayout12"));
        textLabel9_2 = new QLabel(layoutFrame);
        textLabel9_2->setObjectName(QString::fromUtf8("textLabel9_2"));
        textLabel9_2->setAlignment(Qt::AlignCenter);
        textLabel9_2->setWordWrap(false);

        hboxLayout12->addWidget(textLabel9_2);

        maxUserXLabel = new QLabel(layoutFrame);
        maxUserXLabel->setObjectName(QString::fromUtf8("maxUserXLabel"));
        maxUserXLabel->setMinimumSize(QSize(70, 0));
        maxUserXLabel->setAlignment(Qt::AlignCenter);
        maxUserXLabel->setWordWrap(false);

        hboxLayout12->addWidget(maxUserXLabel);

        maxUserYLabel = new QLabel(layoutFrame);
        maxUserYLabel->setObjectName(QString::fromUtf8("maxUserYLabel"));
        maxUserYLabel->setMinimumSize(QSize(70, 0));
        maxUserYLabel->setAlignment(Qt::AlignCenter);
        maxUserYLabel->setWordWrap(false);

        hboxLayout12->addWidget(maxUserYLabel);

        maxUserZLabel = new QLabel(layoutFrame);
        maxUserZLabel->setObjectName(QString::fromUtf8("maxUserZLabel"));
        maxUserZLabel->setMinimumSize(QSize(70, 0));
        maxUserZLabel->setAlignment(Qt::AlignCenter);
        maxUserZLabel->setWordWrap(false);

        hboxLayout12->addWidget(maxUserZLabel);

        spacer52_2 = new QSpacerItem(3, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);

        hboxLayout12->addItem(spacer52_2);

        maxGridXLabel = new QLabel(layoutFrame);
        maxGridXLabel->setObjectName(QString::fromUtf8("maxGridXLabel"));
        maxGridXLabel->setAlignment(Qt::AlignCenter);
        maxGridXLabel->setWordWrap(false);

        hboxLayout12->addWidget(maxGridXLabel);

        maxGridYLabel = new QLabel(layoutFrame);
        maxGridYLabel->setObjectName(QString::fromUtf8("maxGridYLabel"));
        maxGridYLabel->setAlignment(Qt::AlignCenter);
        maxGridYLabel->setWordWrap(false);

        hboxLayout12->addWidget(maxGridYLabel);

        maxGridZLabel = new QLabel(layoutFrame);
        maxGridZLabel->setObjectName(QString::fromUtf8("maxGridZLabel"));
        maxGridZLabel->setAlignment(Qt::AlignCenter);
        maxGridZLabel->setWordWrap(false);

        hboxLayout12->addWidget(maxGridZLabel);


        vboxLayout1->addLayout(hboxLayout12);

        minMaxLonLatFrame = new QFrame(layoutFrame);
        minMaxLonLatFrame->setObjectName(QString::fromUtf8("minMaxLonLatFrame"));
        minMaxLonLatFrame->setFrameShape(QFrame::NoFrame);
        minMaxLonLatFrame->setFrameShadow(QFrame::Plain);
        hboxLayout13 = new QHBoxLayout(minMaxLonLatFrame);
        hboxLayout13->setSpacing(6);
        hboxLayout13->setContentsMargins(11, 11, 11, 11);
        hboxLayout13->setObjectName(QString::fromUtf8("hboxLayout13"));
        vboxLayout4 = new QVBoxLayout();
        vboxLayout4->setSpacing(6);
        vboxLayout4->setObjectName(QString::fromUtf8("vboxLayout4"));
        hboxLayout14 = new QHBoxLayout();
        hboxLayout14->setSpacing(6);
        hboxLayout14->setObjectName(QString::fromUtf8("hboxLayout14"));
        textLabel9_2_2 = new QLabel(minMaxLonLatFrame);
        textLabel9_2_2->setObjectName(QString::fromUtf8("textLabel9_2_2"));
        textLabel9_2_2->setMinimumSize(QSize(0, 15));
        textLabel9_2_2->setAlignment(Qt::AlignCenter);
        textLabel9_2_2->setWordWrap(false);

        hboxLayout14->addWidget(textLabel9_2_2);

        maxLonLabel = new QLabel(minMaxLonLatFrame);
        maxLonLabel->setObjectName(QString::fromUtf8("maxLonLabel"));
        maxLonLabel->setMinimumSize(QSize(70, 0));
        maxLonLabel->setAlignment(Qt::AlignCenter);
        maxLonLabel->setWordWrap(false);

        hboxLayout14->addWidget(maxLonLabel);

        maxLatLabel = new QLabel(minMaxLonLatFrame);
        maxLatLabel->setObjectName(QString::fromUtf8("maxLatLabel"));
        maxLatLabel->setMinimumSize(QSize(70, 0));
        maxLatLabel->setAlignment(Qt::AlignCenter);
        maxLatLabel->setWordWrap(false);

        hboxLayout14->addWidget(maxLatLabel);


        vboxLayout4->addLayout(hboxLayout14);

        hboxLayout15 = new QHBoxLayout();
        hboxLayout15->setSpacing(6);
        hboxLayout15->setObjectName(QString::fromUtf8("hboxLayout15"));
        textLabel9_2_2_2 = new QLabel(minMaxLonLatFrame);
        textLabel9_2_2_2->setObjectName(QString::fromUtf8("textLabel9_2_2_2"));
        textLabel9_2_2_2->setMinimumSize(QSize(0, 15));
        textLabel9_2_2_2->setAlignment(Qt::AlignCenter);
        textLabel9_2_2_2->setWordWrap(false);

        hboxLayout15->addWidget(textLabel9_2_2_2);

        minLonLabel = new QLabel(minMaxLonLatFrame);
        minLonLabel->setObjectName(QString::fromUtf8("minLonLabel"));
        minLonLabel->setMinimumSize(QSize(70, 0));
        minLonLabel->setAlignment(Qt::AlignCenter);
        minLonLabel->setWordWrap(false);

        hboxLayout15->addWidget(minLonLabel);

        minLatLabel = new QLabel(minMaxLonLatFrame);
        minLatLabel->setObjectName(QString::fromUtf8("minLatLabel"));
        minLatLabel->setMinimumSize(QSize(70, 0));
        minLatLabel->setAlignment(Qt::AlignCenter);
        minLatLabel->setWordWrap(false);

        hboxLayout15->addWidget(minLatLabel);


        vboxLayout4->addLayout(hboxLayout15);


        hboxLayout13->addLayout(vboxLayout4);


        vboxLayout1->addWidget(minMaxLonLatFrame);

        spacer51 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacer51);

        orientationFrame = new QFrame(layoutFrame);
        orientationFrame->setObjectName(QString::fromUtf8("orientationFrame"));
        orientationFrame->setFrameShape(QFrame::StyledPanel);
        orientationFrame->setFrameShadow(QFrame::Raised);
        verticalLayout_4 = new QVBoxLayout(orientationFrame);
        verticalLayout_4->setSpacing(6);
        verticalLayout_4->setContentsMargins(11, 11, 11, 11);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        orientationLayout_2 = new QHBoxLayout();
        orientationLayout_2->setSpacing(6);
        orientationLayout_2->setObjectName(QString::fromUtf8("orientationLayout_2"));
        _2 = new QVBoxLayout();
        _2->setSpacing(6);
        _2->setObjectName(QString::fromUtf8("_2"));
        textLabel3_3 = new QLabel(orientationFrame);
        textLabel3_3->setObjectName(QString::fromUtf8("textLabel3_3"));
        textLabel3_3->setAlignment(Qt::AlignCenter);
        textLabel3_3->setWordWrap(false);

        _2->addWidget(textLabel3_3);

        _3 = new QGridLayout();
        _3->setSpacing(6);
        _3->setObjectName(QString::fromUtf8("_3"));
        thetaEdit = new QLineEdit(orientationFrame);
        thetaEdit->setObjectName(QString::fromUtf8("thetaEdit"));
        thetaEdit->setMaximumSize(QSize(50, 32767));
        thetaEdit->setAlignment(Qt::AlignHCenter);

        _3->addWidget(thetaEdit, 1, 0, 1, 1);

        psiEdit = new QLineEdit(orientationFrame);
        psiEdit->setObjectName(QString::fromUtf8("psiEdit"));
        psiEdit->setMaximumSize(QSize(50, 32767));
        psiEdit->setAlignment(Qt::AlignHCenter);

        _3->addWidget(psiEdit, 1, 2, 1, 1);

        textLabel12_3 = new QLabel(orientationFrame);
        textLabel12_3->setObjectName(QString::fromUtf8("textLabel12_3"));
        textLabel12_3->setAlignment(Qt::AlignCenter);
        textLabel12_3->setWordWrap(true);

        _3->addWidget(textLabel12_3, 0, 0, 1, 1);

        phiEdit = new QLineEdit(orientationFrame);
        phiEdit->setObjectName(QString::fromUtf8("phiEdit"));
        phiEdit->setMaximumSize(QSize(50, 32767));
        phiEdit->setAlignment(Qt::AlignHCenter);

        _3->addWidget(phiEdit, 1, 1, 1, 1);

        textLabel1_7 = new QLabel(orientationFrame);
        textLabel1_7->setObjectName(QString::fromUtf8("textLabel1_7"));
        textLabel1_7->setAlignment(Qt::AlignCenter);
        textLabel1_7->setWordWrap(false);

        _3->addWidget(textLabel1_7, 0, 2, 1, 1);

        textLabel12_4 = new QLabel(orientationFrame);
        textLabel12_4->setObjectName(QString::fromUtf8("textLabel12_4"));
        textLabel12_4->setAlignment(Qt::AlignCenter);
        textLabel12_4->setWordWrap(true);

        _3->addWidget(textLabel12_4, 0, 1, 1, 1);


        _2->addLayout(_3);

        _4 = new QHBoxLayout();
        _4->setSpacing(6);
        _4->setObjectName(QString::fromUtf8("_4"));
        axisAlignCombo = new QComboBox(orientationFrame);
        axisAlignCombo->setObjectName(QString::fromUtf8("axisAlignCombo"));
        axisAlignCombo->setMaximumSize(QSize(100, 32767));
        QFont font3;
        font3.setPointSize(9);
        axisAlignCombo->setFont(font3);

        _4->addWidget(axisAlignCombo);

        rotate90Combo = new QComboBox(orientationFrame);
        rotate90Combo->setObjectName(QString::fromUtf8("rotate90Combo"));
        rotate90Combo->setMaximumSize(QSize(100, 32767));
        rotate90Combo->setFont(font3);

        _4->addWidget(rotate90Combo);


        _2->addLayout(_4);


        orientationLayout_2->addLayout(_2);

        _9 = new QVBoxLayout();
        _9->setSpacing(6);
        _9->setObjectName(QString::fromUtf8("_9"));
        textLabel2_3 = new QLabel(orientationFrame);
        textLabel2_3->setObjectName(QString::fromUtf8("textLabel2_3"));
        textLabel2_3->setAlignment(Qt::AlignCenter);
        textLabel2_3->setWordWrap(false);

        _9->addWidget(textLabel2_3);

        _10 = new QHBoxLayout();
        _10->setSpacing(6);
        _10->setObjectName(QString::fromUtf8("_10"));
        textLabel1_8 = new QLabel(orientationFrame);
        textLabel1_8->setObjectName(QString::fromUtf8("textLabel1_8"));
        textLabel1_8->setMinimumSize(QSize(20, 0));
        textLabel1_8->setAlignment(Qt::AlignCenter);
        textLabel1_8->setWordWrap(false);

        _10->addWidget(textLabel1_8);

        xThumbWheel = new QtThumbWheel(orientationFrame);
        xThumbWheel->setObjectName(QString::fromUtf8("xThumbWheel"));
        xThumbWheel->setMinimumSize(QSize(140, 20));

        _10->addWidget(xThumbWheel);


        _9->addLayout(_10);

        _11 = new QHBoxLayout();
        _11->setSpacing(6);
        _11->setObjectName(QString::fromUtf8("_11"));
        textLabel1_5_3 = new QLabel(orientationFrame);
        textLabel1_5_3->setObjectName(QString::fromUtf8("textLabel1_5_3"));
        textLabel1_5_3->setMinimumSize(QSize(20, 0));
        textLabel1_5_3->setAlignment(Qt::AlignCenter);
        textLabel1_5_3->setWordWrap(false);

        _11->addWidget(textLabel1_5_3);

        yThumbWheel = new QtThumbWheel(orientationFrame);
        yThumbWheel->setObjectName(QString::fromUtf8("yThumbWheel"));
        yThumbWheel->setMinimumSize(QSize(140, 20));

        _11->addWidget(yThumbWheel);


        _9->addLayout(_11);

        _12 = new QHBoxLayout();
        _12->setSpacing(6);
        _12->setObjectName(QString::fromUtf8("_12"));
        textLabel1_5_2_3 = new QLabel(orientationFrame);
        textLabel1_5_2_3->setObjectName(QString::fromUtf8("textLabel1_5_2_3"));
        textLabel1_5_2_3->setMinimumSize(QSize(20, 0));
        textLabel1_5_2_3->setAlignment(Qt::AlignCenter);
        textLabel1_5_2_3->setWordWrap(false);

        _12->addWidget(textLabel1_5_2_3);

        zThumbWheel = new QtThumbWheel(orientationFrame);
        zThumbWheel->setObjectName(QString::fromUtf8("zThumbWheel"));
        zThumbWheel->setMinimumSize(QSize(140, 20));

        _12->addWidget(zThumbWheel);


        _9->addLayout(_12);


        orientationLayout_2->addLayout(_9);


        verticalLayout_4->addLayout(orientationLayout_2);


        vboxLayout1->addWidget(orientationFrame);

        spacer57_2 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacer57_2);

        hboxLayout16 = new QHBoxLayout();
        hboxLayout16->setSpacing(6);
        hboxLayout16->setObjectName(QString::fromUtf8("hboxLayout16"));
        spacer55_2 = new QSpacerItem(17, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout16->addItem(spacer55_2);

        fitRegionButton = new QPushButton(layoutFrame);
        fitRegionButton->setObjectName(QString::fromUtf8("fitRegionButton"));
        fitRegionButton->setMinimumSize(QSize(60, 0));

        hboxLayout16->addWidget(fitRegionButton);

        cropRegionButton = new QPushButton(layoutFrame);
        cropRegionButton->setObjectName(QString::fromUtf8("cropRegionButton"));
        cropRegionButton->setMinimumSize(QSize(70, 0));

        hboxLayout16->addWidget(cropRegionButton);

        fitDomainButton = new QPushButton(layoutFrame);
        fitDomainButton->setObjectName(QString::fromUtf8("fitDomainButton"));
        fitDomainButton->setMinimumSize(QSize(70, 0));

        hboxLayout16->addWidget(fitDomainButton);

        cropDomainButton = new QPushButton(layoutFrame);
        cropDomainButton->setObjectName(QString::fromUtf8("cropDomainButton"));
        cropDomainButton->setMinimumSize(QSize(70, 0));

        hboxLayout16->addWidget(cropDomainButton);

        spacer56 = new QSpacerItem(27, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout16->addItem(spacer56);


        vboxLayout1->addLayout(hboxLayout16);


        horizontalLayout_7->addLayout(vboxLayout1);


        verticalLayout_6->addWidget(layoutFrame);


        horizontalLayout_8->addLayout(verticalLayout_6);


        verticalLayout_7->addWidget(frame);

        frame_4 = new QFrame(IsolineTab);
        frame_4->setObjectName(QString::fromUtf8("frame_4"));
        frame_4->setMaximumSize(QSize(422, 16777215));
        frame_4->setFrameShape(QFrame::StyledPanel);
        frame_4->setFrameShadow(QFrame::Plain);
        frame_4->setLineWidth(0);
        horizontalLayout_9 = new QHBoxLayout(frame_4);
        horizontalLayout_9->setSpacing(6);
        horizontalLayout_9->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setSpacing(6);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        showHideImageButton = new QPushButton(frame_4);
        showHideImageButton->setObjectName(QString::fromUtf8("showHideImageButton"));

        verticalLayout_5->addWidget(showHideImageButton);

        imageFrame = new QFrame(frame_4);
        imageFrame->setObjectName(QString::fromUtf8("imageFrame"));
        imageFrame->setFrameShape(QFrame::NoFrame);
        imageFrame->setFrameShadow(QFrame::Plain);
        imageFrame->setLineWidth(0);
        horizontalLayout_2 = new QHBoxLayout(imageFrame);
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        isolineFrameHolder = new QFrame(imageFrame);
        isolineFrameHolder->setObjectName(QString::fromUtf8("isolineFrameHolder"));
        isolineFrameHolder->setFrameShape(QFrame::StyledPanel);
        isolineFrameHolder->setFrameShadow(QFrame::Raised);
        hboxLayout17 = new QHBoxLayout(isolineFrameHolder);
        hboxLayout17->setSpacing(6);
        hboxLayout17->setContentsMargins(11, 11, 11, 11);
        hboxLayout17->setObjectName(QString::fromUtf8("hboxLayout17"));
        vboxLayout5 = new QVBoxLayout();
        vboxLayout5->setSpacing(6);
        vboxLayout5->setObjectName(QString::fromUtf8("vboxLayout5"));
        textLabel3 = new QLabel(isolineFrameHolder);
        textLabel3->setObjectName(QString::fromUtf8("textLabel3"));
        textLabel3->setAlignment(Qt::AlignCenter);
        textLabel3->setWordWrap(false);

        vboxLayout5->addWidget(textLabel3);

        isolineImageFrame = new IsolineFrame(isolineFrameHolder);
        isolineImageFrame->setObjectName(QString::fromUtf8("isolineImageFrame"));
        sizePolicy.setHeightForWidth(isolineImageFrame->sizePolicy().hasHeightForWidth());
        isolineImageFrame->setSizePolicy(sizePolicy);
        isolineImageFrame->setMinimumSize(QSize(380, 261));
        isolineImageFrame->setMaximumSize(QSize(16777215, 300));

        vboxLayout5->addWidget(isolineImageFrame);

        spacer49 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout5->addItem(spacer49);


        hboxLayout17->addLayout(vboxLayout5);


        verticalLayout->addWidget(isolineFrameHolder);

        frame8 = new QFrame(imageFrame);
        frame8->setObjectName(QString::fromUtf8("frame8"));
        frame8->setFrameShape(QFrame::StyledPanel);
        frame8->setFrameShadow(QFrame::Raised);
        hboxLayout18 = new QHBoxLayout(frame8);
        hboxLayout18->setSpacing(6);
        hboxLayout18->setContentsMargins(11, 11, 11, 11);
        hboxLayout18->setObjectName(QString::fromUtf8("hboxLayout18"));
        vboxLayout6 = new QVBoxLayout();
        vboxLayout6->setSpacing(6);
        vboxLayout6->setObjectName(QString::fromUtf8("vboxLayout6"));
        textLabel2_6 = new QLabel(frame8);
        textLabel2_6->setObjectName(QString::fromUtf8("textLabel2_6"));
        textLabel2_6->setAlignment(Qt::AlignCenter);
        textLabel2_6->setWordWrap(false);

        vboxLayout6->addWidget(textLabel2_6);

        hboxLayout19 = new QHBoxLayout();
        hboxLayout19->setSpacing(6);
        hboxLayout19->setObjectName(QString::fromUtf8("hboxLayout19"));
        spacer149 = new QSpacerItem(21, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout19->addItem(spacer149);

        textLabel5 = new QLabel(frame8);
        textLabel5->setObjectName(QString::fromUtf8("textLabel5"));
        textLabel5->setWordWrap(false);

        hboxLayout19->addWidget(textLabel5);

        selectedXLabel = new QLabel(frame8);
        selectedXLabel->setObjectName(QString::fromUtf8("selectedXLabel"));
        selectedXLabel->setMinimumSize(QSize(60, 0));
        selectedXLabel->setAlignment(Qt::AlignCenter);
        selectedXLabel->setWordWrap(false);

        hboxLayout19->addWidget(selectedXLabel);

        selectedYLabel = new QLabel(frame8);
        selectedYLabel->setObjectName(QString::fromUtf8("selectedYLabel"));
        selectedYLabel->setMinimumSize(QSize(60, 0));
        selectedYLabel->setAlignment(Qt::AlignCenter);
        selectedYLabel->setWordWrap(false);

        hboxLayout19->addWidget(selectedYLabel);

        selectedZLabel = new QLabel(frame8);
        selectedZLabel->setObjectName(QString::fromUtf8("selectedZLabel"));
        selectedZLabel->setMinimumSize(QSize(60, 0));
        selectedZLabel->setAlignment(Qt::AlignCenter);
        selectedZLabel->setWordWrap(false);

        hboxLayout19->addWidget(selectedZLabel);

        spacer150 = new QSpacerItem(21, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout19->addItem(spacer150);


        vboxLayout6->addLayout(hboxLayout19);

        latLonFrame = new QFrame(frame8);
        latLonFrame->setObjectName(QString::fromUtf8("latLonFrame"));
        latLonFrame->setFrameShape(QFrame::NoFrame);
        latLonFrame->setFrameShadow(QFrame::Plain);
        hboxLayout20 = new QHBoxLayout(latLonFrame);
        hboxLayout20->setSpacing(6);
        hboxLayout20->setContentsMargins(11, 11, 11, 11);
        hboxLayout20->setObjectName(QString::fromUtf8("hboxLayout20"));
        hboxLayout21 = new QHBoxLayout();
        hboxLayout21->setSpacing(6);
        hboxLayout21->setObjectName(QString::fromUtf8("hboxLayout21"));
        spacer149_2 = new QSpacerItem(21, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout21->addItem(spacer149_2);

        textLabel5_2 = new QLabel(latLonFrame);
        textLabel5_2->setObjectName(QString::fromUtf8("textLabel5_2"));
        textLabel5_2->setWordWrap(false);

        hboxLayout21->addWidget(textLabel5_2);

        selectedLonLabel = new QLabel(latLonFrame);
        selectedLonLabel->setObjectName(QString::fromUtf8("selectedLonLabel"));
        selectedLonLabel->setMinimumSize(QSize(60, 0));
        selectedLonLabel->setAlignment(Qt::AlignCenter);
        selectedLonLabel->setWordWrap(false);

        hboxLayout21->addWidget(selectedLonLabel);

        selectedLatLabel = new QLabel(latLonFrame);
        selectedLatLabel->setObjectName(QString::fromUtf8("selectedLatLabel"));
        selectedLatLabel->setMinimumSize(QSize(60, 0));
        selectedLatLabel->setAlignment(Qt::AlignCenter);
        selectedLatLabel->setWordWrap(false);

        hboxLayout21->addWidget(selectedLatLabel);

        spacer150_2 = new QSpacerItem(21, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout21->addItem(spacer150_2);


        hboxLayout20->addLayout(hboxLayout21);


        vboxLayout6->addWidget(latLonFrame);

        hboxLayout22 = new QHBoxLayout();
        hboxLayout22->setSpacing(6);
        hboxLayout22->setObjectName(QString::fromUtf8("hboxLayout22"));
        spacer151 = new QSpacerItem(61, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout22->addItem(spacer151);

        variableLabel = new QLabel(frame8);
        variableLabel->setObjectName(QString::fromUtf8("variableLabel"));
        variableLabel->setAlignment(Qt::AlignCenter);
        variableLabel->setWordWrap(false);

        hboxLayout22->addWidget(variableLabel);

        valueMagLabel = new QLabel(frame8);
        valueMagLabel->setObjectName(QString::fromUtf8("valueMagLabel"));
        valueMagLabel->setMinimumSize(QSize(60, 0));
        valueMagLabel->setAlignment(Qt::AlignCenter);
        valueMagLabel->setWordWrap(false);

        hboxLayout22->addWidget(valueMagLabel);

        spacer152_2 = new QSpacerItem(21, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout22->addItem(spacer152_2);


        vboxLayout6->addLayout(hboxLayout22);

        spacer148 = new QSpacerItem(20, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout6->addItem(spacer148);

        hboxLayout23 = new QHBoxLayout();
        hboxLayout23->setSpacing(6);
        hboxLayout23->setObjectName(QString::fromUtf8("hboxLayout23"));
        attachSeedCheckbox = new QCheckBox(frame8);
        attachSeedCheckbox->setObjectName(QString::fromUtf8("attachSeedCheckbox"));

        hboxLayout23->addWidget(attachSeedCheckbox);

        addSeedButton = new QPushButton(frame8);
        addSeedButton->setObjectName(QString::fromUtf8("addSeedButton"));

        hboxLayout23->addWidget(addSeedButton);


        vboxLayout6->addLayout(hboxLayout23);

        spacer144 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout6->addItem(spacer144);

        textLabel11 = new QLabel(frame8);
        textLabel11->setObjectName(QString::fromUtf8("textLabel11"));
        textLabel11->setAlignment(Qt::AlignCenter);
        textLabel11->setWordWrap(false);

        vboxLayout6->addWidget(textLabel11);

        hboxLayout24 = new QHBoxLayout();
        hboxLayout24->setSpacing(6);
        hboxLayout24->setObjectName(QString::fromUtf8("hboxLayout24"));
        regionCenterButton = new QPushButton(frame8);
        regionCenterButton->setObjectName(QString::fromUtf8("regionCenterButton"));

        hboxLayout24->addWidget(regionCenterButton);

        viewCenterButton = new QPushButton(frame8);
        viewCenterButton->setObjectName(QString::fromUtf8("viewCenterButton"));

        hboxLayout24->addWidget(viewCenterButton);

        rakeCenterButton = new QPushButton(frame8);
        rakeCenterButton->setObjectName(QString::fromUtf8("rakeCenterButton"));

        hboxLayout24->addWidget(rakeCenterButton);

        probeCenterButton = new QPushButton(frame8);
        probeCenterButton->setObjectName(QString::fromUtf8("probeCenterButton"));

        hboxLayout24->addWidget(probeCenterButton);


        vboxLayout6->addLayout(hboxLayout24);


        hboxLayout18->addLayout(vboxLayout6);


        verticalLayout->addWidget(frame8);


        horizontalLayout_2->addLayout(verticalLayout);


        verticalLayout_5->addWidget(imageFrame);


        horizontalLayout_9->addLayout(verticalLayout_5);


        verticalLayout_7->addWidget(frame_4);

        showHideAppearanceButton = new QPushButton(IsolineTab);
        showHideAppearanceButton->setObjectName(QString::fromUtf8("showHideAppearanceButton"));

        verticalLayout_7->addWidget(showHideAppearanceButton);

        appearanceFrame = new QFrame(IsolineTab);
        appearanceFrame->setObjectName(QString::fromUtf8("appearanceFrame"));
        QFont font4;
        font4.setFamily(QString::fromUtf8("Arial"));
        appearanceFrame->setFont(font4);
        appearanceFrame->setFrameShape(QFrame::StyledPanel);
        appearanceFrame->setFrameShadow(QFrame::Raised);
        verticalLayout_3 = new QVBoxLayout(appearanceFrame);
        verticalLayout_3->setSpacing(6);
        verticalLayout_3->setContentsMargins(11, 11, 11, 11);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_8 = new QVBoxLayout();
        verticalLayout_8->setSpacing(6);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        label_6 = new QLabel(appearanceFrame);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        QFont font5;
        font5.setFamily(QString::fromUtf8("Arial"));
        font5.setPointSize(12);
        font5.setBold(false);
        font5.setWeight(50);
        label_6->setFont(font5);
        label_6->setAlignment(Qt::AlignCenter);

        verticalLayout_8->addWidget(label_6);

        _5 = new QHBoxLayout();
        _5->setSpacing(6);
        _5->setObjectName(QString::fromUtf8("_5"));
        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _5->addItem(horizontalSpacer_7);

        textLabel1_12 = new QLabel(appearanceFrame);
        textLabel1_12->setObjectName(QString::fromUtf8("textLabel1_12"));
        textLabel1_12->setFont(font5);
        textLabel1_12->setWordWrap(false);

        _5->addWidget(textLabel1_12);

        spacer75_4 = new QSpacerItem(61, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _5->addItem(spacer75_4);

        isolineWidthEdit = new QLineEdit(appearanceFrame);
        isolineWidthEdit->setObjectName(QString::fromUtf8("isolineWidthEdit"));
        isolineWidthEdit->setMinimumSize(QSize(50, 0));
        QFont font6;
        font6.setPointSize(12);
        font6.setBold(false);
        font6.setWeight(50);
        isolineWidthEdit->setFont(font6);
        isolineWidthEdit->setAlignment(Qt::AlignHCenter);

        _5->addWidget(isolineWidthEdit);


        verticalLayout_8->addLayout(_5);

        _6 = new QHBoxLayout();
        _6->setSpacing(6);
        _6->setObjectName(QString::fromUtf8("_6"));
        spacer75_5 = new QSpacerItem(61, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _6->addItem(spacer75_5);

        textLabel1_14 = new QLabel(appearanceFrame);
        textLabel1_14->setObjectName(QString::fromUtf8("textLabel1_14"));
        textLabel1_14->setFont(font5);
        textLabel1_14->setWordWrap(false);

        _6->addWidget(textLabel1_14);

        horizontalSpacer_10 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _6->addItem(horizontalSpacer_10);

        textSizeEdit = new QLineEdit(appearanceFrame);
        textSizeEdit->setObjectName(QString::fromUtf8("textSizeEdit"));
        textSizeEdit->setMinimumSize(QSize(50, 0));
        textSizeEdit->setFont(font6);
        textSizeEdit->setAlignment(Qt::AlignHCenter);

        _6->addWidget(textSizeEdit);


        verticalLayout_8->addLayout(_6);

        label_7 = new QLabel(appearanceFrame);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setFont(font5);
        label_7->setAlignment(Qt::AlignCenter);

        verticalLayout_8->addWidget(label_7);

        _7 = new QHBoxLayout();
        _7->setSpacing(6);
        _7->setObjectName(QString::fromUtf8("_7"));
        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _7->addItem(horizontalSpacer_6);

        horizontalSpacer_14 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _7->addItem(horizontalSpacer_14);

        textLabel1_15 = new QLabel(appearanceFrame);
        textLabel1_15->setObjectName(QString::fromUtf8("textLabel1_15"));
        textLabel1_15->setFont(font5);
        textLabel1_15->setWordWrap(false);

        _7->addWidget(textLabel1_15);

        spacer75_6 = new QSpacerItem(61, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _7->addItem(spacer75_6);

        horizontalSpacer_11 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _7->addItem(horizontalSpacer_11);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _7->addItem(horizontalSpacer_4);

        isolinePanelBackgroundColorButton = new QPushButton(appearanceFrame);
        isolinePanelBackgroundColorButton->setObjectName(QString::fromUtf8("isolinePanelBackgroundColorButton"));
        isolinePanelBackgroundColorButton->setMinimumSize(QSize(70, 0));
        isolinePanelBackgroundColorButton->setMaximumSize(QSize(80, 16777215));
        isolinePanelBackgroundColorButton->setFont(font5);

        _7->addWidget(isolinePanelBackgroundColorButton);

        isolinePanelBackgroundColorEdit = new QLineEdit(appearanceFrame);
        isolinePanelBackgroundColorEdit->setObjectName(QString::fromUtf8("isolinePanelBackgroundColorEdit"));
        isolinePanelBackgroundColorEdit->setMinimumSize(QSize(25, 0));
        QPalette palette;
        QBrush brush(QColor(255, 6, 0, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Base, brush);
        QBrush brush1(QColor(232, 0, 8, 255));
        brush1.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Window, brush1);
        palette.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Window, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::Base, brush1);
        palette.setBrush(QPalette::Disabled, QPalette::Window, brush1);
        isolinePanelBackgroundColorEdit->setPalette(palette);
        isolinePanelBackgroundColorEdit->setCursor(QCursor(Qt::ArrowCursor));
        isolinePanelBackgroundColorEdit->setFocusPolicy(Qt::StrongFocus);
        isolinePanelBackgroundColorEdit->setAcceptDrops(true);
        isolinePanelBackgroundColorEdit->setInputMask(QString::fromUtf8(""));
        isolinePanelBackgroundColorEdit->setReadOnly(true);

        _7->addWidget(isolinePanelBackgroundColorEdit);

        horizontalSpacer_12 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _7->addItem(horizontalSpacer_12);

        panelLineWidthEdit = new QLineEdit(appearanceFrame);
        panelLineWidthEdit->setObjectName(QString::fromUtf8("panelLineWidthEdit"));
        panelLineWidthEdit->setMinimumSize(QSize(50, 0));
        panelLineWidthEdit->setFont(font6);
        panelLineWidthEdit->setAlignment(Qt::AlignHCenter);

        _7->addWidget(panelLineWidthEdit);


        verticalLayout_8->addLayout(_7);

        _8 = new QHBoxLayout();
        _8->setSpacing(6);
        _8->setObjectName(QString::fromUtf8("_8"));
        spacer75_7 = new QSpacerItem(61, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _8->addItem(spacer75_7);

        textLabel1_16 = new QLabel(appearanceFrame);
        textLabel1_16->setObjectName(QString::fromUtf8("textLabel1_16"));
        textLabel1_16->setFont(font5);
        textLabel1_16->setWordWrap(false);

        _8->addWidget(textLabel1_16);

        horizontalSpacer_13 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        _8->addItem(horizontalSpacer_13);

        panelTextSizeEdit = new QLineEdit(appearanceFrame);
        panelTextSizeEdit->setObjectName(QString::fromUtf8("panelTextSizeEdit"));
        panelTextSizeEdit->setMinimumSize(QSize(50, 0));
        panelTextSizeEdit->setFont(font6);
        panelTextSizeEdit->setAlignment(Qt::AlignHCenter);

        _8->addWidget(panelTextSizeEdit);


        verticalLayout_8->addLayout(_8);


        verticalLayout_3->addLayout(verticalLayout_8);


        verticalLayout_7->addWidget(appearanceFrame);

        frame_6 = new QFrame(IsolineTab);
        frame_6->setObjectName(QString::fromUtf8("frame_6"));
        frame_6->setFrameShape(QFrame::StyledPanel);
        frame_6->setFrameShadow(QFrame::Raised);

        verticalLayout_7->addWidget(frame_6);

        frame_5 = new QFrame(IsolineTab);
        frame_5->setObjectName(QString::fromUtf8("frame_5"));
        frame_5->setFrameShape(QFrame::StyledPanel);
        frame_5->setFrameShadow(QFrame::Raised);

        verticalLayout_7->addWidget(frame_5);


        horizontalLayout_10->addLayout(verticalLayout_7);


        retranslateUi(IsolineTab);

        QMetaObject::connectSlotsByName(IsolineTab);
    } // setupUi

    void retranslateUi(QWidget *IsolineTab)
    {
        IsolineTab->setWindowTitle(QApplication::translate("IsolineTab", "Data Isoline Plane", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        IsolineTab->setToolTip(QString());
#endif // QT_NO_TOOLTIP
        textLabel1->setText(QApplication::translate("IsolineTab", "Renderer Control", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        instanceTable->setToolTip(QApplication::translate("IsolineTab", "Select current instance by clicking at left.  Check the box to enable rendering.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        instanceTable->setWhatsThis(QApplication::translate("IsolineTab", "The instance checkbox determines which\n"
"Isoline planes are actively \n"
"rendering in the active visualizer.  You \n"
"can only use it after a data set has been\n"
"loaded.  The identifying number (at the \n"
"left) selects the isoline instance whose \n"
"parameters are currently edited in this \n"
"panel.  Any  number of instances can \n"
"render at the same time.  Use the \"New\" \n"
"button or the \"Duplicate In\" selector to \n"
"create new Isoline plane instances.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        newInstanceButton->setToolTip(QApplication::translate("IsolineTab", "Create a new, default instance of this renderer", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        newInstanceButton->setWhatsThis(QApplication::translate("IsolineTab", "The instance checkbox determines which\n"
"Isoline planes are actively \n"
"rendering in the active visualizer.  You \n"
"can only use it after a data set has been\n"
"loaded.  The identifying number (at the \n"
"left) selects the isoline instance whose \n"
"parameters are currently edited in this \n"
"panel.  Any  number of instances can \n"
"render at the same time.  Use the \"New\" \n"
"button or the \"Duplicate In\" selector to \n"
"create new Isoline instances.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        newInstanceButton->setText(QApplication::translate("IsolineTab", "New", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        deleteInstanceButton->setToolTip(QApplication::translate("IsolineTab", "Delete current renderer Instance", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        deleteInstanceButton->setWhatsThis(QApplication::translate("IsolineTab", "The instance checkbox determines which\n"
"Isoline planes are actively \n"
"rendering in the active visualizer.  You \n"
"can only use it after a data set has been\n"
"loaded.  The identifying number (at the \n"
"left) selects the isoline instance whose \n"
"parameters are currently edited in this \n"
"panel.  Any  number of instances can \n"
"render at the same time.  Use the \"New\" \n"
"button or the \"Duplicate In\" selector to \n"
"create new Isoline instances.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        deleteInstanceButton->setText(QApplication::translate("IsolineTab", "Delete", 0, QApplication::UnicodeUTF8));
        copyCombo->clear();
        copyCombo->insertItems(0, QStringList()
         << QApplication::translate("IsolineTab", "Duplicate In:", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        copyCombo->setToolTip(QApplication::translate("IsolineTab", "Select a visualizer where this renderer instance will be copied", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        copyCombo->setWhatsThis(QApplication::translate("IsolineTab", "The instance checkbox determines which\n"
"Isoline planes are actively \n"
"rendering in the active visualizer.  You \n"
"can only use it after a data set has been\n"
"loaded.  The identifying number (at the \n"
"left) selects the isoline instance whose \n"
"parameters are currently edited in this \n"
"panel.  Any  number of instances can \n"
"render at the same time.  Use the \"New\" \n"
"button or the \"Duplicate In\" selector to \n"
"create new Isoline instances.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        label_2->setText(QApplication::translate("IsolineTab", "Basic Settings:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        fidelityDefaultButton->setToolTip(QApplication::translate("IsolineTab", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Click to set default Fidelity to current LOD and Refinement levels.  The default can optionally be saved in user preferences.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        fidelityDefaultButton->setText(QApplication::translate("IsolineTab", "Set Default Fidelity", 0, QApplication::UnicodeUTF8));
        fidelityBox->setTitle(QApplication::translate("IsolineTab", "low .. Fidelity .. high", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        textLabel1_13->setToolTip(QApplication::translate("IsolineTab", "Select the refinement level (resolution) of the current isoline/contour plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        textLabel1_13->setText(QApplication::translate("IsolineTab", "LOD", 0, QApplication::UnicodeUTF8));
        lodCombo->clear();
        lodCombo->insertItems(0, QStringList()
         << QApplication::translate("IsolineTab", "1:1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "10:1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "100:1", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        lodCombo->setToolTip(QApplication::translate("IsolineTab", "<html><head/><body><p>Specify the approximation level (compression) for the data being used to calculate isolines.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        lodCombo->setWhatsThis(QApplication::translate("IsolineTab", "The Refinement parameter and (in the case of VDC2 data) the LOD parameter control the quality of the approximation used when visualizing a variable.  The Refinement parameter selects the grid resolutiion.  Coarser resolutions require less memory (RAM); less computation by the visualization algorithms; and for VDC1 data, less storage space on disk, therefore less time to read the data.  For VDC2 data the Refinement level has no impact on disk storage.  The LOD parameter, which is only available for VDC2 data sets, selects a compression level.  Variables with greater compression require less disk storage and can thus be read from disk by vaporgui more quickly, but have no impact on memory  (RAM) or computation time required by the visualziation algorithms.  For both LOD and Refinement, level 0 corresponds to the coarsest approximation available.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        textLabel1_2->setToolTip(QApplication::translate("IsolineTab", "Select the refinement level (resolution) of the current isoline/contour plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        textLabel1_2->setText(QApplication::translate("IsolineTab", "Refinement", 0, QApplication::UnicodeUTF8));
        refinementCombo->clear();
        refinementCombo->insertItems(0, QStringList()
         << QApplication::translate("IsolineTab", "0", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "3", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "5", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        refinementCombo->setToolTip(QApplication::translate("IsolineTab", "<html><head/><body><p>Select the refinement level (resolution) of the data used to calculate isolines.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        refinementCombo->setWhatsThis(QApplication::translate("IsolineTab", "The Refinement parameter and (in the case of VDC2 data) the LOD parameter control the quality of the approximation used when visualizing a variable.  The Refinement parameter selects the grid resolutiion.  Coarser resolutions require less memory (RAM); less computation by the visualization algorithms; and for VDC1 data, less storage space on disk, therefore less time to read the data.  For VDC2 data the Refinement level has no impact on disk storage.  The LOD parameter, which is only available for VDC2 data sets, selects a compression level.  Variables with greater compression require less disk storage and can thus be read from disk by vaporgui more quickly, but have no impact on memory  (RAM) or computation time required by the visualziation algorithms.  For both LOD and Refinement, level 0 corresponds to the coarsest approximation available.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel1_3->setText(QApplication::translate("IsolineTab", "IsoValue Selection", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_STATUSTIP
        label_9->setStatusTip(QApplication::translate("IsolineTab", "Select either isolines of 3D or 2D variables", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
#ifndef QT_NO_WHATSTHIS
        label_9->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>Isolines can be calculated from 3D variables along arbitrarily oriented planes, or from 2D variables along a horizontally oriented plane.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        label_9->setText(QApplication::translate("IsolineTab", "Variable Dimension:", 0, QApplication::UnicodeUTF8));
        dimensionCombo->clear();
        dimensionCombo->insertItems(0, QStringList()
         << QApplication::translate("IsolineTab", "2D", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "3D", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        dimensionCombo->setToolTip(QApplication::translate("IsolineTab", "Specify whether the isolines are of 2D or 3D variables", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        dimensionCombo->setStatusTip(QApplication::translate("IsolineTab", "Select either isolines of 3D or 2D variables", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
#ifndef QT_NO_WHATSTHIS
        dimensionCombo->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>Isolines can be calculated from 3D variables along arbitrarily oriented planes, or from 2D variables along a horizontally oriented plane.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        label_10->setText(QApplication::translate("IsolineTab", "Name:", 0, QApplication::UnicodeUTF8));
        variableCombo->clear();
        variableCombo->insertItems(0, QStringList()
         << QApplication::translate("IsolineTab", "var", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        variableCombo->setToolTip(QApplication::translate("IsolineTab", "Specify name of variable used in isolines", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        label_11->setText(QApplication::translate("IsolineTab", "Isovalue Min:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        minIsoEdit->setToolTip(QApplication::translate("IsolineTab", "Specify minimum isovalue", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        minIsoEdit->setText(QApplication::translate("IsolineTab", "0.0", 0, QApplication::UnicodeUTF8));
        label_12->setText(QApplication::translate("IsolineTab", "Max:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        maxIsoEdit->setToolTip(QApplication::translate("IsolineTab", "Specify maximum isovalue", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        maxIsoEdit->setText(QApplication::translate("IsolineTab", "24.5", 0, QApplication::UnicodeUTF8));
        label_13->setText(QApplication::translate("IsolineTab", "Count:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        countIsoEdit->setToolTip(QApplication::translate("IsolineTab", "Specify number of isovalues between min and max", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        countIsoEdit->setText(QApplication::translate("IsolineTab", "3", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        editButton->setToolTip(QApplication::translate("IsolineTab", "Control position of isovalue and histogram bounds", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        editButton->setText(QApplication::translate("IsolineTab", "Edit", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        navigateButton->setToolTip(QApplication::translate("IsolineTab", "Use mouse to zoom and pan edit window", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        navigateButton->setWhatsThis(QApplication::translate("IsolineTab", "The Edit and Zoom/Pan button select\n"
"two modes of mouse interaction in the\n"
"transfer function window.  In Zoom/Pan\n"
"mode, click-drag the left mouse to change\n"
"the region of the transfer function that is\n"
"visible in the window, without changing\n"
"the transfer function itself.  In Edit mode\n"
"the left mouse button is used to select\n"
"and move control points.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        navigateButton->setText(QApplication::translate("IsolineTab", "Zoom/Pan", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        alignButton->setToolTip(QApplication::translate("IsolineTab", "Align edit window with histogram bounds", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        alignButton->setWhatsThis(QApplication::translate("IsolineTab", "The \"Fit to View\" button is used to make the current editing region coincide with the histogram bounds.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        alignButton->setText(QApplication::translate("IsolineTab", "Fit to View", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        newHistoButton->setToolTip(QApplication::translate("IsolineTab", "Click to build a new histogram based on the current region settings, animation timestep, and transfer function bounds", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        newHistoButton->setWhatsThis(QApplication::translate("IsolineTab", "Click the Histo button to refresh the\n"
"histogram that is displayed in the transfer\n"
"function edit window.  The data that is \n"
"histogrammed is determined by the \n"
"current variable, the refinement level,\n"
"the current time step, the probe extents\n"
"and rotation, and the TF data bounds.\n"
"Use the right mouse menu in the \n"
"transfer function window to choose\n"
"how the histogram is scaled.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        newHistoButton->setText(QApplication::translate("IsolineTab", "Histo", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        isoSelectionFrame->setToolTip(QApplication::translate("IsolineTab", "This window displays a histogram of the variable in the specified interval, and allows you to pick the isovalue", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        isoSelectionFrame->setWhatsThis(QApplication::translate("IsolineTab", "This window displays a histogram of the current variable and allows you to position the isovalue by moving the slider at the bottom.  You can change the left and right bounds of the histogram by sliding the arrow at the top of this window.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        leftHistoEdit->setToolTip(QApplication::translate("IsolineTab", "Minimum of histogrammed domain", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        leftHistoEdit->setWhatsThis(QApplication::translate("IsolineTab", "The displayed data bounds are based on\n"
"the range of data values in the first time step\n"
"of the dataset.  These values are only a\n"
"guide.  The TF Domain Bounds can be\n"
"reset to any range.  Click on the \"histo\"\n"
"button to construct, a histogram and thereby\n"
"to determine the actual data values in the\n"
"specified range.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        leftHistoEdit->setText(QApplication::translate("IsolineTab", "0.0", 0, QApplication::UnicodeUTF8));
        textLabel2_4->setText(QApplication::translate("IsolineTab", "Histo Bounds", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        rightHistoEdit->setToolTip(QApplication::translate("IsolineTab", "Maximum of histogrammed domain", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        rightHistoEdit->setWhatsThis(QApplication::translate("IsolineTab", "The displayed data bounds are based on\n"
"the range of data values in the first time step\n"
"of the dataset.  These values are only a\n"
"guide.  The histogram bounds can be\n"
"reset to any range.  Click on the \"histo\"\n"
"button to construct, a histogram and thereby\n"
"to determine the actual data values in the\n"
"specified range.  If you can't see the isovalue\n"
"in the display, set it to a value between the\n"
"left and right bounds.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        rightHistoEdit->setText(QApplication::translate("IsolineTab", "1.0", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        minDataBound->setToolTip(QApplication::translate("IsolineTab", "Data bounds are estimated from the variable at the first available time step.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        minDataBound->setWhatsThis(QApplication::translate("IsolineTab", "The displayed data bounds are based on\n"
"the range of data values in the first time step\n"
"of the dataset.  These values are only a\n"
"guide.  The TF Domain Bounds can be\n"
"reset to any range.  Click on the \"histo\"\n"
"button to construct, a histogram and thereby\n"
"to determine the actual data values in the\n"
"specified range.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        minDataBound->setText(QApplication::translate("IsolineTab", "0.0", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        textLabel2_5->setToolTip(QApplication::translate("IsolineTab", "Data bounds are estimated from the variable at the first available time step.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        textLabel2_5->setText(QApplication::translate("IsolineTab", "Data Bounds", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        maxDataBound->setToolTip(QApplication::translate("IsolineTab", "Data bounds are estimated from the variable at the first available time step.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        maxDataBound->setWhatsThis(QApplication::translate("IsolineTab", "The displayed data bounds are based on\n"
"the range of data values in the first time step\n"
"of the dataset.  These values are only a\n"
"guide.  The TF Domain Bounds can be\n"
"reset to any range.  Click on the \"histo\"\n"
"button to construct, a histogram and thereby\n"
"to determine the actual data values in the\n"
"specified range.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        maxDataBound->setText(QApplication::translate("IsolineTab", "1.0", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        fitDataButton->setToolTip(QApplication::translate("IsolineTab", "Click to match the histo bounds to the bounds of the current variable", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        fitDataButton->setText(QApplication::translate("IsolineTab", "Fit Data", 0, QApplication::UnicodeUTF8));
        editIsovaluesButton->setText(QApplication::translate("IsolineTab", "Edit Isovalues", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        fitIsovalsButton->setToolTip(QApplication::translate("IsolineTab", "Move the isovalues to fit in histo bounds", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        fitIsovalsButton->setText(QApplication::translate("IsolineTab", "Fit Isovalues", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        loadButton->setToolTip(QApplication::translate("IsolineTab", "Load transfer function from file or session", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        loadButton->setText(QApplication::translate("IsolineTab", "Load TF", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        loadInstalledButton->setToolTip(QApplication::translate("IsolineTab", "Load Transfer Function from directory of pre-installed transfer functions", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        loadInstalledButton->setText(QApplication::translate("IsolineTab", "Load Installed TF", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        saveButton->setToolTip(QApplication::translate("IsolineTab", "Save this transfer function to a file or session", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        saveButton->setText(QApplication::translate("IsolineTab", "Save TF", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        copyToProbeButton->setToolTip(QApplication::translate("IsolineTab", "Move the isovalues to fit in histo bounds", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        copyToProbeButton->setText(QApplication::translate("IsolineTab", "Copy To Probe", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        textLabel1_17->setWhatsThis(QApplication::translate("IsolineTab", "The Histo scaling factor rescales the displayed histogram linearly by that factor.  Use the right mouse in the opacity window to choose nonlinear scaling options.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel1_17->setText(QApplication::translate("IsolineTab", "Histo scale", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        histoScaleEdit->setToolTip(QApplication::translate("IsolineTab", "Scale factor for histogram display", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        histoScaleEdit->setWhatsThis(QApplication::translate("IsolineTab", "The Histo scaling factor rescales the displayed histogram linearly by that factor.  Use the right mouse in the opacity window to choose nonlinear scaling options.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        histoScaleEdit->setText(QApplication::translate("IsolineTab", "1", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        showHideLayoutButton->setToolTip(QApplication::translate("IsolineTab", "Click to toggle hide/show the isoline plane layout options", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        showHideLayoutButton->setText(QApplication::translate("IsolineTab", "Show Isoline Layout Options", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        textLabel2_2_2->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the location of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline by\n"
"one volume cell (at the current\n"
"refinement level) in any direction\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The X, Y, and Z center refer to \n"
"the coordinates in the 3D volume\n"
"where the center of the isoline is\n"
"positioned.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel2_2_2->setText(QApplication::translate("IsolineTab", "Plane Center", 0, QApplication::UnicodeUTF8));
        textLabel3_12_2->setText(QApplication::translate("IsolineTab", "X", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        xCenterSlider->setToolTip(QApplication::translate("IsolineTab", "Specify the X center of isoline plane in scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        xCenterSlider->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the location of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline by\n"
"one volume cell (at the current\n"
"refinement level) in any direction\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The X, Y, and Z center refer to \n"
"the coordinates in the 3D volume\n"
"where the center of the isoline is\n"
"positioned.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        xCenterEdit->setToolTip(QApplication::translate("IsolineTab", "Specify the X center of isoline plane in scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        xCenterEdit->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the location of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline by\n"
"one volume cell (at the current\n"
"refinement level) in any direction\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The X, Y, and Z center refer to \n"
"the coordinates in the 3D volume\n"
"where the center of the isoline is\n"
"positioned.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        xCenterEdit->setText(QApplication::translate("IsolineTab", "0.3", 0, QApplication::UnicodeUTF8));
        textLabel3_12_3_3->setText(QApplication::translate("IsolineTab", "Y", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        yCenterSlider->setToolTip(QApplication::translate("IsolineTab", "Specify the Y center of isoline plane in scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        yCenterSlider->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the location of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline by\n"
"one volume cell (at the current\n"
"refinement level) in any direction\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The X, Y, and Z center refer to \n"
"the coordinates in the 3D volume\n"
"where the center of the isoline is\n"
"positioned.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        yCenterEdit->setToolTip(QApplication::translate("IsolineTab", "Specify the Y center of isoline plane in scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        yCenterEdit->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the location of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline by\n"
"one volume cell (at the current\n"
"refinement level) in any direction\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The X, Y, and Z center refer to \n"
"the coordinates in the 3D volume\n"
"where the center of the isoline is\n"
"positioned.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        yCenterEdit->setText(QApplication::translate("IsolineTab", "0.132", 0, QApplication::UnicodeUTF8));
        textLabel3_12_3_2_2->setText(QApplication::translate("IsolineTab", "Z", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        zCenterSlider->setToolTip(QApplication::translate("IsolineTab", "Specify the Z center of isoline plane in scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        zCenterSlider->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the location of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline by\n"
"one volume cell (at the current\n"
"refinement level) in any direction\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The X, Y, and Z center refer to \n"
"the coordinates in the 3D volume\n"
"where the center of the isoline is\n"
"positioned.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        zCenterEdit->setToolTip(QApplication::translate("IsolineTab", "Specify the Z center of isoline plane in scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        zCenterEdit->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the location of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline by\n"
"one volume cell (at the current\n"
"refinement level) in any direction\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The X, Y, and Z center refer to \n"
"the coordinates in the 3D volume\n"
"where the center of the isoline is\n"
"positioned.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        zCenterEdit->setText(QApplication::translate("IsolineTab", "0.0", 0, QApplication::UnicodeUTF8));
        textLabel2_2->setText(QApplication::translate("IsolineTab", "Planar Size", 0, QApplication::UnicodeUTF8));
        textLabel3_12->setText(QApplication::translate("IsolineTab", "Xp", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        xSizeSlider->setToolTip(QApplication::translate("IsolineTab", "Specify the width (X size) of plane in scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        xSizeSlider->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the size of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline size \n"
"by two volume cells (at the current\n"
"refinement level) in any dimension,\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The Xp, Yp, and Zp sizes refer to \n"
"the isoline sizes before it is \n"
"mapped into the data volume.  \n"
"The Xp and Yp axes are the (x,y) \n"
"coordinates in the cross-section \n"
"view (below).\n"
"If there is a nonzero rotation, the Xp,\n"
"Yp, and Zp axes will not be the\n"
"same as the X, Y, and Z axes.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        xSizeEdit->setToolTip(QApplication::translate("IsolineTab", "Specify the width (X size) of isoline plane in scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        xSizeEdit->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the size of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline size \n"
"by two volume cells (at the current\n"
"refinement level) in any dimension,\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The Xp, Yp, and Zp sizes refer to \n"
"the isoline sizes before it is \n"
"mapped into the data volume.  \n"
"The Xp and Yp axes are the (x,y) \n"
"coordinates in the cross-section \n"
"view (below).\n"
"If there is a nonzero rotation, the Xp,\n"
"Yp, and Zp axes will not be the\n"
"same as the X, Y, and Z axes.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        xSizeEdit->setText(QApplication::translate("IsolineTab", "0.3", 0, QApplication::UnicodeUTF8));
        textLabel3_12_3->setText(QApplication::translate("IsolineTab", "Yp", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        ySizeSlider->setToolTip(QApplication::translate("IsolineTab", "Specify the Y size height (Y-size)  of isoline in scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        ySizeSlider->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the size of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline size \n"
"by two volume cells (at the current\n"
"refinement level) in any dimension,\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The Xp, Yp, and Zp sizes refer to \n"
"the isoline sizes before it is \n"
"mapped into the data volume.  \n"
"The Xp and Yp axes are the (x,y) \n"
"coordinates in the cross-section \n"
"view (below).\n"
"If there is a nonzero rotation, the Xp,\n"
"Yp, and Zp axes will not be the\n"
"same as the X, Y, and Z axes.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        ySizeEdit->setToolTip(QApplication::translate("IsolineTab", "Specify the Y size height (Y-size)  of isoline plane in scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        ySizeEdit->setWhatsThis(QApplication::translate("IsolineTab", "Manipulate the size of the isoline\n"
"with these sliders and/or text boxes.\n"
"You can \"nudge\" the isoline size \n"
"by two volume cells (at the current\n"
"refinement level) in any dimension,\n"
"by clicking on the bar of a slider to\n"
"the left or right of the slider handle.\n"
"The Xp, Yp, and Zp sizes refer to \n"
"the isoline sizes before it is \n"
"mapped into the data volume.  \n"
"The Xp and Yp axes are the (x,y) \n"
"coordinates in the cross-section \n"
"view (below).\n"
"If there is a nonzero rotation, the Xp,\n"
"Yp, and Zp axes will not be the\n"
"same as the X, Y, and Z axes.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        ySizeEdit->setText(QApplication::translate("IsolineTab", "0.132", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        textLabel1_11->setToolTip(QApplication::translate("IsolineTab", "Min and max user coordinates of smallest box containing the isoline", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        textLabel1_11->setText(QApplication::translate("IsolineTab", "User extents (X,Y,Z)", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        textLabel1_11_4->setToolTip(QApplication::translate("IsolineTab", "Min and max grid coordinates of smallest box containing the isoline at full resolution", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        textLabel1_11_4->setText(QApplication::translate("IsolineTab", "Grid extents (X,Y,Z)", 0, QApplication::UnicodeUTF8));
        textLabel9->setText(QApplication::translate("IsolineTab", "Min:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        minUserXLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max user coordinates of smallest box containing the isoline plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        minUserXLabel->setText(QApplication::translate("IsolineTab", "1.2345e5", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        minUserYLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max user coordinates of smallest box containing the isoline plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        minUserYLabel->setText(QApplication::translate("IsolineTab", "1.2345e5", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        minUserZLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max user coordinates of smallest box containing the isoline plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        minUserZLabel->setText(QApplication::translate("IsolineTab", "1.2345e5", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        minGridXLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max grid coordinates of smallest box containing the isoline at full resolution", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        minGridXLabel->setText(QApplication::translate("IsolineTab", "1234", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        minGridYLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max grid coordinates of smallest box containing the isoline at full resolution", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        minGridYLabel->setText(QApplication::translate("IsolineTab", "1234", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        minGridZLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max grid coordinates of smallest box containing the isoline at full resolution", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        minGridZLabel->setText(QApplication::translate("IsolineTab", "1234", 0, QApplication::UnicodeUTF8));
        textLabel9_2->setText(QApplication::translate("IsolineTab", "Max", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        maxUserXLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max user coordinates of smallest box containing the isoline plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        maxUserXLabel->setText(QApplication::translate("IsolineTab", "1.2345e5", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        maxUserYLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max user coordinates of smallest box containing the isoline plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        maxUserYLabel->setText(QApplication::translate("IsolineTab", "1.2345e5", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        maxUserZLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max user coordinates of smallest box containing the isoline plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        maxUserZLabel->setText(QApplication::translate("IsolineTab", "1.2345e5", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        maxGridXLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max grid coordinates of smallest box containing the isoline at full resolution", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        maxGridXLabel->setText(QApplication::translate("IsolineTab", "1234", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        maxGridYLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max grid coordinates of smallest box containing the isoline at full resolution", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        maxGridYLabel->setText(QApplication::translate("IsolineTab", "1234", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        maxGridZLabel->setToolTip(QApplication::translate("IsolineTab", "Min and max grid coordinates of smallest box containing the isoline at full resolution", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        maxGridZLabel->setText(QApplication::translate("IsolineTab", "1234", 0, QApplication::UnicodeUTF8));
        textLabel9_2_2->setText(QApplication::translate("IsolineTab", "Upper-right lon/lat", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        maxLonLabel->setToolTip(QApplication::translate("IsolineTab", "Longitude and latitude of upper-right corner of isoline plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        maxLonLabel->setText(QApplication::translate("IsolineTab", "1.2345e5", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        maxLatLabel->setToolTip(QApplication::translate("IsolineTab", "Longitude and latitude of upper-right corner of isoline plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        maxLatLabel->setText(QApplication::translate("IsolineTab", "1.2345e5", 0, QApplication::UnicodeUTF8));
        textLabel9_2_2_2->setText(QApplication::translate("IsolineTab", "Lower-left lon/lat", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        minLonLabel->setToolTip(QApplication::translate("IsolineTab", "longitude and latitude of lower-left corner of isoline plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        minLonLabel->setText(QApplication::translate("IsolineTab", "1.2345e5", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        minLatLabel->setToolTip(QApplication::translate("IsolineTab", "longitude and latitude of lower-left corner of isoline plane", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        minLatLabel->setText(QApplication::translate("IsolineTab", "1.2345e5", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        textLabel3_3->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The orientation of the isoline plane is controlled by three angles, theta, phi, and psi.  Theta and phi are spherical angles determining the direction of the line normal to the plane of the isolines.  Psi determines the angle of rotation about the normal vector.  The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the &quot;Axis-Align&quot; selector will reset the orientation to the nearest axis or a selected axis alignment.  Clicking on the  &quot;90 deg rotate&quot; selector rotates the isoline plane 90 degrees about the selected axis.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel3_3->setText(QApplication::translate("IsolineTab", "Orientation Angles", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        thetaEdit->setToolTip(QApplication::translate("IsolineTab", "theta (rotation angle from +X axis)of isoline plane normal vector", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        thetaEdit->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The orientation of the isoline plane is controlled by three angles, theta, phi, and psi. Theta and phi are spherical angles determining the direction of the line normal to the plane of the isolines. Psi determines the angle of rotation about the normal vector. The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the &quot;Axis-Align&quot; selector will reset the orientation to the nearest axis or a selected axis alignment. Clicking on the &quot;90 deg rotate&quot; selector rotates the isoline plane 90 degrees about the selected axis.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        thetaEdit->setText(QApplication::translate("IsolineTab", "0", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        psiEdit->setToolTip(QApplication::translate("IsolineTab", "Psi angle (Rotation around axis determined by phi, theta)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        psiEdit->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The orientation of the isoline plane is controlled by three angles, theta, phi, and psi. Theta and phi are spherical angles determining the direction of the line normal to the plane of the isolines. Psi determines the angle of rotation about the normal vector. The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the &quot;Axis-Align&quot; selector will reset the orientation to the nearest axis or a selected axis alignment. Clicking on the &quot;90 deg rotate&quot; selector rotates the isoline plane 90 degrees about the selected axis.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        psiEdit->setText(QApplication::translate("IsolineTab", "90", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        textLabel12_3->setWhatsThis(QApplication::translate("IsolineTab", "The orientation of the isoline is controlled by three angles, theta, phi, and psi.  Theta and phi are spherical angles determining the direction of the plane normal to the center plane (contour plane) of the isoline.  Psi determines the angle of rotation about the normal vector.  The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the \"Axis-Align\" selector will reset the orientation to the nearest axis or a selected axis alignment.  Clicking on the  \"90 deg rotate\" selector rotates the isoline 90 degrees about the selected axis.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel12_3->setText(QApplication::translate("IsolineTab", "Theta", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        phiEdit->setToolTip(QApplication::translate("IsolineTab", "Phi angle (angle from +Z) of isoline plane normal vector", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        phiEdit->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The orientation of the isoline plane is controlled by three angles, theta, phi, and psi. Theta and phi are spherical angles determining the direction of the line normal to the plane of the isolines. Psi determines the angle of rotation about the normal vector. The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the &quot;Axis-Align&quot; selector will reset the orientation to the nearest axis or a selected axis alignment. Clicking on the &quot;90 deg rotate&quot; selector rotates the isoline plane 90 degrees about the selected axis.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        phiEdit->setText(QApplication::translate("IsolineTab", "90", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        textLabel1_7->setWhatsThis(QApplication::translate("IsolineTab", "The orientation of the isoline is controlled by three angles, theta, phi, and psi.  Theta and phi are spherical angles determining the direction of the plane normal to the center plane (contour plane) of the isoline.  Psi determines the angle of rotation about the normal vector.  The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the \"Axis-Align\" selector will reset the orientation to the nearest axis or a selected axis alignment.  Clicking on the  \"90 deg rotate\" selector rotates the isoline 90 degrees about the selected axis.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel1_7->setText(QApplication::translate("IsolineTab", "Psi", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        textLabel12_4->setWhatsThis(QApplication::translate("IsolineTab", "The orientation of the isoline is controlled by three angles, theta, phi, and psi.  Theta and phi are spherical angles determining the direction of the plane normal to the center plane (contour plane) of the isoline.  Psi determines the angle of rotation about the normal vector.  The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the \"Axis-Align\" selector will reset the orientation to the nearest axis or a selected axis alignment.  Clicking on the  \"90 deg rotate\" selector rotates the isoline 90 degrees about the selected axis.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel12_4->setText(QApplication::translate("IsolineTab", "Phi", 0, QApplication::UnicodeUTF8));
        axisAlignCombo->clear();
        axisAlignCombo->insertItems(0, QStringList()
         << QApplication::translate("IsolineTab", "Axis Align", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "Nearest Axis", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "+ X", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "+ Y", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "+ Z", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "- X", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "- Y", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "- Z", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        axisAlignCombo->setToolTip(QApplication::translate("IsolineTab", "Align isoline plane with a coordinate axis", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        axisAlignCombo->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The orientation of the isoline plane is controlled by three angles, theta, phi, and psi. Theta and phi are spherical angles determining the direction of the line normal to the plane of the isolines. Psi determines the angle of rotation about the normal vector. The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the &quot;Axis-Align&quot; selector will reset the orientation to the nearest axis or a selected axis alignment. Clicking on the &quot;90 deg rotate&quot; selector rotates the isoline plane 90 degrees about the selected axis.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        rotate90Combo->clear();
        rotate90Combo->insertItems(0, QStringList()
         << QApplication::translate("IsolineTab", "90 deg rotate", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "+ X", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "+ Y", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "+ Z", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "- X", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "- Y", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("IsolineTab", "- Z", 0, QApplication::UnicodeUTF8)
        );
#ifndef QT_NO_TOOLTIP
        rotate90Combo->setToolTip(QApplication::translate("IsolineTab", "Rotate isoline plane 90 degrees about its center using selected axis direction", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        rotate90Combo->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The orientation of the isoline plane is controlled by three angles, theta, phi, and psi. Theta and phi are spherical angles determining the direction of the line normal to the plane of the isolines. Psi determines the angle of rotation about the normal vector. The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the &quot;Axis-Align&quot; selector will reset the orientation to the nearest axis or a selected axis alignment. Clicking on the &quot;90 deg rotate&quot; selector rotates the isoline plane 90 degrees about the selected axis.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_WHATSTHIS
        textLabel2_3->setWhatsThis(QApplication::translate("IsolineTab", "The orientation of the isoline is controlled by three angles, theta, phi, and psi.  Theta and phi are spherical angles determining the direction of the plane normal to the center plane (contour plane) of the isoline.  Psi determines the angle of rotation about the normal vector.  The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the \"Axis-Align\" selector will reset the orientation to the nearest axis or a selected axis alignment.  Clicking on the  \"90 deg rotate\" selector rotates the isoline 90 degrees about the selected axis.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel2_3->setText(QApplication::translate("IsolineTab", "Rotate about axes", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        textLabel1_8->setWhatsThis(QApplication::translate("IsolineTab", "The orientation of the isoline is controlled by three angles, theta, phi, and psi.  Theta and phi are spherical angles determining the direction of the plane normal to the center plane (contour plane) of the isoline.  Psi determines the angle of rotation about the normal vector.  The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the \"Axis-Align\" selector will reset the orientation to the nearest axis or a selected axis alignment.  Clicking on the  \"90 deg rotate\" selector rotates the isoline 90 degrees about the selected axis.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel1_8->setText(QApplication::translate("IsolineTab", "X:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        xThumbWheel->setToolTip(QApplication::translate("IsolineTab", "Rotate About X-Axis", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        xThumbWheel->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The orientation of the isoline plane is controlled by three angles, theta, phi, and psi. Theta and phi are spherical angles determining the direction of the line normal to the plane of the isolines. Psi determines the angle of rotation about the normal vector. The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the &quot;Axis-Align&quot; selector will reset the orientation to the nearest axis or a selected axis alignment. Clicking on the &quot;90 deg rotate&quot; selector rotates the isoline plane 90 degrees about the selected axis.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_WHATSTHIS
        textLabel1_5_3->setWhatsThis(QApplication::translate("IsolineTab", "The orientation of the isoline is controlled by three angles, theta, phi, and psi.  Theta and phi are spherical angles determining the direction of the plane normal to the center plane (contour plane) of the isoline.  Psi determines the angle of rotation about the normal vector.  The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the \"Axis-Align\" selector will reset the orientation to the nearest axis or a selected axis alignment.  Clicking on the  \"90 deg rotate\" selector rotates the isoline 90 degrees about the selected axis.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel1_5_3->setText(QApplication::translate("IsolineTab", "Y:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        yThumbWheel->setToolTip(QApplication::translate("IsolineTab", "Rotate about Y-Axis", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        yThumbWheel->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The orientation of the isoline plane is controlled by three angles, theta, phi, and psi. Theta and phi are spherical angles determining the direction of the line normal to the plane of the isolines. Psi determines the angle of rotation about the normal vector. The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the &quot;Axis-Align&quot; selector will reset the orientation to the nearest axis or a selected axis alignment. Clicking on the &quot;90 deg rotate&quot; selector rotates the isoline plane 90 degrees about the selected axis.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_WHATSTHIS
        textLabel1_5_2_3->setWhatsThis(QApplication::translate("IsolineTab", "The orientation of the isoline is controlled by three angles, theta, phi, and psi.  Theta and phi are spherical angles determining the direction of the plane normal to the center plane (contour plane) of the isoline.  Psi determines the angle of rotation about the normal vector.  The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the \"Axis-Align\" selector will reset the orientation to the nearest axis or a selected axis alignment.  Clicking on the  \"90 deg rotate\" selector rotates the isoline 90 degrees about the selected axis.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel1_5_2_3->setText(QApplication::translate("IsolineTab", "Z:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        zThumbWheel->setToolTip(QApplication::translate("IsolineTab", "Rotate about Z-axis", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        zThumbWheel->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The orientation of the isoline plane is controlled by three angles, theta, phi, and psi. Theta and phi are spherical angles determining the direction of the line normal to the plane of the isolines. Psi determines the angle of rotation about the normal vector. The orientation can be controlled by dragging the X, Y, and Z thumb wheels, or by typing angles (in degrees) into the text boxes. Clicking on the &quot;Axis-Align&quot; selector will reset the orientation to the nearest axis or a selected axis alignment. Clicking on the &quot;90 deg rotate&quot; selector rotates the isoline plane 90 degrees about the selected axis.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_TOOLTIP
        fitRegionButton->setToolTip(QApplication::translate("IsolineTab", "Modify isoline plane to fit within current region", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        fitRegionButton->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>Clicking the &quot;Fit to Region&quot; or &quot;Fit to Domain&quot; buttons modifies the isoline plane so as to make its  planar slice as large as possible while fitting inside the extents of the region or the domain, while retaining the 3D plane on which the isoline region is positioned.  The isoline orientation is not changed.  If the isoline plane does not intersect the region or domain extents, the isoline is translated into the region or domain extents.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        fitRegionButton->setText(QApplication::translate("IsolineTab", "Fit to\n"
"Region", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        cropRegionButton->setToolTip(QApplication::translate("IsolineTab", "Crop isoline plane to fit insidecurrent region", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        cropRegionButton->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>Clicking the &quot;Region Crop&quot; or &quot;Domain Crop&quot; buttons reduces the isoline planar extents so that the isoline planar slice fits within the current region or the full data domain.  The isoline orientation is not changed.  If the isoline plane does not intersect the region or domain, then nothing is changed.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        cropRegionButton->setText(QApplication::translate("IsolineTab", "Crop to\n"
"Region", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        fitDomainButton->setToolTip(QApplication::translate("IsolineTab", "Modify isoline plane to fit inside full data domain", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        fitDomainButton->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>Clicking the &quot;Fit to Region&quot; or &quot;Fit to Domain&quot; buttons modifies the isoline plane so as to make its  planar slice as large as possible while fitting inside the extents of the region or the domain, while retaining the 3D plane on which the isoline region is positioned.  The isoline orientation is not changed.  If the isoline plane does not intersect the region or domain extents, the isoline is translated into the region or domain extents.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        fitDomainButton->setText(QApplication::translate("IsolineTab", "Fit to\n"
"Domain", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        cropDomainButton->setToolTip(QApplication::translate("IsolineTab", "Crop isoline plane to fit inside data domain", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        cropDomainButton->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>Clicking the &quot;Region Crop&quot; or &quot;Domain Crop&quot; buttons reduces the isoline planar extents so that the isoline planar slice fits within the current region or the full data domain.  The isoline orientation is not changed.  If the isoline plane does not intersect the region or domain, then nothing is changed.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        cropDomainButton->setText(QApplication::translate("IsolineTab", "Crop to \n"
"Domain", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        showHideImageButton->setToolTip(QApplication::translate("IsolineTab", "Click to toggle hide/show the isoline image options", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        showHideImageButton->setText(QApplication::translate("IsolineTab", "Hide Image Settings", 0, QApplication::UnicodeUTF8));
        textLabel3->setText(QApplication::translate("IsolineTab", "Planar View", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_WHATSTHIS
        isolineImageFrame->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The planar view shows an image of the isolines in the plane where they are being calculated. Click on a point in this image to identify the 3D coordinates of the point.  The selected point will also be shown in the 3D scene (when in isoline mode).  The selected point can be used for centering, or for specifying flow seed points.</p><p><br/></p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        textLabel2_6->setText(QApplication::translate("IsolineTab", "Cursor (cross-hair) Selection and Usage:", 0, QApplication::UnicodeUTF8));
        textLabel5->setText(QApplication::translate("IsolineTab", "Selected Point", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        selectedXLabel->setToolTip(QApplication::translate("IsolineTab", "X-coordinate of point at cross-hairs", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        selectedXLabel->setText(QApplication::translate("IsolineTab", "0.0", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        selectedYLabel->setToolTip(QApplication::translate("IsolineTab", "Y-coordinate of selected point at cross-hairs", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        selectedYLabel->setText(QApplication::translate("IsolineTab", "0.0", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        selectedZLabel->setToolTip(QApplication::translate("IsolineTab", "Z-coordinate of selected point at cross-hairs", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        selectedZLabel->setText(QApplication::translate("IsolineTab", "0.0", 0, QApplication::UnicodeUTF8));
        textLabel5_2->setText(QApplication::translate("IsolineTab", "Longitude, latitude", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        selectedLonLabel->setToolTip(QApplication::translate("IsolineTab", "Longitude and latitude of selected point at cross-hairs", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        selectedLonLabel->setText(QApplication::translate("IsolineTab", "0.0", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        selectedLatLabel->setToolTip(QApplication::translate("IsolineTab", "Longitude and latitude of selected point at cross-hairs", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        selectedLatLabel->setText(QApplication::translate("IsolineTab", "0.0", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        variableLabel->setToolTip(QApplication::translate("IsolineTab", "value of variable or magnitude of variable(s)at selected point", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        variableLabel->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The value of the current variable at the selected point is displayed.   The value is only displayed if the isoline is enabled and if the selected point is in the current region.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        variableLabel->setText(QApplication::translate("IsolineTab", " Variable at Selected Point", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        valueMagLabel->setToolTip(QApplication::translate("IsolineTab", "value or magnitude of isoline variable at selected point ", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        valueMagLabel->setText(QApplication::translate("IsolineTab", "7.25", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        attachSeedCheckbox->setToolTip(QApplication::translate("IsolineTab", "Check this box to attach the selected point to a seed in the currently applicable flow instance at the current time step.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        attachSeedCheckbox->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The current selected point in the isoline plane</p><p>can be inserted into the list of seeds in</p><p>the flow instance that is currently active</p><p>in the active visualizer.  When you click &quot;Add</p><p>Point to Flow Seeds&quot;, the point will be</p><p>added to the list of seeds at the current</p><p>time step. The resulting flow line will be </p><p>immediately visualized if that flow instance</p><p>is enabled and set to use the seed list,</p><p>provided of course that the seed is inside</p><p>the current region.</p><p>If you check the &quot;Attach Point to flow seed&quot;,</p><p>then the seed will move around as you</p><p>change the selection in the isoline</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        attachSeedCheckbox->setText(QApplication::translate("IsolineTab", "Attach Point to Flow Seed", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        addSeedButton->setToolTip(QApplication::translate("IsolineTab", "Click to add the selected point to the seeds for the applicable flow panel, at the current time step", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        addSeedButton->setWhatsThis(QApplication::translate("IsolineTab", "<html><head/><body><p>The current selected point in the isoline plane</p><p>can be inserted into the list of seeds in</p><p>the flow instance that is currently active</p><p>in the active visualizer.  When you click &quot;Add</p><p>Point to Flow Seeds&quot;, the point will be</p><p>added to the list of seeds at the current</p><p>time step. The resulting flow line will be </p><p>immediately visualized if that flow instance</p><p>is enabled and set to use the seed list,</p><p>provided of course that the seed is inside</p><p>the current region.</p><p>If you check the &quot;Attach Point to flow seed&quot;,</p><p>then the seed will move around as you</p><p>change the selection in the isoline</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        addSeedButton->setText(QApplication::translate("IsolineTab", "Add Point to Flow Seeds", 0, QApplication::UnicodeUTF8));
        textLabel11->setText(QApplication::translate("IsolineTab", "Use selected point to center:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        regionCenterButton->setToolTip(QApplication::translate("IsolineTab", "Click to move the current region center to the selected point", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        regionCenterButton->setWhatsThis(QApplication::translate("IsolineTab", "When this is clicked, the selected point is made the center of the current region.  The region is shrunk as necessary to fit within the full data extents.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        regionCenterButton->setText(QApplication::translate("IsolineTab", "Region ", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        viewCenterButton->setToolTip(QApplication::translate("IsolineTab", "Click to recenter viewpoint at selected point", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        viewCenterButton->setWhatsThis(QApplication::translate("IsolineTab", "When this is clicked, the current viewpoint is modified to point at the selected point and the selected point becomes the rotation center.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        viewCenterButton->setText(QApplication::translate("IsolineTab", "View", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        rakeCenterButton->setToolTip(QApplication::translate("IsolineTab", "Click to reenter flow rake at selected point", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        rakeCenterButton->setWhatsThis(QApplication::translate("IsolineTab", "When clicked, the current selected point becomes the center of the current rake (for the current active flow instance).  The rake will be shrunk as necessary to fit within the data domain.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        rakeCenterButton->setText(QApplication::translate("IsolineTab", "Rake ", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        probeCenterButton->setToolTip(QApplication::translate("IsolineTab", "Click to recenter this isoline at selected point", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
        probeCenterButton->setWhatsThis(QApplication::translate("IsolineTab", "When this button is clicked, this isoline is moved so that its center becomes the selected point.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_WHATSTHIS
        probeCenterButton->setText(QApplication::translate("IsolineTab", "Probe ", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        showHideAppearanceButton->setToolTip(QApplication::translate("IsolineTab", "Click to toggle hide/show the isoline appearance options", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        showHideAppearanceButton->setText(QApplication::translate("IsolineTab", "Hide Appearance Parameters", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("IsolineTab", "Appearance of Isolines in Visualizer:", 0, QApplication::UnicodeUTF8));
        textLabel1_12->setText(QApplication::translate("IsolineTab", "Isoline width:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        isolineWidthEdit->setToolTip(QApplication::translate("IsolineTab", "Specify width in voxels of isolines", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        isolineWidthEdit->setText(QApplication::translate("IsolineTab", "1.0", 0, QApplication::UnicodeUTF8));
        textLabel1_14->setText(QApplication::translate("IsolineTab", "Text font size", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        textSizeEdit->setToolTip(QApplication::translate("IsolineTab", "Specify font size (points) of annotation in 3D scene", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        textSizeEdit->setText(QApplication::translate("IsolineTab", "10", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("IsolineTab", "Appearance of Isolines in Tab Panel:", 0, QApplication::UnicodeUTF8));
        textLabel1_15->setText(QApplication::translate("IsolineTab", "Background color, width:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        isolinePanelBackgroundColorButton->setToolTip(QApplication::translate("IsolineTab", "Click to specify the background color of the isoline image in this panel", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        isolinePanelBackgroundColorButton->setText(QApplication::translate("IsolineTab", "Select", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        isolinePanelBackgroundColorEdit->setToolTip(QApplication::translate("IsolineTab", "Clisk to specify color used in constant map", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        isolinePanelBackgroundColorEdit->setText(QString());
#ifndef QT_NO_TOOLTIP
        panelLineWidthEdit->setToolTip(QApplication::translate("IsolineTab", "Specify width of isolines in this panel", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        panelLineWidthEdit->setText(QApplication::translate("IsolineTab", "1.0", 0, QApplication::UnicodeUTF8));
        textLabel1_16->setText(QApplication::translate("IsolineTab", "Text font size", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        panelTextSizeEdit->setToolTip(QApplication::translate("IsolineTab", "Specify font size of text annotation in this panel", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        panelTextSizeEdit->setText(QApplication::translate("IsolineTab", "10", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class IsolineTab: public Ui_IsolineTab {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ISOLINETAB_H
