//************************************************************************
//																	*	
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
#include <qcolor.h>
#include "regionparams.h"
class QApplication;

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
	QApplication* getApp() {return theApp;}
	//Reset the datastatus when a new datamgr is opened.
	//This avoids all "Set" methods:
	bool reset(DataMgr* dm, size_t cachesize, QApplication* app);
	static void setDefaultPrefs();
	//Update based on current stretch factor:
	void stretchExtents(float factor[3]){
		for (int i = 0; i< 3; i++) {
			stretchedExtents[i] = extents[i]*factor[i];
			stretchedExtents[i+3] = extents[i+3]*factor[i];
			stretchFactors[i] = factor[i];
		}
	}
	// Get methods:
	//
	const float* getExtents() { return extents; }
	const float* getStretchedExtents() { return stretchedExtents; }
	
	//Determine the min and max extents at a level
	void getExtentsAtLevel(int level, float exts[6]);
	
	static size_t getCacheMB() {return cacheMB;}
	size_t getMinTimestep() {return minTimeStep;}
	size_t getMaxTimestep() {return maxTimeStep;}
	bool dataIsPresent(int sesvarnum, int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		if (!variableExists[sesvarnum]) return false;
		return (maxNumTransforms[sesvarnum][timestep] >= 0);
	}
	bool dataIsPresent2D(int sesvarnum, int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		if (!variableExists2D[sesvarnum]) return false;
		return (maxNumTransforms2D[sesvarnum][timestep] >= 0);
	}
	bool dataIsPresent(int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		for (int i = 0; i<variableExists.size(); i++){
			if (variableExists[i] && maxNumTransforms[i][timestep] >= 0)
				return true;
		}
		for (int i = 0; i<variableExists2D.size(); i++){
			if (variableExists2D[i] && maxNumTransforms2D[i][timestep] >= 0)
				return true;
		}
		return false;
	}

	bool dataIsLayered();

	double getDataMax(int sesvarNum, int timestep){
		if (!dataIsPresent(sesvarNum, timestep))return 1.0f;
		if (dataMax[sesvarNum][timestep] == -1.e30f)
			calcDataRange(sesvarNum,timestep);
		return dataMax[sesvarNum][timestep];
	}
	double getDataMax2D(int sesvarNum, int timestep){
		if (!dataIsPresent2D(sesvarNum, timestep))return 1.0f;
		if (dataMax2D[sesvarNum][timestep] == -1.e30f)
			calcDataRange2D(sesvarNum,timestep);
		return dataMax2D[sesvarNum][timestep];
	}
	double getDataMin(int sesvarNum, int timestep){
		if (!dataIsPresent(sesvarNum, timestep))return -1.0f;
		if (dataMin[sesvarNum][timestep] == 1.e30f)
			calcDataRange(sesvarNum,timestep);
		return dataMin[sesvarNum][timestep];
	}
	double getDataMin2D(int sesvarNum, int timestep){
		if (!dataIsPresent2D(sesvarNum, timestep))return -1.0f;
		if (dataMin2D[sesvarNum][timestep] == 1.e30f)
			calcDataRange2D(sesvarNum,timestep);
		return dataMin2D[sesvarNum][timestep];
	}
	double getDefaultDataMax(int varnum){return getDataMax(varnum, (int)minTimeStep);}
	double getDefaultDataMin(int varnum){return getDataMin(varnum, (int)minTimeStep);}
	double getDefaultDataMax2D(int varnum){return getDataMax2D(varnum, (int)minTimeStep);}
	double getDefaultDataMin2D(int varnum){return getDataMin2D(varnum, (int)minTimeStep);}
	
	int maxXFormPresent(int varnum, int timestep){
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return -1;
		if (!variableExists[varnum]) return -1;
		return (maxNumTransforms[varnum][timestep]);
	}
	int maxXFormPresent2D(int varnum, int timestep){
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return -1;
		if (!variableExists2D[varnum]) return -1;
		return (maxNumTransforms2D[varnum][timestep]);
	}
	//Determine if variable is present for *any* timestep 
	//Needed for setting DVR panel
	bool variableIsPresent(int varnum) {
		if (varnum < variableExists.size()) return variableExists[varnum];
		else return false;
	}
	bool variableIsPresent2D(int varnum) {
		if (varnum < variableExists.size()) return variableExists[varnum];
		else return false;
	}
	//Verify that field data is present at required resolution and timestep.
	//Ignore variable if varnum is < 0
	bool fieldDataOK(int refLevel, int tstep, int varx, int vary, int varz);
	bool fieldDataOK2D(int refLevel, int tstep, int varx, int vary, int varz);
	
	int getNumTimesteps() {return numTimesteps;}
	//determine the maxnumtransforms in the vdf, may not actually have any data at
	//that level...
	int getNumTransforms() {return numTransforms;}
	//Find the first timestep that has any data with specified session variable num
	int getFirstTimestep(int sesvarnum);
	int getFirstTimestep2D(int sesvarnum);
	size_t getFullDataSize(int dim){return fullDataSize[dim];}
	size_t getFullSizeAtLevel(int lev, int dim) 
	{ size_t fullHeight = RegionParams::getFullGridHeight();
		if (dim < 2 || fullHeight == 0) return dataAtLevel[lev][dim];
		 return (fullHeight >> (numTransforms - lev));
	}
	float getVoxelSize(int lev, int dim){
		return ((stretchedExtents[dim+3]-stretchedExtents[dim])/(float)getFullSizeAtLevel(lev,dim));
	}
	const size_t* getFullDataSize() {return fullDataSize;}
	void mapVoxelToUserCoords(int refLevel, const size_t voxCoords[3], double userCoords[3]){
		myReader->MapVoxToUser(0, voxCoords, userCoords, refLevel);
	}
	const VDFIOBase* getRegionReader() {return myReader;}
	const Metadata* getCurrentMetadata() {return currentMetadata;}
	DataMgr* getDataMgr() {return dataMgr;}
	Metadata* getMetadata() {return currentMetadata;}

	//Get/set methods for global vizfeatures
	static const QColor getBackgroundColor() {return backgroundColor;}
	static const QColor getRegionFrameColor() {return regionFrameColor;}
	static const QColor getSubregionFrameColor() {return subregionFrameColor;}
	static void setBackgroundColor(const QColor c) {backgroundColor = c;}
	static void setRegionFrameColor(const QColor c) {regionFrameColor = c;}
	static void setSubregionFrameColor(const QColor c) {subregionFrameColor = c;}
	static void enableRegionFrame(bool enable) {regionFrameEnabled = enable;}
	static void enableSubregionFrame(bool enable) {subregionFrameEnabled = enable;}
	static bool regionFrameIsEnabled() {return regionFrameEnabled;}
	static bool subregionFrameIsEnabled() {return subregionFrameEnabled;}

	//find the *session* variable name associated with session index
	static std::string& getVariableName(int sesvarNum) {return variableNames[sesvarNum];}
	static std::string& getVariableName2D(int sesvarNum) {return variableNames2D[sesvarNum];}
	static float getBelowValue(int sesvarNum) {return belowValues[sesvarNum];}
	static float getAboveValue(int sesvarNum) {return aboveValues[sesvarNum];}

	//Find the session num of a name, or -1 if it's not metadata:
	static int getSessionVariableNum(const std::string& str);
	static int getSessionVariableNum2D(const std::string& str);
	//Insert variableName if necessary; return sessionVariableNum
	static int mergeVariableName(const std::string& str);
	static int mergeVariableName2D(const std::string& str);


	static void addVarName(const std::string newName) {
		variableNames.push_back(newName);
		aboveValues.push_back(VetsUtil::ABOVE_GRID);
		belowValues.push_back(VetsUtil::BELOW_GRID);
	}
	static void addVarName2D(const std::string newName) {
		variableNames2D.push_back(newName);
	}
	//Set outside values for an existing session variable 
	static void setOutsideValues(int varnum, float belowVal, float aboveVal){
		belowValues[varnum] = belowVal;
		aboveValues[varnum] = aboveVal;
	}
	
	//"Metadata" variables are those that are in current metadata, as opposed to
	//"session" variables are those in session
	static int getNumMetadataVariables() {return numMetadataVariables;}
	static int getNumMetadataVariables2D() {return numMetadataVariables2D;}
	static int mapMetadataToSessionVarNum(int varnum) 
		{ if(!mapMetadataVars) return 0; 
		return mapMetadataVars[varnum];}
	static int mapMetadataToSessionVarNum2D(int varnum) 
		{ if(!mapMetadataVars2D) return 0; 
		return mapMetadataVars2D[varnum];}
	static int mapSessionToMetadataVarNum(int var);
	static int mapSessionToMetadataVarNum2D(int var);
	// Find the name that corresponds to a metadata variable num
	// should be the same as getting it from the metadata directly
	static std::string& getMetadataVarName(int mdvarnum) {
		if (!mapMetadataVars) return variableNames[0];
		return (variableNames[mapMetadataVars[mdvarnum]]);}
	static std::string& getMetadataVarName2D(int mdvarnum) {
		if (!mapMetadataVars2D) return variableNames2D[0];
		return (variableNames2D[mapMetadataVars2D[mdvarnum]]);}
	int getMetadataVarNum(std::string varname);
	int getMetadataVarNum2D(std::string varname);
	//getNumSessionVariables returns the number of session variables
	static int getNumSessionVariables(){return (int)variableNames.size();}
	static int getNumSessionVariables2D(){return (int)variableNames2D.size();}
	static void clearVariableNames() {
		variableNames.clear();
		aboveValues.clear();
		belowValues.clear();
		variableNames2D.clear();
	}
	static void removeMetadataVars(){
		if (mapMetadataVars) delete mapMetadataVars;
		if (mapMetadataVars2D) delete mapMetadataVars2D;
		clearMetadataVars();
	}
	
	static void clearMetadataVars() {
		mapMetadataVars = 0; numMetadataVariables = 0;
		mapMetadataVars2D = 0; numMetadataVariables2D = 0;
	}
	//Get full stretched data extents in cube coords
	void getMaxStretchedExtentsInCube(float maxExtents[3]);
	const float* getStretchFactors() {return stretchFactors;}
	
	bool renderReady() {return renderOK;}
	void setRenderReady(bool nowOK) {renderOK = nowOK;}

    bool sphericalTransform();

	vector<string> getVariableNames() {return variableNames;}
	vector<float> getBelowValues() {return belowValues;}
	vector<float> getAboveValues() {return aboveValues;}
	vector<string> getVariableNames2D() {return variableNames2D;}

	//used for specifying nondefault graphics hardware texture size:
	static void specifyTextureSize(bool val) {textureSizeSpecified = val;}
	static bool textureSizeIsSpecified(){return textureSizeSpecified;}
	static int getTextureSize() {return textureSize;}
	static void setTextureSize(int val) { textureSize = val;}

	static bool warnIfDataMissing() {return doWarnIfDataMissing;}
	static bool useLowerRefinementLevel() {return doUseLowerRefinementLevel;}
	static void setWarnMissingData(bool val) {doWarnIfDataMissing = val;}
	static void setUseLowerRefinementLevel(bool val){doUseLowerRefinementLevel = val;}
	//Note missing data if a request for the data fails:
	void setDataMissing(int timestep, int refLevel, int sessionVarNum){
		if (maxNumTransforms[sessionVarNum][timestep] >= refLevel)
			maxNumTransforms[sessionVarNum][timestep] = refLevel -1;
	}
	void setDataMissing2D(int timestep, int refLevel, int sessionVarNum){
		if (maxNumTransforms2D[sessionVarNum][timestep] >= refLevel)
			maxNumTransforms2D[sessionVarNum][timestep] = refLevel -1;
	}
	const string& getSessionVersion(){ return sessionVersion;}
	void setSessionVersion(std::string& ver){sessionVersion = ver;}

	

	//Attribute names:
	static const string _backgroundColorAttr;
	static const string _regionFrameColorAttr;
	static const string _subregionFrameColorAttr;
	static const string _regionFrameEnabledAttr;
	static const string _subregionFrameEnabledAttr;
	static const string _useLowerRefinementAttr;
	static const string _missingDataWarningAttr;
	
