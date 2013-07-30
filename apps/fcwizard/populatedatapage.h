#ifndef POPULATEDATAPAGE_H
#define POPULATEDATAPAGE_H

#include <QWizardPage>
#include <QtGui/QtGui>
#include "selectfilepage.h"
#include "popdataadvanced.h"
#include "showtimeconflicts.h"
#include "ui/Page4.h"

namespace Ui {
class PopulateDataPage;
}

class PopulateDataPage : public QWizardPage, public Ui_Page4
{
    Q_OBJECT
    
public:
    PopulateDataPage(SelectFilePage *Page, QWidget *parent = 0);
    PopDataAdvanced *popAdvancedOpts;
    ShowTimeConflicts *timeConflicts;
    SelectFilePage *selectFilePage;

    void printSomething();

private slots:
    void on_goButton_clicked();
    void on_advancedOptionsButton_clicked();
    void on_scanVDF_clicked();
    void on_variableList_activated(const QModelIndex &index);
    void warnButton_clicked();

private:
    void checkArguments();
    void run2vdf();
};

#endif // POPULATEDATAPAGE_H
