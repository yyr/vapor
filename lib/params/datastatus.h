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
#include <qcolor.h>
#include "regionparams.h" 
class QApplication;

namespace VAPoR {
//! \class DataStatus
//! \brief A class for describing the currently loaded dataset
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!
//! The DataStatus class keeps track of available variables, timesteps, resolutions, and data ranges.
//! It is constructed by the Session whenever a new metadata is loaded.
//! It keeps a lazily evaluated value of min/max of each variable for each timestep.
//! Variables can be referenced using the variable name, the session variable num (a numbering all the variables in
//! the session) or by the active variable num.  Active variables are all those in the metadata plus all
//! the derived variables, and are a subset of the session variables. 
//! Session variables are those that were specified in the session plus those that are derived, and these may not all be available in the metadata.
//! To support using active variables and session variable nums,
//! mappings are provided between active names/nums and session nums, and also between variable names and
//! their 2D and 3D session variable numbers and active variable numbers.
class PARAMS_API DataStatus{
public:
	
	

	//! static getInstance() method is used to obtain the singleton instance of the Datastatus.
	//! \retval current DataStatus instance
	static DataStatus* getInstance() {
		if (!theDataStatus) theDataStatus = new DataStatus;
		return theDataStatus;
	}

	//! Obtain the full extents of the data in user coordinates.
	//! Values in this array are in the order: minx, miny, minz, maxx, maxy, maxz.
	//! \retval const float[6] extents array
	const float* getLocalExtents() { return extents; }
	const float* getFullSizes() {return fullSizes;}
	const float* getFullStretchedSizes() {return fullStretchedSizes;}

	//! Determines extents of the data at a particular refinement level.
	//! \param[in] int level  refinement level
	//! \param[in] int ts Time step
	//! \param[out] float exts[6]  extents at specified level
	void getExtentsAtLevel(size_t ts, int level, float exts[6]);

	//! Return the extents of the data in user coordinates multiplied by current stretch factors.
	//! \retval float[6] stretched extents array
	const float* getStretchedExtents() { return stretchedExtents; }

	//! Returns the minimum time step for which there is any data.
	//! \retval size_t value of smallest time step
	size_t getMinTimestep() {return minTimeStep;}

	//! Returns the maximum time step for which there is any data.
	//! \retval size_t value of largest time step
	size_t getMaxTimestep() {return maxTimeStep;}

	//! Tells whether the current VDC is type 1 or 2
	//! \retval integer VDC type
	int getVDCType() {return VDCType;}

	//! Indicates whether a 3D session variable exists at a particular timestep.
	//! \param[in] int sesvarnum 3D Session variable number
	//! \param[in] int timestep Time step
	//! \retval true if the data exists
	bool dataIsPresent3D(int sesvarnum, int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		if (!variableExists[sesvarnum]) return false;
		return (maxLevel3D[sesvarnum][timestep] >= 0);
	}

	//! Indicates whether a 2D session variable exists at a particular timestep.
	//! \param[in] int sesvarnum 2D Session variable number
	//! \param[in] int timestep Time step
	//! \retval true if the data exists
	bool dataIsPresent2D(int sesvarnum, int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		if (!variableExists2D[sesvarnum]) return false;
		return (maxLevel2D[sesvarnum][timestep] >= 0);
	}

	//! Indicates whether any 3D variable exists at a particular timestep.
	//! \param[in] int timestep Time step
	//! \retval true if the data exists
	bool dataIsPresent3D(int timestep){
		if (!dataMgr) return false;
		if (timestep < (int)minTimeStep || timestep > (int)maxTimeStep) return false;
		for (int i = 0; i<variableExists.size(); i++){
			if (variableExists[i] && maxLevel3D[i][timestep] >= 0)
				return true;
		}
		return false;
	}

	//! Indicates whether any 2D variable exists at a particular timestep.
	//! \param[in] int timestep Time step
	//! \retval true if the data exists
	bool dataIsPresent2D(int timestep){
		if (!dataMgr) return false;
		if (timestep < 0 || timestep >= numTimesteps) return false;
		for (int i = 0; i<variableExists2D.size(); i++){
			if (variableExists2D[i] && maxLevel2D[i][timestep] >= 0)
				return true;
		}
		return false;
	}

