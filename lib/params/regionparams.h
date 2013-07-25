//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		regionparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Defines the RegionParams class.
//		This class supports parameters associted with the
//		region panel, describing the rendering region
//
#ifndef REGIONPARAMS_H
#define REGIONPARAMS_H


#include <qwidget.h>
#include <vector>
#include <map>
#include <vapor/common.h>
#include "params.h"
#include <vapor/MyBase.h>


using namespace VetsUtil;

namespace VAPoR {

class ViewpointParams;
class XmlNode;
class ParamNode;
//! \class RegionParams
//! \brief A class for describing a 3D axis-aligned region in user space.
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//! The RegionParams class controls the extents of a 3D box of data for visualization.
//! The DVR, Isosurface and Flow renderers use only the data specified by the current RegionParams.
//! There is a global RegionParams, that 
//! is shared by all windows whose region is set to "global".  There is also
//! a local RegionParams for each window, that users can select whenever there are multiple windows.
//! When local settings are used, they only affect one currently active visualizer.
//! The RegionParams class also has several methods that are useful in setting up data requests from the DataMgr.
class PARAMS_API RegionParams : public Params {
	
public: 

	//! \param[in] int winnum The window number, or -1 for a global RegionParams
	RegionParams(int winnum);

	//! Destructor
	~RegionParams();
	
	//! Method to obtain voxel coords of a region, but doesn't verify the existence
	//! of data in the region.
	//! \param[in] int reflevel			Refinement level of requested coordinates
	//! \param[in] int lod				LOD of data for requested coordinates, needed for ELEVATION
	//! \param[out] size_t min_dim[3]	Minimum voxel coordinates of region
	//! \param[out] size_t max_dim[3]	Maximum voxel coordinates of region
	//! \param[in] int timestep			Time step at which the coordinates are being requested.
	void getRegionVoxelCoords(int reflevel, int lod, size_t min_dim[3], size_t max_dim[3], int timestep);
	
	//! Method to obtain voxel and user coordinates of the available data
	//! in the region.  The region extents may be shrunk so as to include
	//! only the portion of the region for which the specified variables are available.
	//! If the requested refinement level is not available, and the user allows use of
	//! lowered accuracy, then returns the highest refinement level that is available.
	//! Returns -1 if required data is unavailable.
	//! Optionally provides user data extents.
	//! \retval int Available refinenement level, or -1 if no data available
	//! \param[in] int reflevel			Refinement level of requested coordinates
	//! \param[in] int lod				LOD of data
	//! \param[out] size_t min_dim[3]	Minimum voxel coordinates of available region
	//! \param[out] size_t max_dim[3]	Maximum voxel coordinates of available region
	//! \param[in] size_t timestep		Time step at which the data is being requested.
	//! \param[in] int* sesVarNums		An array of integer session 3D variable nums for requested variables
	//! \param[in] int numVars			Number of variables, i.e. size of sesVarnums
	//! \param[out] double* regMin		Minimum user coordinates, if this pointer is non-null
	//! \param[out] double* regMax		Maximum user coordinates, if this pointer is non-null
	int getAvailableVoxelCoords(int reflevel, int lod, size_t min_dim[3], size_t max_dim[3], 
		size_t timestep, 
		const int* sesVarNums, int numVars, double* regMin = 0, double* regMax = 0);
	

	//! Static method to prepare for retrieval of data arrays from DataMgr. 
	//! \param[in] int numxforms Requested refinement level of data
	//! \param[in] int lod Requested LOD of data
	//! \param[in] size_t timestep				Time for which data is requested.
	//! \param[in] vector<string>& varnames		Vector of 2D or 3D variable names being requested
	//! \param[inout] double* regMin			Minimum user coordinate extents requested (in) and actual available (out)
	//! \param[inout] double* regMax			Maximum user coordinate extents requested (in) and actual available (out)
	//! \param[out] size_t min_dim[3]			Minimum voxel coordinates of available region
	//! \param[out] size_t max_dim[3]			Maximum voxel coordinates of available region
	//! \retval int Actual refinement level available or -1 if not all variables available.
	static int PrepareCoordsForRetrieval(int numxforms, int lod, size_t timestep, const vector<string>& varnames,
		double* regMin, double* regMax, 
		size_t min_dim[3], size_t max_dim[3]);


