#include <QDebug>
#include <QtGui/QMessageBox>

#include "fcwizard.h"
/*#include "dataholder.h"
#include "intropage.h"
#include "selectfilepage.h"
#include "createvdfpage.h"
#include "populatedatapage.h"*/


FCWizard::FCWizard(QWidget *parent) :
    QWizard(parent)
{
    setWizardStyle(QWizard::AeroStyle);
    //setWizardStyle(QWizard::ClassicStyle);
    //setWizardStyle(QWizard::ModernStyle);
    //setWizardStyle(QWizard::MacStyle);
    resize(QSize(500,450).expandedTo(minimumSizeHint()));

    dataHolder = new DataHolder;
    introPage = new IntroPage(dataHolder);
    //selectFilePage = new SelectFilePage(introPage, dataHolder);
    selectFilePage = new SelectFilePage(dataHolder);
    createVdfPage = new CreateVdfPage(dataHolder);
    populateDataPage = new PopulateDataPage(dataHolder);

    //selectFilePage->getOtherPages(createVdfPage,populateDataPage);

    setPage(Intro_Page, introPage);
    setPage(SelectFile_Page, selectFilePage);
    setPage(Create_VdfPage, createVdfPage);
    setPage(Populate_DataPage, populateDataPage);

    setOption(QWizard::NoBackButtonOnStartPage,true);

    //QAbstractButton *nextButton = button(QWizard::NextButton);
    //nextButton->hide();
    //button(QWizard::NextButton)->hide();//->setVisible(false);
}

/*void FCWizard::enableNextButton()
{
    qDebug() << "enable next";
    FCWizard::button(QWizard::NextButton)->setVisible(true);
    //FCWizard::button(QWizard::NextButton)->setEnabled(true);
}*/
