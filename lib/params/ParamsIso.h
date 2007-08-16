
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

namespace VAPoR{

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
 
 //Note:  This is not right.  Unclear if datarange is used in Iso
 virtual const float* getCurrentDatarange(){
	 return GetHistoBounds();
 }
 
 //Following not yet available for iso params:
 virtual MapperFunction* getMapperFunc() {return dummyMapperFunc;}
 void setMinColorMapBound(float) {}
 void setMaxColorMapBound(float) {}

 //Opacity map/edit bounds are being used for histo bounds:
 void setMinOpacMapBound(float minhisto);
 void setMaxOpacMapBound(float maxhisto);
 virtual float getMinOpacMapBound(){
	 return GetHistoBounds()[0];
 }
 virtual float getMaxOpacMapBound(){
	 return GetHistoBounds()[1];
 }
 virtual void setMinOpacEditBound(float val, int ) {
		minOpacEditBounds[0] = val;
}
virtual void setMaxOpacEditBound(float val, int ) {
	maxOpacEditBounds[0] = val;
}
virtual float getMinOpacEditBound(int ) {
	return minOpacEditBounds[0];
}
virtual float getMaxOpacEditBound(int ) {
	return maxOpacEditBounds[0];
}

 void SetIsoValue(double value);
 double GetIsoValue();
 void RegisterIsoValueDirtyFlag(ParamNode::DirtyFlag *df);

 void SetNormalOnOff(bool flag);
 bool GetNormalOnOff();
 void RegisterNormalOnOffDirtyFlag(ParamNode::DirtyFlag *df);

 void SetConstantColor(float rgba[4]);
 const float *GetConstantColor();
 void RegisterConstantColorDirtyFlag(ParamNode::DirtyFlag *df);

 //Force histo bounds to match dummyMapperFunction bounds
 void updateHistoBounds();


 void SetHistoBounds(float bnds[2]);
 const float* GetHistoBounds();

 void SetHistoStretch(float scale);
 

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

 void SetVariableName(const string& varName);
 const string& GetVariableName();
 void RegisterVariableDirtyFlag(ParamNode::DirtyFlag *df);
 virtual XmlNode* buildNode();

private:
 static const string _IsoValueTag;
 static const string _NormalOnOffTag;
 static const string _ConstantColorTag;
 static const string _HistoBoundsTag;
 static const string _HistoScaleTag;
 static const string _SelectedPointTag;
 static const string _RefinementLevelTag;
 static const string _VisualizerNumTag;
 static const string _VariableNameTag;
 static const string _NumBitsTag;


 float _constcolorbuf[4];
 float _histoBounds[2];

 MapperFunction* dummyMapperFunc;
 
};

};
#endif // _ParamsIso_h_
