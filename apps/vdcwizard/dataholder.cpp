//************************************************************************
//																																			  *
//				   Copyright (C)  2013																				*
//	 University Corporation for Atmospheric Research								  *
//				   All Rights Reserved																				*
//																																			  *
//************************************************************************/
//
//	  File:		   dataholder.cpp
//
//	  Author:		 Scott Pearse
//					  National Center for Atmospheric Research
//					  PO 3000, Boulder, Colorado
//
//	  Date:		   August 2013
//
//	  Description:	Implements an entity class that holds and shares
//					  data values that are selected by the user in
//					  VDCWizard's contained pages.
//

#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iterator>
#include <cstdio>
#include <QDebug>
#include <QString>
#include <vapor/WrfVDFcreator.h>
#include <vapor/vdfcreate.h>
#include <vapor/Copy2VDF.h>
#include <vapor/MetadataVDC.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/DCReader.h>
#include <vapor/DCReaderGRIB.h>
#include <vapor/DCReaderMOM.h>
#include <vapor/DCReaderROMS.h>
#include <vapor/DCReaderWRF.h>
#include <vapor/WrfVDCcreator.h>
#include "dataholder.h"

using namespace VAPoR;

vector<const char*> errors;

void ErrMsgCBHandler(const char* error, int){
	errors.push_back(error);
}

vector<const char*> DataHolder::getErrors() { return errors; }
void DataHolder::clearErrors() { errors.clear(); }

DataHolder::DataHolder(){
	MyBase::SetErrMsgCB(ErrMsgCBHandler);
	VDFstartTime = "0";
	PDstartTime = "0";

	reader = NULL;
	ncdfFilesChanged = true;
	vdfSettingsChanged = true;
	vdcSettingsChanged = true;
}

DataHolder::~DataHolder(){
}

void DataHolder::deleteReader() {
	if (reader) delete reader;
	reader = NULL;
}

void DataHolder::getExtents(){
	reader->GetLatLonExtents(0,lonExtents,latExtents);
	for (size_t i=1;i<atoi(VDFnumTS.c_str());i++){
		double lonBuffer[2];
		double latBuffer[2];
		reader->GetLatLonExtents(i,lonBuffer,latBuffer);
		
		if (latBuffer[0]<latExtents[0]) latExtents[0]=latBuffer[0];
		if (latBuffer[1]>latExtents[1]) latExtents[1]=latBuffer[1];
		if (lonBuffer[0]<lonExtents[0]) lonExtents[0]=lonBuffer[0];
		if (lonBuffer[1]>lonExtents[1]) lonExtents[1]=lonBuffer[1];
	}
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
	if (reader==NULL){
		if (fileType == "ROMS") 
			reader = new DCReaderROMS(dataFiles);
		else if (fileType == "CAM") 
			reader = new DCReaderROMS(dataFiles);
		else if (fileType == "GRIB") 
			reader = new DCReaderGRIB(dataFiles);
		else if (fileType == "WRF") 
			reader = new DCReaderWRF(dataFiles);
		else 
			reader = new DCReaderMOM(dataFiles);
	
		if (MyBase::GetErrCode()!=0) return 1;
	
		std::stringstream strstream;
		strstream << reader->GetNumTimeSteps();
		strstream >> VDFnumTS;
		setPDnumTS(VDFnumTS);
		ncdfVars = reader->GetVariableNames();
		std::sort(ncdfVars.begin(),ncdfVars.end());

		if (MyBase::GetErrCode()!=0) return 1;
	}
	return 0;
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
	delete wcwriter;
}

