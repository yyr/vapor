
//
//      $Id$
//
//***********************************************************************
//                                                                      *
//                      Copyright (C)  2006	                            *
//          University Corporation for Atmospheric Research             *
//                      All Rights Reserved                             *
//                                                                      *
//***********************************************************************
//
//	File:		ParamsIso.h
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Tue Dec 5 14:23:51 MST 2006
//
//	Description:	
//
//

#ifndef _ParamsIso_h_
#define _ParamsIso_h_


#include "vapor/XmlNode.h"
#include "vapor/ParamNode.h"
#include "vapor/ExpatParseMgr.h"
#include "params.h"
#include "datastatus.h"
#include "mapperfunction.h"
#include <vector>

namespace VAPoR{

class TransferFunction;
class IsoControl;
class MapperFunction;
class RegionParams;
class ViewpointParams;
//
//! \class ParamsIso
//! \brief A class for managing (storing and retrieving) 
//! isosurface parameters.
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! 
class PARAMS_API ParamsIso : public RenderParams {

	
public: 


 //! Create a ParamsIso abstract base class from scratch
 //!
 ParamsIso(XmlNode *parent, int winnum);

 virtual ~ParamsIso();

 //! Reset parameter state to the default
 //!
 //! This pure virtual method must be implented by the derived class to
 //! restore state to the default
 //
 virtual void restart();
 static void setDefaultPrefs();
 //Required for registration:
 static ParamsBase* CreateDefaultInstance() {return new ParamsIso(0,-1);}
 const std::string& getShortName() {return _shortName;}
 //Methods required by Params class:
 virtual int getNumRefinements() {
	 return GetRefinementLevel();
 }
 
 virtual RenderParams* deepRCopy();
 virtual Params* deepCopy(ParamNode* =0) {return (Params*)deepRCopy();}

 virtual int getSessionVarNum(){
	 return DataStatus::getInstance()->getSessionVariableNum(
		 GetIsoVariableName());
 }
 virtual bool reinit(bool override);

 virtual float GetHistoStretch();
 void SetHistoStretch(float scale);
 void SetIsoHistoStretch(float scale);
 float GetIsoHistoStretch();
 
 //Following is obsolete, left for compatibility
 virtual const float* getCurrentDatarange(){
	 return GetHistoBounds();
 }
 
//Obtain the transfer function
virtual MapperFunction* getMapperFunc(); 
IsoControl* getIsoControl();
TransferFunction* GetTransFunc(int sesVarNum);
IsoControl* GetIsoControl(int sesVarNum);

int GetNumVariables(){
	return(GetRootNode()->GetNode(_variablesTag)->GetNumChildren());
}
void SetMinMapEdit(int var, float val){
	const vector<double>& vec = GetRootNode()->GetNode(_variablesTag)->GetChild(var)->GetElementDouble(_editBoundsTag);
	vector<double> newvec(vec);
	newvec[1] = val;
	GetRootNode()->GetNode(_variablesTag)->GetChild(var)->SetElementDouble(_editBoundsTag, newvec);
}
void SetMaxMapEdit(int var, float val){
	const vector<double>& vec = GetRootNode()->GetNode(_variablesTag)->GetChild(var)->GetElementDouble(_editBoundsTag);
	vector<double> newvec(vec);
	newvec[3] = val;
	GetRootNode()->GetNode(_variablesTag)->GetChild(var)->SetElementDouble(_editBoundsTag, newvec);
}
void SetMinIsoEdit(int var, float val){
	const vector<double>& vec = GetRootNode()->GetNode(_variablesTag)->GetChild(var)->GetElementDouble(_editBoundsTag);
	vector<double> newvec(vec);
	newvec[0] = val;
	GetRootNode()->GetNode(_variablesTag)->GetChild(var)->SetElementDouble(_editBoundsTag, newvec);
}
void SetMaxIsoEdit(int var, float val){
	const vector<double>& vec = GetRootNode()->GetNode(_variablesTag)->GetChild(var)->GetElementDouble(_editBoundsTag);
	vector<double> newvec(vec);
	newvec[2] = val;
	GetRootNode()->GetNode(_variablesTag)->GetChild(var)->SetElementDouble(_editBoundsTag, newvec);
}
float GetMinMapEdit(int var){
	return (GetRootNode()->GetNode(_variablesTag)->GetChild(var)->GetElementDouble(_editBoundsTag)[1]);
}
float GetMaxMapEdit(int var){
	return (GetRootNode()->GetNode(_variablesTag)->GetChild(var)->GetElementDouble(_editBoundsTag)[3]);
}
float GetMinIsoEdit(int var){
	return(GetRootNode()->GetNode(_variablesTag)->GetChild(var)->GetElementDouble(_editBoundsTag)[0]);
}
float GetMaxIsoEdit(int var){
	return(GetRootNode()->GetNode(_variablesTag)->GetChild(var)->GetElementDouble(_editBoundsTag)[2]);
}