private:
	static DataStatus* theDataStatus;
	
	void calcDataRange(int sesvarnum, int ts);
	void calcDataRange2D(int sesvarnum, int ts);
	//Identify if a session variable is active.  This requires it to be a metadata variable,
	//and for there to be actual data behind it.
	std::vector<bool> variableExists;
	std::vector<bool> variableExists2D;
	//for each int variable there is an int vector of num transforms for each time step.
	//value is -1 if no data at that timestep
	std::vector<int*> maxNumTransforms;
	std::vector<int*> maxNumTransforms2D;
	Metadata* currentMetadata;
	DataMgr* dataMgr;
	bool renderOK;
	QApplication* theApp;
	
	//track min and max data values for each variable and timestep (at max transform level)
	//Indexed by session variable nums
	std::vector<float*> dataMin;
	std::vector<float*> dataMax;
	std::vector<float*> dataMin2D;
	std::vector<float*> dataMax2D;
	//specify the minimum and max time step that actually have data:
	size_t minTimeStep;
	size_t maxTimeStep;
	int numTransforms;
	//numTimeSteps may include lots of times that are not used. 
	int numTimesteps;

	
	
	size_t fullDataSize[3];
	std::vector<size_t*> dataAtLevel;
	
	float extents[6];
	float stretchedExtents[6];
	float stretchFactors[3];
	VDFIOBase* myReader;

	// the variableNames array specifies the name associated with each variable num.
	//Note that this
	//contains (possibly properly) the corresponding variableNames in the
	//dataMgr.  The number of metadataVariables should coincide with the 
	//number of variables in the datastatus that have actual data associated with them.
	static std::vector<std::string> variableNames;
	static std::vector<float> belowValues;
	static std::vector<float> aboveValues;
	static int numMetadataVariables;
	static int* mapMetadataVars;
	static std::vector<std::string> variableNames2D;
	static int numMetadataVariables2D;
	static int* mapMetadataVars2D;
	string sessionVersion;
	
	
	//User prefs are static:
	static int textureSize;
	static bool textureSizeSpecified;
	//values from vizFeatures
	static QColor backgroundColor;
	static QColor regionFrameColor;
	static QColor subregionFrameColor;
	static bool regionFrameEnabled;
	static bool subregionFrameEnabled;
	//Specify how to handle missing data
	static bool doWarnIfDataMissing;
	static bool doUseLowerRefinementLevel;
	//Cache size in megabytes
	static size_t cacheMB;
	
	
};

}; //end VAPoR namespace
#endif //DATASTATUS_H