string DataHolder::getCreateVDFcmd() {
	vector<std::string> argv;
	if (getFileType() == "ROMS") argv.push_back("romsvdfcreate");
	else if (getFileType() == "CAM") argv.push_back("camvdfcreate");
	else if (getFileType() == "WRF") argv.push_back("wrfvdfcreate");//Users/pears
	else if (getFileType() == "GRIB") argv.push_back("gribvdfcreate");
	else argv.push_back("momvdfcreate");
	argv.push_back("-quiet");
	if (getFileType()=="WRF") argv.push_back("-vdc2");

	if (VDFSBFactor != "") {
		argv.push_back("-bs");
		argv.push_back(VDFSBFactor);
	}   
	if (VDFcomment != "") {
		argv.push_back("-comment");
		argv.push_back(VDFcomment);
	}   
	if (VDFcrList != "") {
		argv.push_back("-cratios");
		argv.push_back(VDFcrList);
	}   
	if (VDFPeriodicity != "") {
		argv.push_back("-periodic");
		argv.push_back(VDFPeriodicity);
	}   
	if (VDFSelectedVars.size() != 0) {
		argv.push_back("-vars");

		string stringVars;
		for(vector<string>::iterator it = VDFSelectedVars.begin();
			it != VDFSelectedVars.end(); ++it) {
			if(it != VDFSelectedVars.begin()) stringVars += ":";
			stringVars += *it;
		}   
		argv.push_back(stringVars);
	}   

	for (int i=0;i<dataFiles.size();i++){
		argv.push_back(dataFiles.at(i));
	}   
	   
	if (VDFfileName != "") {
		argv.push_back(VDFfileName);
	}   


	std::ostringstream imploded;
	const char* const delim = " ";
	std::copy(argv.begin(), argv.end(),std::ostream_iterator<string>(imploded,delim));

	return imploded.str();
}

string DataHolder::getPopDataCmd() {
	vector<std::string> argv;
	if (getFileType() == "ROMS") argv.push_back("roms2vdf");
	else if (getFileType() == "GRIB") argv.push_back("grib2vdf");
	else if (getFileType() == "CAM") argv.push_back("cam2vdf");
	else if (getFileType() == "WRF") argv.push_back("wrf2vdf");
	else argv.push_back("mom2vdf");
	argv.push_back("-quiet");

	if (PDrefinement != "") {
		argv.push_back("-level");
		argv.push_back(PDrefinement);
	}   
	if (PDcompression != "") {
		argv.push_back("-lod");
		argv.push_back(PDcompression);
	}   
	if (PDnumThreads != "") {
		argv.push_back("-nthreads");
		argv.push_back(PDnumThreads);
	}   

	if (getFileType() != "GRIB") {	
		argv.push_back("-numts");
		argv.push_back(PDnumTS);
  
		argv.push_back("-startts");
		argv.push_back(PDstartTime);
	}  

	if (PDSelectedVars.size() != 0) {
		argv.push_back("-vars");

		if (getFileType()!="MOM"){
			// If ELEVATION var is already included
			if (std::find(PDSelectedVars.begin(), PDSelectedVars.end(), "ELEVATION") == PDSelectedVars.end()) {
				PDSelectedVars.push_back("ELEVATION");
			}			
		}

		string stringVars;
		for(vector<string>::iterator it = PDSelectedVars.begin();
			it != PDSelectedVars.end(); ++it) {
			if(it != PDSelectedVars.begin()) stringVars += ":";
			stringVars += *it;
		}   
		argv.push_back(stringVars);
	}
 
	if (getFileType()=="WRF"){
		argv.push_back(PDinputVDFfile);
	}   

 
	for (int i=0;i<dataFiles.size();i++){
		argv.push_back(dataFiles.at(i));
	}   

	if (getFileType() != "WRF"){
		argv.push_back(PDinputVDFfile);
	}   

	char** args = new char*[ argv.size() + 1 ];
	for(size_t a=0; a<argv.size(); a++) {
		args[a] = strdup(argv[a].c_str());
	}

	std::ostringstream imploded;
	const char* const delim = " ";
	std::copy(argv.begin(), argv.end(), std::ostream_iterator<string>(imploded,delim));

	return imploded.str();
}


