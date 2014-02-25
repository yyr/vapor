//************************************************************************
//									*
//		     Copyright (C)  2004				*
//     University Corporation for Atmospheric Research			*
//		     All Rights Reserved				*
//									*
//************************************************************************/
//
//	File:		params.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Defines the Params and the RendererParams classes.
//		This is an abstract class for all the tabbed panel params classes.
//		Supports functionality common to all the tabbed panel params.
//
#ifndef PARAMS_H
#define PARAMS_H

#include <vapor/XmlNode.h>
#include <vapor/ExpatParseMgr.h>
#include <vapor/ParamNode.h>

#include "assert.h"
#include <vapor/common.h>
#include <vapor/ParamsBase.h>
#include <vapor/RegularGrid.h>
#include "Box.h"

class QWidget;

using namespace VetsUtil;

namespace VAPoR{

class XmlNode;
class ParamNode;
class DummyParams;
class ViewpointParams;
class RegionParams;
class DataMgr;
class Command;

enum ValidationMode {
		NO_CHECK,
		CHECK,
		CHECK_AND_FIX
	};
//! \class Params
//! \brief A pure virtual class for managing parameters used in visualization
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!
class PARAMS_API Params : public MyBase, public ParamsBase {
	
public: 
//! Standard Params constructor
//! \param[in] parent  XmlNode corresponding to this Params class instance
//! \param[in] name  std::string name, can be the tag
//! \param[in] winNum  integer visualizer num, -1 for global or default params
Params(
	XmlNode *parent, const string &name, int winNum
 );
//! Deprecated constructor, needed for built-in classes that do not have associated XML node:
Params(int winNum, const string& name) : ParamsBase(name) {
	SetVizNum(winNum);
	if(winNum < 0) SetLocal(false); else SetLocal( true);
	
	previousClass = 0;
}
	
//! Default copy constructor
	Params(const Params& p);
//! Destroy object
//!
//! \note This destructor does not delete child XmlNodes created
//! as children of the \p parent constructor parameter.
//!
 	virtual ~Params();

//! Pure virtual method specifying short name e.g. to display on the associated tab.
//! This is not the tag!
//! \retval string name to identify associated tab
//! \sa GetName
	 virtual const std::string& getShortName()=0;

//! virtual method indicates instance index, only used with RenderParams
//! \retval integer instance index
//!
	virtual int GetInstanceIndex();
//! Virtual method sets current instance index
//! Not saved for undo/redo
//! \param[in] val  instance index
//! \retval integer 0 if successful
//!
	virtual int SetInstanceIndex(int val);

//! Pure virtual method for validation of all settings
//! Sets everything to default if default is true
//! param[in] bool default 
	virtual void Validate(bool setdefault)=0;

//! Method for making a change in the value(s) associated with a tag
//! \param [in] string tag
//! \param [in] long value
//! \param [in] char* description
//! \retval int zero if successful
	virtual int CaptureSetLong(string tag, const char* description, long value)
		{return ParamsBase::CaptureSetLong(tag,description,value, this);}

//! Method for making a change in the value(s) associated with a tag
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] vector<long> value
//! \retval int zero if successful
	virtual int CaptureSetLong(string tag, const char* description, const vector<long>& value)
		{return ParamsBase::CaptureSetLong(tag, description, value, this);}

//! Method for making a change in the value(s) associated with a tag
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] double value
//! \retval int zero if successful
	virtual int CaptureSetDouble(string tag, const char* description, double value)
		{return ParamsBase::CaptureSetDouble(tag,description,value,this);}

//! Method for making a change in the value(s) associated with a tag
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] vector<double> value
//! \retval int zero if successful
	virtual int CaptureSetDouble(string tag, const char* description, const vector<double>& value)
		{return ParamsBase::CaptureSetDouble(tag,description,value,this);}


//! Method for making a change in the value(s) associated with a tag
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] string value
//! \retval int zero if successful
	virtual int CaptureSetString(string tag, const char* description, const string& value)
		{return ParamsBase::CaptureSetString(tag,description,value,this);}

//! Method for capturing a set of the value(s) associated with a tag
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] vector<string> value
//! \retval int zero if successful
	virtual int CaptureSetStringVec(string tag, const char* description, const vector<string>& value)
		{return ParamsBase::CaptureSetStringVec(tag, description, value, this);}

//! Initiate a set of changes to this params
//! \param [in] char* description
//! \retval int zero if successful
	virtual Command* CaptureStart(const char* description);

//! Finalize a set of changes to this params, put result in Command queue
//! \retval int zero if successful
	virtual void CaptureEnd(Command* cmd);