 //Virtual methods to set map bounds.  Get() is in parent class
//this causes it to be set in the mapperfunction (transfer function)
virtual void setMinColorMapBound(float val);
virtual void setMaxColorMapBound(float val);
virtual void setMinOpacMapBound(float val);
virtual void setMaxOpacMapBound(float val);

virtual bool isOpaque();


virtual void setMinOpacEditBound(float val, int sesvarnum) {
	SetMinMapEdit(sesvarnum,val);
}
virtual void setMaxOpacEditBound(float val, int sesvarnum) {
	SetMaxMapEdit(sesvarnum,val);
}
virtual float getMinOpacEditBound(int sesvarnum) {
	return GetMinMapEdit(sesvarnum);
}
virtual float getMaxOpacEditBound(int sesvarnum) {
	return GetMaxMapEdit(sesvarnum);
}
void setMinIsoEditBound(float val) {SetMinIsoEdit(GetIsoVariableNum(),val);} 
void setMaxIsoEditBound(float val) {SetMaxIsoEdit(GetIsoVariableNum(),val);} 

float getMinIsoEditBound() {
	return GetMinIsoEdit(GetIsoVariableNum());
}
float getMaxIsoEditBound() {
	return GetMaxIsoEdit(GetIsoVariableNum());
}
 void SetIsoValue(double value);
 double GetIsoValue();
 
 void RegisterIsoValueDirtyFlag(ParamNode::DirtyFlag *df);
 void RegisterColorMapDirtyFlag(ParamNode::DirtyFlag *df);
 void RegisterMapBoundsDirtyFlag(ParamNode::DirtyFlag *df);
 void RegisterHistoBoundsDirtyFlag(ParamNode::DirtyFlag *df);

 void SetNormalOnOff(bool flag);
 bool GetNormalOnOff();
 void RegisterNormalOnOffDirtyFlag(ParamNode::DirtyFlag *df);

 void SetConstantColor(float rgba[4]);
 const float *GetConstantColor();
 void RegisterConstantColorDirtyFlag(ParamNode::DirtyFlag *df);

 bool getMapEditMode();
 bool getIsoEditMode();
 void setMapEditMode(bool val);
 void setIsoEditMode(bool val);
 float getOpacityScale(); 
 void setOpacityScale(float val); 

 void SetHistoBounds(float bnds[2]);
 void SetMapBounds(float bnds[2]);
 const float* GetHistoBounds();
 const float* GetMapBounds();

 void SetSelectedPoint(const float pnt[3]);
 const vector<double>& GetSelectedPoint();

 void SetRefinementLevel(int level);
 int GetRefinementLevel();
 void RegisterRefinementDirtyFlag(ParamNode::DirtyFlag *df);

 void SetVisualizerNum(int viznum);
 int GetVisualizerNum();

 void SetNumBits(int nbits);
 int GetNumBits();
 void RegisterNumBitsDirtyFlag(ParamNode::DirtyFlag*);
int GetMapVariableNum(){
	//Note:  -1 is returned if there is no match.  That indicates no mapping.
	return  DataStatus::getInstance()->getSessionVariableNum(GetMapVariableName());
}
int GetIsoVariableNum(){
	int varnum = DataStatus::getInstance()->getSessionVariableNum(GetIsoVariableName());
	if (varnum < 0) return 0; else return varnum;
}
 void SetIsoVariableName(const string& varName);
 const string& GetIsoVariableName();
 void SetMapVariableName(const string& varName);
 const string& GetMapVariableName();
 void RegisterVariableDirtyFlag(ParamNode::DirtyFlag *df);
 void RegisterMapVariableDirtyFlag(ParamNode::DirtyFlag *df);

 
 float (&getClut())[256][4] {
		refreshCtab();
		return ctab;
	}
 void refreshCtab();
 
 virtual bool elementStartHandler(
		ExpatParseMgr* pm, int depth, string& tag, const char ** attribs);
 virtual bool elementEndHandler(ExpatParseMgr* pm, int depth, string& tag);

 virtual void hookupTF(TransferFunction* tf, int index);

 static int getDefaultBitsPerVoxel() {return defaultBitsPerVoxel;}
 static void setDefaultBitsPerVoxel(int val){defaultBitsPerVoxel = val;}

 static const string _IsoValueTag;
 static const string _IsoControlTag;
 static const string _ColorMapTag;
 static const string _HistoBoundsTag;
 static const string _MapBoundsTag;

protected:
static const string _shortName;

private:
 static const string _editBoundsTag;
 static const string _mapEditModeTag;
 static const string _isoEditModeTag;
 static const string _NormalOnOffTag;
 static const string _ConstantColorTag;

 static const string _HistoScaleTag;
 static const string _MapHistoScaleTag;
 static const string _SelectedPointTag;
 static const string _RefinementLevelTag;
 static const string _VisualizerNumTag;
 static const string _VariableNameTag;
 static const string _MapVariableNameTag;
 static const string _NumBitsTag;


 float _constcolorbuf[4];
 float _histoBounds[2];
 float _mapperBounds[2];

 
 int parsingVarNum;
 float ctab[256][4];
 
 // The noIsoControlTags flag indicates that the last time we parsed a ParamsIso there
 // were not Iso control tags in it (i.e., it was an obsolete ParamsIso that was parsed).
 // In that situation, the isovalue and histo bounds were saved, so they will be used
 // when new metadata is read.
 bool noIsoControlTags; 
 float oldIsoValue;
 float oldHistoBounds[2];
 static int defaultBitsPerVoxel;
 
 
};

};
#endif // _ParamsIso_h_
