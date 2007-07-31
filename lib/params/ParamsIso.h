
//
//      $Id$
//
//***********************************************************************
//                                                                      *
//                      Copyright (C)  2006	                        *
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
 ParamsIso(XmlNode *parent);

 virtual ~ParamsIso();

 //! Reset parameter state to the default
 //!
 //! This pure virtual method must be implented by the derived class to
 //! restore state to the default
 //
 virtual void restart();

 void SetIsoValue(double value);
 double GetIsoValue();
 void RegisterIsoValueDirtyFlag(ParamNode::DirtyFlag *df);

 void SetNormalOnOff(bool flag);
 bool GetNormalOnOff();
 void RegisterNormalOnOffDirtyFlag(ParamNode::DirtyFlag *df);


 void SetConstantColor(float rgba[4]);
 const float *GetConstantColor();
 void RegisterConstantColorDirtyFlag(ParamNode::DirtyFlag *df);


private:
 static const string _IsoValueTag;
 static const string _NormalOnOffTag;
 static const string _ConstantColorTag;

 float _constcolorbuf[4];
 
};

};
#endif // _ParamsIso_h_
