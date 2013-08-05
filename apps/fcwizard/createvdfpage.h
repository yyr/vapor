#ifndef CREATEVDFPAGE_H
#define CREATEVDFPAGE_H

#include <QWizardPage>
#include <QtGui/QtGui>
#include <QTableWidgetItem>
#include "dataholder.h"
//#include "populatedatapage.h"
#include "createvdfadvanced.h"
#include "createvdfcomment.h"
#include "ui/Page3.h"
#include <vector>

namespace Ui {
class CreateVdfPage;
}

class CreateVdfPage : public QWizardPage, public Ui_Page3
{
    Q_OBJECT
    
public:
    CreateVdfPage(DataHolder *DH, QWidget *parent = 0);
    CreateVdfAdvanced *vdfAdvancedOpts;
    CreateVdfComment *vdfTLComment;
    DataHolder *dataHolder;

    vector<string> varList;
    vector<string> varSelectionList;
    vector<QTableWidgetItem> varWidgetItems;

private slots:
    void on_goButton_clicked();
    void on_advanceOptionButton_clicked();
    void on_vdfCommentButton_clicked();

private:
    void initializePage();
    void checkArguments();
    void runMomVdfCreate();
    //void runMomVdfCreate(vector<string> list, QString execution);
    QString Comment;
    QString CRList;
    QString SBFactor;
    QString Periodicity;
};

#endif // CREATEVDFPAGE_H
