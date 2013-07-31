#ifndef DATAHOLDER_H
#define DATAHOLDER_H

#include <iostream>
#include <vector>
#include <string>
#include "momvdfcreate.cpp"

class DataHolder
{
public:
    DataHolder();

    //File selector set functions
    void setType(string type);
    void setFiles(vector<string> files);

    //Create vdf setter functions
    void setVDFcomment(string comment);
    void setVDFfileName(string fileName);
    void setVDFstartTime(int startTime);
    void setVDFnumTS(int numTS);
    void setVDFcrList(string crList);
    void setVDFsbFactor(int x, int y, int z);
    void setVDFAxis(bool x, bool y, bool z);
    void setVDFSelectionVars(vector<string> selectionVars);

    int getDefaultTs();

    //Populate data setter fucntions
    void setPDVDFfile(string vdfFile);
    void setPDstartTime(int startTime);
    void setPDnumTS(int numTS);
    void setPDrefLevel(int refinement);
    void setPDcompLevel(int compression);
    void setPDnumThreads(int numThreads);
    void setPDselectionVars(vector<string> selectionVars);

    //Get functions (used by createVDF and Populate Data)
    vector<string> getFileVars();
    vector<string> getSelectionVars();

private:
    // Shared variables
    string fileType;
    vector<string> files;

    // Create VDF variables
    // (prefixed with VDF where necessary)
    int VDFstartTime;
    int VDFnumTS;
    int sbx, sby, sbz;
    bool apx, apy, apz;
    string VDFcomment;
    string VDFfileName;
    string VDFcrList;
    vector<string> VDFselectionVars;

    // Populate Data variables
    // (prefixed with PD where necessary)
    int PDstartTime;
    int PDnumTS;
    int PDrefinement;
    int PDcompression;
    int PDnumThreads;
    string PDinputVDFfile;
    vector<string> PDselectionVars;
};

#endif // DATAHOLDER_H
