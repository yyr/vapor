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
//! \ingroup Public_Params
//! \brief A class for describing the renderer instances in use in VAPOR
//! \author Alan Norton
//! \version 3.0
//! \date    May 2014
//!
//! This BasicParams class tracks the set of all renderer instances, including the current selection.
//!	It does not track the "current" instance, because that information can be retrieved from the
//! Params class.
//! Whenever a renderer instance is added, removed, or selected, methods on this class must be called, and this
//! class supports undo and redo of those operations.
//! It is also notified when a visualizer is added or removed.
//! Methods are also provided to retrieve the state in this class because the Renderer Instance Table will
//! refresh itself based on the current state of this class.
//! This Params class tracks the following state:
//!		The RenderParams instance added or removed during the most recent update (as a ParamNode)
//!     The current visualizers
//!		For each visualizer, all of the RenderParams instances (enabled or not) for that visualizer,
//!			identified by type and instance index and name.
//!		The ordering of these instances (e.g. as displayed in the instance table for that visualizer)
//!		The currently selected instance 
//!		In order to support undo this class also tracks the
//!			previous current index of the same type as the newly selected instance
//!			(on removeInstance, use instead the instance index that was removed)
//!
//!		The XML subtree of this node is as follows:
//!
//!		Root
//!		Visualizers		ParamNode(parent of identified RenderParams ParamNode ):  Two child nodes
//!			The ParamNode is only needed after Add or Remove instance operations, otherwise NULL.
//!		VizNN	VizMM	etc.  where NN, MM are visualizer indices: One child node for each visualizer.  
//!		(SelectedChildIndex, PreviousCurrentInstanceIndex) Name1,	Name2	etc.  (long 2-vector of selected instance index, previousCurrentIndex), plus names of instances in the order in which they appear.
//!			Indices and type a vector<long>, the Name's are child nodes.  The 2-vector has tag _selectionInfoTag
//!		(type1, index1) etc. (vector<long> Integers indicating ParamsBaseTypeId, instance index for the instance Name(s)
//!			vector<long> tag is _instanceInfoTag.
//!			Note that which instance of a given type is "current" is kept in the Params class.

class PARAMS_API InstanceParams : public BasicParams {
	
public: 

	//! Static method used to add a instance to the list of instances for a visualizer
	//! On error returns -1.
	//! Otherwise returns the instance index, which is the total number
	//! of instances minus 1.
	//! The instance that is added is selected, and becomes the current instance of its type (Tag)
	//! \param[in] rendererName render name associated with the instance
	//! \param[in] int visualizer index (as returned by ControlExec::NewVisualizer)
	//! \param[in] RenderParams* RenderParams instance that is being added.
	//! \retval int instance index associated with this visualizer
	static int AddInstance(const std::string rendererName, int viznum, RenderParams* p);

	//! Static method used to remove the selected instance from the set of instances for a particular visualizer
	//! The selected instance becomes the next instance, unless there is no next instance, in which case it becomes
	//! the previous instance.
	//! The current instance (for the renderer type) is also modified in accordance with Params::RemoveParams
	//! or the previous instance if next instance does not exist.
	//! Similarly if the instance is the selected instance.
	//! \param[in] index is visualizer index associated with the instance
	//! \retval int indicates 0 if successful
	static int RemoveSelectedInstance(int viz);

	//! Static method identifies index of the selected instance for a visualizer
	//! The returned index is the child index of the selected instance
	//! \param[in] index is visualizer index associated with the instance
	//! \retval -1 if error, otherwise returns the current instance index
	static int GetSelectedIndex(int viz){
		return ((InstanceParams*)Params::GetParamsInstance(_instanceParamsTag))->getSelectedIndex(viz);
	}
	//! Static method identifies the selected params by returning the ParamType and instance index
	//! \param[in] int viz visualizer index
	//! \param[out] int* (ParamsBaseType) pType
	//! \param[out] int* instance
	//! \retval -1 if error
	static int GetSelectedInstance(int viz, int* pType, int* instance);

