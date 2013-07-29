#ifndef CREATEVDFCOMMENT_H
#define CREATEVDFCOMMENT_H

#include <QDialog>
#include "ui/Page3cmt.h"


namespace Ui {
class CreateVdfComment;
}

class CreateVdfComment : public QDialog, public Ui_Page3cmt
{
    Q_OBJECT
    
public:
    explicit CreateVdfComment(QWidget *parent = 0);
};

#endif // CREATEVDFCOMMENT_H
