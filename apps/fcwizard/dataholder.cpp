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
    setVDFstartTime(1);
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

// File selection set functions
void DataHolder::setType(string type) { fileType = type; }
void DataHolder::setFiles(vector<string> files) { dataFiles = files; }

// Create vdf setter functions
void DataHolder::setVDFcomment(string comment) { VDFcomment = comment; }
void DataHolder::setVDFfileName(string fileName) { VDFfileName = fileName; }
void DataHolder::setVDFstartTime(int startTime) { VDFstartTime = startTime; }
void DataHolder::setVDFnumTS(int numTS) { VDFnumTS = numTS; }
void DataHolder::setVDFcrList(string crList) { VDFcrList = crList; }
void DataHolder::setVDFSBFactor(string sbFactor) { VDFSBFactor = sbFactor; }
void DataHolder::setVDFPeriodicity(string periodicity) { VDFPeriodicity = periodicity; }
//void DataHolder::setVDFSelectionVars(vector<string> selectionVars) { VDFSelectionVars = selectionVars; }
void DataHolder::setVDFSelectionVars(string vars) { VDFSelectionVars = vars; }

// Populate data setter fucntions
void DataHolder::setPDVDFfile(string vdfFile) { PDinputVDFfile = vdfFile; }
void DataHolder::setPDstartTime(int startTime) { PDstartTime = startTime; }
void DataHolder::setPDnumTS(int numTS) { PDnumTS = numTS; }
void DataHolder::setPDrefLevel(int refinement) { PDrefinement = refinement; }
void DataHolder::setPDcompLevel(int compression) { PDcompression = compression; }
void DataHolder::setPDnumThreads(int numThreads) { PDnumThreads = numThreads; }
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
    const char* delim = " ";
    int argc = 0;
    vector<string> argv;

    //cout << VDFcomment << endl;
    if (VDFstartTime != "") {
        argv.push_back(VDFstartTime);
        argc++;
    }
    if (VDFnumTS != "") {
        argv.push_back(VDFnumTS);
        argc++;
    }
    if (VDFSBFactor != "") {
        argv.push_back(VDFSBFactor);
        argc++;
    }
    if (VDFcomment != "") {
        argv.push_back(VDFcomment);
        argc++;
    }
    if (VDFfileName != "") {
        argv.push_back(VDFfileName);
        argc++;
    }
    if (VDFcrList != "") {
        argv.push_back(VDFcrList);
        argc++;
    }
    if (VDFPeriodicity != "") {
        argv.push_back(VDFPeriodicity);
        argc++;
    }
    /*if (VDFSelectionVars.size()>0){
        std::stringstream selectionVars;
        std::copy(VDFSelectionVars.begin(), VDFSelectionVars.end(),
                  std::ostream_iterator<std::string> (selectionVars,delim));*/
    if (VDFSelectionVars != "") {
        argv.push_back(VDFSelectionVars);
        argc++;
    }


    std::stringstream strArgv;
    std::copy(argv.begin(), argv.end(),
              std::ostream_iterator<std::string> (strArgv,delim));

    cout << strArgv.str() << endl;
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
