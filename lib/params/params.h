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
#include <map>

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

//!
//!

//! \class Params
//! \ingroup Public_Params
//! \brief A pure virtual class for managing parameters used in visualization
//! 
//! \author Alan Norton
//! \version 3.0
//! \date    March, 2014
//!
//! Params classes provide containers for parameters used by the various renderers in VAPOR
//! The parameters all all represented as an XML tree.  Each Params instance has a ParamNode root
//! and the ParamNode class provides methods for setting and getting the values from the XML representation.
//! The Params class is also the basic entity that is used in Undo/Redo by the Command class, so that
//! all changes in state occurring during the VAPOR application can be represented as a change in the
//! state of a Params instance.  The state of each Params instance is automatically saved and restored
//! with the VAPOR Session file, using the XML representation.
//! \par
//! In the VAPOR GUI, each tab corresponds to a Params subclass, and the renderer tabs correspond to
//! subclasses of RenderParams, a subclass of Params.  Several Params classes (BasicParams) have a unique
//! instance and are not associated with tabs in the GUI.  The BasicParams classes are used for
//! Undo/Redo and for saving/restoring session state.
//! At run time there is a collection of Params instances that describe the full state of the application.
//! These Params instances can be retrieved using GetParamsInstance() and GetDefaultParams().
//! There is one default instance of each Params class.  This instance is initially in the default state
//! after data is loaded, but can be modified later.  There is in addition one instance of Region, Animation, and Viewpoint
//! params for each Visualizer, that represents the state of the local params for that visualizer.  There are
//! in addition one or more instances of each RenderParams Class for each visualizer, depending on how many
//! instances have been created.
//! \sa Command, ParamsBase, ParamNode, RenderParams
//!
//! Instructions for VAPOR Params implementers:
//!
//! Implementers of VAPOR Params classes should implement a Params class with all of the state described by its ParamNode root.
//! \par 
//! If the Params class corresponds to a renderer, the Params class should be derived from RenderParams.
//! \par 
//! The implementer should also provide Get and Set methods for all of the parameters in the new Params class.
//! \par 
//! The Get methods can directly access the corresponding ParamNode methods that retrieve data from the XML tree.
//! \par 
//! The Set methods should invoke the methods Params::SetValueLong(), Params::SetValueDouble(), Params::SetValueString(),
//! and Params::SetValueStringVec(), so that changes in the Params state will automatically be captured in the
//! Undo/Redo queue, and so that values that are set are confirmed to be valid based on the current DataMgr.
//! \par 
//! Params implementers must also implement the various pure virtual methods described below.
//!
//! Instructions for VAPOR application implementers:
//! \par 
//! Application code (such as scripting and GUI applications) must be aware of the various Params instances that
//! are in use.  Methods for accessing these instances are provided as static methods on Params described below.  These
//! methods allow retrieval of Params instances based on the type of Params, the visualizer that it is associated with,
//! and the instance index of RenderParams.
//!
//! Terminology:
//! \par 
//! Type:  Each Params subclass has a unique string type, that also serves as the XML tag associated with the Params
//! \par 
//! Instance index:  Each RenderParams has an instance index, allowing control of multiple RenderParams instances in
//! the same rendering.
//! \par 
//! Visualizer:  A visualizer is an OpenGL rendering window.  These are numbered by positive integers.  Each RenderParams
//! instance is associated with a particular visualizer.  Params instances that are not RenderParams can apply in one
//! or more visualizer.
//! \par 
//! Current Instance:  When there are multiple instances of a RenderParams, one of them is the "Current Instance".
//! The current instance determines the values that are displayed in the GUI, and is also used as the
//! default instance when referenced by another Params instance.
//!
//!
class PARAMS_API Params : public MyBase, public ParamsBase {
	
public: 
//! @name Internal
//! Internal methods not intended for general use
///@{

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

//! virtual method indicates instance index, only used with RenderParams
//! Implementers need not implement this method.
//! Should only be used if the Params instance has been installed
//! by AppendParamsInstance() or InsertParamsInstance()
//! Returns -1 if the Params instance is not installed.
//! \retval integer instance index
//!
	virtual int GetInstanceIndex();

//! Static method that identifies the instance that is current in the identified window.
//! Useful for application developers.
//! \param[in] pType ParamsBase is the typeID of the params class
//! \param[in] winnum index of identified window
//! \retval instance index that is current
	static int GetCurrentParamsInstanceIndex(int pType, int winnum){
		return currentParamsInstance[make_pair(pType,winnum)];
	}
//! Static method that identifies the instance that is current in the identified window.
//! Useful for application developers.
//! Uses \p tag to identify the Params class.
//! \param[in] tag Tag (name) of the params class
//! \param[in] winnum index of identified window
//! \retval instance index that is current
	static int GetCurrentParamsInstanceIndex(const std::string tag, int winnum){
		return GetCurrentParamsInstanceIndex(GetTypeFromTag(tag),winnum);
	}
	 
//! Static method that specifies the instance that is current in the identified window.
//! For non-Render Params, the current instance should always be 0;
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of identified window
//! \param[in] instance index of instance to be made current
	static void SetCurrentParamsInstanceIndex(int pType, int winnum, int instance){
		currentParamsInstance[make_pair(pType,winnum)] = instance;
	}
	
//! Static method that finds the Params instance.
//! Useful for application developers.
//! if \p instance is -1, the current instance is found.
//! if \p winnum is -1, the default instance is found.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of  window
//! \param[in] instance index 
	static Params* GetParamsInstance(int pType, int winnum = -1, int instance = -1);

//! Static method that finds the Params instance based on tag. 
//! Useful for application developers.
//! if \p instance is -1, the current instance is found.
//! if \p winnum is -1, the default instance is found.
//! \param[in] tag XML Tag (name) of the params class
//! \param[in] winnum index of  window
//! \param[in] instance index 
	static Params* GetParamsInstance(const std::string tag, int winnum = -1, int instance = -1){
		return GetParamsInstance(GetTypeFromTag(tag), winnum, instance);
	}

//! Static method that returns the instance that is current in the identified window.
//! Useful for application developers.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of identified window
//! \retval Pointer to specified Params instance
	static Params* GetCurrentParamsInstance(int pType, int winnum){
		Params* p = GetParamsInstance(pType, winnum, -1);
		if (p->IsLocal()) return p;
		return GetDefaultParams(pType);
	}
	
//! Static method that returns the default Params instance.
//! Useful for application developers.
//! With non-render params this is the global Params instance.
//! \param[in] pType ParamsBase TypeId of the params class
//! \retval Pointer to specified Params instance
	static Params* GetDefaultParams(ParamsBase::ParamsBaseType pType){
		return defaultParamsInstance[pType];
	}

//! Static method that returns the default Params instance.
//! Useful for application developers.
//! Based on XML tag (name) of Params class.
//! With non-render params this is the global Params instance.
//! \param[in] tag XML tag of the Params class
//! \retval Pointer to specified Params instance
	static Params* GetDefaultParams(const string& tag){
		return defaultParamsInstance[GetTypeFromTag(tag)];
	}

//! Static method that sets the default Params instance.
//! Useful for application developers.
//! With non-render params this is the global Params instance.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] p Pointer to default Params instance
	static void SetDefaultParams(int pType, Params* p) {
		defaultParamsInstance[pType] = p;
	}

//! Static method that specifies the default Params instance
//! Based on Xml Tag of Params class.
//! With non-render params this is the global Params instance.
//! Useful for application developers.
//! \param[in] tag XML Tag of the params class
//! \param[in] p Pointer to default Params instance
	static void SetDefaultParams(const string& tag, Params* p) {
		defaultParamsInstance[GetTypeFromTag(tag)] = p;
	}
//! Static method that constructs a default Params instance.
//! Useful for application developers.
//! \param[in] pType ParamsBase TypeId of the params class
//! \retval Pointer to new default Params instance
	static Params* CreateDefaultParams(int pType);
	
		
//! Static method that tells how many instances of a Params class
//! exist for a particular visualizer.
//! Useful for application developers.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \retval number of instances that exist 
	static int GetNumParamsInstances(int pType, int winnum);

//! Static method that tells how many instances of a Params class
//! exist for a particular visualizer.
//! Based on the XML tag of the Params class.
//! Useful for application developers.
//! \param[in] tag XML tag associated with Params class
//! \param[in] winnum index of specified visualizer window
//! \retval number of instances that exist 
	static int GetNumParamsInstances(const std::string tag, int winnum){
		return GetNumParamsInstances(GetTypeFromTag(tag), winnum);
	}
	
//! Static method that appends a new instance to the list of existing 
//! Params instances for a particular visualizer.
//! Useful for application developers.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \param[in] p pointer to Params instance being appended 
	static void AppendParamsInstance(int pType, int winnum, Params* p){
		paramsInstances[make_pair(pType,winnum)].push_back(p);
		if (pType == 3) assert (paramsInstances[make_pair(pType,winnum)].size() <= 1);
	}

//! Static method that appends a new instance to the list of existing 
//! Params instances for a particular visualizer.
//! Based on the XML tag of the Params class.
//! Useful for application developers.
//! \param[in] tag XML tag associated with Params class
//! \param[in] winnum index of specified visualizer window
//! \param[in] p pointer to Params instance being appended 
	static void AppendParamsInstance(const std::string tag, int winnum, Params* p){
		AppendParamsInstance(GetTypeFromTag(tag),winnum, p);
	}

//! Static method that removes an instance from the list of existing 
//! Params instances for a particular visualizer.
//! Useful for application developers.
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \param[in] instance index of instance to remove 
	static void RemoveParamsInstance(int pType, int winnum, int instance);
	
//! Static method that inserts a new instance into the list of existing 
//! Params instances for a particular visualizer.
//! Useful for application developers.
//! \param[in] pType ParamsBase TypeId of the params class.
//! \param[in] winnum index of specified visualizer window.
//! \param[in] posn index where new instance will be inserted. 
//! \param[in] dp pointer to Params instance being appended. 
	static void InsertParamsInstance(int pType, int winnum, int posn, Params* dp){
		vector<Params*>& instances = paramsInstances[make_pair(pType,winnum)];
		instances.insert(instances.begin()+posn, dp);
	}

//! Static method that produces a list of all the Params instances of a type
//! for a particular visualizer.
//! Useful for application developers.
//! \param[in] pType ParamsBase TypeId of the params class.
//! \param[in] winnum index of specified visualizer window.
//! \retval vector of the Params pointers associated with the window .
	static vector<Params*>& GetAllParamsInstances(int pType, int winnum){
		map< pair<int,int>, vector<Params*> >::const_iterator it = paramsInstances.find(make_pair(pType,winnum));
		if (it == paramsInstances.end()) return (*new vector<Params*>);
		return paramsInstances[make_pair(pType,winnum)];
	}

//! Static method that deletes all the Params instances for a particular visualizer
//! of any type.
//! \param[in] viznum window number associated with the visualizer
//! \retval number of Params instances that were deleted.
	static int DeleteVisualizer(int viznum);

//! Static method that produces a list of all the Params instances 
//! for a particular visualizer,
//! based on the XML Tag of the Params class.
//! Useful for application developers.
//! \param[in] tag XML tag associated with Params class
//! \param[in] winnum index of specified visualizer window
//! \retval vector of the Params pointers associated with the window 
	static vector<Params*>& GetAllParamsInstances(const std::string tag, int winnum){
		return GetAllParamsInstances(GetTypeFromTag(tag),winnum);
	}

//! Static method that produces clones of all the Params instances 
//! for a particular visualizer.
//! Useful for application developers.
//! \param[in] winnum index of specified visualizer window.
//! \retval std::map mapping from Params typeIDs to std::vectors of Params pointers associated with the window 
	static map <int, vector<Params*> >* cloneAllParamsInstances(int winnum);

//! Static method that produces clones of all the default Params instances 
//! for a particular visualizer.
//! Useful for application developers.
//! \param[in] winnum index of specified visualizer window
//! \retval std::vector of default Params pointers associated with the window, indexed by ParamsBase TypeIDs 
	static vector <Params*>* cloneAllDefaultParams();

//! Specify the visualizer index of a Params instance.
//! Not covered by undo/redo; not a user-level setting
//! \param[in]  vnum is the integer visualizer number
//! 
	virtual void SetVizNum(int vnum){
		GetRootNode()->SetElementLong(_VisualizerNumTag,(long)vnum);
	}
///@}


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
	 virtual const std::string getShortName()=0;

//! Method indicates that a params instance has changed, e.g. during Undo/Redo
//! Must be cleared after all users of the instance have checked it.
//! Setting on a shared params results in it being set for all the different
//! visualizers that share it;
//! \param[in] val indicates that a change has occurred
	void SetChanged(bool val);

//! method to test if there has been a change.
//! checks the bit associated with the visualizer.
//! \param[in] viz index of visualizer, only needed for shared params
//! \retval bool true if changed
	bool HasChanged(int viz = -1);

//! Pure virtual method for validation of all settings
//! It is important that implementers of Params classes write this method so that
//! it checks the values of all parameters in the Params instance, ensuring that they
//! are consistent with the current DataMgr
//! When the "default" argument is true, this method must all parameters to their default values
//! associated with the current Data Manager.
//! When the "default" argument is false, the method should change only those parameters that are
//! inconsistent with the current Data Manager, setting them to values that are consistent.
//! \param[in] bool default 
//! \sa DataMgr
	virtual void Validate(bool setdefault)=0;

//! Virtual method indicates that the geometry of this Params instance fits within a plane.
//! Needed for Probe, Isolines, to prevent invalid manip stretching.
//! Default returns false; must reimplement if the geometry is constrained to a plane.
//! \retval bool returns true if geometry is constrained to a plane.
	virtual bool IsPlanar() {return false;}

//! Static method that tells whether or not any renderer is enabled in a visualizer. 
//! Useful for application developers.
//! \param[in] winnum index of specified visualizer window
//! \retval True if any renderer is enabled 
	static bool IsRenderingEnabled(int winnum);
	
//! Virtual method indicating whether a Params is a RenderParams instance.
//! Default returns false.
//! Useful for application developers.
//! \retval returns true if it is a RenderParams
	virtual bool isRenderParams() const {return false;}

//! Virtual method indicating whether a Params is "Basic", i.e. a Params
//! class that has only one instance, used only for Undo/redo and sessions.  
//! Basic params are not associated with Tabs in the GUI.
//! Default returns false.
//! Useful for application developers.
//! \retval returns true if it is UndoRedo
	virtual bool isBasicParams() const {return false;}

//! Pure virtual method, sets a Params instance to its default state, without any data present.
//! Params implementers should assign valid values to all elements in the Params class in this method.
	virtual void restart() = 0;
	
//! Identify the visualizer associated with this instance.
//! With global or default or Basic Params this is -1 
	virtual int GetVizNum() {return (int)(GetValueLong(_VisualizerNumTag));}

//! Specify whether a [non-render]params is local or global. 
//! \param[in] lg boolean is true if is local 
	virtual int SetLocal(bool lg){
		return SetValueLong(_LocalTag,"set local or global",(long)lg);
	}

//! Indicate whether a Params is local or not.  Only used by non-render params
//! \retval is true if local
	virtual bool IsLocal() {
		return (GetValueLong(_LocalTag) != 0);
	}
	
//! Virtual method to return the Box associated with a Params class.
//! By default returns NULL.
//! All params classes that use a box to define data extents should reimplement this method.
//! Needed to support manipulators.
//! \retval Box* returns pointer to the Box associated with this Params.
	virtual Box* GetBox() {return 0;}

//! Virtual method supports rotated boxes such as probe
//! Specifies an axis-aligned box containing the rotated box.
//! By default it just finds the box extents.
//! Caller must supply extents array, which gets its values filled in.
//! \param[out] float[6] Extents of containing box
	virtual void calcContainingStretchedBoxExtents(double extents[6], bool rotated = false) 
		{if (!rotated) GetBox()->GetStretchedLocalExtents(extents,-1);
		else calcRotatedStretchedBoxExtents(extents);}

//! If the box is rotated, this method calculated the minimal axis-aligned extents
//! containing all 8 corners of the box.
//! \param[out] double extents[6] is smallest extents containing the box.
	void calcRotatedStretchedBoxExtents(double extents[6]);

//! The orientation is used only with 2D Box Manipulators, and must be implemented for Params supporting such manipulators.  
//! Valid values are 0,1,2 for being orthog to X,Y,Z-axes.
//! Default is -1 (invalid)
//! \retval int orientation direction (0,1,2)
	virtual int getOrientation() { assert(0); return -1;}
	//Following methods, while public, are not part of extensibility API
	
#ifndef DOXYGEN_SKIP_THIS
	
