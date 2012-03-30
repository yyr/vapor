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
	//! \param[out] size_t min_dim[3]	Minimum voxel coordinates of region
	//! \param[out] size_t max_dim[3]	Maximum voxel coordinates of region
	//! \param[out] size_t min_bdim[3]	Minimum block coordinates of region
	//! \param[out] size_t max_bdim[3]	Maximum block coordinates of region
	//! \param[in] int timestep			Time step at which the coordinates are being requested.
	void getRegionVoxelCoords(int reflevel, size_t min_dim[3], size_t max_dim[3], int timestep);
	
	//! Method to obtain voxel and user coordinates of the available data
	//! in the region.  The region extents may be shrunk so as to include
	//! only the portion of the region for which the specified variables are available.
	//! If the requested refinement level is not available, and the user allows use of
	//! lowered accuracy, then returns the highest refinement level that is available.
	//! Returns -1 if required data is unavailable.
	//! Optionally provides user data extents.
	//! \retval int Available refinenement level, or -1 if no data available
	//! \param[in] int reflevel			Refinement level of requested coordinates
	//! \param[out] size_t min_dim[3]	Minimum voxel coordinates of available region
	//! \param[out] size_t max_dim[3]	Maximum voxel coordinates of available region
	//! \param[in] size_t timestep		Time step at which the data is being requested.
	//! \param[in] int* sesVarNums		An array of integer session 3D variable nums for requested variables
	//! \param[in] int numVars			Number of variables, i.e. size of sesVarnums
	//! \param[out] double* regMin		Minimum user coordinates, if this pointer is non-null
	//! \param[out] double* regMax		Maximum user coordinates, if this pointer is non-null
	int getAvailableVoxelCoords(int reflevel, size_t min_dim[3], size_t max_dim[3], 
		size_t timestep, 
		const int* sesVarNums, int numVars, double* regMin = 0, double* regMax = 0);
	

	//! Static method to obtain voxel and user coordinates of the available 2D or 3D data
	//! in any region.  The voxel extents may be shrunk so as to include
	//! only the extents for which the specified variables are available.
	//! If the requested refinement level is not available, and the DataStatus allows
	//! lowered accuracy, then returns the highest refinement level that is available.
	//! Returns -1 if required data is unavailable.
	//! Optionally provides user data extents.
	//! \retval int		Available refinenement level, or -1 if no data available
	//! \param[in] int reflevel			Refinement level of requested coordinates
	//! \param[inout] size_t min_dim[3] Minimum voxel coordinates of available region
	//! \param[inout] size_t max_dim[3] Maximum voxel coordinates of available region
	//! \param[out] size_t min_bdim[3]	Minimum block coordinates of available region
	//! \param[out] size_t max_bdim[3]	Maximum block coordinates of available region
	//! \param[in] size_t timestep		Time step at which the data is being requested.
	//! \param[in] int* sesVarNums		An array of integer session 3D variable nums for requested variables
	//! \param[in] int numVars			Number of variables, i.e. size of sesVarnums
	//! \param[out] double* regMin		Minimum user coordinates, if pointer argument is non-null
	//! \param[out] double* regMax		Maximum user coordinates, if pointer argument is non-null
	//! \param[in] bool twoDims			Indicates whether the variables are 2D (true) or 3D (false)
	static int shrinkToAvailableVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3], size_t timestep, 
		const int* sesVarNums, int numVars, double* regMin, double* regMax, bool twoDims);

	//! Static method to prepare for retrieval of data arrays from DataMgr. 
	//! \param[in] int numxforms Requested refinement level of data
	//! \param[in] size_t timestep				Time for which data is requested.
	//! \param[in] vector<string>& varnames		Vector of 2D or 3D variable names being requested
	//! \param[inout] double* regMin			Minimum extents requested (in) and actual available (out)
	//! \param[inout] double* regMax			Maximum extents requested (in) and actual available (out)
	//! \param[out] size_t min_dim[3]			Minimum voxel coordinates of available region
	//! \param[out] size_t max_dim[3]			Maximum voxel coordinates of available region
	//! \retval int Actual refinement level available or -1 if not all variables available.
	static int PrepareCoordsForRetrieval(int numxforms, size_t timestep, const vector<string>& varnames,
		double* regMin, double* regMax, 
		size_t min_dim[3], size_t max_dim[3]);
	
	//! Static method to index into 3D variable data arrays returned by DataMgr.
	//! Input 3D voxel coordinates are used to find value of data.
	//! \param[in] float* varData		3D array as retrieved from DataMgr
	//! \param[in] size_t coords[3]		voxel coordinates in full domain
	//! \param[in] size_t min_bdim[3]	Minimum block coordinates used to retrieve region from DataMgr
	//! \param[in] size_t max_bdim[3]	Maximum block coordinates used to retrieve region from DataMgr
	//! \retval float Value of specified variable at specified voxel
	static float IndexIn3DData(float* varData, size_t coords[3], const size_t bs[3],  const size_t min_bdim[3], const size_t max_bdim[3]){
		assert( (coords[0] >= min_bdim[0]*bs[0]) && (coords[0] < (max_bdim[0]+1)*bs[0]));
		assert( (coords[1] >= min_bdim[1]*bs[1]) && (coords[1] < (max_bdim[1]+1)*bs[1]));
		assert( (coords[2] >= min_bdim[2]*bs[2]) && (coords[2] < (max_bdim[2]+1)*bs[2]));
		return varData[(coords[0]-bs[0]*min_bdim[0])+
			(coords[1]-bs[1]*min_bdim[1])*(max_bdim[0]-min_bdim[0]+1)*bs[0] +
			(coords[2]-bs[2]*min_bdim[2])*(max_bdim[0]-min_bdim[0]+1)*bs[0]*(max_bdim[1]-min_bdim[1]+1)*bs[1]];
	}

	//! Static method to index into 2D variable data arrays returned by DataMgr.
	//! Input 2D voxel coordinates are used to find value of data.
	//! \param[in] float* varData		2D array as retrieved from DataMgr
	//! \param[in] size_t coords[2]		voxel coordinates in full domain
	//! \param[in] size_t min_bdim[3]	Minimum block coordinates used to retrieve region from DataMgr
	//! \param[in] size_t max_bdim[3]	Maximum block coordinates used to retrieve region from DataMgr
	//! \retval float Value of specified variable at specified voxel
	static float IndexIn2DData(float* varData, size_t coords[3], const size_t bs[3],  const size_t min_bdim[3], const size_t max_bdim[3]){
		assert( (coords[0] >= min_bdim[0]*bs[0]) && (coords[0] < (max_bdim[0]+1)*bs[0]));
		assert( (coords[1] >= min_bdim[1]*bs[1]) && (coords[1] < (max_bdim[1]+1)*bs[1]));
		return varData[(coords[0]-bs[0]*min_bdim[0])+
			(coords[1]-bs[1]*min_bdim[1])*(max_bdim[0]-min_bdim[0]+1)*bs[0]];
	}

	//! Static method that converts a box to its stretched extents in unit cube (as used in rendering).
	//! \param[in] int refLevel			Refinement level of data
	//! \param[in] size_t min_dim[3]	Minimum voxel coordinates 
	//! \param[in] size_t max_dim[3]	Maximum voxel coordinates 
	//! \param[out] float extents[6]	Stretched extents of box mapped into unit cube.
	static void convertToStretchedBoxExtentsInCube(int refLevel, const size_t min_dim[3], const size_t max_dim[3], double extents[6]);

	//! Static method that converts a box to its extents in unit cube (as used in rendering).
	//! \param[in] int refLevel			Refinement level of data
	//! \param[in] size_t min_dim[3]	Minimum voxel coordinates 
	//! \param[in] size_t max_dim[3]	Maximum voxel coordinates 
	//! \param[out] float extents[6]	Extents of box mapped into unit cube.
	static void convertToBoxExtents(int refLevel, const size_t min_dim[3], const size_t max_dim[3], double extents[6]);
	
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
	float getRegionMin(int coord, int timestep){ 
		double exts[6];
		myBox->GetExtents(exts, timestep);
		return exts[coord];
	}
	float getRegionMax(int coord, int timestep){ 
		double exts[6];
		myBox->GetExtents(exts, timestep);
		return exts[coord+3];
	}
	void getRegionExtents(double exts[6],int timestep){
		myBox->GetExtents(exts,timestep);
		return;
	}
	float getRegionCenter(int indx, int timestep) {
		return (0.5f*(getRegionMin(indx,timestep)+getRegionMax(indx,timestep)));
	}
	static int getFullGridHeight(){ return fullHeight;}
	static void setFullGridHeight(size_t val);//will purge if necessary
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
	void setRegionMin(int coord, float minval, int timestep, bool checkMax=true);
	void setRegionMax(int coord, float maxval, int timestep, bool checkMin=true);
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
