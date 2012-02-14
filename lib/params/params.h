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
#include <qwidget.h>
#include "assert.h"
#include <vapor/common.h>
#include <vapor/ParamsBase.h>
#include "Box.h"

class QWidget;
using namespace VetsUtil;
//Error tolerance for gui parameters:
#define ROUND_OFF 1.e-6f
#define MAXVIZWINS 16
#define OUT_OF_BOUNDS -1.e30f

namespace VAPoR{
//! These dirty bits are associated with render windows and are kept in each GLWindow. 
//! These are dirty flags that need to be communicated between different params;
//! i.e. a change in one params forces a renderer to rebuild.
enum DirtyBitType {
//! Region bit indicates the region bounds have changed
	RegionBit,
//! NavigatingBit indicates the viewpoint is currently moving
	NavigatingBit,
//! LightingBit indicates there has been a change in lighting
    LightingBit,
//! ProjMatrixBit indicates there has been a change in the projection matrix (e.g. viewpoint change)
	ProjMatrixBit,
//! ViewportBit indicates a change in viewport, e.g. resize of window
	ViewportBit,
//! AnimationBit indicates a change in current frame
	AnimationBit 
};
class XmlNode;
class ParamNode;
class DummyParams;
class MapperFunction;
class TransferFunction;
class ViewpointParams;
class RegionParams;

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
	vizNum = winNum;
	if(winNum < 0) local = false; else local = true;
	
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

//! Pure virtual method specifying name to display on the associated tab.
//! \retval string name to identify associated tab
	 virtual const std::string& getShortName()=0;

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
		if (p->isLocal()) return p;
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
	
//! Identify the visualizer associated with this instance.
//! With global pr default Params this is -1 
	virtual int getVizNum() {return vizNum;}

//! Specify whether a params is local or global. 
//! \param[in] lg boolean is true if is local 
	virtual void setLocal(bool lg){ if (lg) {
		local = true;
	}
		else local = false;
	}

//! Indicate whether a Params is local or not.
//! \retval is true if local
	bool isLocal() {return local;}
	
//! Specify the visualizer index of a Params instance.
//! \param[in]  vnum is the integer visualizer number
	virtual void setVizNum(int vnum){vizNum = vnum;}
	
//! Virtual method to set up the Params to deal with new metadata.
//! When a new metadata is read, all params are notified.
//! If the params have state that depends on the metadata (e.g. region size,
//! variable, etc., they should implement reinit() to respond.
//! Default does nothing.
//
	virtual bool reinit(bool) {return false;}
//! Virtual method to return the Box associated with a Params class.
//! By default returns NULL.
//! All params classes that use a box to define data extents should reimplement this method.
//! Needed so support manipulators.
//! \retval Box* returns pointer to the Box associated with this Params.
	virtual Box* GetBox() {return 0;}
//! The orientation is used only with 2D Box Manipulators, and must be implemented for Params supporting such manipulators.  
//! Valid values are 0,1,2 for being orthog to X,Y,Z-axes.
//! Default is -1 (invalid)
//! \retval int orientation direction (0,1,2)
	virtual int getOrientation() { assert(0); return -1;}


//! Virtual method that must be re-implemented for rotated boxes, such as with ProbeParams.
//! Specifies an axis-aligned box containing the rotated box.
//! By default it just finds the box extents.
//! Caller must supply extents array, which gets its values filled in.
//! \param[out] float[6] Extents of rotated box
	virtual void calcContainingStretchedBoxExtentsInCube(float extents[6]) 
		{calcStretchedBoxExtentsInCube(extents, -1);}

//! This virtual method specifies that the box associated with this Params is constrained to stay within data extents.
//! Override this method to allow the manipulator to move the box outside of the data extents.
//! Default returns true.
//! \retval bool true if box not allowed to go completely outside of data.
	virtual bool isDomainConstrained() {return true;}

