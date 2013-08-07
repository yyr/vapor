#ifndef SHOWTIMECONFLICTS_H
#define SHOWTIMECONFLICTS_H

#include <QDialog>
#include "ui/Page4conf.h"
#include "dataholder.h"

namespace Ui {
    class ShowTimeConflicts;
}

class ShowTimeConflicts : public QDialog, public Ui_Page4conf
{
    Q_OBJECT
    
public:
    explicit ShowTimeConflicts(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;
};

#endif // SHOWTIMECONFLICTS_H
