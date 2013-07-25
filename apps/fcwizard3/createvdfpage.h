#ifndef CREATEVDFPAGE_H
#define CREATEVDFPAGE_H

#include <QWizardPage>
#include <QtGui/QtGui>
#include "populatedatapage.h"
#include "selectfilepage.h"
#include "createvdfadvanced.h"
#include "createvdfcomment.h"
#include "ui/Page3.h"

namespace Ui {
class CreateVdfPage;
}

class CreateVdfPage : public QWizardPage, public Ui_Page3
{
    Q_OBJECT
    
public:
    CreateVdfPage(SelectFilePage *Page, QWidget *parent = 0);
    CreateVdfAdvanced *vdfAdvancedOpts;
    CreateVdfComment *vdfTLComment;
    SelectFilePage *selectFilePage;

private slots:
    void on_goButton_clicked();
    void on_advanceOptionButton_clicked();
    void on_pushButton_4_clicked();

private:
    void checkArguments();
    void runVdfCreate();
};

#endif // CREATEVDFPAGE_H
