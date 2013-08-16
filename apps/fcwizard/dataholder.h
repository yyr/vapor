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
//#include <vapor/vdfcreate.h>

using namespace std;
using namespace VAPoR;

class DataHolder
{
public:
    DataHolder();

    void createReader();

    // File selection set functions
    void setOperation(string op) { operation = op; }
    void setFileType(string type) { fileType = type; }
    void setFiles(vector<string> files) { dataFiles = files; }

    // Create vdf setter functions
    void setVDFcomment(string comment) { VDFcomment = comment; }
    void setVDFfileName(string fileName) { VDFfileName = fileName; }
    void setVDFstartTime(string startTime) { VDFstartTime = startTime; }
    void setVDFnumTS(string numTS) { VDFnumTS = numTS; }
    void setVDFcrList(string crList) { VDFcrList = crList; }
    void setVDFSBFactor(string sbFactor) { VDFSBFactor = sbFactor; }
    void setVDFPeriodicity(string periodicity) { VDFPeriodicity = periodicity; }
    void setVDFSelectionVars(string selectionVars) { VDFSelectionVars = selectionVars; }

    // Populate data setter fucntions
    void setPDVDFfile(string vdfFile) { PDinputVDFfile = vdfFile; }
    void setPDstartTime(string startTime) { PDstartTime = startTime; }
    void setPDnumTS(string numTS) { PDnumTS = numTS; }
    void setPDrefLevel(string refinement) { PDrefinement = refinement; }
    void setPDcompLevel(string compression) { PDcompression = compression; }
    void setPDnumThreads(string numThreads) { PDnumThreads = numThreads; }
    void setPDselectionVars(string selectionVars) { PDSelectionVars = selectionVars; }

    // Get functions used by create VDF
    string getVDFfileName() const { return VDFfileName; }
    string getVDFnumTS() const { return VDFnumTS; }

    // Get functions used by Populate Data
    string getPDnumTS() const { return PDnumTS; }
    string getVDFStartTime() const { return VDFstartTime; }

    // Get functions (used by createVDF and Populate Data)
    vector<string> getFileVars() const { return fileVars; }
    string getOperation() const {return operation; }
    string getVDFSelectionVars() const { return VDFSelectionVars; }
    string getPDSelectionVars() const { return PDSelectionVars; }
    string getFileType() const { return fileType; }

    // File generation commands
    void buildAndCallVdfCreate();
    //void runMomVDFCreate();
    void VDFCreate();
    void runMom2VDF();
    void runRoms2VDF();

private:
    //void launchVdfCreate(int argc, char **argv, string NetCDFtype);

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
