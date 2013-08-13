//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           dataholder.cpp
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

#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iterator>
#include <cstdio>
#include <QDebug>
#include <QString>

#include "dataholder.h"
#include "momvdfcreate.cpp"

using namespace VAPoR;
using namespace VetsUtil;

DataHolder::DataHolder(){
    setVDFstartTime("1");
}

// Create a DCReader object and pull important data from it
void DataHolder::createReader() {
    if (fileType == "roms") reader = new DCReaderROMS(dataFiles);
    else reader = new DCReaderMOM(dataFiles);

    if (MyBase::GetErrCode() != 0) qDebug() << MyBase::GetErrMsg();

    std::stringstream strstream;
    strstream << reader->GetNumTimeSteps();
    strstream >> VDFnumTS;
    fileVars = reader->GetVariableNames();
}

// File selection set functions
void DataHolder::setOperation(string op) { operation = op; }
void DataHolder::setFileType(string type) { fileType = type; }
void DataHolder::setFiles(vector<string> files) { dataFiles = files; }

// Create vdf setter functions
void DataHolder::setVDFcomment(string comment) { VDFcomment = comment; }
void DataHolder::setVDFfileName(string fileName) { VDFfileName = fileName; }
void DataHolder::setVDFstartTime(string startTime) { VDFstartTime = startTime; }
void DataHolder::setVDFnumTS(string numTS) { VDFnumTS = numTS; }
void DataHolder::setVDFcrList(string crList) { VDFcrList = crList; }
void DataHolder::setVDFSBFactor(string sbFactor) { VDFSBFactor = sbFactor; }
void DataHolder::setVDFPeriodicity(string periodicity) { VDFPeriodicity = periodicity; }
void DataHolder::setVDFSelectionVars(string vars) { VDFSelectionVars = vars; }

// Populate data setter fucntions
void DataHolder::setPDVDFfile(string vdfFile) { PDinputVDFfile = vdfFile; }
void DataHolder::setPDstartTime(string startTime) { PDstartTime = startTime; }
void DataHolder::setPDnumTS(string numTS) { PDnumTS = numTS; }
void DataHolder::setPDrefLevel(string refinement) { PDrefinement = refinement; }
void DataHolder::setPDcompLevel(string compression) { PDcompression = compression; }
void DataHolder::setPDnumThreads(string numThreads) { PDnumThreads = numThreads; }
void DataHolder::setPDselectionVars(string selectionVars) { PDSelectionVars = selectionVars; }

// Get functions used by create VDF
string DataHolder::getVDFfileName() { return VDFfileName; }
string DataHolder::getVDFnumTS() { return VDFnumTS; }

// Get functions used by Populate Data
string DataHolder::getPDnumTS() { return PDnumTS; }
string DataHolder::getVDFStartTime() { return VDFstartTime; }

// Get functions (used by createVDF and Populate Data)
vector<string> DataHolder::getFileVars() { return fileVars; }
string DataHolder::getOperation() {return operation; }
string DataHolder::getVDFSelectionVars() { return VDFSelectionVars; }
string DataHolder::getPDSelectionVars() { return PDSelectionVars; }
string DataHolder::getFileType() { return fileType; }

// Generate an array of chars from our vector<string>, which holds
// the user's selected arguments.  momvdfcreate receives this array,
// as well as a count (argc) which is the size of the array.
void DataHolder::runMomVDFCreate() {
    const char* delim = "";
    int argc = 2;
    vector<std::string> argv;
    argv.push_back("momvdfcreate");
    argv.push_back("-quiet");

    if (VDFstartTime != "") {
        argv.push_back("-startt");
        argv.push_back(VDFstartTime);
        argc+=2;
    }
    if (VDFnumTS != "") {
        argv.push_back("-numts");
        argv.push_back(VDFnumTS);
        argc+=2;
    }
    if (VDFSBFactor != "") {
        argv.push_back("-bs");
        argv.push_back(VDFSBFactor);
        argc+=2;
    }
    if (VDFcomment != "") {
        argv.push_back("-comment");
        argv.push_back(VDFcomment);
        argc+=2;
    }
    if (VDFcrList != "") {
        argv.push_back("-cratios");
        argv.push_back(VDFcrList);
        argc+=2;
    }
    if (VDFPeriodicity != "") {
        argv.push_back("-periodic");
        argv.push_back(VDFPeriodicity);
        argc+=2;
    }
    if (VDFSelectionVars != "") {
        argv.push_back("-vars");
        argv.push_back(VDFSelectionVars);
        argc+=2;
    }
    for (int i=0;i<dataFiles.size();i++){
        argv.push_back(dataFiles.at(i));
        argc++;
    }
    if (VDFfileName != "") {
        argv.push_back(VDFfileName);
        argc++;
    }

    std::stringstream strArgv;
    std::copy(argv.begin(), argv.end(),
              std::ostream_iterator<std::string> (strArgv,delim));

    char** args = new char*[ argv.size() + 1 ];
    for(size_t a=0; a<argv.size(); a++) {
        args[a] = strdup(argv[a].c_str());
    }

    launchMomVdfCreate(argc,args);
}

// To be completed in the future
void DataHolder::runRomsVDFCreate() {}
void DataHolder::runMom2VDF() {}
void DataHolder::runRoms2VDF() {}