	//Following methods, while public, are not part of extensibility API
	
#ifndef DOXYGEN_SKIP_THIS
	//! For params that have a box (Necessary if using a Manipulator).
	//! This must be overridden to use a Manipulator.
	//! The Params class must implement this by setting its box extents.
	//! \param[in] float[3] boxMin  The minimum coordinates of the box.
	//! \param[in] float[3] boxMax  The maximum coordinates of the box.
	//! \param[in] int time step Current time step (only for moving boxes).
	void setBox(const float boxMin[3], const float boxMax[3], int timestep = -1 ) {
		double extents[6];
		for (int i = 0; i<3; i++){
			extents[i] = boxMin[i];
			extents[i+3] = boxMax[i];
		}
		GetBox()->SetExtents(extents,timestep);
	}
	
	void setTheta(float th) {
		double angles[3];
		GetBox()->GetAngles(angles);
		angles[0]=th;
		GetBox()->SetAngles(angles);
	}
	void setPhi(float ph) {
		double angles[3];
		GetBox()->GetAngles(angles);
		angles[1]=ph;
		GetBox()->SetAngles(angles);
	}
	void setPsi(float ps) {
		double angles[3];
		GetBox()->GetAngles(angles);
		angles[2]=ps;
		GetBox()->SetAngles(angles);
	}
	
	//! For params that have a box (Necessary if using a Manip).
	//! The params must implement this by supplying the current box extents.
	//! This must be overridden to use a manipulator.
	//! \param[out] float[3] boxMin  The minimum coordinates of the box.
	//! \param[out] float[3] boxMax  The maximum coordinates of the box.
	//! \param[in] int time step Current time step (only for moving boxes).
	void getBox(float boxMin[3], float boxMax[3], int timestep = -1) {
		double extents[6];
		GetBox()->GetExtents(extents, timestep);
		for (int i = 0; i<3; i++){
			boxMin[i] = extents[i];
			boxMax[i] = extents[i+3];
		}
	}
	
	float getPhi() {
		if (GetBox()->GetAngles().size() == 0) return 0.f;
		return((float)GetBox()->GetAngles()[1]);
	}
	float getTheta() {
		if (GetBox()->GetAngles().size() == 0) return 0.f;
		return((float)GetBox()->GetAngles()[0]);
	}
	float getPsi() {
		if (GetBox()->GetAngles().size() == 0) return 0.f;
		return((float)GetBox()->GetAngles()[2]);
	}

	static Params* CreateDummyParams(std::string tag);
	static void	BailOut (const char *errstr, const char *fname, int lineno);

	static int getNumDummyClasses(){return dummyParamsInstances.size();}
	static Params* getDummyParamsInstance(int i) {return dummyParamsInstances[i];}
	static void addDummyParamsInstance(Params*const & p ) {dummyParamsInstances.push_back(p);}

	static void clearDummyParamsInstances();
	static const std::string& paramName(ParamsBaseType t);
	static const string _dvrParamsTag;
	static const string _isoParamsTag;
	
	static const string _probeParamsTag;
	static const string _twoDParamsTag;
	static const string _twoDDataParamsTag;
	static const string _twoDImageParamsTag;
	static const string _regionParamsTag;
	static const string _viewpointParamsTag;
	static const string _animationParamsTag;
	static const string _flowParamsTag;
	static const string _RefinementLevelTag;
	static const string _CompressionLevelTag;
	static const string _VariableNameTag;
	static const string _VariableNamesTag;
	static const string _VisualizerNumTag;
	static const string _VariablesTag;

	static const string _vizNumAttr;
	static const string _localAttr;
	static const string _numVariablesAttr;
	static const string _variableTag;
	static const string _leftEditBoundAttr;
	static const string _rightEditBoundAttr;
	static const string _variableNumAttr;
	static const string _variableNameAttr;
	static const string _opacityScaleAttr;
	static const string _numTransformsAttr;
	static const string _useTimestepSampleListAttr;
	static const string _timestepSampleListAttr;


	void setStretchedBox(const float[3], const float[3], int);
	void getStretchedBox(float boxmin[3], float boxmax[3], int timestep);
	//Determine the box extents in the unit cube.
	void calcStretchedBoxExtentsInCube(float extents[6], int timestep);
	
