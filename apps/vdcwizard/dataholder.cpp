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
//                      VDCWizard's contained pages.
//

#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iterator>
#include <cstdio>
#include <QDebug>
#include <QString>
#include <vapor/vdfcreate.h>
#include <vapor/Copy2VDF.h>
#include <vapor/MetadataVDC.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/DCReader.h>
#include <vapor/DCReaderMOM.h>
#include <vapor/DCReaderROMS.h>
#include <vapor/DCReaderWRF.h>
#include "dataholder.h"

using namespace VAPoR;
//using namespace VetsUtil;

vector<const char*> errors;

void ErrMsgCBHandler(const char* error, int){
	errors.push_back(error);
}

vector<const char*> DataHolder::getErrors() { return errors; }
void DataHolder::clearErrors() { errors.clear(); }

DataHolder::DataHolder(){
	MyBase::SetErrMsgCB(ErrMsgCBHandler);
    VDFstartTime = "0";//setVDFstartTime("0");
	PDstartTime = "0";//setPDstartTime("0");

	ncdfFilesChanged = true;
	vdfSettingsChanged = true;
	vdcSettingsChanged = true;
}


void DataHolder::setVDFstartTime(string startTime) {
	VDFstartTime = startTime;
	vdfSettingsChanged = true;
}

void DataHolder::setVDFnumTS(string numTS) {
	VDFnumTS = numTS;
	vdfSettingsChanged = true;
}

void DataHolder::setVDFDisplayedVars(vector<string> selectedVars) {
	VDFDisplayedVars = selectedVars;
	vdfSettingsChanged = true;
}

void DataHolder::setVDFSelectedVars(vector<string> selectedVars) { 
	if (VDFSelectedVars != selectedVars) vdfSettingsChanged = true;

	VDFSelectedVars = selectedVars;
	//vdfSettingsChanged = true;
}

void DataHolder::addVDFDisplayedVar(string var) { 
	VDFDisplayedVars.push_back(var); 
	vdfSettingsChanged = true;
}

void DataHolder::addVDFSelectedVar(string var) { 
	VDFSelectedVars.push_back(var); 
    vdfSettingsChanged = true;
}

void DataHolder::deleteVDFSelectedVar(string var) {
    for (int i=0;i<VDFSelectedVars.size();i++) {
        if (VDFSelectedVars[i] == var) VDFSelectedVars.erase(VDFSelectedVars.begin()+i);
    }
	vdfSettingsChanged = true;
}

void DataHolder::clearVDFSelectedVars() { 
	VDFSelectedVars.clear();
	vdfSettingsChanged = true;
}

// Create a DCReader object and pull important data from it
int DataHolder::createReader() {
	if (fileType == "roms") reader = new DCReaderROMS(dataFiles);
    else if (fileType == "wrf") reader = new DCReaderWRF(dataFiles);
	else reader = new DCReaderMOM(dataFiles);
	
	if (MyBase::GetErrCode()!=0) return 1;
	
    std::stringstream strstream;
    strstream << reader->GetNumTimeSteps();
    strstream >> VDFnumTS;
    setPDnumTS(VDFnumTS);
    ncdfVars = reader->GetVariableNames();
    std::sort(ncdfVars.begin(),ncdfVars.end());

	if (MyBase::GetErrCode()==0 ) return 0;
	else return 1;
}

void DataHolder::findPopDataVars() {
    VDFIOBase *vdfio = NULL;
    WaveCodecIO *wcwriter = NULL;
    MetadataVDC metadata(getPDinputVDFfile());
    wcwriter = new WaveCodecIO(metadata,1);
    vdfio = wcwriter;
    vector<string> emptyVars;
    vector<string> outVars;
    launcher2VDF.GetVariables(vdfio, reader, emptyVars, outVars);
    std::sort(outVars.begin(),outVars.end());
    setPDDisplayedVars(outVars);
}

// Generate an array of chars from our vector<string>, which holds
// the user's selected arguments.  momvdfcreate receives this array,
// as well as a count (argc) which is the size of the array.
int DataHolder::VDFCreate() {
    int argc = 2;
    vector<std::string> argv;
    if (getFileType() == "roms") argv.push_back("romsvdfcreate");
    else if (getFileType() == "wrf") argv.push_back("wrfvdfcreate");
	else argv.push_back("momvdfcreate");
    argv.push_back("-quiet");

    /*if ((getFileType()!="wrf")&&(VDFstartTime != "")) {
        argv.push_back("-startt");
        argv.push_back(VDFstartTime);
		argc+=2;
    }

    if ((getFileType()!="wrf")&&(VDFnumTS != "")) {
        argv.push_back("-numts");
        argv.push_back(VDFnumTS);
        argc+=2;
    }*/

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
        //cout << argv[a].c_str() << endl;
        args[a] = strdup(argv[a].c_str());
    }
	return launcherVdfCreate.launchVdfCreate(argc,args,getFileType());
}

int DataHolder::run2VDFcomplete() {
    int argc = 2;
    vector<std::string> argv;
    if (getFileType() == "roms") argv.push_back("roms2vdf");
    else if (getFileType() == "wrf") argv.push_back("wrf2vdf");
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

    char** args = new char*[ argv.size() + 1 ];
    for(size_t a=0; a<argv.size(); a++) {
        args[a] = strdup(argv[a].c_str());
    }

    return launcher2VDF.launch2vdf(argc, args, getFileType());
}

int DataHolder::run2VDFincremental(string start, string var) {
    int argc = 2;
    vector<std::string> argv;
    if (getFileType() == "roms") argv.push_back("roms2vdf");
    else if (getFileType() == "wrf") argv.push_back("wrf2vdf");
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
    
    argv.push_back("-numts");
    argv.push_back("1");
    argc+=2;
  
	argv.push_back("-startts");
    argv.push_back(start);
    argc+=2;
  
	argv.push_back("-vars");
	argv.push_back(var);
	argc+=2;
     
    for (int i=0;i<dataFiles.size();i++){
        argv.push_back(dataFiles.at(i));
        argc++;
    }   

    argv.push_back(PDinputVDFfile);
    argc++;

    char** args = new char*[ argv.size() + 1 ];
    for(size_t a=0; a<argv.size(); a++) {
        //cout << argv[a].c_str() << " ";
        args[a] = strdup(argv[a].c_str());
    }   
	//cout << endl;

    return launcher2VDF.launch2vdf(argc, args, getFileType());
}
