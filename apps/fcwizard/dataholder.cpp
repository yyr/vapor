#include <stdlib.h>
#include <stdio.h>
#include <cstdio>
#include <QDebug>
#include <QString>

#include "dataholder.h"
#include "momvdfcreate.cpp"

//using namespace VAPoR;

DataHolder::DataHolder(){
    setVDFstartTime(1);

    //dataFiles.push_back("/Users/pearse/Documents/vaporTestData/00010101.ocean_month.NWPp2.Clim.nc");
    //testReader = new DCReaderMOM(dataFiles);
    //testReader->GetVariableNames();//testReader->GetNumTimeSteps() << endl;
}

void DataHolder::createReader() {
    if (fileType == "roms") reader = new DCReaderROMS(dataFiles);
    else reader = new DCReaderMOM(dataFiles);

    if (MyBase::GetErrCode() != 0) qDebug() << MyBase::GetErrMsg();
    VDFnumTS = reader->GetNumTimeSteps();
}

void DataHolder::setType(string type) {
    fileType = type;
}

void DataHolder::setFiles(vector<string> files) {
    dataFiles = files;
    //qDebug() << QString::fromStdString(files.at(0));
}

//Create vdf setter functions
void DataHolder::setVDFcomment(string comment) { VDFcomment = comment; }

void DataHolder::setVDFfileName(string fileName) { VDFfileName = fileName; }

void DataHolder::setVDFstartTime(int startTime) { VDFstartTime = startTime; }

void DataHolder::setVDFnumTS(int numTS) { VDFnumTS = numTS; }

void DataHolder::setVDFcrList(string crList) { VDFcrList = crList; }

void DataHolder::setVDFSBFactor(string sbFactor) { VDFSBFactor = sbFactor; }

void DataHolder::setVDFPeriodicity(string periodicity) { VDFPeriodicity = periodicity; }

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
