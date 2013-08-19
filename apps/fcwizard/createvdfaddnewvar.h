#ifndef CREATEVDFADDNEWVAR_H
#define CREATEVDFADDNEWVAR_H

#include <QDialog>
#include "ui/Page3addnewvar.h"
#include "dataholder.h"

namespace Ui {
class CreateVdfAddNewVar;
}

class CreateVdfAddNewVar : public QDialog, public Ui_Page3addnewvar
{
    Q_OBJECT

public:
    explicit CreateVdfAddNewVar(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;

private slots:
    void on_buttonBox_accepted();
};

/*namespace Ui {
class CreateVdfAddNewVar;
}

class CreateVdfAddNewVar : public QDialog, public Ui_Page3var
{
    Q_OBJECT
public:
    explicit CreateVdfAddNewVar(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;
signals:
    
private slots:
        void on_buttonBox_accepted();
};*/

#endif // CREATEVDFADDNEWVAR_H
