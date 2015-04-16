//************************************************************************
//									*
//		     Copyright (C)  2004				*
//     University Corporation for Atmospheric Research			*
//		     All Rights Reserved				*
//									*
//************************************************************************/
//
//	File:		renderparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2014
//
//	Description:	Defines the RendererParams class.
//		This is an abstract class for all the tabbed panel render params classes.
//		Supports functionality common to all the tabbed panel render params.

//
#ifndef RENDERPARAMS_H
#define RENDERPARAMS_H

#include <vapor/XmlNode.h>
#include <vapor/ExpatParseMgr.h>
#include <vapor/ParamNode.h>
#include <map>

#include "assert.h"

#include <vapor/common.h>
#include <vapor/ParamsBase.h>
#include <vapor/RegularGrid.h>
#include "Box.h"
#include "params.h"
using namespace VetsUtil;

namespace VAPoR{


class XmlNode;
class ParamNode;
class ViewpointParams;
class DataMgrV3_0;
class Command;
class MapperFunction;


//! \class RenderParams
//! \ingroup Public_Params
//! \brief A Params subclass for managing parameters used by Renderers
//! \author Alan Norton
//! \version 3.0
//! \date    February 2014
//!
class PARAMS_API RenderParams : public Params {
public: 
//! @name Internal
//! Internal methods not intended for general use
///@{

//! Standard RenderParams constructor.
//! \param[in] parent  XmlNode corresponding to this Params class instance
//! \param[in] name  std::string name, can be the tag
//! \param[in] winNum  integer visualizer num, -1 for global or default params
	RenderParams(XmlNode *parent, const string &name, int winnum); 
///@}

	//! Determine if this params has been enabled for rendering
	//! \retval bool true if enabled
	virtual bool IsEnabled(){
		int enabled = GetValueLong(_EnabledTag);
		return (enabled != 0);
	}
	//! Enable or disable this params for rendering
	//! This should be executed between start and end capture
	//! which provides the appropriate undo/redo support
	//! Accordingly this will not make an entry in the undo/redo queue.
	//! \param[in] bool true to enable, false to disable.
	virtual void SetEnabled(bool val);

	//! Pure virtual method indicates if a particular variable name is currently used by the renderer.
	//! \param[in] varname name of the variable
	//!
	virtual bool usingVariable(const std::string& varname) = 0;
	//! Specify primary variable name; e.g. used in color mapping
	//! \param[in] string varName
	virtual void SetVariableName(const string& varName);
	//! Get the primary variable name, e.g. used in color mapping.
	//! \retval string variable name
	virtual const string GetVariableName();
	//! Specify renderer instance name; e.g. in instance identification
	//! \param[in] string renName
	virtual void SetRendererName(const string& renName);
	//! Get the renderer instance name, e.g. used in instance identification
	//! \retval string renderer name
	virtual const string GetRendererName();
	//! Pure virtual method sets current number of refinements of this Params.
	//! \param[in] int refinements
	//!
	virtual int SetRefinementLevel(int numrefinements);
	//! Pure virtual method indicates current number of refinements of this Params.
	//! \retval integer number of refinements
	//!
	virtual int GetRefinementLevel();
	//! Pure virtual method indicates current Compression level.
	//! \retval integer compression level, 0 is most compressed
	//!
	virtual int GetCompressionLevel();
	//! Pure virtual method sets current Compression level.
	//! \param[in] val  compression level, 0 is most compressed
	//!
	virtual int SetCompressionLevel(int val);
	//! Pure virtual method indicates whether or not the object will render as opaque.
	//! Important to support multiple transparent (nonoverlapping) objects in the scene
	//! \retval bool true if all geometry is opaque.
	virtual bool IsOpaque() = 0;

	//! Bypass flag is used to indicate a renderer should
	//! not render until its state is changed.
	//! Should be called when a rendering fails in a way that might repeat.
	//! \param[in] timestep that should be bypassed
	void setBypass(int timestep) {bypassFlags[timestep] = 2;}

