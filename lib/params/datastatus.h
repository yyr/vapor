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
#include "regionparams.h" 

namespace VAPoR {
//! \class DataStatus
//! \ingroup Public_Params
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

	//! Obtain the full extents of the current data in local coordinates.
	//! Values in this array are in the order: minx, miny, minz, maxx, maxy, maxz.
	//! \retval const double[6] extents array
	const double* getLocalExtents() { return extents; }
	//! Obtain the full extents of the current data in user coordinates.
	//! Values in this array are in the order: minx, miny, minz, maxx, maxy, maxz.
	//! \retval const double[6] extents array
	const double* getFullSizes() {return fullSizes;}
	//! Obtain the full extents of the current data in stretched user coordinates.
	//! Values in this array are in the order: minx, miny, minz, maxx, maxy, maxz.
	//! \retval const double[6] extents array
	static const double* getFullStretchedSizes() {return fullStretchedSizes;}
	//! Obtain the full extents of the current data in stretched local coordinates.
	//! Values in this array are in the order: minx, miny, minz, maxx, maxy, maxz.
	//! \retval const double[6] extents array
	const double* getStretchedLocalExtents() {return stretchedExtents;}


	//! Returns the minimum time step for which there is any data.
	//! \retval size_t value of smallest time step
	size_t getMinTimestep() {return minTimeStep;}

	//! Returns the maximum time step for which there is any data.
	//! \retval size_t value of largest time step
	size_t getMaxTimestep() {return maxTimeStep;}

	//! Indicates whether any variable exists at a particular timestep.
	//! \param[in] int timestep Time step
	//! \retval true if the data exists
	//! \note Currently always returns true
	bool dataIsPresent(int timestep) {return true;}

	//! Indicates the number of time steps in the current VDC.
	//! \retval int number of timesteps
	int getNumTimesteps() {return numTimesteps;}

	//! Indicates the number of refinement levels in the VDC.
	//! \retval int number of refinement levels
	int getNumTransforms() {return numTransforms;}

	//! Indicates the number of compression levels in the VDC.
	//! \retval int number ofcompressionlevels
	int getNumLODs() {return numLODs;}
	 
	int getNumActiveVariables3D() {return dataMgr->GetVariables3D().size();}
	int getNumActiveVariables2D() {return dataMgr->GetVariables2DXY().size();}
	int getNumActiveVariables() {return getNumActiveVariables3D()+getNumActiveVariables2D();}
	int getActiveVarNum3D(string vname) {return getVarNum3D(vname);}
	int getActiveVarNum2D(string vname) {return getVarNum2D(vname);}
	int getVarNum3D(string vname);
	int getVarNum2D(string vname);
	float getDefaultDataMax(string vname);
	float getDefaultDataMin(string vname);
	float getDefaultLODFidelity2D(){return defaultLODFidelity2D;}
	float getDefaultRefinementFidelity2D(){return defaultRefFidelity2D;}
	float getDefaultLODFidelity3D(){return defaultLODFidelity3D;}
	float getDefaultRefinementFidelity3D(){return defaultRefFidelity3D;}
	void setDefaultLODFidelity2D(float q) {defaultLODFidelity2D = q;}
	void setDefaultLODFidelity3D(float q) {defaultLODFidelity3D = q;}
	void setDefaultRefinementFidelity2D(float q){defaultRefFidelity2D = q;}
	void setDefaultRefinementFidelity3D(float q){defaultRefFidelity3D = q;}
	//Convert user point coordinates in-place.  Return bool if can't do it.
	//If the timestep is negative, then the coords are in a time-varying
	//extent.
	static bool convertToLonLat(double coords[], int npoints = 1);
	static bool convertFromLonLat(double coords[], int npoints = 1);
	static bool convertLocalToLonLat(int timestep, double coords[], int npoints = 1);
	static bool convertLocalFromLonLat(int timestep,double coords[], int npoints = 1);
	
	void mapBoxToVox(Box* box, int refLevel, int lod, int timestep, size_t voxExts[6]);

	float getVoxelSize(int numrefinements, int dir){
		size_t dim[3];
		dataMgr->GetDim(dim,numrefinements);
		return fullStretchedSizes[dir]/dim[dir];
	}
	
	//! Returns the current data manager (if it exists).
	//! Returns null if it does not exist.
	//! \retval DataMgr* pointer to current Data Manager
	DataMgr* getDataMgr() {return dataMgr;}

	// Utility methods for dealing with extents and stretching

	//! Return the current scene stretch factors
	//! \retval const float* current stretch factors
	static const double* getStretchFactors() {return stretchFactors;}

	//! Stretch a 3-vector
	//! \param[in/out] vector<double> coords[3]
	void stretchCoords(vector<double> coords){
		for (int i = 0; i<3; i++) coords[i] = coords[i]*stretchFactors[i];
	}
	//! Stretch a 3-vector
	//! \param[in/out] float coords[3]
	void stretchCoords(double coords[3]){
		for (int i = 0; i<3; i++) coords[i] = coords[i]*stretchFactors[i];
	}
	//! Find the max domain extent in stretched coords
	//! \retval float maximum stretched extent
	static double getMaxStretchedSize(){
		return (Max(fullStretchedSizes[0],Max(fullStretchedSizes[1],fullStretchedSizes[2])));
	}
	static void localToStretchedCube(const double fromCoords[3], double toCoords[3]);
	int maxXFormPresent(string varname, size_t timestep);
	int maxLODPresent(string varname, size_t timestep);
	//! Method indicates if user requests using lower accuracy, when specified LOD or refinement is not available.
	//! \retval bool true if lower accuracy is requested.
	static bool useLowerAccuracy() {return doUseLowerAccuracy;}
	static void setUseLowerAccuracy(bool val){doUseLowerAccuracy = val;}
	
#ifndef DOXYGEN_SKIP_THIS
	DataStatus();
	~DataStatus();

	//Reset the datastatus when a new datamgr is opened.
	//This avoids all "Set" methods:
	bool reset(DataMgr* dm, size_t cachesize);
	
	//Update based on current stretch factor:
	void stretchExtents(double factor[3]){
		for (int i = 0; i< 3; i++) {
			stretchedExtents[i] = extents[i]*factor[i];
			stretchedExtents[i+3] = extents[i+3]*factor[i];
			stretchFactors[i] = factor[i];
			fullStretchedSizes[i] = fullSizes[i]*factor[i];
		}
	}
	static size_t getCacheMB() {return cacheMB;}
	
	
	
private:
	
	static DataStatus* theDataStatus;
	static const vector<string> emptyVec;
	static const string _emptyString;
	
	DataMgr* dataMgr;
	
	//specify the minimum and max time step that actually have data:
	size_t minTimeStep;
	size_t maxTimeStep;
	int numTransforms;
	int numLODs;
	//numTimeSteps may include lots of times that are not used. 
	int numTimesteps;

	size_t fullDataSize[3];
	float defaultRefFidelity2D;
	float defaultRefFidelity3D;
	float defaultLODFidelity2D;
	float defaultLODFidelity3D;
	double extents[6];
	double stretchedExtents[6];
	static double stretchFactors[3];
	double fullSizes[3];
	static double fullStretchedSizes[3];
	//Cache size in megabytes
	static size_t cacheMB;
	static bool doUseLowerAccuracy;
	
#endif //DOXYGEN_SKIP_THIS
};

}; //end VAPoR namespace
#endif //DATASTATUS_H

