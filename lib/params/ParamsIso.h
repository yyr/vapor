
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
#include "vapor/ExpatParseMgr.h"
#include "params.h"
#include "datastatus.h"
#include "mapperfunction.h"
#include <vector>

namespace VAPoR{

class TransferFunction;
class IsoControl;
class MapperFunction;
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

 //Methods required by Params class:
 virtual int getNumRefinements() {
	 return GetRefinementLevel();
 }
 virtual RenderParams* deepRCopy();
 virtual Params* deepCopy() {return (Params*)deepRCopy();}

 virtual int getSessionVarNum(){
	 return DataStatus::getInstance()->getSessionVariableNum(
		 GetVariableName());
 }
 virtual bool reinit(bool override);

 virtual float GetHistoStretch();
 void SetHistoStretch(float scale);
 void SetIsoHistoStretch(float scale);
 float GetIsoHistoStretch();
 
 virtual const float* getCurrentDatarange(){
	 return GetHistoBounds();
 }
 
//Obtain the transfer function
virtual MapperFunction* getMapperFunc(); 
IsoControl* getIsoControl();
 //Virtual methods to set map bounds.  Get() is in parent class
//this causes it to be set in the mapperfunction (transfer function)
virtual void setMinColorMapBound(float val);
virtual void setMaxColorMapBound(float val);
virtual void setMinOpacMapBound(float val);
virtual void setMaxOpacMapBound(float val);

void setMinMapEditBound(float val) {
	setMinColorEditBound(val, GetMapVariableNum());
	setMinOpacEditBound(val, GetMapVariableNum());
}
void setMaxMapEditBound(float val) {
	setMaxColorEditBound(val, GetMapVariableNum());
	setMaxOpacEditBound(val, GetMapVariableNum());
}
float getMinMapEditBound() {
	return minColorEditBounds[GetMapVariableNum()];
}
float getMaxMapEditBound() {
	return maxColorEditBounds[GetMapVariableNum()];
}
void setMinIsoEditBound(float val) {minIsoEditBounds[GetIsoVariableNum()] = val;} 
void setMaxIsoEditBound(float val) {maxIsoEditBounds[GetIsoVariableNum()] = val;}  

float getMinIsoEditBound() {
	return minIsoEditBounds[GetIsoVariableNum()];
}
float getMaxIsoEditBound() {
	return maxIsoEditBounds[GetIsoVariableNum()];
}
 void SetIsoValue(double value);
 double GetIsoValue();
 void RegisterIsoControlDirtyFlag(ParamNode::DirtyFlag *df);

 void RegisterColorMapDirtyFlag(ParamNode::DirtyFlag *df);

 void SetNormalOnOff(bool flag);
 bool GetNormalOnOff();
 void RegisterNormalOnOffDirtyFlag(ParamNode::DirtyFlag *df);

 void SetConstantColor(float rgba[4]);
 const float *GetConstantColor();
 void RegisterConstantColorDirtyFlag(ParamNode::DirtyFlag *df);
 bool getIsoEditMode(){return isoEditMode;}
 bool getMapEditMode(){return mapEditMode;}
 void setIsoEditMode(bool val){isoEditMode = val;}
 void setMapEditMode(bool val){mapEditMode = val;}
 float getOpacityScale(); 
 void setOpacityScale(float val); 

 //Force histo bounds to match IsoControl bounds
 void updateHistoBounds();
 

 void SetHistoBounds(float bnds[2]);
 void SetMapBounds(float bnds[2]){
	 getMapperFunc()->setMinOpacMapValue(bnds[0]);
	 getMapperFunc()->setMaxOpacMapValue(bnds[1]);
	 _rootParamNode->SetNodeDirty(_ColorMapTag);
 }
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
	return DataStatus::getInstance()->getSessionVariableNum(GetMapVariableName());
}
int GetIsoVariableNum(){
	return DataStatus::getInstance()->getSessionVariableNum(GetVariableName());
}
 void SetVariableName(const string& varName);
 const string& GetVariableName();
 void SetMapVariableName(const string& varName);
 const string& GetMapVariableName();
 void RegisterVariableDirtyFlag(ParamNode::DirtyFlag *df);
 void RegisterMapVariableDirtyFlag(ParamNode::DirtyFlag *df);

 virtual XmlNode* buildNode();
 float (&getClut())[256][4] {
		refreshCtab();
		return ctab;
	}
 void refreshCtab();
 
 virtual bool elementStartHandler(
		ExpatParseMgr* pm, int depth, string& tag, const char ** attribs);
 virtual bool elementEndHandler(ExpatParseMgr* pm, int depth, string& tag);

 virtual void hookupTF(TransferFunction* tf, int index);

 static const string _IsoValueTag;
 static const string _IsoControlTag;
 static const string _ColorMapTag;

protected:
	

private:
 
 static const string _NormalOnOffTag;
 static const string _ConstantColorTag;
 static const string _HistoBoundsTag;//obsolete
 static const string _HistoScaleTag;
 static const string _MapHistoScaleTag;
 static const string _SelectedPointTag;
 static const string _RefinementLevelTag;
 static const string _VisualizerNumTag;
 static const string _VariableNameTag;
 static const string _MapVariableNameTag;
 static const string _NumBitsTag;

 float* minIsoEditBounds;
 float* maxIsoEditBounds;

 float _constcolorbuf[4];
 float _histoBounds[2];
 bool isoEditMode;
 bool mapEditMode;
 
 std::vector<IsoControl*> isoControls;
 std::vector<TransferFunction*> transFunc;
 int parsingVarNum;
 int numSessionVariables;
 float ctab[256][4];
 
};

};
#endif // _ParamsIso_h_
