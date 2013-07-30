#include "fcwizard.h"
#include "selectfilepage.h"
#include "ui/Page2.h"
#include "intropage.h"
//#include "createvdfpage.h"
//#include "populatedatapage.h"
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReaderMOM.h>
#include <vapor/DCReaderROMS.h>
#include <vapor/CFuncs.h>
//#include "momvdfcreate.cpp"

using namespace VAPoR;

SelectFilePage::SelectFilePage(IntroPage *Page, QWidget *parent) :
    QWizardPage(parent), Ui_Page2()
{
    setupUi(this);

    momPopOrRoms = "mom";
    introPage = Page;
    //DCReaderMOM *fileData;
    vector <string> stdFileList;
}

void SelectFilePage::on_addFileButton_clicked() {
    QStringList fileNames = QFileDialog::getOpenFileNames(this,"Select basis files for metadata creation.");
    int count = fileList->count();
    for (int i=0;i<count;i++){
        fileNames.append(fileList->item(i)->text());
    }
    fileNames.removeDuplicates();
    fileList->clear();
    fileList->addItems(fileNames);

    // convert fileList into a vector of std::string
    // (not QStrings) to feed into DCReaderMOM
    stdFileList = getSelectedFiles();

    //delete fileData;
    //if (momPopOrRoms == "roms") fileData = new DCReaderMOM(stdFileList);
    //else fileData = new DCReaderMOM(stdFileList);
}

void SelectFilePage::on_removeFileButton_clicked() {
    qDeleteAll(fileList->selectedItems());

    QStringList fileNames;
    int count = fileList->count();
    for (int i=0;i<count;i++) {
        fileNames.append(fileList->item(i)->text());
    }
}

void SelectFilePage::on_momRadioButton_clicked() {
    momPopOrRoms = "mom";
    //delete fileData;
    //fileData = new DCReaderMOM(stdFileList);
}

void SelectFilePage::on_popRadioButton_clicked() {
    momPopOrRoms = "pop";
    //delete fileData;
    //fileData = new DCReaderMOM(stdFileList);
    //size_t count = fileData->GetVariableNames().size();
    //for (size_t i=0;i<count;i++){
    //    cout << fileData->GetVariableNames().at(i) << endl;
    //}
}

void SelectFilePage::on_romsRadioButton_clicked() {
    momPopOrRoms = "roms";
}

vector<string> SelectFilePage::getSelectedFiles() {
    vector <string> stdStrings;
    for (int i=0;i<fileList->count();i++)
        stdStrings.push_back(fileList->item(i)->text().toStdString());
    return stdStrings;
}

//void SelectFilePage::getOtherPages(CreateVdfPage *cVdfPage, PopulateDataPage *pDataPage){
//    createVdfPage = cVdfPage;
//    populateDataPage = pDataPage;
//}

bool SelectFilePage::validatePage(){
    qDebug() << "validate page passed";
    //delete fileData;

    if (momPopOrRoms == "roms") fileData = new DCReaderROMS(stdFileList);
    else fileData = new DCReaderMOM(stdFileList);

    return true;
}

int SelectFilePage::nextId() const
{
    if (introPage->operation == "vdfcreate") return FCWizard::Create_VdfPage;
    else return FCWizard::Populate_DataPage;
}