// Generate an array of chars from our vector<string>, which holds
// the user's selected arguments.  momvdfcreate receives this array,
// as well as a count (argc) which is the size of the array.
int DataHolder::VDFCreate() {
	int argc = 2;
	vector<std::string> argv;
	if (getFileType() == "ROMS") argv.push_back("romsvdfcreate");
	else if (getFileType() == "CAM") argv.push_back("camvdfcreate");
	else if (getFileType() == "GRIB") argv.push_back("gribvdfcreate");
	else if (getFileType() == "WRF") argv.push_back("wrfvdfcreate");
	else argv.push_back("momvdfcreate");
	argv.push_back("-quiet");
	
	if (getFileType() == "WRF")	{
		argv.push_back("-vdc2");
		argc++;
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

		if (getFileType()!="MOM"){
			// If ELEVATION var is already included
			if (std::find(PDSelectedVars.begin(), PDSelectedVars.end(), "ELEVATION") == PDSelectedVars.end()) {
				PDSelectedVars.push_back("ELEVATION");
			}			   
		}   

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
		args[a] = strdup(argv[a].c_str());
	}

	if (getFileType()=="WRF") {
		wrfvdfcreate launcherWrfVdfCreate;
		int rc = launcherWrfVdfCreate.launchVdfCreate(argc,args);
		return rc;
	}
	else return launcherVdfCreate.launchVdfCreate(argc,args,getFileType());
}

int DataHolder::run2VDFcomplete() {
	int argc = 2;
	vector<std::string> argv;
	if (getFileType() == "ROMS") argv.push_back("roms2vdf");
	else if (getFileType() == "CAM") argv.push_back("cam2vdf");
	else if (getFileType() == "GRIB") argv.push_back("grib2vdf");
	else if (getFileType() == "WRF") argv.push_back("wrf2vdf");
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
		argv.push_back("-nthreads");
		argv.push_back(PDnumThreads);
		argc+=2;
	}

	if (getFileType() != "GRIB") {
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
	}

	if (PDSelectedVars.size() != 0) {
		argv.push_back("-vars");
		argc++;

		if (getFileType()!="MOM"){
			// If ELEVATION var is already included
			if (std::find(PDSelectedVars.begin(), PDSelectedVars.end(), "ELEVATION") == PDSelectedVars.end()) {
				PDSelectedVars.push_back("ELEVATION");
			}			   
		}   

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

	return launcher2VDF.launch2vdf(argc, args, getFileType(), reader);
}

int DataHolder::run2VDFincremental(string start, string var) {
	int argc = 2;
	vector<std::string> argv;
	if (getFileType() == "ROMS") argv.push_back("roms2vdf");
	else if (getFileType() == "CAM") argv.push_back("cam2vdf");
	else if (getFileType() == "GRIB") argv.push_back("grib2vdf");
	else if (getFileType() == "WRF") argv.push_back("wrf2vdf");
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
		argv.push_back("-nthreads");
		argv.push_back(PDnumThreads);
		argc+=2;
	}   

	if (getFileType() != "GRIB") {	
		argv.push_back("-numts");
		argv.push_back("1");
		argc+=2;
  
		argv.push_back("-startts");
		argv.push_back(start);
		argc+=2;
	}
  
	argv.push_back("-vars");
	argc++;
	argv.push_back(var);
	argc++;
 
	if (getFileType()=="WRF"){
		argv.push_back(PDinputVDFfile);
		argc++;
	}

 
	for (int i=0;i<dataFiles.size();i++){
		argv.push_back(dataFiles.at(i));
		argc++;
	}   

	if (getFileType() != "WRF"){
		argv.push_back(PDinputVDFfile);
		argc++;
	}

	//cout << "COMMAND: " << endl;
	char** args = new char*[ argv.size() + 1 ];
	for(size_t a=0; a<argv.size(); a++) {
	//	cout << argv[a].c_str() << " ";
		args[a] = strdup(argv[a].c_str());
	}   
	//cout << endl;
	
	int rc;
	if (getFileType()=="WRF") {
		//DCReaderWRF *foo = dynamic_cast <DCReaderWRF*>(reader);
		rc = w2v.launchWrf2Vdf(argc, args, (DCReaderWRF*) reader);
	}
	else {
		rc = launcher2VDF.launch2vdf(argc, args, getFileType(), reader);
	}

	if (args) delete [] args;
	return rc;
}

void DataHolder::purgeObjects() {
	launcher2VDF.deleteObjects();
	w2v.deleteObjects();
}