//! Static method that identifies the instance that is current in the identified window.
//! \param[in] pType ParamsBase is the typeID of the params class
//! \param[in] winnum index of identified window
//! \retval instance index that is current
	static int GetCurrentParamsInstanceIndex(int pType, int winnum){
		return currentParamsInstance[make_pair(pType,winnum)];
	}
//! Static method that identifies the instance that is current in the identified window.
//! Uses \p tag to identify the Params class.
//! \param[in] tag Tag (name) of the params class
//! \param[in] winnum index of identified window
//! \retval instance index that is current
	static int GetCurrentParamsInstanceIndex(const std::string tag, int winnum){
		return GetCurrentParamsInstanceIndex(GetTypeFromTag(tag),winnum);
	}
	 
//! Static method that specifies the instance that is current in the identified window.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of identified window
//! \param[in] instance index of instance to be made current
	static void SetCurrentParamsInstanceIndex(int pType, int winnum, int instance){
		currentParamsInstance[make_pair(pType,winnum)] = instance;
	}
	
//! Static method that finds the Params instance.
//! if \p instance is -1, the current instance is found.
//! if \p winnum is -1, the default instance is found.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of  window
//! \param[in] instance index 
	static Params* GetParamsInstance(int pType, int winnum = -1, int instance = -1);

//! Static method that finds the Params instance based on tag. 
//! if \p instance is -1, the current instance is found.
//! if \p winnum is -1, the default instance is found.
//! \param[in] tag XML Tag (name) of the params class
//! \param[in] winnum index of  window
//! \param[in] instance index 
	static Params* GetParamsInstance(const std::string tag, int winnum = -1, int instance = -1){
		return GetParamsInstance(GetTypeFromTag(tag), winnum, instance);
	}

//! Static method that returns the instance that is current in the identified window.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of identified window
//! \retval Pointer to specified Params instance
	static Params* GetCurrentParamsInstance(int pType, int winnum){
		Params* p = GetParamsInstance(pType, winnum, -1);
		if (p->IsLocal()) return p;
		return GetDefaultParams(pType);
	}
	
//! Static method that returns the default Params instance.
//! With non-render params this is the global Params instance.
//! \param[in] pType ParamsBase TypeId of the params class
//! \retval Pointer to specified Params instance
	static Params* GetDefaultParams(ParamsBase::ParamsBaseType pType){
		return defaultParamsInstance[pType];
	}

//! Static method that returns the default Params instance.
//! Based on XML tag (name) of Params class.
//! With non-render params this is the global Params instance.
//! \param[in] tag XML tag of the Params class
//! \retval Pointer to specified Params instance
	static Params* GetDefaultParams(const string& tag){
		return defaultParamsInstance[GetTypeFromTag(tag)];
	}

//! Static method that sets the default Params instance.
//! With non-render params this is the global Params instance.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] p Pointer to default Params instance
	static void SetDefaultParams(int pType, Params* p) {
		defaultParamsInstance[pType] = p;
	}

//! Static method that specifies the default Params instance
//! Based on Xml Tag of Params class.
//! With non-render params this is the global Params instance.
//! \param[in] tag XML Tag of the params class
//! \param[in] p Pointer to default Params instance
	static void SetDefaultParams(const string& tag, Params* p) {
		defaultParamsInstance[GetTypeFromTag(tag)] = p;
	}
//! Static method that constructs a default Params instance.
//! \param[in] pType ParamsBase TypeId of the params class
//! \retval Pointer to new default Params instance
	static Params* CreateDefaultParams(int pType){
		Params*p = (Params*)(createDefaultFcnMap[pType])();
		return p;
	}
	
		
//! Static method that tells how many instances of a Params class
//! exist for a particular visualizer.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \retval number of instances that exist 
	static int GetNumParamsInstances(int pType, int winnum){
		return paramsInstances[make_pair(pType, winnum)].size();
	}

//! Static method that tells how many instances of a Params class
//! exist for a particular visualizer.
//! Based on the XML tag of the Params class.
//! \param[in] tag XML tag associated with Params class
//! \param[in] winnum index of specified visualizer window
//! \retval number of instances that exist 
	static int GetNumParamsInstances(const std::string tag, int winnum){
		return GetNumParamsInstances(GetTypeFromTag(tag), winnum);
	}
	
//! Static method that appends a new instance to the list of existing 
//! Params instances for a particular visualizer.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \param[in] p pointer to Params instance being appended 
	static void AppendParamsInstance(int pType, int winnum, Params* p){
		p->SetInstanceIndex(paramsInstances[make_pair(pType,winnum)].size());
		paramsInstances[make_pair(pType,winnum)].push_back(p);
	}

