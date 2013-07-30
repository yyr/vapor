#include "fcwizard.h"
#include "selectfilepage.h"
#include "ui/Page2.h"
#include "intropage.h"

SelectFilePage::SelectFilePage(IntroPage *Page, QWidget *parent) :
    QWizardPage(parent), Ui_Page2()
{
    setupUi(this);

    momPopOrRoms = "mom";
    introPage = Page;
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

    /*vector <string> stdStrings;
    count = fileList->count();
    for (int i=0;i<count;i++){
        stdStrings.push_back(fileList->item(i)->text()->toStdString());
    }*/
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
}

void SelectFilePage::on_popRadioButton_clicked() {
    momPopOrRoms = "pop";
    qDebug() << fileList->count();
}

void SelectFilePage::on_romsRadioButton_clicked() {
    momPopOrRoms = "roms";
}

//QList<QString> SelectFilePage::getSelectedFiles() {
vector<string> SelectFilePage::getSelectedFiles() {
    //QList<QString> texts;
    vector <string> stdStrings;
    for (int i=0;i<fileList->count();i++)
        //texts.append(fileList->item(i)->text());
        stdStrings.push_back(fileList->item(i)->text().toStdString());
    //return texts;
    return stdStrings;
}

int SelectFilePage::nextId() const
{
    if (introPage->operation == "vdfcreate") return FCWizard::Create_VdfPage;
    else return FCWizard::Populate_DataPage;
}
