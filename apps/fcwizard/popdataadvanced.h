#ifndef POPDATAADVANCED_H
#define POPDATAADVANCED_H

#include <QDialog>
#include "ui/Page4adv.h"

namespace Ui {
class PopDataAdvanced;
}

class PopDataAdvanced : public QDialog, public Ui_Page4adv
{
    Q_OBJECT
    
public:
    explicit PopDataAdvanced(QWidget *parent = 0);
    
private slots:
    void on_acceptButton_clicked();
};

#endif // POPDATAADVANCED_H