//! Static method that appends a new instance to the list of existing 
//! Params instances for a particular visualizer.
//! Based on the XML tag of the Params class.
//! \param[in] tag XML tag associated with Params class
//! \param[in] winnum index of specified visualizer window
//! \param[in] p pointer to Params instance being appended 
	static void AppendParamsInstance(const std::string tag, int winnum, Params* p){
		AppendParamsInstance(GetTypeFromTag(tag),winnum, p);
	}

//! Static method that removes an instance from the list of existing 
//! Params instances for a particular visualizer.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \param[in] instance index of instance to remove 
	static void RemoveParamsInstance(int pType, int winnum, int instance);
	
//! Static method that inserts a new instance into the list of existing 
//! Params instances for a particular visualizer.
//! \param[in] pType ParamsBase TypeId of the params class.
//! \param[in] winnum index of specified visualizer window.
//! \param[in] posn index where new instance will be inserted. 
//! \param[in] dp pointer to Params instance being appended. 
	static void InsertParamsInstance(int pType, int winnum, int posn, Params* dp){
		vector<Params*>& instances = paramsInstances[make_pair(pType,winnum)];
		instances.insert(instances.begin()+posn, dp);
	}

//! Static method that produces a list of all the Params instances 
//! for a particular visualizer.
//! \param[in] pType ParamsBase TypeId of the params class.
//! \param[in] winnum index of specified visualizer window.
//! \retval vector of the Params pointers associated with the window .
	static vector<Params*>& GetAllParamsInstances(int pType, int winnum){
		return paramsInstances[make_pair(pType,winnum)];
	}

//! Static method that produces a list of all the Params instances 
//! for a particular visualizer,
//! based on the XML Tag of the Params class.
//! \param[in] tag XML tag associated with Params class
//! \param[in] winnum index of specified visualizer window
//! \retval vector of the Params pointers associated with the window 
	static vector<Params*>& GetAllParamsInstances(const std::string tag, int winnum){
		return GetAllParamsInstances(GetTypeFromTag(tag),winnum);
	}

//! Static method that produces clones of all the Params instances 
//! for a particular visualizer.
//! \param[in] winnum index of specified visualizer window.
//! \retval std::map mapping from Params typeIDs to std::vectors of Params pointers associated with the window 
	static map <int, vector<Params*> >* cloneAllParamsInstances(int winnum);

//! Static method that produces clones of all the default Params instances 
//! for a particular visualizer.
//! \param[in] winnum index of specified visualizer window
//! \retval std::vector of default Params pointers associated with the window, indexed by ParamsBase TypeIDs 
	static vector <Params*>* cloneAllDefaultParams();

//! Static method that tells whether or not any renderer is enabled in a visualizer. 
//! \param[in] winnum index of specified visualizer window
//! \retval True if any renderer is enabled 
	static bool IsRenderingEnabled(int winnum);
	
//! Virtual method indicating whether a Params is a RenderParams instance.
//! Default returns false.
//! \retval returns true if it is a RenderParams
	virtual bool isRenderParams() {return false;}
	
//! Pure virtual method that clones a Params instance.
//! Derived from ParamsBase.  With Params instances, the argument is ignored.
//! \param[in] nd ParamNode* instance corresponding to the ParamsBase instance
	virtual Params* deepCopy(ParamNode* nd = 0);

//! Pure virtual method, sets a Params instance to its default state
	virtual void restart() = 0;
	
//! Sets the current validation mode of this Params instance. Possible Values are:
//! NO_CHECK indicates that SetValues will not check values for validity
//! CHECK indicates that SetValues will check for validity, and, if invalid, no setting occurs, with -1 return code
//! CHECK_AND_FIX indicates that SetValues will check for validity 
//! and will modify the value to a valid value, returning -1 if the value needed to be modified.
//! If it is not possible to set a valid value, the return code is -2 and no change occurs.
//! The ValidationMode does not change unless this method is invoked.
//! \param[in] ValidationMode v specifies the current validation mode.
	void SetValidationMode(ValidationMode v) {currentValidationMode = v;}

//! Obtain the current ValidationMode
//! \returns ValidationMode obtains the current ValidationMode.
	ValidationMode GetValidationMode() {return currentValidationMode;}

//! Identify the visualizer associated with this instance.
//! With global pr default Params this is -1 
	virtual int GetVizNum() {return (int)(GetRootNode()->GetElementLong(_VisualizerNumTag))[0];}

