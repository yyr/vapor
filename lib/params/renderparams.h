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
class DataMgr;
class Command;

//!
//! \defgroup Public_Params VAPOR RenderParams Developer API
//!



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
	
	Params* deepCopy(ParamNode* nd = 0);
	virtual bool isRenderParams() const {return true;}
	
	void initializeBypassFlags();
	
protected:
	static const string _EnabledTag;
	
	vector<int> bypassFlags;
#endif //DOXYGEN_SKIP_THIS
};

}; //End namespace VAPoR
#endif //RENDERPARAMS_H 
