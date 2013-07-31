#include "dataholder.h"

//DataHolder::DataHolder()
//{

void DataHolder::setType(string type) {
    fileType = type;
}

void DataHolder::setFiles(vector<string> files) { dataFiles = files; }

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

int DataHolder::getDefaultTs() {

}

//Populate data setter fucntions
void DataHolder::setPDVDFfile(string vdfFile);
void DataHolder::setPDstartTime(int startTime);
void DataHolder::setPDnumTS(int numTS);
void DataHolder::setPDrefLevel(int refinement);
void DataHolder::setPDcompLevel(int compression);
void DataHolder::setPDnumThreads(int numThreads);
void DataHolder::setPDselectionVars(vector<string> selectionVars);

//Get functions (used by createVDF and Populate Data)
vector<string> DataHolder::getFileVars();
vector<string> DataHolder::getSelectionVars();

}
