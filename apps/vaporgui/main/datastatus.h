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

namespace VAPoR {
class DataMgr;

// Class used by session to keep track of variables, timesteps, resolutions, datarange.
// It is constructed by the Session whenever a new metadata is opened.
// It keeps a value of min/max data for each timestep that can be refreshed.
// Variables can be referenced either by the varNum (numbering all the variables in
// the session) or by the varName.  
class DataStatus{
public:
	DataStatus();
	~DataStatus();
	//Reset the datastatus when a new datamgr is opened.
	//This avoids all "Set" methods:
	bool reset(DataMgr* dm);
	
	// Get methods:
	//
	size_t getMinTimestep() {return minTimeStep;}
	size_t getMaxTimestep() {return maxTimeStep;}
	bool dataIsPresent(int varnum, int timestep){
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
	double getDefaultDataMax(int varnum){return getDataMax(varnum, minTimeStep);}
	double getDefaultDataMin(int varnum){return getDataMin(varnum, minTimeStep);}
	
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
	
		
	
private:
	void calcDataRange(int varnum, int ts);
	//Identify if a variable is active:
	std::vector<bool> variableExists;
	//for each int variable there is an int vector of num transforms for each time step.
	//value is -1 if no data at that timestep
	std::vector<int*> maxNumTransforms;
	
	
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
	
};

}; //end VAPoR namespace
#endif //DATASTATUS_H

