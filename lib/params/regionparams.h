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
	
	// Reinitialize due to new Session:
	bool reinit(bool doOverride);
	virtual void restart();
	
	
	
	
	virtual Box* GetBox() {return myBox;}
	//Methods to set the region max and min from a float value.
	//public so accessible from router
	//
	void setLocalRegionMin(int coord, float minval, int timestep, bool checkMax=true);
	void setLocalRegionMax(int coord, float maxval, int timestep, bool checkMin=true);
	
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
	
	
	Box* myBox;
#endif //DOXYGEN_SKIP_THIS
};

};
#endif //REGIONPARAMS_H 