//! Specify whether a [non-render]params is local or global. 
//! \param[in] lg boolean is true if is local 
	virtual void SetLocal(bool lg){
		GetRootNode()->SetElementLong(_LocalTag,(long)lg);
	}

//! Indicate whether a Params is local or not.
//! \retval is true if local
	bool IsLocal() {
		return (GetRootNode()->GetElementLong(_LocalTag)[0] != 0);
	}
	
//! Specify the visualizer index of a Params instance.
//! \param[in]  vnum is the integer visualizer number
	virtual void SetVizNum(int vnum){
		GetRootNode()->SetElementLong(_VisualizerNumTag,(long)vnum);
	}
	

//! Virtual method to return the Box associated with a Params class.
//! By default returns NULL.
//! All params classes that use a box to define data extents should reimplement this method.
//! Needed so support manipulators.
//! \retval Box* returns pointer to the Box associated with this Params.
	virtual Box* GetBox() {return 0;}

	//Following methods, while public, are not part of extensibility API
	
#ifndef DOXYGEN_SKIP_THIS
	
	static Params* CreateDummyParams(std::string tag);
	static void	BailOut (const char *errstr, const char *fname, int lineno);

	static int getNumDummyClasses(){return dummyParamsInstances.size();}
	static Params* getDummyParamsInstance(int i) {return dummyParamsInstances[i];}
	static void addDummyParamsInstance(Params*const & p ) {dummyParamsInstances.push_back(p);}

	static void clearDummyParamsInstances();
	static const std::string& paramName(ParamsBaseType t);
	
	static const string _regionParamsTag;
	static const string _viewpointParamsTag;
	static const string _animationParamsTag;
	static const string _RefinementLevelTag;
	static const string _CompressionLevelTag;
	static const string _VariableNamesTag;
	static const string _VisualizerNumTag;
	static const string _VariablesTag;
	static const string _LocalTag;
	static const string _InstanceTag;
	
protected:

	ValidationMode currentValidationMode;
	//Params instances are vectors of Params*, one per instance, indexed by paramsBaseType, winNum
	static map<pair<int,int>,vector<Params*> > paramsInstances;
	//CurrentRenderParams indexed by paramsBaseType, winNum
	static map<pair<int,int>, int> currentParamsInstance;
	//Dummy params are those that are found in a session file but not 
	//available in the current code.
	//default params instances indexed by paramsBaseType
	static map<int, Params*> defaultParamsInstance;
	static vector<Params*> dummyParamsInstances;
#endif //DOXYGEN_SKIP_THIS
};

//! \class RenderParams
//! \brief A Params subclass for managing parameters used by Renderers
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!
class PARAMS_API RenderParams : public Params {
public: 
	
//! Standard RenderParams constructor.
//! \param[in] parent  XmlNode corresponding to this Params class instance
//! \param[in] name  std::string name, can be the tag
//! \param[in] winNum  integer visualizer num, -1 for global or default params
	RenderParams(XmlNode *parent, const string &name, int winnum); 
	
		
	virtual bool IsEnabled(){
		int enabled = GetRootNode()->GetElementLong(_EnabledTag)[0];
		return (enabled != 0);
	}
	virtual void SetEnabled(bool val){
		long lval = (long)val;
		GetRootNode()->SetElementLong(_EnabledTag,lval);
	}

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
	//Following methods are deprecated, used by some built-in renderparams classes
	
	virtual ~RenderParams(){
	}
	
	virtual Params* deepCopy(ParamNode* nd = 0);
	virtual bool isRenderParams() {return true;}
	
	void initializeBypassFlags();
	
protected:
	static const string _EnabledTag;
	
	vector<int> bypassFlags;
#endif //DOXYGEN_SKIP_THIS
};
#ifndef DOXYGEN_SKIP_THIS
class DummyParams : public Params {
	public:
		DummyParams(XmlNode *parent, const std::string tag, int winnum);
	virtual ~DummyParams(){}
	virtual void restart(){}
	virtual int GetRefinementLevel() {
		return 0;
	}

	virtual int GetCompressionLevel() {return 0;}
	
	virtual int SetCompressionLevel(int){return 0;}
	virtual int SetRefinementLevel(int){return 0;}

	virtual void Validate(bool) {return;}
	
	virtual bool usingVariable(const std::string& ){
		return false;
	}
	const std::string &getShortName(){return myTag;}

	std::string myTag;

};


#endif //DOXYGEN_SKIP_THIS
}; //End namespace VAPoR
#endif //PARAMS_H 