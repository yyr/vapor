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


	//! Return the extents of the data in user coordinates multiplied by current stretch factors.
	//! \retval float[6] stretched extents array
	const float* getStretchedExtents() { return stretchedExtents; }

	//! Returns the minimum time step for which there is any data.
	//! \retval size_t value of smallest time step
	size_t getMinTimestep() {return minTimeStep;}

	//! Returns the maximum time step for which there is any data.
	//! \retval size_t value of largest time step
	size_t getMaxTimestep() {return maxTimeStep;}

	

	
	//! Indicates whether any variable exists at a particular timestep.
	//! \param[in] int timestep Time step
	//! \retval true if the data exists
	bool dataIsPresent(int timestep) {return true;}
		
	

	
	

	//! Indicates the number of time steps in the current VDC.
	//! \retval int number of timesteps
	int getNumTimesteps() {return numTimesteps;}

	//! Indicates the number of refinement levels in the VDC.
	//! \retval int number of refinement levels
	int getNumTransforms() {return numTransforms;}

	//! Indicates the number of levels of detail in the VDC.
	//! \retval int number of LOD's
	int getNumLODs() { return numLODs;}

	

	

	//! Returns the current data manager (if it exists).
	//! Returns null if it does not exist.
	//! \retval DataMgr* pointer to current Data Manager
	DataMgr* getDataMgr() {return dataMgr;}

	
	
#ifndef DOXYGEN_SKIP_THIS
	DataStatus();
	~DataStatus();

	//Reset the datastatus when a new datamgr is opened.
	//This avoids all "Set" methods:
	bool reset(DataMgr* dm, size_t cachesize);
	
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
	

	const float* getStretchFactors() {return stretchFactors;}
	
	
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
	
	float extents[6];
	float stretchedExtents[6];
	float stretchFactors[3];
	float fullSizes[3];
	float fullStretchedSizes[3];

	
	
	
	//Cache size in megabytes
	static size_t cacheMB;
	

#endif //DOXYGEN_SKIP_THIS
};

}; //end VAPoR namespace
#endif //DATASTATUS_H

