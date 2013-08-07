#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iterator>
#include <cstdio>
#include <QDebug>
#include <QString>

#include "dataholder.h"
#include "momvdfcreate.cpp"
//#include "mom2vdf.cpp"
//#include "romsvdfcreate.cpp"
//#include "roms2vdf.cpp"

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
    strstream >> VDFnumTS;// = reader->GetNumTimeSteps();
    fileVars = reader->GetVariableNames();
}

void DataHolder::setOperation(string op) { operation = op; }
string DataHolder::getOperation() {return operation; }

// File selection set functions
void DataHolder::setType(string type) { fileType = type; }
void DataHolder::setFiles(vector<string> files) { dataFiles = files; }

// Create vdf setter functions
void DataHolder::setVDFcomment(string comment) { VDFcomment = comment; }
void DataHolder::setVDFfileName(string fileName) { VDFfileName = fileName; }
void DataHolder::setVDFstartTime(string startTime) { VDFstartTime = startTime; }
void DataHolder::setVDFnumTS(string numTS) { VDFnumTS = numTS; }
void DataHolder::setVDFcrList(string crList) { VDFcrList = crList; }
void DataHolder::setVDFSBFactor(string sbFactor) { VDFSBFactor = sbFactor; }
void DataHolder::setVDFPeriodicity(string periodicity) { VDFPeriodicity = periodicity; }
//void DataHolder::setVDFSelectionVars(vector<string> selectionVars) { VDFSelectionVars = selectionVars; }
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
//int DataHolder::getVDFnumTS() { return VDFnumTS; }
string DataHolder::getVDFnumTS() { return VDFnumTS; }

// Get functions used by Populate Data
//int DataHolder::getPDnumTS() { return PDnumTS; }
string DataHolder::getPDnumTS() { return PDnumTS; }
//int DataHolder::getVDFStartTime() { return VDFstartTime; }
string DataHolder::getVDFStartTime() { return VDFstartTime; }

// Get functions (used by createVDF and Populate Data)
vector<string> DataHolder::getFileVars() { return fileVars; }
//vector<string> DataHolder::getVDFSelectionVars() {return VDFSelectionVars; }
string DataHolder::getVDFSelectionVars() { return VDFSelectionVars; }
string DataHolder::getPDSelectionVars() { return PDSelectionVars; }

// File generation commands
void DataHolder::runMomVDFCreate() {


    //string args = "momvdfcreate";
    const char* delim = "";
    int argc = 0;
    vector<std::string> argv;
    argv.push_back("momvdfcreate");

    //cout << VDFcomment << endl;
    if (VDFstartTime != "") {
        argv.push_back("-startt");
        argv.push_back(VDFstartTime);
        argc++;
    }
    if (VDFnumTS != "") {
        argv.push_back("-numts");
        argv.push_back(VDFnumTS);
        argc++;
    }
    if (VDFSBFactor != "") {
        argv.push_back("-bs");
        argv.push_back(VDFSBFactor);
        argc++;
    }
    if (VDFcomment != "") {
        argv.push_back("-comment");
        argv.push_back(VDFcomment);
        argc++;
    }
    if (VDFcrList != "") {
        argv.push_back("-cratios");
        argv.push_back(VDFcrList);
        argc++;
    }
    if (VDFPeriodicity != "") {
        argv.push_back("-periodic");
        argv.push_back(VDFPeriodicity);
        argc++;
    }
    /*if (VDFSelectionVars.size()>0){
        std::stringstream selectionVars;
        std::copy(VDFSelectionVars.begin(), VDFSelectionVars.end(),
                  std::ostream_iterator<std::string> (selectionVars,delim));*/
    if (VDFSelectionVars != "") {
        argv.push_back("-vars");
        argv.push_back(VDFSelectionVars);
        argc++;
    }
    for (int i=0;i<dataFiles.size();i++){
        argv.push_back(dataFiles.at(i));
    }
    if (VDFfileName != "") {
        argv.push_back(VDFfileName);
        argc++;
    }


    std::stringstream strArgv;
    std::copy(argv.begin(), argv.end(),
              std::ostream_iterator<std::string> (strArgv,delim));

    cout << argv.size();
    char** args = new char*[ argv.size() + 1 ];
    for(size_t a=0; a<argv.size(); a++) {
        args[a] = strdup(argv[a].c_str());
        cout << args[a] << endl;
    }



    //cout << a << argv[a].c_str() << argv[a].size() << argv.size() << endl;

    launchMomVdfCreate(argc,args);

    // cleanup when error happened
    //delete[] argv;

    /**********
    const char** args = new char*[argv.size()];
    for(size_t i=0;i<argv.size();++i) args[i] = argv[i].c_str();
    launchMomVdfCreate(argc,args);//strArgv.str());*/


    /**************
    char** cstr = new char*[argv.size()];
    for(int i=0;i<argv.size();i++){
        //cstr[i] = new char[argv[i].size()+1];
        //strncpy(cstr[i],argv[i].c_str(),argv[i].size());
        char* newString;
        strncpy(newString, argv[i].c_str(),argv[i].size());
        strncat(newString," ",1);
        cstr[i] = new char[argv[i].size()+2];
        //strncpy(cstr[i],argv[i].c_str(),argv[i].size());
        strncpy(cstr[i],newString,argv[i].size()+1);
    }

    for (unsigned long i=0; i<argv.size(); i++) {
        cout << cstr[i];
    }*/


    /*************
    cout << strArgv.str() << endl;
    cout << argc << endl;

    char** args = new char * [argc*2];

    for (int i=0;i<argc;i=i+2){
                    cout << "x" << endl;
        std::strcpy(args[i],argv.at(i).);

        std::strcpy(args[i+1],argv.at(i+1).c_str());
        cout << args[i] << endl << args[i+1] << endl;
    }*/



    //char* a = &argv[0];
    //launchMomVdfCreate(argc,cstr);//strArgv.str());
    /*int argc = 3;
    vector<string> args;
    args.push_back("./momvdfcreate");
    for (int i=0;i<dataFiles.size();i++) args.push_back(dataFiles.at(i));
    args.push_back(VDFfileName);

    launchMomVdfCreate(argc,args);*/



    //args += " barf";
    //cout << args;
    //qDebug() << QString::fromStdString(args);
}

void DataHolder::runRomsVDFCreate() {}
void DataHolder::runMom2VDF() {}
void DataHolder::runRoms2VDF() {}
