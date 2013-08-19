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
#include <vapor/vdfcreate.h>

#include "dataholder.h"
//#include "momvdfcreate.cpp"

using namespace VAPoR;
using namespace VetsUtil;

DataHolder::DataHolder(){
    setVDFstartTime("1");
}

/*void DataHolder::addVDFSelectionVar(string var) {
    //ncdfVars.push_back(var);
    VDFVarBuffer = var;
}*/

void DataHolder::deleteVDFSelectedVar(string var) {
    for (int i=0;i<VDFSelectedVars.size();i++) {
        if (VDFSelectedVars[i] == var) VDFSelectedVars.erase(VDFSelectedVars.begin()+i);
    }
}

// Create a DCReader object and pull important data from it
void DataHolder::createReader() {
    if (fileType == "roms") reader = new DCReaderROMS(dataFiles);
    else reader = new DCReaderMOM(dataFiles);

    if (MyBase::GetErrCode() != 0) qDebug() << MyBase::GetErrMsg();

    std::stringstream strstream;
    strstream << reader->GetNumTimeSteps();
    strstream >> VDFnumTS;
    ncdfVars = reader->GetVariableNames();
}

// Generate an array of chars from our vector<string>, which holds
// the user's selected arguments.  momvdfcreate receives this array,
// as well as a count (argc) which is the size of the array.
void DataHolder::VDFCreate() {
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
    if (VDFSelectedVars.size() != 0) { // != "") {
        argv.push_back("-vars");
        argc++;

        string stringVars;
        for(vector<string>::iterator it = VDFSelectedVars.begin();
            it != VDFSelectedVars.end(); ++it) {
            if(it != VDFSelectedVars.begin()) stringVars += ":";
            stringVars += *it;
        }
        argv.push_back(stringVars);
        argc++;
        /*for(int i=0;i<VDFSelectedVars.size();i++) {
            argv.push_back(VDFSelectedVars[i]);
            //cout << VDFSelectedVars[i] << endl;
            argc++;
        }*/
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
        cout << argv[a].c_str() << endl;
        args[a] = strdup(argv[a].c_str());
    }

    launchVdfCreate(argc,args,getFileType());
}

// To be completed in the future
//void DataHolder::runRomsVDFCreate() {}
void DataHolder::runMom2VDF() {}
void DataHolder::runRoms2VDF() {}
