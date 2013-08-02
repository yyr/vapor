#ifndef SELECTFILEPAGE_H
#define SELECTFILEPAGE_H

#include <QtGui/QtGui>
#include <QWizardPage>
#include "intropage.h"
#include "dataholder.h"
//#include "createvdfpage.h"
//#include "populatedatapage.h"
#include <ui/Page2.h>
//#include "momvdfcreate.cpp"
//#include <vapor/OptionParser.h>
//#include <vapor/MetadataVDC.h>
//#include <vapor/DCReaderMOM.h>
//#include <vapor/DCReaderROMS.h>
//#include <vapor/DCReader.h>
//#include <vapor/CFuncs.h>

namespace Ui {
class SelectFilePage;
}

class SelectFilePage : public QWizardPage, public Ui_Page2
{
    Q_OBJECT
    
public:
    explicit SelectFilePage(IntroPage *Page, DataHolder *DH, QWidget *parent = 0);
    
    //VAPoR::DCReader *fileData;
    string momPopOrRoms;
    IntroPage *introPage;
    DataHolder *dataHolder;
    vector<string> stdFileList;
    vector<string> getSelectedFiles();

    //CreateVdfPage *createVdfPage;
    //PopulateDataPage *populateDataPage;
    //QList<QString> getSelectedFiles();

    //void getOtherPages(CreateVdfPage *cVdfPage, PopulateDataPage *pDataPage);

private slots:
    void on_addFileButton_clicked();
    void on_removeFileButton_clicked();
    void on_momRadioButton_clicked();
    void on_popRadioButton_clicked();
    void on_romsRadioButton_clicked();

private:
    int nextId() const;
    bool validatePage();
};

#endif // SELECTFILEPAGE_H
