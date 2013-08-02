#ifndef CREATEVDFADVANCED_H
#define CREATEVDFADVANCED_H

#include <QtGui/QtGui>
#include <QDialog>
#include "ui/Page3adv.h"
#include "dataholder.h"

namespace Ui {
class CreateVdfAdvanced;
}

class CreateVdfAdvanced : public QDialog, public Ui_Page3adv
{
    Q_OBJECT

public:
    explicit CreateVdfAdvanced(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;
    QString CRList;
    QString SBFactor;
    QString Periodicity;
    
private slots:
    void on_acceptButton_clicked();
    void on_cancelButton_clicked();
};

#endif // CREATEVDFADVANCED_H
