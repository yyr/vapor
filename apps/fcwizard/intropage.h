#ifndef INTROPAGE_H
#define INTROPAGE_H

#include <QWizardPage>
#include <QtCore>
#include <QtGui>
#include "ui/Page1.h"
#include "dataholder.h"

namespace Ui {
class IntroPage;
}

class IntroPage : public QWizardPage, public Ui_Page1
{
    Q_OBJECT
    
public:
    IntroPage(DataHolder *DH, QWidget *parent = 0);
    DataHolder *dataHolder;
    bool isComplete() const;

private slots:
    void on_createVDFButton_clicked();
    void on_populateDataButton_clicked();
};

#endif // INTROPAGE_H
