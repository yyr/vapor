#ifndef INTROPAGE_H
#define INTROPAGE_H

#include <QWizardPage>
#include <QtCore>
#include <QtGui>
#include "ui/Page1.h"

namespace Ui {
class IntroPage;
}

class IntroPage : public QWizardPage, public Ui_Page1
{
    Q_OBJECT
    
public:
    IntroPage(QWidget *parent = 0);
    QString operation;
    bool isComplete() const;
    
private slots:
    void on_createVDFButton_clicked();
    void on_populateDataButton_clicked();
};

#endif // INTROPAGE_H