	//! Static method causes an instance to be selected.
	//! \param[in] index is visualizer index associated with the instance
	//! \param[in] int (child) index to be selected 
	//! \retval 0 if successful.
	static int SetSelectedIndex(int viz, int index){
		return ((InstanceParams*)Params::GetParamsInstance(_instanceParamsTag))->setSelectedIndex(viz, index);
	}
	//! Static method determines the RenderParams that is selected
	//! \param[in] viz is the visualizer index
	//! \retval RenderParams* is the RenderParams* that is selected.
	static RenderParams* GetSelectedRenderParams(int viz);

	//! Static method identifies the RenderParams* with specified index
	//! \param[in] viz is the visualizer index
	//! \param[in] renindex is the renderer index
	//! \retval RenderParams* is the RenderParams* associated with the index
	static RenderParams* GetRenderParamsInstance(int viz, int renIndex);
	
	//! Static method indicates the number of renderer instances for a visualizer
	//! \param[in] viz is the visualizer index
	//! \retval int is the number of renderer instances
	static int GetNumInstances(int viz);

//! @name Internal
//! Internal methods not intended for general use
///@{

	//! Constructor
	//! \param[in] int winnum The window number, -1 since it's global
	InstanceParams(XmlNode* parent, int winnum);

	//! Destructor
	virtual ~InstanceParams();
	
	//! Method to validate all values in a InstanceParams instance
	//! \param[in] bool default indicates whether or not to set to default values associated with the current DataMgr
	//! \sa DataMgr
	virtual void Validate(int type);
	//! Method to initialize a new InstanceParams instance
	virtual void restart();
	
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
	//! Static method invoked during Undo and Redo of Instance params
	//! This performs undo and redo of creation and destruction of Params instances,
	//! as well as resetting of the current instance.
	//! This function must be passed in Command::CaptureStart
	//! \sa UndoRedoHelpCB_T
	//! \param[in] isUndo indicates whether an Undo or Redo is being performed
	//! \param[in] instance is not used for this Params
	//! \param[in] beforeP is a copy of the InstanceParams at the start of the Command
	//! \param[in] afterP is a copy of the InstanceParams at the end of the Command 
	static void UndoRedo(bool isUndo, int /*instance*/, Params* beforeP, Params* afterP);

	//! Static method that should be called whenever a visualizer is deleted
	//! This must agree with the VizWinParams state, so InstanceParams::RemoveVizWin()
	//! is invoked by VizWinParams::RemoveVizWin()
	//! \param[in] viznum Visualizer index.
	//! \retval 0 if successful.
	static int RemoveVizWin(int viznum);
	
	//! Required static method (for extensibility):
	//! \retval ParamsBase* pointer to a default Params instance
	static ParamsBase* CreateDefaultInstance() {return new InstanceParams(0, -1);}
	//! Pure virtual method on Params. Provide a short name suitable for use in the GUI
	//! \retval string name
	const std::string getShortName() {return _shortName;}
	//Determine what visualizer has changed between prev and next InstanceParams, 
	//and whether it's Add(1), Remove(-1), or Select(0) change.
	static int changeType(InstanceParams* p1, InstanceParams* p2, int* viz, int* type);
	
	//Convert a visualizer index into a string name, to be used for ParamNode tag
	static ParamNode* getVizNode(int viz);
	//Renumber the renderer instances associated with a visualizer, needed after
	//a new instance is inserted or an existing instance is removed.
	//! \param[in] int viz visualizer index
	//! \param[in] int position of the insertion or removal
	//! \param[in] bool isInsert true if insertion, false if removal
	//Return 0 if successful.
	static int renumberInstances(int viz, int changedType);
	
	///@}

#ifndef DOXYGEN_SKIP_THIS
	
	static const string _instanceParamsTag;
	static const string _shortName;
	static const string _visualizersTag;
	
	static const string _renderParamsNodeTag;
	static const string _instanceInfoTag;
	static const string _selectionInfoTag;
	int getCurrentInstance(std::string tag, int viz);
protected:
	
	ParamNode* getRenderParamsNode();
	void setRenderParamsNode(ParamNode* pnode);
	void removeRenderParamsNode(){
		GetRootNode()->GetNode(_renderParamsNodeTag)->DeleteAll();
	}

	//! following non-static Set/get methods are not public.
	int addVizWin(int viznum);
	
	int setSelectedIndex(int viz, int index);
	int getSelectedIndex(int viz);

	
#endif //DOXYGEN_SKIP_THIS
};

};
#endif //VIZWINPARAMS_H 
