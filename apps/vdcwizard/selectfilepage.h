//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           selectfilepage.h
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    A QWizardPage that steps the user through selecting
//                      which vdf file to write/include for their processing,
//                      which NetCDF files to reference, and what their
//

#ifndef SELECTFILEPAGE_H
#define SELECTFILEPAGE_H

#include <QtGui/QtGui>
#include <QWizardPage>
#include "ui/Page2.h"
#include "intropage.h"
#include "dataholder.h"
#include "vdfbadfile.h"
#include "errormessage.h"

namespace Ui {
class SelectFilePage;
}

class SelectFilePage : public QWizardPage, public Ui_Page2
{
    Q_OBJECT
    
public:
    explicit SelectFilePage(DataHolder *DH, QWidget *parent = 0);

	bool validatePage();
    //VAPoR::DCReader *fileData;
    string momPopOrRoms;
    IntroPage *introPage;
    DataHolder *dataHolder;
    ErrorMessage *errorMessage;
	VdfBadFile *vdfBadFile;
	vector<string> stdFileList;
    vector<string> getSelectedFiles();
    QPixmap vdfCreatePixmap;
    QPixmap toVdfPixmap;
	QString selectedDirectory;
	int Complete;
	bool isComplete() const;

private slots:
    void on_browseOutputVdfFile_clicked();
    void on_outputVDFtext_textChanged();
    void on_addFileButton_clicked();
    void on_removeFileButton_clicked();
    void on_dataTypeComboBox_currentIndexChanged(const QString &dataType);
	//void on_momRadioButton_clicked();
    //void on_popRadioButton_clicked();
    //void on_romsRadioButton_clicked();
	//void on_wrfRadioButton_clicked();

private:
	int nextId() const;
    void cleanupPage();
    void initializePage();
};

#endif // SELECTFILEPAGE_H
