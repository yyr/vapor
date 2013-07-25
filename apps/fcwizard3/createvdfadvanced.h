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
    
private slots:
    void on_acceptButton_clicked();
};

#endif // CREATEVDFADVANCED_H