	//! Evaluate a variable at a point, as requested by a RenderParams
	//! \param[in] string varname		requested variable name (2d or 3d)
	//! \param[in] double point[3]		User coordinates of requested value
	//! \param[in] int numRefinements	Refinement level of requested data
	//! \param[in] int lod				Compression level of requested data
	//! \param[in] int timeStep			Time step of data
	static float calcCurrentValue( const string& varname, const double point[3], int numRefinements, int lod, size_t timeStep);

#ifndef DOXYGEN_SKIP_THIS

	virtual Params* deepCopy(ParamNode* n = 0);
	static ParamsBase* CreateDefaultInstance() {return new RegionParams(-1);}
	const std::string& getShortName() {return _shortName;}
	float getLocalRegionMin(int coord, int timestep){ 
		double exts[6];
		myBox->GetLocalExtents(exts, timestep);
		return exts[coord];
	}
	float getLocalRegionMax(int coord, int timestep){ 
		double exts[6];
		myBox->GetLocalExtents(exts, timestep);
		return exts[coord+3];
	}
	void getLocalRegionExtents(double exts[6],int timestep){
		myBox->GetLocalExtents(exts,timestep);
		return;
	}
	float getLocalRegionCenter(int indx, int timestep) {
		return (0.5f*(getLocalRegionMin(indx,timestep)+getLocalRegionMax(indx,timestep)));
	}
	
	
	//Version of DataMgr::GetValidRegion that knows about layered data
	//Callers must supply the full voxel extents, these get reduced depending on what data is available for
	//the specified variable, or (for derived variables) the input variables to the script.
	static int getValidRegion(size_t timestep, const char* varname, int minRefLevel, size_t min_coord[3], size_t max_coord[3]);
	
	//Determine how many megabytes will be needed for one variable at specified
	//refinement level, specified box extents.

	static int getMBStorageNeeded(const double exts[6], int refLevel);
	// Reinitialize due to new Session:
	bool reinit(bool doOverride);
	virtual void restart();
	static void setDefaultPrefs() {}  //No default preferences
	
	ParamNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	//See if the proposed number of transformations is OK.  Return a valid value
	int validateNumTrans(int n, int timestep);

	
	virtual Box* GetBox() {return myBox;}
	//Methods to set the region max and min from a float value.
	//public so accessible from router
	//
	void setLocalRegionMin(int coord, float minval, int timestep, bool checkMax=true);
	void setLocalRegionMax(int coord, float maxval, int timestep, bool checkMin=true);
	void setInfoNumRefinements(int n){infoNumRefinements = n;}
	void setInfoTimeStep(int n) {infoTimeStep = n;}
	void setInfoVarNum(int n) {infoVarNum = n;}
	const vector<double>& GetAllExtents(){ return myBox->GetRootNode()->GetElementDouble(Box::_extentsTag);}
	const vector<long>& GetTimes(){ return myBox->GetRootNode()->GetElementLong(Box::_timesTag);}
	void clearRegionsMap();
	bool extentsAreVarying(){ return myBox->GetTimes().size()>1;}
	//Insert a time in the list.  Return false if it's already there
	bool insertTime(int timestep);
	//Remove a time from the time-varying timesteps.  Return false if unsuccessful
	bool removeTime(int timestep);

protected:
	static const string _shortName;
	static const string _regionMinTag;
	static const string _regionMaxTag;
	static const string _regionCenterTag;
	static const string _regionSizeTag;
	static const string _regionAtTimeTag;
	static const string _regionListTag;
	static const string _maxSizeAttr;
	static const string _numTransAttr;
	static const string _fullHeightAttr;
	static const string _timestepAttr;
	static const string _extentsAttr;
	
	//Methods to make sliders and text valid and consistent for region:

	int infoNumRefinements, infoVarNum, infoTimeStep;

	
	//Full grid height for layered data.  0 otherwise.
	static size_t fullHeight;
	
	Box* myBox;
#endif //DOXYGEN_SKIP_THIS
};

};
#endif //REGIONPARAMS_H 