	//! Partial bypass is similar to the bypass flag.  It is currently only set by DVR.
	//! This indicates a renderer should be bypassed at
	//! full resolution but not at interactive resolution.
	//! \param[in] timestep that should be bypassed
	void setPartialBypass(int timestep) {bypassFlags[timestep] = 1;}

	//! SetAllBypass is set to indicate all timesteps should be bypassed.
	//! Should be set true when a render failure is independent of timestep.
	//! Should be set false when state changes and rendering can be reattempted.
	//! \param[in] val indicates whether it is being turned on or off. 
	void setAllBypass(bool val);

	//! This method returns the status of the bypass flag.
	//! \param[in] int ts Time step
	//! \retval bool value of flag
	bool doBypass(int ts) {return ((ts < bypassFlags.size()) && bypassFlags[ts]);}

	//! This method is used in the presence of partial bypass.
	//! Indicates that the rendering should be bypassed at all resolutions.
	//! \param[in] int ts Time step
	//! \retval bool value of flag
	bool doAlwaysBypass(int ts) {return ((ts < bypassFlags.size()) && bypassFlags[ts]>1);}

	//! virtual method indicates current fidelity level
	//! \retval float between 0 and 1
	//!
	virtual int GetFidelityLevel();
	//! virtual method sets current fidelity level
	//! \param[in] float level
	//!
	virtual void SetFidelityLevel(int level);
	//! virtual method indicates fidelity is ignored
	//! \retval bool
	//!
	virtual bool GetIgnoreFidelity();
	//! virtual method sets whether fidelity is ignored
	//! \param[in] bool 
	//!
	virtual void SetIgnoreFidelity(bool val);
	void SetHistoStretch(float factor){
		SetValueDouble(_histoScaleTag,"Set histo stretch",(double)factor);
	}
	float GetHistoStretch(){
		return GetValueDouble(_histoScaleTag);
	}
	double getMinEditBound(){
		return GetRootNode()->GetElementDouble(_editBoundsTag)[0];
	}
	double getMaxEditBound(){
		return GetRootNode()->GetElementDouble(_editBoundsTag)[1];
	}
	void setMinEditBound(double val){
		vector<double>vals = GetValueDoubleVec(_editBoundsTag);
		if (vals.size()<1) vals.push_back(val);
		vals[0]=val;
		SetValueDouble(_editBoundsTag,"change edit min bound",vals);
	}
	void setMaxEditBound(double val){
		vector<double>vals = GetValueDoubleVec(_editBoundsTag);
		while (vals.size()<2) vals.push_back(val);
		vals[1]=val;
		SetValueDouble(_editBoundsTag,"change edit max bound",vals);
	}
	void SetEditBounds(vector<double>bounds){
		SetValueDouble(_editBoundsTag,"set edit bounds",bounds);
	}
	virtual MapperFunction* GetMapperFunc(){
		return 0;
	}
//! Static method that inserts a new instance into the list of existing 
//! RenderParams instances for a particular visualizer.
//! Useful for application developers.
//! \param[in] pType ParamsBase TypeId of the renderparams class.
//! \param[in] winnum index of specified visualizer window.
//! \param[in] posn index where new instance will be inserted. 
//! \param[in] dp pointer to RenderParams instance being appended. 
	static void InsertParamsInstance(int pType, int winnum, int posn, RenderParams* dp){
		vector<Params*>& instances = paramsInstances[make_pair(pType,winnum)];
		instances.insert(instances.begin()+posn, dp);
	}


#ifndef DOXYGEN_SKIP_THIS

	int SetLocal(bool lg){
		assert(lg);
		return false;
	}

	bool IsLocal() {
		return true;
	}
	
	virtual ~RenderParams(){
	}
	
	
	void initializeBypassFlags();
	
protected:
	static const string _EnabledTag;
	static const string _FidelityLevelTag;
	static const string _IgnoreFidelityTag;
	static const string _histoScaleTag;
	static const string _editBoundsTag;
	static const string _histoBoundsTag;
	static const string _RendererNameTag;
	
	vector<int> bypassFlags;
#endif //DOXYGEN_SKIP_THIS
};

}; //End namespace VAPoR
#endif //RENDERPARAMS_H 
