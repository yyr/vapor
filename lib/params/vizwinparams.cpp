//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		vizwinparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2014
//
//	Description:	Implements the VizWinParams class.
//		This class supports parameters associted with Vapor Visualizers
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif



#include <vector>
#include <string>
#include "params.h"
#include "command.h"
#include <vapor/ParamNode.h>

using namespace VAPoR;
#include "vizwinparams.h"
#include "instanceparams.h"


const std::string VizWinParams::_shortName = "VizWin";
const string VizWinParams::_vizWinParamsTag = "VizWinParams";
const string VizWinParams::_windowWidthsTag = "WindowWidths";
const string VizWinParams::_windowHeightsTag = "WindowHeight";
const string VizWinParams::_windowNamesTag = "WindowNames";
const string VizWinParams::_currentWindowTag = "CurrentVisualizer";
const string VizWinParams::_visualizerNumsTag = "VisualizerNums";
const string VizWinParams::_emptyString = "";


VizWinParams::VizWinParams(XmlNode* parent, int winnum): BasicParams(parent, VizWinParams::_vizWinParamsTag){
	restart();
}


VizWinParams::~VizWinParams(){
}


//Reset settings to initial state
void VizWinParams::
restart(){
	setCurrentVizWin(-1);
}
//Reinitialize settings, session has changed

void VizWinParams::
Validate(bool doOverride){
	//Command capturing should be disabled
	assert(!Command::isRecording());
	
	
	//Otherwise, verify that this is a registered mouse mode
	//else if (GetCurrentMouseMode()
	return;	
	
}
int VizWinParams::AddVizWin(const std::string name, int viznum, int width, int height){ 
	
	VizWinParams* p = (VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag);
	Command* cmd = Command::CaptureStart(p,"Add new visualizer");
	vector<long> viznums= p->getVisualizerNums();
	InstanceParams::AddVizWin(viznum);
	//If this index is already in use, do nothing!
	for (int i = 0; i<viznums.size(); i++){
		if (viznums[i] == viznum ) {
			return 0;
		}
	}
	
	// add the new element at the end of the arrays:
	viznums.push_back(viznum);
	vector<long> widths = p->getWindowWidths();
	vector<long> heights = p->getWindowHeights();
	vector<string>winnames = p->getWindowNames();
	
	widths.push_back(width);
	heights.push_back(height);
	winnames.push_back(name);
	
	p->setVisualizerNums(viznums);
	p->setWindowHeights(heights);
	p->setWindowWidths(widths);
	p->setWindowNames(winnames);
	
	Command::CaptureEnd(cmd, p);
	return 0;
}

int VizWinParams::RemoveVizWin(int viz){
	int win = mapVizToWin(viz);
	if (win<0) return -1;
	VizWinParams* p = (VizWinParams*)Params::GetParamsInstance(_vizWinParamsTag);
	vector<long> widths = p->getWindowWidths();
	vector<long> heights = p->getWindowHeights();
	vector<long> viznums= p->getVisualizerNums();
	vector<string>winnames = p->getWindowNames();
	if (viznums.size()<=1) return -1;
	int curviz = p->getCurrentVizWin();

	widths.erase(widths.begin()+win);
	heights.erase(heights.begin()+win);
	winnames.erase(winnames.begin()+win);
	viznums.erase(viznums.begin()+win);
	if (curviz == viz)
		p->setCurrentVizWin(viznums[0]);
	p->setWindowHeights(heights);
	p->setWindowWidths(widths);
	p->setWindowNames(winnames);
	p->setVisualizerNums(viznums);
	InstanceParams::RemoveVizWin(viz);
	return 0;
}