	//Not part of public API
	void calcLocalBoxCorners(double corners[8][3], float extraThickness, int timestep, double rotation = 0., int axis = -1);
	void buildLocalCoordTransform(double transformMatrix[12], double extraThickness, int timestep, double rotation, int axis);
	void convertThetaPhiPsi(double *newTheta, double* newPhi, double* newPsi, int axis, double rotation);
	virtual Params* deepCopy(ParamNode* nd = 0);
	static Params* CreateDummyParams(std::string tag);
	static void	BailOut (const char *errstr, const char *fname, int lineno);

	static int getNumDummyClasses(){return dummyParamsInstances.size();}
	static Params* getDummyParamsInstance(int i) {return dummyParamsInstances[i];}
	static void addDummyParamsInstance(Params*const & p ) {dummyParamsInstances.push_back(p);}

	static void clearDummyParamsInstances();
	static const std::string paramName(ParamsBaseType t);
	
	static const string _regionParamsTag;
	static const string _viewpointParamsTag;
	static const string _animationParamsTag;
	static const string _RefinementLevelTag;
	static const string _CompressionLevelTag;
	static const string _VariableNamesTag;
	static const string _VisualizerNumTag;
	static const string _VariablesTag;
	static const string _LocalTag;
	
#endif
protected:

//! Method for setting a single long value associated with a tag.
//! This will capture the state of the Params before the value is set, 
//! then set the value(s), then Validate(), and finally capture the state of
//! the Params after the Validate()
//! \sa Validate(), ParamsBase::GetValueLong()
//! Returns 0 if successful, -1 if the value cannot be set
//! \param [in] string tag
//! \param [in] long value
//! \param [in] char* description
//! \retval int zero if successful
	virtual int SetValueLong(string tag, const char* description, long value)
		{return ParamsBase::SetValueLong(tag,description,value, this);}

//! Method for setting long values associated with a tag.
//! This will capture the state of the Params before the value is set, 
//! then set the value(s), then Validate(), and finally capture the state of
//! the Params after the Validate()
//! Returns 0 if successful, -1 if the value cannot be set
//! \sa Validate(), ParamsBase::GetValueLongVec()
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] vector<long> value
//! \retval int zero if successful
	virtual int SetValueLong(string tag, const char* description, const vector<long>& value)
		{return ParamsBase::SetValueLong(tag, description, value, this);}

