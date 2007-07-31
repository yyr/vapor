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
	void getRegionVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], size_t min_bdim[3], size_t max_bdim[3]);
	
	//New version of above, to supply available region bounds when not full
	//Must specify the variable(s) that is/are being rendered.
	//Returns false if there is no data
	//Optionally provides user extents.
	bool getAvailableVoxelCoords(int numxforms, size_t min_dim[3], size_t max_dim[3], 
		size_t min_bdim[3], size_t max_bdim[3], size_t timestep, 
		const int* varNums, int numVars, double* regMin = 0, double* regMax = 0);
	//Static method that converts box to extents in cube, independent of actual
	//extents in region.
	static void convertToStretchedBoxExtentsInCube(int refLevel, const size_t min_dim[3], const size_t max_dim[3], float extents[6], size_t fullHeight);
	static void convertToBoxExtents(int refLevel, const size_t min_dim[3], const size_t max_dim[3], float extents[6]);
	
	
	float getRegionMin(int coord){ return regionMin[coord];}
	float getRegionMax(int coord){ return regionMax[coord];}
	float* getRegionMin() {return regionMin;}
	float* getRegionMax() {return regionMax;}
	float getRegionCenter(int indx) {
		return (0.5f*(regionMin[indx]+regionMax[indx]));
	}
	int getFullGridHeight(){ return fullHeight;}
	void setFullGridHeight(int val) {fullHeight = val;}
	
	//Determine how many megabytes will be needed for one variable at specified
	//refinement level, specified box extents.
	static int getMBStorageNeeded(const float* boxMin, const float* boxMax, int refLevel);
	// Reinitialize due to new Session:
	bool reinit(bool doOverride);
	virtual void restart();
	
	XmlNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	//See if the proposed number of transformations is OK.  Return a valid value
	int validateNumTrans(int n);

	virtual void setBox(const float boxmin[], const float boxmax[]){
		for(int i = 0; i<3; i++){
			//Don't check max>min until min is set:
			setRegionMin(i, boxmin[i],false);
			setRegionMax(i, boxmax[i],true);
		}
	}
	virtual void getBox(float boxMin[], float boxMax[]){
		for (int i = 0; i< 3; i++){
			boxMin[i] = getRegionMin(i);
			boxMax[i] = getRegionMax(i);
		}
	}
	//Methods to set the region max and min from a float value.
	//public so accessible from router
	//
	void setRegionMin(int coord, float minval, bool checkMax=true);
	void setRegionMax(int coord, float maxval, bool checkMin=true);
	void setInfoNumRefinements(int n){infoNumRefinements = n;}
	void setInfoTimeStep(int n) {infoTimeStep = n;}
	void setInfoVarNum(int n) {infoVarNum = n;}

protected:
	static const string _regionMinTag;
	static const string _regionMaxTag;
	static const string _regionCenterTag;
	static const string _regionSizeTag;
	static const string _maxSizeAttr;
	static const string _numTransAttr;
	static const string _fullHeightAttr;
	
	//Methods to make sliders and text valid and consistent for region:

	int infoNumRefinements, infoVarNum, infoTimeStep;

	//Actual region bounds
	float regionMax[3],regionMin[3];
	//Full grid height for layered data.  0 otherwise.
	size_t fullHeight;
	
	//RegionTab* myRegionTab;
	
};

};
#endif //REGIONPARAMS_H 
