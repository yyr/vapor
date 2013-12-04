//************************************************************************
//                                                                                                                                              *
//                   Copyright (C)  2013                                                                                *
//     University Corporation for Atmospheric Research                                  *
//                   All Rights Reserved                                                                                *
//                                                                                                                                              *
//************************************************************************/
//
//      File:           dataholder.h
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

#ifndef DATAHOLDER_H
#define DATAHOLDER_H

#include <iostream>
#include <vector>
#include <string>
#include <vapor/WrfVDCcreator.h>
#include <vapor/Copy2VDF.h>
//#include <vapor/wrfvdfcreate.h>
#include <vapor/vdfcreate.h>

using namespace std;
using namespace VAPoR;

//namespace VAPoR{
class DataHolder
{
public:
    DataHolder();

	//void ErrMsgCBHandler();//(const char* error, int);
	
	bool ncdfFilesChanged;
	bool vdfSettingsChanged;
	bool vdcSettingsChanged;
	vector<const char*> getErrors();
	void clearErrors();
    int createReader();

    // File selection set functions
    void setOperation(string op) { operation = op; }
    void setFileType(string type) { fileType = type; }
    void setFiles(vector<string> files) { dataFiles = files; }

    // Create vdf setter functions
    void setVDFcomment(string comment) { VDFcomment = comment; }
    void setVDFfileName(string fileName) { VDFfileName = fileName; }
    void setVDFstartTime(string startTime);
    void setVDFnumTS(string numTS);
    void setVDFcrList(string crList) { VDFcrList = crList; }
    void setVDFSBFactor(string sbFactor) { VDFSBFactor = sbFactor; }
    void setVDFPeriodicity(string periodicity) { VDFPeriodicity = periodicity; }

    void setVDFDisplayedVars(vector<string> selectedVars);// { VDFDisplayedVars = selectedVars; }
    void setVDFSelectedVars(vector<string> selectedVars);// { VDFSelectedVars = selectedVars; }
    void addVDFDisplayedVar(string var);// { VDFDisplayedVars.push_back(var); }
    void addVDFSelectedVar(string var);// { VDFSelectedVars.push_back(var); }
    void deleteVDFSelectedVar(string var);
    void clearVDFSelectedVars();// { VDFSelectedVars.clear(); }

    // Populate data setter fucntions
    void setPDVDFfile(string vdfFile) { PDinputVDFfile = vdfFile; }
    void setPDstartTime(string startTime) { PDstartTime = startTime; }
    void setPDnumTS(string numTS) { PDnumTS = numTS; }
    void setPDrefLevel(string refinement) { PDrefinement = refinement; }
    void setPDcompLevel(string compression) { PDcompression = compression; }
    void setPDnumThreads(string numThreads) { PDnumThreads = numThreads; }
    void setPDSelectedVars(vector<string> selectedVars) { PDSelectedVars = selectedVars; }
    void setPDDisplayedVars(vector<string> vars) { PDDisplayedVars = vars; }
	void clearPDSelectedVars() { PDSelectedVars.clear(); }

    // Get functions used by create VDF
    string getVDFfileName() const { return VDFfileName; }
    string getVDFnumTS() const { return VDFnumTS; }
    string getVDFStartTime() const { return VDFstartTime; }
    string getCreateVDFcmd();
	vector<string> getVDFSelectedVars() const { return VDFSelectedVars; }

    // Get functions used by Populate Data
    vector<string> getPDDisplayedVars() { return PDDisplayedVars; }
	vector<string> getPDSelectedVars() { return PDSelectedVars; }
    string getPDinputVDFfile() const { return PDinputVDFfile; }
    string getPDnumTS() const { return PDnumTS; }
    string getPDStartTime() const { return PDstartTime; }
	string getPopDataCmd();

    // Get functions (used by createVDF and Populate Data)
    vector<string> getNcdfVars() const { return ncdfVars; }
    string getOperation() const {return operation; }
    vector<string> getVDFDisplayedVars() const { return VDFDisplayedVars; }
    string getFileType() const { return fileType; }
    vector<string> getPDSelectedVars() const { return PDSelectedVars; }

    // File generation commands
    void findPopDataVars();
    int VDFCreate();
    int run2VDFcomplete();
	int run2VDFincremental(string start, string var);
    // Error Message setter/getter
    void setErrorMessage(string err) { errorMsg = err; }
    string getErrorMessage() { return errorMsg; }

private:
    Wrf2vdf w2v;
	//wrfvdfcreate launcherWrfVdfCreate;
    Copy2VDF launcher2VDF;
	vdfcreate launcherVdfCreate;

	//void launchVdfCreate(int argc, char **argv, string NetCDFtype);
    string operation;

    // Shared variables
    DCReader *reader;
    string fileType;
    vector<string> dataFiles;
    vector<string> ncdfVars;

    // Mybase Error Message
    string errorMsg;

    // Create VDF variables
    // (prefixed with VDF where necessary)
    string VDFstartTime;
    string VDFnumTS;
    string VDFSBFactor;
    string VDFcomment;
    string VDFfileName;
    string VDFcrList;
    string VDFPeriodicity;
    vector<string> VDFSelectedVars;
    vector<string> VDFDisplayedVars;

    // Populate Data variables
    // (prefixed with PD where necessary)
    string PDstartTime;
    string PDnumTS;
    string PDrefinement;
    string PDcompression;
    string PDnumThreads;
    string PDinputVDFfile;
    vector<string> PDSelectedVars;
    vector<string> PDDisplayedVars;
};
//};

#endif // DATAHOLDER_H