	//! Indicates whether any variable exists at a particular timestep.
	//! \param[in] int timestep Time step
	//! \retval true if the data exists
	bool dataIsPresent(int timestep){
		if (!dataMgr) return false;
		if (timestep < 0 || timestep >= numTimesteps) return false;
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

	//! Indicates whether any 3D variable exists at any timestep. 
	//! \retval true if the data exists
	bool dataIsPresent3D(){
		for (int t = (int)minTimeStep; t <= (int)maxTimeStep; t++) {
			if (dataIsPresent3D(t)) return true;
		}
		return false;
	}

	//! Indicates whether any 2D variable exists at any timestep.
	//! \retval true if the data exists
	bool dataIsPresent2D(){
		for (int t = (int)minTimeStep; t <= (int)maxTimeStep; t++) {
			if (dataIsPresent2D(t)) return true;
		}
		return false;
	}

	//! Indicates the largest value of a 3D variable at a timestep.
	//! Returns 1 if the data bounds have not yet been calculated,
	//! for example if the variable is derived, the bounds will not
	//! be known until the input data has been retrieved and the
	//! derived variable has been calculated.
	//! \param[in] int sesvarNum session variable number
	//! \param[in] int timestep Time Step
	//! \param[in] bool mustGet indicates that data will be retrieved if max not known
	//! \retval double maximum value
	double getDataMax3D(int sesvarNum, int timestep, bool mustGet = true);

	//! Indicates the largest value of a 2D variable at a timestep.
	//! Returns 1 if the data bounds have not yet been calculated,
	//! for example, if the variable is derived, the bounds will not
	//! be known until the input data has been retrieved and the
	//! derived variable has been calculated.
	//! \param[in] int sesvarNum 2D session variable number
	//! \param[in] int timestep Time Step
	//! \param[in] bool mustGet indicates that data will be retrieved if max not known
	//! \retval double maximum value
	double getDataMax2D(int sesvarNum, int timestep, bool mustGet = true);

	//! Indicates the smallest value of a 3D variable at a timestep.
	//! Returns -1 if the data bounds have not yet been calculated;
	//! for example, if the variable is derived, the bounds will not
	//! be known until the input data has been retrieved and the
	//! derived variable has been calculated.
	//! \param[in] int sesvarNum 3D session variable number
	//! \param[in] int timestep Time Step
	//! \param[in] bool mustGet indicates that data will be retrieved if min not known
	//! \retval double minimum value
	double getDataMin3D(int sesvarNum, int timestep, bool mustGet = true);

	//! Indicates the smallest value of a 2D variable at a timestep.
	//! Returns -1 if the data bounds have not yet been calculated;
	//! for example, if the variable is derived, the bounds will not
	//! be known until the input data has been retrieved and the
	//! derived variable has been calculated.
	//! \param[in] sesvarNum 2D session variable number
	//! \param[in] timestep Time Step
	//! \param[in] bool mustGet indicates that data will be retrieved if min not known
	//! \retval double minimum value
	double getDataMin2D(int sesvarNum, int timestep, bool mustGet = true);

	//! Indicates the default maximum value of a 3D variable.
	//! This is usually the maximum value at the first time step.
	//! \param[in] int varnum 3D Session variable number
	//! \retval double maximum value
	double getDefaultDataMax3D(int varnum);

	//! Indicates the default minimum value of a 3D variable.
	//! This is usually the minimum value at the first time step.
	//! \param[in] int varnum Session variable number
	//! \retval double minimum value
	double getDefaultDataMin3D(int varnum);

	//! Indicates the default maximum value of a 2D variable.
	//! This is usually the maximum value at the first time step.
	//! \param[in] int varnum Session variable number
	//! \retval double maximum value
	double getDefaultDataMax2D(int varnum);

	//! Indicates the default minimum value of a 2D variable.
	//! This is usually the minimum value at the first time step.
	//! \param[in] int varnum Session variable number
	//! \retval double minimum value
	double getDefaultDataMin2D(int varnum);
	
	//! Indicates the maximum refinement level of a 3D variable
	//! that is present at a specified time step.
	//! Returns -1 if no data is present.
	//! \param[in] int sesvarnum Session variable number
	//! \param[in] int timestep  Time Step
	//! \retval int maximum refinement level
	int maxXFormPresent3D(int sesvarnum, int timestep);

	//! Indicates the maximum refinement level of a 2D variable
	//! that is present at a specified time step.
	//! Returns -1 if no data is present.
	//! \param[in] int sesvarnum Session variable number
	//! \param[in] int timestep  Time Step
	//! \retval int maximum refinement level
	int maxXFormPresent2D(int sesvarnum, int timestep);

	//! Indicates the maximum LOD level of a 3D variable
	//! that is present at a specified time step.
	//! Returns -1 if no data is present.
	//! \param[in] int sesvarnum Session variable number
	//! \param[in] int timestep  Time Step
	//! \retval int maximum LOD
	int maxLODPresent3D(int sesvarnum, int timestep);

	//! Indicates the maximum LOD level of a 2D variable
	//! that is present at a specified time step.
	//! Returns -1 if no data is present.
	//! \param[in] int sesvarnum Session variable number
	//! \param[in] int timestep  Time Step
	//! \retval int maximum LOD
	int maxLODPresent2D(int sesvarnum, int timestep);
	
	//! Indicates the maximum refinement level of a variable
	//! that is present at a specified time step.
	//! Returns -1 if no data is present.
	//! \param[in] const string& varname Variable name
	//! \param[in] int timestep Time Step
	//! \retval int maximum refinement level
	int maxXFormPresent(const string& varname, int timestep){
		int dim = 3;
		int sesvarnum = getSessionVariableNum3D(varname);
		if (sesvarnum < 0) {
			sesvarnum = getSessionVariableNum2D(varname);
			dim = 2;
			if (sesvarnum < 0) return -1;
		}
		if (dim == 3)return maxXFormPresent3D(sesvarnum,timestep);
		else return maxXFormPresent2D(sesvarnum,timestep);
	}

	//! Indicates the maximum LOD level of a variable
	//! that is present at a specified time step.
	//! Returns -1 if no data is present.
	//! \param[in] const string& varname Variable name
	//! \param[in] int timestep Time Step
	//! \retval int maximum LOD
	int maxLODPresent(const string& varname, int timestep){
		int dim = 3;
		int sesvarnum = getSessionVariableNum3D(varname);
		if (sesvarnum < 0) {
			sesvarnum = getSessionVariableNum2D(varname);
			dim = 2;
			if (sesvarnum < 0) return -1;
		}
		if (dim == 3)return maxLODPresent3D(sesvarnum,timestep);
		else return maxLODPresent2D(sesvarnum,timestep);
	}

	//! Indicates if a 3D variable is present at any time step.
	//! Returns false if no data is present.
	//! \param[in] int varnum 3D session variable number 
	//! \retval bool is true if it is present.
	bool variableIsPresent3D(int varnum) {
		if (varnum < variableExists.size()) return variableExists[varnum];
		else return false;
	}

	//! Indicates if a 2D variable is present at any time step.
	//! Returns false if no data is present.
	//! \param[in] int varnum 2D session variable number 
	//! \retval bool is true if it is present.
	bool variableIsPresent2D(int varnum) {
		if (varnum < variableExists2D.size()) return variableExists2D[varnum];
		else return false;
	}

	//! Verify that up to three field variables are present at specified resolution, lod, and timestep.
	//! Ignores a variable if varnum is < 0.
	//! Accepts lowered refinement level or lod if user has specified use of lower accuracy when requested accuracy is not available.
	//! \param[in] int refLevel is requested refinement level
	//! \param[in] int LOD is requested LOD level
	//! \param[in] int varx is session variable number of x component, -1 to ignore
	//! \param[in] int vary is session variable number of y component, -1 to ignore
	//! \param[in] int varz is session variable number of z component, -1 to ignore
	//! \retval true if the variable exists at resolution and timestep
	bool fieldDataOK(int refLevel, int lod, int tstep, int varx, int vary, int varz);
	
	//! Indicates the number of time steps in the current VDC.
	//! \retval int number of timesteps
	int getNumTimesteps() {return numTimesteps;}

	//! Indicates the number of refinement levels in the VDC.
	//! \retval int number of refinement levels
	int getNumTransforms() {return numTransforms;}

	//! Indicates the number of levels of detail in the VDC.
	//! \retval int number of LOD's
	int getNumLODs() { return numLODs;}

	//! Specify that a 3D variable is not available at a particular refinement, LOD, timestep.
	//! Avoids repeated error messages when requested data is not found.
	//! param[in] int timestep Time Step
	//! param[in] int refLevel Refinement level
	//! param[in] int lod	Level of Detail
	//! param[in] int sessionVarNum Session Variable Number
	void setDataMissing3D(int timestep, int refLevel, int lod, int sessionVarNum){
		MetadataVDC* md = dynamic_cast<MetadataVDC*> (dataMgr);
		if(md && md->GetVDCType() == 2) {
			if (maxLevel3D[sessionVarNum][timestep] >= lod)
			maxLevel3D[sessionVarNum][timestep] = lod -1;
			return;
		}
		if (maxLevel3D[sessionVarNum][timestep] >= refLevel)
			maxLevel3D[sessionVarNum][timestep] = refLevel -1;
	}

	//! Specify that a 2D variable is not available at a particular refinement, LOD, timestep.
	//! Avoids repeated error messages when requested data is not found.
	//! param[in] int timestep Time Step
	//! param[in] int refLevel Refinement level
	//! param[in] int lod	Level of Detail
	//! param[in] int sessionVarNum Session Variable Number
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


	//! Static method to determine the 3D *session* variable name associated with session index.
	//! \param[in] int sesvarNum 3D Session variable number
	//! \retval std::string variable name.
	static std::string& getVariableName3D(int sesvarNum) {return variableNames[sesvarNum];}

	//! Static method to determine the 2D *session* variable name associated with session index.
	//! \param[in] int sesvarNum 2D Session variable number
	//! \retval std::string variable name.
	static std::string& getVariableName2D(int sesvarNum) {return variableNames2D[sesvarNum];}

	//! Identify the 3D session variable num of a name, or -1 if it's not in session.
	//! \param[in] str std::string variable name
	//! \retval int 3D session variable number
	static int getSessionVariableNum3D(const std::string& str);

	//! Identify the 2D session variable num of a name, or -1 if it's not in session.
	//! \param[in] str std::string variable name
	//! \retval int 2D session variable number
	static int getSessionVariableNum2D(const std::string& str);
	
	//! Static method indicates the number of active 3D variables.
	//! \retval int number of active 3D variables
	static int getNumActiveVariables3D()  {return activeVariableNums3D.size();}

	//! Static method indicates the number of active 2D variables.
	//! \retval int number of active 2D variables
	static int getNumActiveVariables2D()  {return activeVariableNums2D.size();}
		
	//! Static method maps a 3D active variable num to a session variable num.
	//! \param[in] varnum active 3D variable number
	//! \retval int session variable number
	static int mapActiveToSessionVarNum3D(int varnum)
		{ if( varnum >= activeVariableNums3D.size()) return -1; 
		return activeVariableNums3D[varnum];}

	//! Static method maps a 2D active variable num to a session variable num.
	//! \param[in] varnum active 2D variable number
	//! \retval int session variable number
	static int mapActiveToSessionVarNum2D(int varnum) 
		{ if( varnum >= activeVariableNums2D.size()) return -1; 
		return activeVariableNums2D[varnum];}

	//! Static method maps a 3D session variable num to an active variable num.
	//! \param[in] sesvar session 3D variable number
	//! \retval int active 3D variable number
	static int mapSessionToActiveVarNum3D(int sesvar);

	//! Static method maps a 2D session variable num to an active variable num.
	//! \param[in] sesvar session 2D variable number
	//! \retval int active 2D variable number
	static int mapSessionToActiveVarNum2D(int sesvar);

	//! Static method finds the name that corresponds to an active 3D variable num.
	//! Should be the same as getting it from the metadata directly, if it's in metadata.
	//! If no such index, returns first variable in VDC.
	//! \param[in] int activevarnum active variable number
	//! \retval std::string name of variable
	static std::string& getActiveVarName3D(int activevarnum){
		if (activeVariableNums3D.size()<= activevarnum) return variableNames[0];
		return (variableNames[activeVariableNums3D[activevarnum]]);}

	//! Static method finds the name that corresponds to an active 2D variable num.
	//! Should be the same as getting it from the metadata directly, if it's in metadata.
	//! If no such index, returns first variable in VDC.
	//! \param[in] int activevarnum active variable number
	//! \retval std::string name of variable
	static std::string& getActiveVarName2D(int activevarnum){
		if (activeVariableNums2D.size()<= activevarnum) return variableNames2D[0];
		return (variableNames2D[activeVariableNums2D[activevarnum]]);}

	//! Method finds the active variable number associated with a 3D variable name.
	//! \param[in] std::string varname  Name of variable
	//! \retval int active variable number
	int getActiveVarNum3D(std::string varname) const;

	//! Method finds the active variable number associated with a 2D variable name.
	//! \param[in] std::string varname  Name of variable
	//! \retval int active variable number
	int getActiveVarNum2D(std::string varname) const;

	//! Returns the current data manager (if it exists).
	//! Returns null if it does not exist.
	//! \retval DataMgr* pointer to current Data Manager
	DataMgr* getDataMgr() {return dataMgr;}

	//! Method indicates if user requested a warning when data is missing.
	//! \retval bool true if warning is requested.
	static bool warnIfDataMissing() {return doWarnIfDataMissing;}

	//! Method indicates if user requested mouse tracking in TF editors
	//! \retval bool true if tracking is requested.
	static bool trackMouse() {return trackMouseInTfe;}

	//! Method indicates if user requests using lower accuracy, when specified LOD or refinement is not available.
	//! \retval bool true if lower accuracy is requested.
	static bool useLowerAccuracy() {return doUseLowerAccuracy;}

	
#ifndef DOXYGEN_SKIP_THIS
	DataStatus();
	~DataStatus();

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
			fullStretchedSizes[i] = fullSizes[i]*factor[i];
		}
	}
	static size_t getCacheMB() {return cacheMB;}
	static void setInteractiveRefinementLevel(int lev) {
		interactiveRefLevel = lev;
	}
	static int getInteractiveRefinementLevel() {return interactiveRefLevel;}
	
