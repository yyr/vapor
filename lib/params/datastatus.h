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
#include <vapor/common.h>

#include <vapor/DataMgr.h>
#include <vapor/MetadataVDC.h>
#include <vapor/MyBase.h>
#include <qcolor.h>
#include "regionparams.h"
class QApplication;

namespace VAPoR {
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
	static void setInteractiveRefinementLevel(int lev) {
		interactiveRefLevel = lev;
	}
	static int getInteractiveRefinementLevel() {return interactiveRefLevel;}
	size_t getMinTimestep() {return minTimeStep;}
	size_t getMaxTimestep() {return maxTimeStep;}
	int getVDCType() {return VDCType;}
	bool dataIsPresent(int sesvarnum, int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		if (!variableExists[sesvarnum]) return false;
		return (maxLevel3D[sesvarnum][timestep] >= 0);
	}
	bool dataIsPresent2D(int sesvarnum, int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		if (!variableExists2D[sesvarnum]) return false;
		return (maxLevel2D[sesvarnum][timestep] >= 0);
	}
	bool dataIsPresent3D(int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		for (int i = 0; i<variableExists.size(); i++){
			if (variableExists[i] && maxLevel3D[i][timestep] >= 0)
				return true;
		}
		return false;
	}
	bool dataIsPresent2D(int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		for (int i = 0; i<variableExists2D.size(); i++){
			if (variableExists2D[i] && maxLevel2D[i][timestep] >= 0)
				return true;
		}
		return false;
	}
	bool dataIsPresent(int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		for (int i = 0; i<variableExists.size(); i++){
			if (variableExists[i] && maxLevel3D[i][timestep] >= 0)
				return true;
		}
		for (int i = 0; i<variableExists2D.size(); i++){
			if (variableExists2D[i] && maxLevel2D[i][timestep] >= 0)
				return true;
		}
		return false;
	}
	bool dataIsPresent3D(){
		for (int t = (int)minTimeStep; t <= (int)maxTimeStep; t++) {
			if (dataIsPresent3D(t)) return true;
		}
		return false;
	}
	bool dataIsPresent2D(){
		for (int t = (int)minTimeStep; t <= (int)maxTimeStep; t++) {
			if (dataIsPresent2D(t)) return true;
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
	double getDefaultDataMax(int varnum);
	double getDefaultDataMin(int varnum);
	double getDefaultDataMax2D(int varnum);
	double getDefaultDataMin2D(int varnum);
	
	int maxXFormPresent(int sesvarnum, int timestep){
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return -1;
		if (!variableExists[sesvarnum]) return -1;
		if (getVDCType()==2) return getNumTransforms();
		return (maxLevel3D[sesvarnum][timestep]);
	}
	int maxXFormPresent2D(int sesvarnum, int timestep){
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return -1;
		if (!variableExists2D[sesvarnum]) return -1;
		if (getVDCType()==2) return getNumTransforms();
		return (maxLevel2D[sesvarnum][timestep]);
	}
	int maxLODPresent3D(int sesvarnum, int timestep){
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return -1;
		if (!variableExists[sesvarnum]) return -1;
		if (getVDCType()!=2) return 0;
		return (maxLevel3D[sesvarnum][timestep]);
	}
	int maxLODPresent2D(int sesvarnum, int timestep){
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return -1;
		if (!variableExists2D[sesvarnum]) return -1;
		if (getVDCType()!=2) return 0;
		return (maxLevel2D[sesvarnum][timestep]);
	}
	//Determine if variable is present for *any* timestep 
	//Needed for setting DVR panel
	bool variableIsPresent(int varnum) {
		if (varnum < variableExists.size()) return variableExists[varnum];
		else return false;
	}
	bool variableIsPresent2D(int varnum) {
		if (varnum < variableExists2D.size()) return variableExists2D[varnum];
		else return false;
	}
	//Verify that field data is present at required resolution and timestep.
	//Ignore variable if varnum is < 0
	bool fieldDataOK(int refLevel, int lod, int tstep, int varx, int vary, int varz);
	
	int getNumTimesteps() {return numTimesteps;}
	//determine the maxnumtransforms in the vdf, may not actually have any data at
	//that level...
	int getNumTransforms() {return numTransforms;}
	int getNumLODs() { return numLODs;}
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
		return ((extents[dim+3]-extents[dim])/(float)getFullSizeAtLevel(lev,dim));
	}
	const size_t* getFullDataSize() {return fullDataSize;}
	void mapVoxelToUserCoords(int refLevel, const size_t voxCoords[3], double userCoords[3]){
		dataMgr->MapVoxToUser((size_t)-1, voxCoords, userCoords, refLevel);
	}
	DataMgr* getDataMgr() {return dataMgr;}
	int get2DOrientation(int mdVarNum); //returns 0,1,2 for XY,YZ, or XZ

	//Used for georeferencing and moving region:
	static const std::string getProjectionString() {return projString;}
	static const float* getExtents(int timestep){ 
		if (timeVaryingExtents[timestep] != 0) return timeVaryingExtents[timestep];
	else 
		return getInstance()->getExtents();
	}



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
	static bool isExtendedUp(int sesvarnum) {return extendUp[sesvarnum];}
	static bool isExtendedDown(int sesvarnum) {return extendDown[sesvarnum];}

	//Find the session num of a name, or -1 if it's not in session
	static int getSessionVariableNum(const std::string& str);
	static int getSessionVariableNum2D(const std::string& str);
	//Insert variableName if necessary; return sessionVariableNum
	static int mergeVariableName(const std::string& str);
	static int mergeVariableName2D(const std::string& str);


	static void addVarName(const std::string newName) {
		for (int i = 0; i<variableNames.size(); i++)
			if (variableNames[i] == newName) return;
		variableNames.push_back(newName);
		aboveValues.push_back(VetsUtil::ABOVE_GRID);
		belowValues.push_back(VetsUtil::BELOW_GRID);
		extendUp.push_back(true);
		extendDown.push_back(true);
	}
	static void addVarName2D(const std::string newName) {
		for (int i = 0; i<variableNames2D.size(); i++)
			if (variableNames2D[i] == newName) return;
		variableNames2D.push_back(newName);
	}
	//Set outside values for an existing session variable 
	static void setOutsideValues(int varnum, float belowVal, float aboveVal, bool down, bool up){
		belowValues[varnum] = belowVal;
		aboveValues[varnum] = aboveVal;
		extendDown[varnum] = down;
		extendUp[varnum] = up;
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
		extendUp.clear();
		extendDown.clear();
		variableNames2D.clear();
		clearDerivedVars();
	}
	static void removeMetadataVars(){
		if (mapMetadataVars) delete [] mapMetadataVars;
		if (mapMetadataVars2D) delete [] mapMetadataVars2D;
		clearMetadataVars();
	}
	
	static void clearMetadataVars() {
		mapMetadataVars = 0; numMetadataVariables = 0;
		mapMetadataVars2D = 0; numMetadataVariables2D = 0;
	}
	static void clearDerivedVars() {
		clearActiveVars();
		derivedMethodMap.clear();
		derived2DInputMap.clear();
		derived3DInputMap.clear();
		derived2DOutputMap.clear();
		derived3DOutputMap.clear();
	}
	static void purgeAllCachedDerivedVariables();

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
	void setDataMissing(int timestep, int refLevel, int lod, int sessionVarNum){
		MetadataVDC* md = dynamic_cast<MetadataVDC*> (dataMgr);
		if(md && md->GetVDCType() == 2) {
			if (maxLevel3D[sessionVarNum][timestep] >= lod)
			maxLevel3D[sessionVarNum][timestep] = lod -1;
			return;
		}
		if (maxLevel3D[sessionVarNum][timestep] >= refLevel)
			maxLevel3D[sessionVarNum][timestep] = refLevel -1;
	}
	void setDataMissing2D(int timestep, int refLevel, int lod, int sessionVarNum){
		MetadataVDC* md = dynamic_cast<MetadataVDC*>(dataMgr);
		if(md && md->GetVDCType() == 2) {
			if (maxLevel2D[sessionVarNum][timestep] >= lod)
			maxLevel2D[sessionVarNum][timestep] = lod -1;
			return;
		}
		if (maxLevel2D[sessionVarNum][timestep] >= refLevel)
			maxLevel2D[sessionVarNum][timestep] = refLevel -1;
	}
	const string& getSessionVersion(){ return sessionVersion;}
	void setSessionVersion(std::string& ver){sessionVersion = ver;}

	//Convert point coordinates in-place.  Return bool if can't do it.
	//If the timestep is negative, then the coords are in a time-varying
	//extent.
	static bool convertToLatLon(int timestep, double coords[], int npoints = 1);
	static bool convertFromLatLon(int timestep, double coords[], int npoints = 1);

	//Active variable names and variable nums refer to variables that either occur in
	//the metadata or are derived (i.e. output of a python script).  To support them,
	//there are mappings from active names/nums to session nums
	
	static int getNumActiveVariables3D()  {return activeVariableNums3D.size();}
	static int getNumActiveVariables2D()  {return activeVariableNums2D.size();}
		
	static int mapActiveToSessionVarNum3D(int varnum)
		{ if( varnum >= activeVariableNums3D.size()) return -1; 
		return activeVariableNums3D[varnum];}
	static int mapActiveToSessionVarNum2D(int varnum) 
		{ if( varnum >= activeVariableNums2D.size()) return -1; 
		return activeVariableNums2D[varnum];}
	static int mapSessionToActiveVarNum3D(int var);
	static int mapSessionToActiveVarNum2D(int var);
	// Find the name that corresponds to an active variable num
	// should be the same as getting it from the metadata directly,if its in metadata
	static std::string& getActiveVarName3D(int activevarnum){
		if (activeVariableNums3D.size()<= activevarnum) return variableNames[0];
		return (variableNames[activeVariableNums3D[activevarnum]]);}
	static std::string& getActiveVarName2D(int activevarnum){
		if (activeVariableNums2D.size()<= activevarnum) return variableNames2D[0];
		return (variableNames2D[activeVariableNums2D[activevarnum]]);}
	int getActiveVarNum3D(std::string varname) const;
	int getActiveVarNum2D(std::string varname) const;
	
	static void clearActiveVars() {
		activeVariableNums2D.clear();
		activeVariableNums3D.clear();
	}

	//Attribute names:
	static const string _backgroundColorAttr;
	static const string _regionFrameColorAttr;
	static const string _subregionFrameColorAttr;
	static const string _regionFrameEnabledAttr;
	static const string _subregionFrameEnabledAttr;
	static const string _useLowerRefinementAttr;
	static const string _missingDataWarningAttr;

	//Support for Python methods
	//Specify a new python script, return the integer ID (-1 if error)
	int addDerivedScript(const std::vector<string> &,const std::vector<string> &,const std::vector<string> &,
		const std::vector<string> &,const std::string &, bool useMetadata = true);
	//Determine how many scripts there are
	static int getNumDerivedScripts(){
		return (derivedMethodMap.size());
	}
	//Replace an existing python script with a new one.  -1 if error
	int replaceDerivedScript(int id, const vector<string> &in2DVars, const vector<string> &out2DVars, const vector<string>& in3Dvars, const vector<string>& out3Dvars, const string& script);
	//Obtain the id for a given output variable, return -1 if it does not exist
	//mapping from index to output 2D variables
	//Client classes need to iterate (read-only) over these
	static const map<int,vector<string> >& getDerived2DOutputMap() {return derived2DOutputMap;}
	static const map<int,vector<string> >& getDerived3DOutputMap() {return derived3DOutputMap;}
	//Remove a python script and associated variable lists.  Return false if it's already gone
	bool removeDerivedScript(int index);

	static bool isDerivedVariable(std::string varname) {
		return (getDerivedScriptId(varname) >= 0);
	}
	static const std::string getDerivedMethod(std::string varname) {
		int id = getDerivedScriptId(varname);
		if (id <0) return string("");
		return (getDerivedScript(id)); 
	}
	 //Change the just the method on an existing script
	 static bool setDerivedMethod(const std::string varname, const std::string& method){
		 int id = getDerivedScriptId(varname);
		 if (id <0) return false;
		 derivedMethodMap[id] = method;
		 return true;
	}
	
	static int getDerivedScriptId(const string& outvar) ;
	static const string& getDerivedScript(int id) ;
	static int getMaxDerivedScriptId();
	static const string& getDerivedScriptName(int id);
	static const vector<string>& getDerived2DInputVars(int id) ;
	static const vector<string>& getDerived3DInputVars(int id) ;
	static const vector<string>& getDerived2DOutputVars(int id) ;
	static const vector<string>& getDerived3DOutputVars(int id) ;
	static const string& getDerivedScript(const string& outvar) {
		return getDerivedScript(getDerivedScriptId(outvar));
	}
	//Add a python variable to the session, update mappings, bounds, etc. appropriately.
	//Return the new session var num
	//The variable should already be the output of an existing script.
	//It's OK if the variable is already set as a python variable, but 
	//make sure the input variables are all in the vdc; if not remove them.
	int setDerivedVariable3D(const string& derivedVarName);
	int setDerivedVariable2D(const string& derivedVarName);
	//Remove a python variable, return false if it's not there.
	//It should already be removed from the python variable mapping
	static bool removeDerivedVariable2D(const string& derivedVarName);
	static bool removeDerivedVariable3D(const string& derivedVarName);
	
	
private:
	
	static DataStatus* theDataStatus;
	static const vector<string> emptyVec;
	static const string _emptyString;
	//Python script mappings:
	//mapping from index to Python function
	static map<int,string> derivedMethodMap;
	//mapping from index to input 2D variables
	static map<int,vector<string> > derived2DInputMap;
	//mapping from index to input 3D variables
	static map<int,vector<string> > derived3DInputMap;
	//mapping to 2d outputs
	static map<int,vector<string> > derived2DOutputMap;
	//mapping from index to output 3D variables
	static map<int,vector<string> > derived3DOutputMap;

	void calcDataRange(int sesvarnum, int ts);
	void calcDataRange2D(int sesvarnum, int ts);
	//Identify if a session variable is active.  This requires it to be an active variable
	//and for there to be actual data behind it, or for it to be a python variable
	std::vector<bool> variableExists;
	std::vector<bool> variableExists2D;
	//for each int variable there is an int vector of num transforms for each time step.
	//value is -1 if no data at that timestep
	std::vector<int*> maxLevel3D;
	std::vector<int*> maxLevel2D;
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
	int numLODs;
	//numTimeSteps may include lots of times that are not used. 
	int numTimesteps;

	static int numOriented2DVars[3];

	
	
	size_t fullDataSize[3];
	std::vector<size_t*> dataAtLevel;
	
	float extents[6];
	float stretchedExtents[6];
	float stretchFactors[3];

	// the variableNames array specifies the name associated with each session variable num.
	//Note that this
	//contains (possibly properly) the corresponding variableNames in the
	//dataMgr.  The number of metadataVariables should coincide with the 
	//number of variables in the datastatus that have actual data associated with them.
	static std::vector<std::string> variableNames;
	static std::vector<float> belowValues;
	static std::vector<float> aboveValues;
	static std::vector<bool> extendUp;
	static std::vector<bool> extendDown;
	static int numMetadataVariables;
	static int* mapMetadataVars;
	static std::vector<std::string> variableNames2D;
	static int numMetadataVariables2D;
	static int* mapMetadataVars2D;
	static vector<int> activeVariableNums2D;
	static vector<int> activeVariableNums3D;
	
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
	//Interactive refinement level:
	static int interactiveRefLevel;
	static std::string projString;
	static vector <float*> timeVaryingExtents;
	int VDCType;
	
	//Static tables where all ParamsBase classes are registered.
	//Each class has a unique:
	//		classId (positive for Params classes)
	//		std::string tag
	//		CreateDefaultInstance() method, returns default instance of the class
	
	

	
};

}; //end VAPoR namespace
#endif //DATASTATUS_H

