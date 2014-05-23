//************************************************************************
//																		*
//		     Copyright (C)  1024										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		vizwinparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2014
//
//	Description:	Defines the VizWinParams class.
//		This Params class is global.  Specifies properties of the VizWindows
//		Also has various static methods for obtaining and setting properties of the VizWindows
//		Each vizwin has associated with it the following:
//		index >= 0, indicating the visualizer number (as used by the ControlExec)
//		name, a string
//		width and height 
//		Several attributes apply to the set of visualizers:
//		the "current" visualizer
//		the number of visualizers
//		
//
#ifndef VIZWINPARAMS_H
#define VIZWINPARAMS_H


#include <vector>
#include <map>
#include <algorithm>
#include <vapor/common.h>
#include "params.h"
#include <vapor/MyBase.h>


using namespace VetsUtil;

namespace VAPoR {


//! \class VizWinParams
//! \brief A class for describing the vizualization windows in use in VAPOR
//! \author Alan Norton
//! \version 3.0
//! \date    April 2014
//!
class PARAMS_API VizWinParams : public Params {
	
public: 

	//! \param[in] int winnum The window number, -1 since it's global
	VizWinParams(XmlNode* parent, int winnum);

	//! Destructor
	virtual ~VizWinParams();
	
	//! Method to validate all values in a VizWinParams instance
	//! \param[in] bool default indicates whether or not to set to default values associated with the current DataMgr
	//! \sa DataMgr
	virtual void Validate(bool useDefault);
	//! Method to initialize a new VizWinParams instance
	virtual void restart();
	//! The vizwin params are just for UndoRedo and sessions (i.e., do they not show up as tabs in the GUI)
	//! \retval always returns true for this class.
	virtual bool isBasicParams() const {return true;}

	//! Static method used to add a new Viz Win to the list of viz windows
	//! If the specified visualizer already exists returns -1.
	//! Otherwise returns the window index, which is the total number
	//! of visualizers minus 1.
	//! \param[in] const char* name of window
	//! \param[in] int visualizer index (as returned by ControlExec::NewVisualizer)
	//! \param[in] width desired width
	//! \param[in] height desired height
	//! \retval int window index associated with this visualizer
	static int AddVizWin(const std::string name, int viznum, int width, int height);

	//! Static method used to remove a Viz Win from the list of viz windows
	//! \param[in] index is visualizer index associated with the window
	//! \retval int indicates 0 if successful
	static int RemoveVizWin(int viz);

	//! Static method identifies the current visualizer
	//! \retval visualizer index
	static int  GetCurrentVizWin(){
		return ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getCurrentVizWin();
	}
	//! Static method makes a visualizer current
	//! \param[in] int t current viz win
	//! \retval 0 if successful.
	static int SetCurrentVizWin(int viz){
		return ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->setCurrentVizWin(viz);
	}
	//! Static method identifies the width of the specified visualizer
	//! \param[in] winnum is the visualizer index.
	//! \retval width of window, or -1 if it does not exist.
	int GetWindowWidth(int viznum){
		int winnum = mapVizToWin(viznum);
		if (winnum < 0) return -1;
		return ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getWindowWidths()[winnum];
	}

	//! Static method identifies the height of the specified visualizer
	//! \param[in] winnum is the visualizer index.
	//! \retval width of window, or -1 if it does not exist.
	int GetWindowHeight(int viznum){
		int winnum = mapVizToWin(viznum);
		if (winnum < 0) return -1;
		return ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getWindowHeights()[winnum];
	}
	
	//! Static method sets the width of the specified visualizer
	//! \param[in] viznum is the visualizer index.
	//! \param[in] width is window width
	//! \retval 0 if successful
	static int SetWindowWidth(int viznum, int width){
		int winnum = mapVizToWin(viznum);
		if (winnum < 0) return -1;
		std::vector<long> widths =  ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getWindowWidths();
		widths[winnum] = width;
		return ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->setWindowWidths(widths);
	}
	//! Static method sets the height of the specified visualizer
	//! \param[in] viznum is the visualizer index.
	//! \param[in] width is window width
	//! \retval 0 if successful
	static int SetWindowHeight(int viznum, int height){
		int winnum = mapVizToWin(viznum);
		if (winnum < 0) return -1;
		vector<long>heights =  ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getWindowHeights();
		heights[winnum] = height;
		return ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->setWindowHeights(heights);
	}

