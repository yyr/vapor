#ifndef CREATEVDFCOMMENT_H
#define CREATEVDFCOMMENT_H

#include <QDialog>
#include "ui/Page3cmt.h"
#include "dataholder.h"


namespace Ui {
class CreateVdfComment;
}

class CreateVdfComment : public QDialog, public Ui_Page3cmt
{
    Q_OBJECT
    
public:
    explicit CreateVdfComment(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;

private slots:
    void on_buttonBox_accepted();
};

#endif // CREATEVDFCOMMENT_H
