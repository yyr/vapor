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
#include "vaporinternal/common.h"
#include "params.h"
#include "vapor/MyBase.h"


using namespace VetsUtil;

namespace VAPoR {

class ViewpointParams;
class XmlNode;

class PARAMS_API RegionParams : public Params {
	
public: 
	RegionParams(int winnum);
	
	~RegionParams();
	virtual Params* deepCopy();
	
	//Method to calculate the read-only region info that is displayed in the regionTab
	
	//following method gets voxel coords of region, but doesn't verify the existens
	//of data in the region
	//
	void getRegionVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], size_t min_bdim[3], size_t max_bdim[3], int timestep);
	
	//New version of above, to supply available region bounds when not full
	//Must specify the variable(s) that is/are being rendered.
	//If the required data is available, returns the refinement level that is required
	//If required refinement level is not available and the datastatus allows lower refinement level,
	//return the highest refinement level that is available.
	//Returns -1 if there is no data, or the required refinement level is not available.
	//Optionally provides user extents if last two args are non-null.
	 
	int getAvailableVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3], size_t timestep, 
		const int* sesVarNums, int numVars, double* regMin = 0, double* regMax = 0);
	//version similar to above with 2D support.  Region extents are required, but 
	//will be shrunk if not in available data
	static int shrinkToAvailableVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3], size_t timestep, 
		const int* sesVarNums, int numVars, double* regMin, double* regMax, bool twoDims);
	//Static method that converts box to extents in cube, independent of actual
	//extents in region.
	static void convertToStretchedBoxExtentsInCube(int refLevel, const size_t min_dim[3], const size_t max_dim[3], float extents[6]);
	static void convertToBoxExtents(int refLevel, const size_t min_dim[3], const size_t max_dim[3], float extents[6]);
	
	float calcCurrentValue(RenderParams* rp, int sessionVarNum, const float point[3], int numRefinements, int timeStep);

	float getRegionMin(int coord, int timestep){ return *(getRegionExtents(timestep)+coord);}
	float getRegionMax(int coord, int timestep){ return *(getRegionExtents(timestep)+3+coord);}
	float* getRegionExtents(int timestep);
	float* getRegionMin(int timestep) {return getRegionExtents(timestep);}
	float* getRegionMax(int timestep){ return (getRegionExtents(timestep)+3);}

	float getRegionCenter(int indx, int timestep) {
		return (0.5f*(getRegionMin(indx,timestep)+getRegionMax(indx,timestep)));
	}
	static int getFullGridHeight(){ return fullHeight;}
	static void setFullGridHeight(size_t val);//will purge if necessary
	//Version of DataMgr::GetValidRegion that knows about layered data
	static int getValidRegion(size_t timestep, const char* varname, int minRefLevel, size_t min_coord[3], size_t max_coord[3]);
	
	//Determine how many megabytes will be needed for one variable at specified
	//refinement level, specified box extents.
	static int getMBStorageNeeded(const float* boxMin, const float* boxMax, int refLevel);
	// Reinitialize due to new Session:
	bool reinit(bool doOverride);
	virtual void restart();
	static void setDefaultPrefs() {}  //No default preferences
	
	XmlNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	//See if the proposed number of transformations is OK.  Return a valid value
	int validateNumTrans(int n, int timestep);

	virtual void setBox(const float boxmin[], const float boxmax[], int timestep){
		float* extents = getRegionExtents(timestep);
		for(int i = 0; i<3; i++){
			//Don't check max>min until min is set:
			extents[i] = boxmin[i];
			extents[i+3] = boxmax[i];
			if (extents[i+3] < extents[i]) extents[i+3] = extents[i];
		}
	}
	virtual void getBox(float boxMin[], float boxMax[], int timestep){
		float* extents = getRegionExtents(timestep);
		for (int i = 0; i< 3; i++){
			boxMin[i] = extents[i];
			boxMax[i] = extents[i+3];
		}
	}
	
	//Methods to set the region max and min from a float value.
	//public so accessible from router
	//
	void setRegionMin(int coord, float minval, int timestep, bool checkMax=true);
	void setRegionMax(int coord, float maxval, int timestep, bool checkMin=true);
	void setInfoNumRefinements(int n){infoNumRefinements = n;}
	void setInfoTimeStep(int n) {infoTimeStep = n;}
	void setInfoVarNum(int n) {infoVarNum = n;}
	std::map<int,float*>& getExtentsMapping(){ return extentsMap;}
	void clearRegionsMap();
	bool extentsAreVarying(){ return extentsMap.size()>0;}

protected:
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

	//Actual region bounds
	float defaultRegionExtents[6];
	//Full grid height for layered data.  0 otherwise.
	static size_t fullHeight;
	
	//Extents map specifies extents for eacy timestep
	std::map<int, float*> extentsMap;
	
};

};
#endif //REGIONPARAMS_H 
