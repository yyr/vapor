#include "selectfilepage.h"
#include "createvdfpage.h"
#include "createvdfcomment.h"
#include "populatedatapage.h"
#include "ui/Page3.h"
#include "momvdfcreate.cpp"

CreateVdfPage::CreateVdfPage(SelectFilePage *Page, QWidget *parent) :
    QWizardPage(parent), Ui_Page3()
{
    setupUi(this);

    selectFilePage = Page;
    vdfAdvancedOpts = new CreateVdfAdvanced;
    vdfTLComment = new CreateVdfComment;

    //DCReaderMOM *momData;
}

void CreateVdfPage::checkArguments() {
    qDebug() << "VDF creation args look good so far...";
}

void CreateVdfPage::runVdfCreate() {
    qDebug() << "Running VDF Create.";
}

void CreateVdfPage::on_advanceOptionButton_clicked()
{
    vdfAdvancedOpts->show();
}

//void CreateVdfPage::on_pushButton_4_clicked()
void CreateVdfPage::on_vdfCommentButton_clicked()
{
    vdfTLComment->show();
}

void CreateVdfPage::on_goButton_clicked() {
    //QList<QString> list;
    vector<string> list;
    QString momPopOrRoms = selectFilePage->momPopOrRoms;
    //list = selectFilePage->getSelectedFiles();
    list = selectFilePage->getSelectedFiles();
    //qDebug() << list;

    for (int i=0;i<5;i++) {
        cout << list.at(i) << endl;
    }

    if (momPopOrRoms == "mom"){
        cout << "mom" << endl;
        runMomVdfCreate(list,"mom");
    }

    if (momPopOrRoms == "pop"){
        cout << "pop" << endl;;
    }

    if (momPopOrRoms == "roms"){
        cout << "roms" << endl;
    }

    int argcx = 5;
    char **argvx = new char * [argcx];

    argvx[0] = "./momvdfcreate";
    argvx[1] = "/Users/pearse/Documents/vaporTestData/00010101.ocean_month.NWPp2.Clim.nc";
    argvx[2] = "test.vdf";
    argvx[3] = "-vdc2";
    argvx[4] = "-quiet";

    //test();
    launch(argcx,argvx);
}

//void CreateVdfPage::runMomVdfCreate(QList<QString> list, QString execution) {
void CreateVdfPage::runMomVdfCreate(vector<string> list, QString execution) {
    DCReaderMOM *momData;

    Comment = vdfTLComment->Comment;
    CRList = vdfAdvancedOpts->CRList;
    SBFactor = vdfAdvancedOpts->SBFactor;
    Periodicity = vdfAdvancedOpts->Periodicity;

    momData = new DCReaderMOM(list);


    qDebug() << Comment + CRList + SBFactor + Periodicity;
}
