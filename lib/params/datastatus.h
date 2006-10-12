//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		datastatus.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		February 2006
//
//	Description:	Defines the DataStatus class.  
//  This class maintains information about the data that is currently
//  loaded.  Maintained and accessed mostly through the Session
#ifndef DATASTATUS_H
#define DATASTATUS_H


#include <vector>
#include <string>
#include <map>
#include "vaporinternal/common.h"

#include "vapor/VDFIOBase.h"
#include "vapor/MyBase.h"

namespace VAPoR {
class DataMgr;
class VDFIOBase;
class Metadata;
// Class used by session to keep track of variables, timesteps, resolutions, datarange.
// It is constructed by the Session whenever a new metadata is opened.
// It keeps a value of min/max data for each timestep that can be refreshed.
// Variables can be referenced either by the varNum (numbering all the variables in
// the session) or by the varName.  
class PARAMS_API DataStatus : public VetsUtil::MyBase{
public:
	
	DataStatus();
	~DataStatus();
	static DataStatus* getInstance() {
		if (!theDataStatus) theDataStatus = new DataStatus;
		return theDataStatus;
	}
	//Reset the datastatus when a new datamgr is opened.
	//This avoids all "Set" methods:
	bool reset(DataMgr* dm, size_t cachesize);
	
	// Get methods:
	//
	const float* getExtents() {return extents;}
	size_t getCacheMB() {return cacheMB;}
	size_t getMinTimestep() {return minTimeStep;}
	size_t getMaxTimestep() {return maxTimeStep;}
	bool dataIsPresent(int varnum, int timestep){
		if (!theDataStatus) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		if (!variableExists[varnum]) return false;
		return (maxNumTransforms[varnum][timestep] >= 0);
	}
	
	double getDataMax(int varNum, int timestep){
		if (!dataIsPresent(varNum, timestep))return 1.0f;
		if (dataMax[varNum][timestep] == -1.e30f)
			calcDataRange(varNum,timestep);
		return dataMax[varNum][timestep];
	}
	double getDataMin(int varNum, int timestep){
		if (!dataIsPresent(varNum, timestep))return -1.0f;
		if (dataMin[varNum][timestep] == 1.e30f)
			calcDataRange(varNum,timestep);
		return dataMin[varNum][timestep];
	}
	double getDefaultDataMax(int varnum){return getDataMax(varnum, (int)minTimeStep);}
	double getDefaultDataMin(int varnum){return getDataMin(varnum, (int)minTimeStep);}
	
	int maxXFormPresent(int varnum, int timestep){
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return -1;
		if (!variableExists[varnum]) return -1;
		return (maxNumTransforms[varnum][timestep]);
	}
	//Determine if variable is present for *any* timestep 
	//Needed for setting DVR panel
	bool variableIsPresent(int varnum) {return variableExists[varnum];}
	
	int getNumTimesteps() {return numTimesteps;}
	//determine the maxnumtransforms in the vdf, may not actually have any data at
	//that level...
	int getNumTransforms() {return numTransforms;}
	//Find the first timestep that has any data
	int getFirstTimestep(int varnum);
	size_t getFullDataSize(int dim){return fullDataSize[dim];}
	const size_t* getFullDataSize() {return fullDataSize;}
	void mapVoxelToUserCoords(int refLevel, const size_t voxCoords[3], double userCoords[3]){
		myReader->MapVoxToUser(0, voxCoords, userCoords, refLevel);
	}
	const VDFIOBase* getRegionReader() {return myReader;}
	const Metadata* getCurrentMetadata() {return currentMetadata;}
	DataMgr* getDataMgr() {return dataMgr;}
	Metadata* getMetadata() {return currentMetadata;}

		
	static std::string& getVariableName(int varNum) {return variableNames[varNum];}
	//Find the session num of a name, or -1 if it's not metadata:
	static int getSessionVariableNum(const std::string& str);
	//Insert variableName if necessary; return index
	static int mergeVariableName(const std::string& str);


	static void addVarName(const std::string newName) {variableNames.push_back(newName);}
	//"Metadata" variables are those that are in current metadata, as opposed to
	//"real" variables are those in session
	static int getNumMetadataVariables() {return numMetadataVariables;}
	static int mapMetadataToRealVarNum(int varnum) 
		{ if(!mapMetadataVars) return 0; 
		return mapMetadataVars[varnum];}
	static int mapRealToMetadataVarNum(int var);
	static std::string& getMetadataVarName(int varnum) {
		if (!mapMetadataVars) return variableNames[0];
		return (variableNames[mapMetadataVars[varnum]]);}
	static int getNumVariables(){return (int)variableNames.size();}
	static void clearVariableNames() {variableNames.clear();}
	static void removeMetadataVars(){
		if (mapMetadataVars) delete mapMetadataVars;
		clearMetadataVars();
	}
	void fillMetadataVars();
	static void clearMetadataVars() {mapMetadataVars = 0; numMetadataVariables = 0;}
	//Get full data extents in cube coords
	void getMaxExtentsInCube(float maxExtents[3]);
	bool renderReady() {return renderOK;}
	void setRenderReady(bool nowOK) {renderOK = nowOK;}
		
	
private:
	static DataStatus* theDataStatus;
	//Cache size in megabytes
	size_t cacheMB;
	void calcDataRange(int varnum, int ts);
	//Identify if a variable is active:
	std::vector<bool> variableExists;
	//for each int variable there is an int vector of num transforms for each time step.
	//value is -1 if no data at that timestep
	std::vector<int*> maxNumTransforms;
	Metadata* currentMetadata;
	DataMgr* dataMgr;
	bool renderOK;
	
	//track min and max data values for each variable and timestep (at max transform level)
	std::vector<float*> dataMin;
	std::vector<float*> dataMax;
	//specify the minimum and max time step that actually have data:
	size_t minTimeStep;
	size_t maxTimeStep;
	int numTransforms;
	//numTimeSteps may include lots of times that are not used. 
	int numTimesteps;
	//numVariables is total number of variables in the session:
	//int numVariables;
	
	size_t fullDataSize[3];
	float extents[6];
	VDFIOBase* myReader;

	// the variableNames array specifies the name associated with each variable num.
	//Note that this
	//contains (possibly properly) the corresponding variableNames in the
	//dataStatus.  The number of metadataVariables should coincide with the 
	//number of variables in the datastatus that have actual data associated with them.
	static std::vector<std::string> variableNames;
	static int numMetadataVariables;
	static int* mapMetadataVars;
	
};

}; //end VAPoR namespace
#endif //DATASTATUS_H

