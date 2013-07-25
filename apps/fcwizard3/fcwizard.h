#ifndef FCWIZARD_H
#define FCWIZARD_H

#include <QWizard>
#include <QDialog>
#include <QtCore>
#include <QtGui>
#include "intropage.h"
#include "selectfilepage.h"
#include "createvdfpage.h"
#include "populatedatapage.h"

class FCWizard : public QWizard
{
    Q_OBJECT
    
public:
    FCWizard(QWidget *parent = 0);

    enum { Intro_Page, SelectFile_Page, Create_VdfPage, Populate_DataPage };

    QString operation;
    IntroPage *introPage;
    SelectFilePage *selectFilePage;
    CreateVdfPage *createVdfPage;
    PopulateDataPage *populateDataPage;
    
private slots:
    void enableNextButton();

private:
    QAbstractButton *nextButton;
};

#endif // FCWIZARDDIALOG_H
