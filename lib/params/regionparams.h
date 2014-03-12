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
//! \version 3.0
//! \date    February 2014
//! The RegionParams class controls the extents of a 3D box of data for visualization.
//! The DVR, Isosurface and Flow renderers use only the data specified by the current RegionParams.
//! There is a global RegionParams, that 
//! is shared by all windows whose region is set to "global".  There is also
//! a local RegionParams for each window, that users can select whenever there are multiple windows.
//! When local settings are used, they only affect one currently active visualizer.
//! The RegionParams class also has several methods that are useful in setting up data requests from the DataMgr.
//!
class PARAMS_API RegionParams : public Params {
	
public: 

	//! \param[in] int winnum The window number, or -1 for a global RegionParams
	RegionParams(XmlNode* parent, int winnum);

	//! Destructor
	~RegionParams();
	//! Method to obtain the current Box defining the region extents
	//! \retval Box* current Box.
	virtual Box* GetBox() {
		ParamNode* pNode = GetRootNode()->GetNode(Box::_boxTag);
		if (pNode) return (Box*)pNode->GetParamsBase();
		Box* box = new Box();
		GetRootNode()->AddNode(Box::_boxTag, box->GetRootNode());
		return box;
	}
	//! Method to validate all values in a RegionParams instance
	//! \param[in] bool default indicates whether or not to set to default values associated with the current DataMgr
	//! \sa DataMgr
	virtual void Validate(bool useDefault);
	//! Method to initialize a new RegionParams instance
	virtual void restart();
	//! Get the minimum extent of the Box, in local coordinates
	//! \param[in] int coord 0,1,2 for x,y,z
	//! \param[in] int timestep indicates the current timestep, used only with time-varying extents.
	//! \retval double is minimum value of specified coordinate.
	double getLocalRegionMin(int coord, int timestep){ 
		double exts[6];
		GetBox()->GetLocalExtents(exts, timestep);
		return exts[coord];
	}
	//! Get the maximum extent of the Box, in local coordinates
	//! \param[in] int coord 0,1,2 for x,y,z
	//! \param[in] int timestep indicates the current timestep, used only with time-varying extents.
	//! \retval double is minimum value of specified coordinate.
	double getLocalRegionMax(int coord, int timestep){ 
		double exts[6];
		GetBox()->GetLocalExtents(exts, timestep);
		return exts[coord+3];
	}
	//! Get the extents extent of the Box, in local coordinates
	//! \param[out] double[6] extents
	//! \param[in] int timestep indicates the current timestep, used only with time-varying extents.
	
	void getLocalRegionExtents(double exts[6],int timestep){
		GetBox()->GetLocalExtents(exts,timestep);
		return;
	}
	//! Get a center coordinate of the Box, in local coordinates
	//! \param[in] int coord 0,1,2 for x,y,z
	//! \param[in] int timestep indicates the current timestep, used only with time-varying extents.
	//! \retval double value of center for specified coordinate.
	double getLocalRegionCenter(int indx, int timestep) {
		return (0.5*(getLocalRegionMin(indx,timestep)+getLocalRegionMax(indx,timestep)));
	}
	//Methods to set the region max and min from a float value.
	//public so accessible from router
	//
	//! Set the minimum value of a box coordinate
	//! \param [in] int coordinate (0,1,2 for x,y,z)
	//! \param [in] double value to be set
	//! \param [in] int timestep indicates the current timestep, used only with time-varying extents
	//! \retval int is 0 for success
	int SetLocalRegionMin(int coord, double minval, int timestep);
	//! Set the maximum value of a box coordinate
	//! \param [in] int coordinate (0,1,2 for x,y,z)
	//! \param [in] double value to be set
	//! \param [in] int timestep indicates the current timestep, used only with time-varying extents
	//! \retval int is 0 for success
	int SetLocalRegionMax(int coord, double maxval, int timestep);
#ifndef DOXYGEN_SKIP_THIS

	static ParamsBase* CreateDefaultInstance() {return new RegionParams(0, -1);}
	const std::string& getShortName() {return _shortName;}
	
	
	
	const vector<double>& GetAllExtents(){ return GetBox()->GetValueDoubleVec(Box::_extentsTag);}
	const vector<long>& GetTimes(){ return GetBox()->GetValueLongVec(Box::_timesTag);}
	void clearRegionsMap();
	bool extentsAreVarying(){ return GetBox()->GetTimes().size()>1;}
	//Insert a time in the list.  Return false if it's already there
	bool insertTime(int timestep);
	//Remove a time from the time-varying timesteps.  Return false if unsuccessful
	bool removeTime(int timestep);

protected:
	static const string _shortName;
	
#endif //DOXYGEN_SKIP_THIS
};

};
#endif //REGIONPARAMS_H 
