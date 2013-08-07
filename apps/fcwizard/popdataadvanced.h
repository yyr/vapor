#ifndef POPDATAADVANCED_H
#define POPDATAADVANCED_H

#include <QDialog>
#include "ui/Page4adv.h"
#include "dataholder.h"

namespace Ui {
class PopDataAdvanced;
}

class PopDataAdvanced : public QDialog, public Ui_Page4adv
{
    Q_OBJECT
    
public:
    explicit PopDataAdvanced(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;
    
private slots:
    void on_acceptButton_clicked();
    void on_restoreDefaultButton_clicked();
};

#endif // POPDATAADVANCED_H
