#ifndef CREATEVDFADVANCED_H
#define CREATEVDFADVANCED_H

#include <QDialog>
#include "ui/Page3adv.h"

namespace Ui {
class CreateVdfAdvanced;
}

class CreateVdfAdvanced : public QDialog, public Ui_Page3adv
{
    Q_OBJECT
    
public:
    explicit CreateVdfAdvanced(QWidget *parent = 0);

    QString CRList;

    QString SBFactor;
    QString Periodicity;
    
private slots:
    void on_acceptButton_clicked();
    void on_cancelButton_clicked();

private:
    QString px;
    QString py;
    QString pz;
};

#endif // CREATEVDFADVANCED_H