//! Method for setting a single double value associated with a tag.
//! This will capture the state of the Params before the value is set, 
//! then set the value(s), then Validate(), and finally capture the state of
//! the Params after the Validate()
//! Returns 0 if successful, -1 if the value cannot be set
//! \sa Validate(), ParamsBase::GetValueDouble()
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] double value
//! \retval int zero if successful
	virtual int SetValueDouble(string tag, const char* description, double value)
		{return ParamsBase::SetValueDouble(tag,description,value,this);}

//! Method for setting double values associated with a tag.
//! This will capture the state of the Params before the values are set, 
//! then set the value(s), then Validate(), and finally capture the state of
//! the Params after the Validate().
//! Returns 0 if successful, -1 if the value cannot be set
//! \sa Validate(), ParamsBase::GetValueDoubleVec()
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] vector<double> value
//! \retval int zero if successful
	virtual int SetValueDouble(string tag, const char* description, const vector<double>& value)
		{return ParamsBase::SetValueDouble(tag,description,value,this);}


//! Method for setting a single string value associated with a tag.
//! This will capture the state of the Params before the value is set, 
//! then set the value, then Validate(), and finally capture the state of
//! the Params after the Validate()
//! Returns 0 if successful, -1 if the value cannot be set
//! \sa Validate(), ParamsBase::GetValueString()
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] string value
//! \retval int zero if successful
	virtual int SetValueString(string tag, const char* description, const string& value)
		{return ParamsBase::SetValueString(tag,description,value,this);}