	void calcStretchedBoxExtents(float* extents, int timestep);
	void calcBoxExtents(float* extents, int timestep);
	//Calculate the box in world coords, using any theta or phi
	virtual void calcBoxCorners(float corners[8][3], float extraThickness, int timestep, float rotation = 0.f, int axis = -1);
	
	// Construct transformation as a mapping of [-1,1]^3 into volume array
	// coordinates at current resolution
	void buildCoordTransform(float transformMatrix[12], float extraThickness, int timestep, float rotation = 0.f, int axis = -1);


	//Helper function for testing distances,
	//testing whether a point is inside or outside of cube 
	//Return is positive if point is outside.  Arguments are outward pointing
	//unit normal vectors and associated points (corners) of probe faces.
	static float distanceToCube(const float point[3],const float faceNormals[6][3], const float faceCorners[6][3]);

	
    //
    // Parse command line arguements 
    //
    static bool searchCmdLine(const char *flag);
    static const char* parseCmdLine(const char *flag);

	//Determine a new value of theta and phi when the probe is rotated around either the
	//x-, y-, or z- axis.  axis is 0,1,or 1. rotation is in degrees.
	void convertThetaPhiPsi(float *newTheta, float* newPhi, float* newPsi, int axis, float rotation);
	
	
protected:
	bool local;
	
	//Keep track of which window number corresp to this.  -1 for global or default parameters.
	//
	int vizNum;
	
	
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
	
		
	//! Indicate if the renderer is enabled
	//! \retval true if enabled
	bool isEnabled(){return enabled;}

	//! Set renderer to be enabled
	//! \param[in] value true if enabled
	virtual void setEnabled(bool value) {enabled = value; stopFlag = false;}

	//! Pure virtual method indicates if a particular variable name is currently used by the renderer.
	//! \param[in] varname name of the variable
	//!
	virtual bool usingVariable(const std::string& varname) = 0;
	//! Pure virtual method sets current number of refinements of this Params.
	//! \param[in] int refinements
	//!
	virtual void SetRefinementLevel(int numrefinements)=0;
	//! Pure virtual method indicates current number of refinements of this Params.
	//! \retval integer number of refinements
	//!
	virtual int GetRefinementLevel()=0;
	//! Pure virtual method indicates current Compression level.
	//! \retval integer compression level, 0 is most compressed
	//!
	virtual int GetCompressionLevel()=0;
	//! Pure virtual method sets current Compression level.
	//! \param[in] val  compression level, 0 is most compressed
	//!
	virtual void SetCompressionLevel(int val)=0;

	//! virtual method used only by params that support selecting points in 3D space, 
	//! and displaying those points with a 3D cursor.
	//! Default implementation returns null.
	//! \retval const float* pointer to 3D point.
	virtual const float* getSelectedPoint() {
		return 0;
	}

	//! Static method specifies the distance from camera to an axis-aligned box.
	// \param[in] ViewpointParams* vpp Current applicable ViewpointParams instance
	// \param[in] const float extents[6] Box extents in world coordinates.
	// \retval float distance from camera to box
	static float getCameraDistance(ViewpointParams* vpp, const double exts[6]);

	// Pure virtual method indicates whether or not the geometry is opaque.
	// \retval true if it is opaque.
	virtual bool IsOpaque()=0;

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

	
	//! Indicate that this class supports use of the VAPOR MapperFunction
	//! Default is false.  Currently needed to use colorbars.
	//! \retval bool true if this RenderParams can have a MapperFunction
	virtual bool UsesMapperFunction() {return false;}

#ifndef DOXYGEN_SKIP_THIS
	//Following methods are deprecated, used by some built-in renderparams classes
	
	//! virtual method specifies distance from camera to object.
	//! Default implementation finds distance to the applicable region box.
	//! Override this if the box associated with the Params is not the
	//! RegionParams box.
	// \param[in] vpp Current applicable ViewpointParams instance
	// \param[in] rp  Current applicable RegionParams instance
	// \param[in] timestep Current applicable time step
	// \retval float distance from camera
	virtual float getCameraDistance(ViewpointParams* vpp, RegionParams* rp, int timestep);
	
