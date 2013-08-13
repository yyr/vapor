//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           dataholder.h
//
//      Author:         Scott Pearse
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2013
//
//      Description:    Implements an entity class that holds and shares
//                      data values that are selected by the user in
//                      FCWizard's contained pages.
//

#ifndef DATAHOLDER_H
#define DATAHOLDER_H

#include <iostream>
#include <vector>
#include <string>
#include <vapor/DCReaderMOM.h>
#include <vapor/DCReaderROMS.h>

using namespace std;
using namespace VAPoR;

class DataHolder
{
public:
    DataHolder();

    void createReader();

    // File selection set functions
    void setOperation(string op);
    void setFileType(string type);
    void setFiles(vector<string> files);

    // Create vdf setter functions
    void setVDFcomment(string comment);
    void setVDFfileName(string fileName);
    void setVDFstartTime(string startTime);
    void setVDFnumTS(string numTS);
    void setVDFcrList(string crList);
    void setVDFSBFactor(string sbFactor);
    void setVDFPeriodicity(string periodicity);
    void setVDFSelectionVars(string selectionVars);

    // Populate data setter fucntions
    void setPDVDFfile(string vdfFile);
    void setPDstartTime(string startTime);
    void setPDnumTS(string numTS);
    void setPDrefLevel(string refinement);
    void setPDcompLevel(string compression);
    void setPDnumThreads(string numThreads);
    void setPDselectionVars(string selectionVars);

    // Get functions used by create VDF
    string getVDFfileName();
    string getVDFnumTS();

    // Get functions used by Populate Data
    string getPDnumTS();
    string getVDFStartTime();

    // Get functions (used by createVDF and Populate Data)
    vector<string> getFileVars();
    string getOperation();
    string getVDFSelectionVars();
    string getPDSelectionVars();
    string getFileType();

    // File generation commands
    void runMomVDFCreate();
    void runRomsVDFCreate();
    void runMom2VDF();
    void runRoms2VDF();

private:
    string operation;

    // Shared variables
    DCReader *reader;
    string fileType;
    vector<string> dataFiles;
    vector<string> fileVars;

    // Create VDF variables
    // (prefixed with VDF where necessary)
    string VDFstartTime;
    string VDFnumTS;
    string VDFSBFactor;
    string VDFcomment;
    string VDFfileName;
    string VDFcrList;
    string VDFPeriodicity;
    string VDFSelectionVars;

    // Populate Data variables
    // (prefixed with PD where necessary)
    string PDstartTime;
    string PDnumTS;
    string PDrefinement;
    string PDcompression;
    string PDnumThreads;
    string PDinputVDFfile;
    string PDSelectionVars;
};

#endif // DATAHOLDER_H
