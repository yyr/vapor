#ifndef SELECTFILEPAGE_H
#define SELECTFILEPAGE_H

#include <QtGui/QtGui>
#include <QWizardPage>
#include <intropage.h>
#include <ui/Page2.h>

namespace Ui {
class SelectFilePage;
}

class SelectFilePage : public QWizardPage, public Ui_Page2
{
    Q_OBJECT
    
public:
    explicit SelectFilePage(IntroPage *Page, QWidget *parent = 0);
    
    QString momPopOrRoms;
    IntroPage *introPage;
    //QList<QString> getSelectedFiles();
    std::vector<std::string> getSelectedFiles();

private slots:
    void on_addFileButton_clicked();
    void on_removeFileButton_clicked();
    void on_momRadioButton_clicked();
    void on_popRadioButton_clicked();
    void on_romsRadioButton_clicked();

private:
    int nextId() const;
};

#endif // SELECTFILEPAGE_H
