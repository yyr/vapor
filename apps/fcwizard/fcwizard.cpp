#include "dataholder.h"
#include "intropage.h"
#include "selectfilepage.h"
#include "createvdfpage.h"
#include "populatedatapage.h"

#include "fcwizard.h"
//#include "ui_fcwizard.h"
//#include "ui_intropage.h"
//#include "ui_createvdfpage.h"
//#include "ui_populatedatapage.h"

#include <QDebug>
#include <QtGui/QMessageBox>


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
    selectFilePage = new SelectFilePage(introPage);
    createVdfPage = new CreateVdfPage(selectFilePage);
    populateDataPage = new PopulateDataPage(selectFilePage);

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
