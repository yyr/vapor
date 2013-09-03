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
#include <vapor/2vdf.h>
#include <vapor/MetadataVDC.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/DCreader.h>
#include <vapor/DCReaderMOM.h>
#include <vapor/DCReaderROMS.h>
#include "dataholder.h"

using namespace VAPoR;
//using namespace VetsUtil;

DataHolder::DataHolder(){
    setVDFstartTime("0");
    setPDstartTime("0");
}

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
    setPDnumTS(VDFnumTS);
    ncdfVars = reader->GetVariableNames();
    std::sort(ncdfVars.begin(),ncdfVars.end());
}

void DataHolder::findPopDataVars() {
    VDFIOBase *vdfio = NULL;
    WaveCodecIO *wcwriter = NULL;
    MetadataVDC metadata(getPDinputVDFfile());
    wcwriter = new WaveCodecIO(metadata,1);
    vdfio = wcwriter;
    vector<string> emptyVars;
    vector<string> outVars;
    createReader();
    GetVariables(vdfio, reader, emptyVars, outVars);
    std::sort(outVars.begin(),outVars.end());
    setPDDisplayedVars(outVars);
}

// Generate an array of chars from our vector<string>, which holds
// the user's selected arguments.  momvdfcreate receives this array,
// as well as a count (argc) which is the size of the array.
void DataHolder::VDFCreate() {
    //const char* delim = "";
    int argc = 2;
    vector<std::string> argv;
    if (getFileType() == "roms") argv.push_back("romsvdfcreate");
    else argv.push_back("momvdfcreate");
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
    if (VDFSelectedVars.size() != 0) {
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
    }

    for (int i=0;i<dataFiles.size();i++){
        argv.push_back(dataFiles.at(i));
        argc++;
    }
    if (VDFfileName != "") {
        argv.push_back(VDFfileName);
        argc++;
    }

    char** args = new char*[ argv.size() + 1 ];
    for(size_t a=0; a<argv.size(); a++) {
        cout << argv[a].c_str() << endl;
        args[a] = strdup(argv[a].c_str());
    }

    launchVdfCreate(argc,args,getFileType());
}

// To be completed in the future
//void DataHolder::runRomsVDFCreate() {}
void DataHolder::run2VDF() {
    int argc = 2;
    vector<std::string> argv;
    if (getFileType() == "roms") argv.push_back("roms2vdf");
    else argv.push_back("mom2vdf");
    argv.push_back("-quiet");

    if (PDrefinement != "") {
        argv.push_back("-level");
        argv.push_back(PDrefinement);
        argc+=2;
    }
    if (PDcompression != "") {
        argv.push_back("-lod");
        argv.push_back(PDcompression);
        argc+=2;
    }
    if (PDnumThreads != "") {
        argv.push_back("-numthreads");
        argv.push_back(PDnumThreads);
        argc+=2;
    }
    if (PDnumTS != "") {
        argv.push_back("-numts");
        argv.push_back(PDnumTS);
        argc+=2;
    }
    if (PDstartTime != "") {
        argv.push_back("-startts");
        argv.push_back(PDstartTime);
        argc+=2;
    }
    if (PDSelectedVars.size() != 0) {
        argv.push_back("-vars");
        argc++;

        string stringVars;
        for(vector<string>::iterator it = PDSelectedVars.begin();
            it != PDSelectedVars.end(); ++it) {
            if(it != PDSelectedVars.begin()) stringVars += ":";
            stringVars += *it;
        }
        argv.push_back(stringVars);
        argc++;
    }

    for (int i=0;i<dataFiles.size();i++){
        argv.push_back(dataFiles.at(i));
        argc++;
    }

    argv.push_back(PDinputVDFfile);
    argc++;

    cout << endl;
    cout << argc << endl;
    char** args = new char*[ argv.size() + 1 ];
    for(size_t a=0; a<argv.size(); a++) {
        cout << argv[a].c_str() << endl;
        args[a] = strdup(argv[a].c_str());
    }


    launch2vdf(argc, args, getFileType());
}