//! Method for setting string values associated with a tag.
//! This will capture the state of the Params before the values are set, 
//! then set the value(s), then Validate(), and finally capture the state of
//! the Params after the Validate()
//! returns 0 if successful, -1 if the value cannot be set.
//! \note strings in string vectors must be non-blank, non-empty, and must not have embedded blank characters.
//! \sa Validate(), ParamsBase::GetValueStringVec()
//! \param [in] string tag
//! \param [in] char* description
//! \param [in] vector<string> value
//! \retval int zero if successful
	virtual int SetValueStringVec(string tag, const char* description, const vector<string>& value)
		{return ParamsBase::SetValueStringVec(tag, description, value, this);}

#ifndef DOXYGEN_SKIP_THIS
	bool changeBit; //accessed via HasChanged() and SetChanged();
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
//! \class BasicParams
//! \ingroup Public_Params
//! \brief A Params subclass Params classes with one unique instance
//! \author Alan Norton
//! \version 3.0
//! \date    July 2014
//!
class PARAMS_API BasicParams : public Params {
public:
//! @name Internal
//! Internal methods not intended for general use
///@{
	BasicParams(XmlNode *parent, const string &name) : Params(parent, name, -1) {}
	virtual bool isBasicParams() const {return true;}
};
///@}

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
#ifndef DOXYGEN_SKIP_THIS
//Note that DummyParams are not part of the Public API
//These provide a repository for Params state for Params classes that are not
//Available at run time.
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
	const std::string getShortName(){return myTag;}

	std::string myTag;

};


#endif //DOXYGEN_SKIP_THIS
}; //End namespace VAPoR
#endif //PARAMS_H 