	//Deprecated constructor used by some built-in classes
	RenderParams(int winNum, const string& name) : Params(winNum, name) {
		
		local = true;
		
		previousClass = 0;
		minColorEditBounds = 0;
		maxColorEditBounds = 0;
		minOpacEditBounds = 0;
		maxOpacEditBounds = 0;
		
	}
	virtual ~RenderParams(){
		if (minColorEditBounds) delete [] minColorEditBounds;
		if (maxColorEditBounds) delete [] maxColorEditBounds;
		if (minOpacEditBounds) delete [] minOpacEditBounds;
		if (maxOpacEditBounds) delete [] maxOpacEditBounds;

	}
	
	virtual Params* deepCopy(ParamNode* nd = 0);
	virtual bool isRenderParams() {return true;}

	//following does nothing for renderParams
	virtual void setLocal(bool ){ assert(0);}

	virtual int getSessionVarNum(){assert(0); return -1;}
	virtual float GetHistoStretch() { assert(0); return 1.f;}
	virtual bool getEditMode() {assert(0); return true;}
	virtual const float* getCurrentDatarange(){assert(0); return(0);}
	virtual void hookupTF(TransferFunction* , int ) {assert(0);}

	
	//The following may be redefined by some renderer params.  Parent version should never be invoked
	virtual void setMinColorMapBound(float) {assert(0);}
	virtual void setMaxColorMapBound(float){assert(0);}
	virtual void setMinOpacMapBound(float){assert(0);}
	virtual void setMaxOpacMapBound(float){assert(0);}


	float getMinColorMapBound();	
	float getMaxColorMapBound(); 
	
	virtual void setMinColorEditBound(float val, int var) {
		minColorEditBounds[var] = val;
	}
	virtual void setMaxColorEditBound(float val, int var) {
		maxColorEditBounds[var] = val;
	}
	virtual float getMinColorEditBound(int var) {
		return minColorEditBounds[var];
	}
	virtual float getMaxColorEditBound(int var) {
		return maxColorEditBounds[var];
	}
	//And opacity:
	
	virtual float getMinOpacMapBound();	
	virtual float getMaxOpacMapBound(); 
	

	virtual void setMinOpacEditBound(float val, int var) {
		minOpacEditBounds[var] = val;
	}
	virtual void setMaxOpacEditBound(float val, int var) {
		maxOpacEditBounds[var] = val;
	}
	virtual float getMinOpacEditBound(int var) {
		return minOpacEditBounds[var];
	}
	virtual float getMaxOpacEditBound(int var) {
		return maxOpacEditBounds[var];
	}

	virtual MapperFunction* GetMapperFunc() {return 0;}
	
	void setStopFlag(bool val = true) {stopFlag = val;}
	bool getStopFlag() {return stopFlag;}
	int getBypassValue(int i) {return bypassFlags[i];} //only used for debugging
	
	void initializeBypassFlags();
	//Get a variable region from the datamanager.  May reduce the compression level, but not the refinement level
	float* getContainingVolume(size_t blkMin[3], size_t blkMax[3], int refinements, int varNum, int timeStep, bool twoDim);

protected:
	
	// The enabled flag is only used by renderer params
	//
	bool enabled;
	bool stopFlag;  //Indicates if user asked to stop (only with flow calc now)
	//Needed for renderer params:
	float* minColorEditBounds;
	float* maxColorEditBounds;
	float* minOpacEditBounds;
	float* maxOpacEditBounds;

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
	
	virtual void SetCompressionLevel(int){}
	
	virtual bool reinit(bool){return false;}
	
	virtual bool IsOpaque() {return true;}
	
	virtual bool usingVariable(const std::string& ){
		return false;
	}
	const std::string &getShortName(){return myTag;}

	std::string myTag;

};
#endif //DOXYGEN_SKIP_THIS
}; //End namespace VAPoR
#endif //PARAMS_H 
