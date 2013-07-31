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
    introPage = new IntroPage;
    selectFilePage = new SelectFilePage(introPage, dataHolder);
    createVdfPage = new CreateVdfPage(selectFilePage, dataHolder);
    populateDataPage = new PopulateDataPage(selectFilePage, dataHolder);

    //selectFilePage->getOtherPages(createVdfPage,populateDataPage);

    setPage(Intro_Page, introPage);
    setPage(SelectFile_Page, selectFilePage);
    setPage(Create_VdfPage, createVdfPage);
    setPage(Populate_DataPage, populateDataPage);

    setOption(QWizard::NoBackButtonOnStartPage,true);
    button(QWizard::NextButton)->setEnabled(false);
}

void FCWizard::enableNextButton()
{
    FCWizard::button(QWizard::NextButton)->setEnabled(true);
}