	//! Static method specifies the name of the specified visualizer
	//! Note that names with embedded underscores will have these characters converted to blanks.
	//! \param[in] winnum is the visualizer index.
	//! \retval 0 if successful
	static int SetVizName(int viznum, string name){
		int winnum = mapVizToWin(viznum);
		if (winnum < 0) return -1;
		vector<string> names = ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getWindowNames();
		names[winnum] = name;
		return ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->setWindowNames(names);
	}
	//! Static method obtains the name of the specified visualizer
	//! \param[in] winnum is the visualizer index.
	//! \retval 0 if successful
	static string GetVizName(int viznum){
		int winnum = mapVizToWin(viznum);
		if (winnum < 0) return _emptyString;
		return ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getWindowNames()[winnum];
	}

	//! Static method indicates how many windows exist
	//! \retval returns the number of available windows.
	static int GetNumVizWins(){
		return ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getVisualizerNums().size();
	}
	//! Static method returns a vector of visualizer indices
	//! indicating the visualizer numbers being used in this session
	static vector<long> GetVisualizerNums(){
		return ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getVisualizerNums();
	}
	
	//! Required static method (for extensibility):
	//! \retval ParamsBase* pointer to a default Params instance
	static ParamsBase* CreateDefaultInstance() {return new VizWinParams(0, -1);}
	//! Pure virtual method on Params. Provide a short name suitable for use in the GUI
	//! \retval string name
	const std::string getShortName() {return _shortName;}
	//! Method returns a vector of visualizer indices
	//! indicating the visualizer numbers being used in the session
	//! associated with this current VizWinParams instance
	vector<long> getVisualizerNums(){
		return GetValueLongVec(_visualizerNumsTag);
	}
	//! Non-static method identifies the current visualizer.  
	//! Only needed for Undo/Redo
	//! \retval visualizer index
	int getCurrentVizWin(){
		return GetValueLong(_currentWindowTag);
	}

#ifndef DOXYGEN_SKIP_THIS
	
	static const string _vizWinParamsTag;
	static const string _shortName;
	static const string _windowWidthsTag;
	static const string _windowHeightsTag;
	static const string _windowNamesTag;
	static const string _currentWindowTag;
	static const string _visualizerNumsTag;
	static const string _emptyString;

protected:
	//! Determine the window number associated with a visualizer index
	//! \param[in] viznum is a visualizer index
	//! \retval is the associated window number, or -1 if it does not exist.
	static int mapVizToWin(int viznum){
		vector<long> viznums = ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getVisualizerNums();
		for (int i = 0; i<viznums.size(); i++){
			if (viznums[i] == viznum) return i;
		}
		return -1;
	}

	//! Determine the visualizer number associated with a window index
	//! \param[in] viznum is a visualizer index
	//! \retval is the associated window number, or -1 if it does not exist.
	static int mapWinToViz(int winnum){
		vector<long> viznums = ((VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag))->getVisualizerNums();
		if (winnum < 0 || winnum >= viznums.size()) return -1;
		return viznums[winnum];
	}

	//! following non-static Set/get methods are not public.
	int setCurrentVizWin(int viz){
		return SetValueLong(_currentWindowTag, "Set current viz win",(long)viz);
	}
	
	vector<long> getWindowWidths(){
		return GetValueLongVec(_windowWidthsTag);
	}
	vector<long>getWindowHeights(){
		return GetValueLongVec(_windowHeightsTag);
	}
	int setWindowWidths(const vector<long> widths){
		return SetValueLong(_windowWidthsTag, "Set window widths",widths);
	}

	int setWindowHeights(const vector<long>& heights){
		return SetValueLong(_windowHeightsTag, "Set window heights",heights);
	}
	
	int setVisualizerNums(const vector<long>& viznums){
		return SetValueLong(_visualizerNumsTag,"Set visualizer numbers",viznums);
	}
	vector<string> getWindowNames(){
		std::vector<string> names;
		GetValueStringVec(_windowNamesTag,names);
		//convert "_" to " ":
		for (int i = 0; i<names.size(); i++){
			std::replace(names[i].begin(),names[i].end(),'_',' ');
		}
		return names;
	}
	int setWindowNames(const vector<string> names){
		//Replace blanks with underscores
		vector<string> namemod = names;
		for (int i = 0; i<namemod.size(); i++){
			std::replace(namemod[i].begin(),namemod[i].end(),' ','_');
		}
		return SetValueStringVec(_windowNamesTag," Set visualizer names", namemod);
	}
	
#endif //DOXYGEN_SKIP_THIS
};

};
#endif //VIZWINPARAMS_H 