	//Find the first timestep that has any data with specified session variable num
	int getFirstTimestep(int sesvarnum);
	int getFirstTimestep2D(int sesvarnum);
	size_t getFullDataSize(int dim){return fullDataSize[dim];}
	size_t getFullSizeAtLevel(int lev, int dim) 
		{ return dataAtLevel[lev][dim];}
	float getVoxelSize(int lev, int dim){
		return ((extents[dim+3]-extents[dim])/(float)getFullSizeAtLevel(lev,dim));
	}
	const size_t* getFullDataSize() {return fullDataSize;}
	
	
	int get2DOrientation(int mdVarNum); //returns 0,1,2 for XY,YZ, or XZ

	//Used for georeferencing and moving region:
	static const std::string getProjectionString() {return projString;}

	//Following method used to give different extents with spherical data:
	void getLocalExtentsCartesian(float extents[6]);
	

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
	
	//Insert variableName if necessary; return sessionVariableNum
	static int mergeVariableName(const std::string& str);
	static int mergeVariableName2D(const std::string& str);


	static void addVarName(const std::string newName) {
		for (int i = 0; i<variableNames.size(); i++)
			if (variableNames[i] == newName) return;
		variableNames.push_back(newName);
	}
	static void addVarName2D(const std::string newName) {
		for (int i = 0; i<variableNames2D.size(); i++)
			if (variableNames2D[i] == newName) return;
		variableNames2D.push_back(newName);
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
	vector<string> getVariableNames2D() {return variableNames2D;}

	//used for specifying nondefault graphics hardware texture size:
	static void specifyTextureSize(bool val) {textureSizeSpecified = val;}
	static bool textureSizeIsSpecified(){return textureSizeSpecified;}
	static int getTextureSize() {return textureSize;}
	static void setTextureSize(int val) { textureSize = val;}

	
	static void setWarnMissingData(bool val) {doWarnIfDataMissing = val;}
	static void setTrackMouse(bool val) {trackMouseInTfe = val;}
	static void setUseLowerAccuracy(bool val){doUseLowerAccuracy = val;}
	
	const string& getSessionVersion(){ return sessionVersion;}
	void setSessionVersion(std::string& ver){sessionVersion = ver;}

	//Convert user point coordinates in-place.  Return bool if can't do it.
	//If the timestep is negative, then the coords are in a time-varying
	//extent.
	static bool convertToLonLat(double coords[], int npoints = 1);
	static bool convertFromLonLat(double coords[], int npoints = 1);
	static bool convertLocalToLonLat(int timestep, double coords[], int npoints = 1);
	static bool convertLocalFromLonLat(int timestep,double coords[], int npoints = 1);


	
	
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
	static const string _trackMouseAttr;

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
	static bool pre22Session(){return sessionBefore22;}
	static void setPre22Session(bool value){ sessionBefore22 = value;}
	static void setPre22Offset(float offset[3]){
		pre22Offset[0] = offset[0];pre22Offset[1] = offset[1];pre22Offset[2] = offset[2];
	}
	static float* getPre22Offset(){return pre22Offset;}
	
	
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
	float fullSizes[3];
	float fullStretchedSizes[3];

	// the variableNames array specifies the name associated with each session variable num.
	//Note that this
	//contains (possibly properly) the corresponding variableNames in the
	//dataMgr.  The number of metadataVariables should coincide with the 
	//number of variables in the datastatus that have actual data associated with them.
	static std::vector<std::string> variableNames;
	
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
	static bool trackMouseInTfe;
	static bool doUseLowerAccuracy;
	//Cache size in megabytes
	static size_t cacheMB;
	//Interactive refinement level:
	static int interactiveRefLevel;
	static std::string projString;
	
	int VDCType;
	static bool sessionBefore22;  //flag indicating that current session is before 2.2
	static float pre22Offset[3];
	
	//Static tables where all ParamsBase classes are registered.
	//Each class has a unique:
	//		classId (positive for Params classes)
	//		std::string tag
	//		CreateDefaultInstance() method, returns default instance of the class
	
	

#endif //DOXYGEN_SKIP_THIS
};

}; //end VAPoR namespace
#endif //DATASTATUS_H

