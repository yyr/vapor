#include "fcwizard.h"
#include "selectfilepage.h"
#include "ui/Page2.h"
#include "intropage.h"
#include "dataholder.h"

using namespace std;

SelectFilePage::SelectFilePage(IntroPage *Page, DataHolder *DH, QWidget *parent) :
    QWizardPage(parent), Ui_Page2()
{
    setupUi(this);

    momPopOrRoms = "mom";
    introPage = Page;
    dataHolder = DH;
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

    if (fileList->count() > 0) {
        momRadioButton->setEnabled(true);
        popRadioButton->setEnabled(true);
        romsRadioButton->setEnabled(true);
    }

    // convert fileList into a vector of std::string
    // (not QStrings) to feed into DCReaderMOM
    stdFileList = getSelectedFiles();
    dataHolder->setFiles(stdFileList);
}

void SelectFilePage::on_removeFileButton_clicked() {
    qDeleteAll(fileList->selectedItems());

    QStringList fileNames;
    int count = fileList->count();
    for (int i=0;i<count;i++) {
        fileNames.append(fileList->item(i)->text());
    }
    stdFileList = getSelectedFiles();
    dataHolder->setFiles(stdFileList);
}

void SelectFilePage::on_momRadioButton_clicked() {
    dataHolder->setType("mom");
}

void SelectFilePage::on_popRadioButton_clicked() {
    dataHolder->setType("pop");
}

void SelectFilePage::on_romsRadioButton_clicked() {
    dataHolder->setType("roms");
}

vector<string> SelectFilePage::getSelectedFiles() {
    vector <string> stdStrings;
    for (int i=0;i<fileList->count();i++)
        stdStrings.push_back(fileList->item(i)->text().toStdString());
    return stdStrings;
}

bool SelectFilePage::validatePage(){
    return true;
}

int SelectFilePage::nextId() const
{
    if (introPage->operation == "vdfcreate") return FCWizard::Create_VdfPage;
    else return FCWizard::Populate_DataPage;
}
