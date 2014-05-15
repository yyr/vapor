//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		instanceparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2014
//
//	Description:	Defines the InstanceParams class.
//		This Params class is global.  Specifies the current instances associated with RenderParams.
//		Also has various static methods for obtaining and setting properties of the Instance Params
//		For each Params (tag) and each Visualizer (number), there is one or more instances
//		and one of the instances is the Current instance.
//		In addition, whenever a new instance is created or deleted, that new instance's paramnode is
//		saved in the InstanceParams, to facilitate undo/redo of creation and deletion.
//		The Paramnode in the Instance params is otherwise not changed
//		
//
#ifndef INSTANCEPARAMS_H
#define INSTANCEPARAMS_H


#include <vector>
#include <map>
#include <algorithm>
#include <vapor/common.h>
#include "params.h"
#include <vapor/MyBase.h>


using namespace VetsUtil;

namespace VAPoR {


//! \class InstanceParams
//! \brief A class for describing the renderer instances in use in VAPOR
//! \author Alan Norton
//! \version 3.0
//! \date    May 2014
//!
class PARAMS_API InstanceParams : public Params {
	
public: 

	//! \param[in] int winnum The window number, -1 since it's global
	InstanceParams(XmlNode* parent, int winnum);

	//! Destructor
	virtual ~InstanceParams();
	
	//! Method to validate all values in a InstanceParams instance
	//! \param[in] bool default indicates whether or not to set to default values associated with the current DataMgr
	//! \sa DataMgr
	virtual void Validate(bool useDefault);
	//! Method to initialize a new InstanceParams instance
	virtual void restart();
	//! The vizwin params are just for UndoRedo and sessions (i.e., do they not show up as tabs in the GUI)
	//! \retval always returns true for this class.
	virtual bool isUndoRedoParams() const {return true;}

	//! Static method used to add a instance to the list of instances for a visualizer
	//! On error returns -1.
	//! Otherwise returns the instance index, which is the total number
	//! of instances minus 1.
	//! \param[in] tag renderParams Tag associated with the instance
	//! \param[in] int visualizer index (as returned by ControlExec::NewVisualizer)
	//! \param[in] Params* RenderParams instance that is being added.
	//! \retval int instance index associated with this visualizer
	static int AddInstance(const std::string tag, int viznum, Params* p);

	//! Static method used to remove an instance from the set of instances for a particular visualizer
	//! \param[in] tag renderParams Tag associated with the instance
	//! \param[in] index is visualizer index associated with the instance
	//! \param[in] instance is instance index associated with the instance
	//! \retval int indicates 0 if successful
	static int RemoveInstance(const std::string tag, int viz, int instance);

	//! Static method that should be called whenever a new visualizer is created
	//! This must agree with the VizWinParams state, so InstanceParams::AddVizWin()
	//! is invoked by VizWinParams::AddVizWin()
	//! If specified visualizer is already there, return 0; if not build the default
	//! node representing the visualizer.
	//! \param[in] viznum Visualizer index.
	//! \retval 0 if successful.
	static int AddVizWin(int viznum){
		return (((InstanceParams*)Params::GetParamsInstance(_instanceParamsTag))->addVizWin(viznum));
	}

	//! Static method that should be called whenever a visualizer is deleted
	//! This must agree with the VizWinParams state, so InstanceParams::RemoveVizWin()
	//! is invoked by VizWinParams::RemoveVizWin()
	//! \param[in] viznum Visualizer index.
	//! \retval 0 if successful.
	static int RemoveVizWin(int viznum);

	//! Static method identifies the current instance for a renderer and visualizer 
	//! \param[in] tag renderParams Tag associated with the instance
	//! \param[in] index is visualizer index associated with the instance
	//! \retval -1 if error, otherwise returns the current instance index
	static int  GetCurrentInstance(const std::string tag, int viz){
		return ((InstanceParams*)Params::GetParamsInstance(_instanceParamsTag))->getCurrentInstance(tag, viz);
	}
	//! Static method makes an instance current
	//! \param[in] tag renderParams Tag associated with the instance
	//! \param[in] index is visualizer index associated with the instance
	//! \param[in] int current instance
	//! \retval 0 if successful.
	static int SetCurrentInstance(const std::string tag, int viz, int instance){
		return ((InstanceParams*)Params::GetParamsInstance(_instanceParamsTag))->setCurrentInstance(tag, viz, instance);
	}
	
	
	//! Required static method (for extensibility):
	//! \retval ParamsBase* pointer to a default Params instance
	static ParamsBase* CreateDefaultInstance() {return new InstanceParams(0, -1);}
	//! Pure virtual method on Params. Provide a short name suitable for use in the GUI
	//! \retval string name
	const std::string getShortName() {return _shortName;}

#ifndef DOXYGEN_SKIP_THIS
	
	static const string _instanceParamsTag;
	static const string _shortName;
	static const string _visualizersTag;
	static const string _numInstancesTag;
	static const string _currentInstanceTag;
	static const string _paramNodeTag;

protected:
	
	//! following non-static Set/get methods are not public.
	int addVizWin(int viznum);
	int setCurrentInstance(std::string tag, int viz, int instance);
	int getCurrentInstance(std::string tag, int viz);
	
	int getNumInstances(std::string tag, int viz);
	
	
#endif //DOXYGEN_SKIP_THIS
};

};
#endif //VIZWINPARAMS_H 
