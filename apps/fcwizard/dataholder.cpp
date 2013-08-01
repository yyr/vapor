#include "dataholder.h"
#include "momvdfcreate.cpp"
#include <iostream>
#include <cstdio>
#include <QDebug>
//using namespace VAPoR;

DataHolder::DataHolder(){
    setVDFstartTime(10);
}

void DataHolder::createReader() {
    if (fileType == "roms") reader = new DCReaderROMS(dataFiles);
    else reader = new DCReaderMOM(dataFiles);
    //reader = new DCReaderMOM(dataFiles);
    //int test = reader->GetNumTimeSteps();
}

void DataHolder::setType(string type) {
    qDebug() << "got to dataholder";
    //cout << "test for cout";
    fileType = type;

    //fileVars = reader->GetVariableNames();
    //setVDFnumTS(reader->GetNumTimeSteps());
    //cout << fileVars.size();
}

void DataHolder::setFiles(vector<string> files) {
    dataFiles = files;
    cout << files.at(0);
    qDebug() << "got to setfiles";


    //setType(fileType);
    //delete reader;
    //if (fileType == "roms") reader = new DCReaderROMS(dataFiles);
    //else reader = new DCReaderMOM(dataFiles);


    /*int count = fileVars.size();
    for (int i=0;i<count;i++) cout << fileVars.at(i);

    setVDFSelectionVars(fileVars);
    setVDFnumTS(reader->GetNumTimeSteps());*/
}

//Create vdf setter functions
void DataHolder::setVDFcomment(string comment) { VDFcomment = comment; }

void DataHolder::setVDFfileName(string fileName) { VDFfileName = fileName; }

void DataHolder::setVDFstartTime(int startTime) { VDFstartTime = startTime; }

void DataHolder::setVDFnumTS(int numTS) { VDFnumTS = numTS; }

void DataHolder::setVDFcrList(string crList) { VDFcrList = crList; }

void DataHolder::setVDFsbFactor(int x, int y, int z) {
    sbx = x;
    sby = y;
    sbz = z;
}

void DataHolder::setVDFAxis(bool x, bool y, bool z) {
    apx = x;
    apy = y;
    apz = z;
}

void DataHolder::setVDFSelectionVars(vector<string> selectionVars) {
    VDFSelectionVars = selectionVars;
}

//Populate data setter fucntions
void DataHolder::setPDVDFfile(string vdfFile) {
    PDinputVDFfile = vdfFile;
}

void DataHolder::setPDstartTime(int startTime) {
    PDstartTime = startTime;
}

void DataHolder::setPDnumTS(int numTS) {
    PDnumTS = numTS;
}

void DataHolder::setPDrefLevel(int refinement) {
    PDrefinement = refinement;
}

void DataHolder::setPDcompLevel(int compression) {
    PDcompression = compression;
}

void DataHolder::setPDnumThreads(int numThreads) {
    PDnumThreads = numThreads;
}

void DataHolder::setPDselectionVars(vector<string> selectionVars) {
    PDSelectionVars = selectionVars;
}

// Get functions used by create VDF

int DataHolder::getVDFnumTS() {
    return VDFnumTS;
}

// Get functions used by Populate Data

int DataHolder::getPDnumTS() {
    return PDnumTS;
}

int DataHolder::getVDFStartTime() {
    return VDFstartTime;
}

//Get functions (used by createVDF and Populate Data)
vector<string> DataHolder::getFileVars() { return fileVars; }
vector<string> DataHolder::getVDFSelectionVars() {return VDFSelectionVars; }
vector<string> DataHolder::getPDSelectionVars() { return PDSelectionVars; }
