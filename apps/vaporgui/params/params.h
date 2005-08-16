//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
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
//	Description:	Defines the Params class.
//		This is an abstract class for all the tabbed panel params classes.
//		Supports functionality common to all the tabbed panel params.
//
#ifndef PARAMS_H
#define PARAMS_H
 
#include <qwidget.h>
#include "assert.h"
#include "vapor/ExpatParseMgr.h"
class QWidget;

//Error tolerance for gui parameters:
#define ROUND_OFF 1.e-6f
namespace VAPoR{
class MainForm;
class VizWinMgr;
class Session;
class PanelCommand;
class XmlNode;
class MapperFunction;
class Params : public ParsedXml  {
	
public: 
	Params(int winNum) {
		vizNum = winNum;
		if(winNum < 0) local = false; else local = true;
		textChangedFlag = false;
		thisParamType = UnknownParamsType;
		previousClass = 0;
		minColorEditBounds = 0;
		maxColorEditBounds = 0;
		minOpacEditBounds = 0;
		maxOpacEditBounds = 0;
	}
	virtual ~Params(){
		if (minColorEditBounds) delete minColorEditBounds;
		if (maxColorEditBounds) delete maxColorEditBounds;
		if (minOpacEditBounds) delete minOpacEditBounds;
		if (maxOpacEditBounds) delete maxOpacEditBounds;
	}
	
	enum ParamType {
		UnknownParamsType,
		ViewpointParamsType,
		RegionParamsType,
		IsoParamsType,
		DvrParamsType,
		ContourParamsType,
		AnimationParamsType,
		FlowParamsType
	};
	static QString& paramName(ParamType);
	static const string _dvrParamsTag;
	static const string _regionParamsTag;
	static const string _viewpointParamsTag;
	static const string _animationParamsTag;
	static const string _flowParamsTag;
	static const string _vizNumAttr;
	static const string _localAttr;
	static const string _numVariablesAttr;
	
	//Each params must be able to make a "deep" copy,
	//I.e. copy everything that is unique to this object
	//
	virtual Params* deepCopy() = 0;

	//Method to make a specific panel current (to the vizwinmgr).
	//Previously current panel for this window is replaced, but state
	//is available if needed.
	//
	virtual void makeCurrent(Params* previousPanel, bool newWin) = 0;
	
	//Update the panel with values from associated tab.
	//Must turn off undo/redo while this occurs!
	//
	virtual void updateDialog() = 0;
	//Method to be called when the all the text box state needs to be copied to the
	//Panel, after the user clicks "enter".  Should always set textChangedFlag to false.
	//
	virtual void updatePanelState() = 0;
	//Send params to the active renderer(s) that depend on these
	//panel settings, tell renderers that the values must be updated
	//Default method does nothing.  
	//Arguments are "wasEnabled, wasLocal, newWindow"
	// identify if previous panel was enabled (for renderer panels)
	//was local, and whether this is the first rendering in a window
	//
	virtual void updateRenderer(bool, bool, bool) {return;}
	//Methods to find the "other" params in a local/global switch:
	//
	virtual Params* getCorrespondingGlobalParams(); 
	virtual Params* getCorrespondingLocalParams() ;

	int getVizNum() {return vizNum;}
	void setLocal(bool lg){ if (lg) {local = true; assert (vizNum != -1);}
		else local = false;}
	bool isLocal() {return local;}
	bool isEnabled(){return enabled;}
	virtual void setEnabled(bool value) {enabled = value;}
	virtual void guiSetEnabled(bool value) {enabled = value;}//default does nothing
	void setVizNum(int vnum){vizNum = vnum;}
	//Methods to be called from gui, get undo/redo support.
	//Need to capture two different params objects (not just two times of the same object)
	//Following is virtual so child classes can attach additional behavior.
	//They should always perform params::guiSetLocal() as well as their own behavior
	//
	virtual void guiSetLocal(bool lg);
	//Dirty bit comes on when any text changes in the panel.
	//Comes off as soon as it gets confirmed.
	//
	void guiSetTextChanged(bool on) {textChangedFlag = on;}
	//When enter is pressed over a text box, the event is recorded and the
	//params are updated, if anything has changed.
	//
	void confirmText(bool render);
	//When a new metadata is read, all params are notified 
	//If the params have state that depends on the metadata (e.g. region size,
	//variable, etc., they should implement reinit() to respond.
	//Default does nothing.
	//
	virtual void reinit(bool) {return;}

	//Following are only implemented for renderer params
	//Should really have a renderer params class!
	// Default does nothing (if class doesn't have a clut or tf)
	virtual void guiStartChangeMapFcn(char* ) {}
	virtual void guiEndChangeMapFcn() {}
	virtual void setClutDirty() {}
	void setMinColorMapBound(float val);
	void setMaxColorMapBound(float val);
	float getMinColorMapBound();	
	float getMaxColorMapBound(); 
	
	void setMinColorEditBound(float val, int var) {
		minColorEditBounds[var] = val;
	}
	void setMaxColorEditBound(float val, int var) {
		maxColorEditBounds[var] = val;
	}
	float getMinColorEditBound(int var) {
		return minColorEditBounds[var];
	}
	float getMaxColorEditBound(int var) {
		return maxColorEditBounds[var];
	}
	//And opacity:
	void setMinOpacMapBound(float val);
	void setMaxOpacMapBound(float val);
	float getMinOpacMapBound();	
	float getMaxOpacMapBound(); 
	void resetMinOpacEditBounds(std::vector<double>& newBounds);
	void resetMaxOpacEditBounds(std::vector<double>& newBounds);
	void setMinOpacEditBound(float val, int var) {
		minOpacEditBounds[var] = val;
	}
	void setMaxOpacEditBound(float val, int var) {
		maxOpacEditBounds[var] = val;
	}
	float getMinOpacEditBound(int var) {
		return minOpacEditBounds[var];
	}
	float getMaxOpacEditBound(int var) {
		return maxOpacEditBounds[var];
	}
	virtual MapperFunction* getMapperFunc(){return 0;}
	//The restart method goes back to initial state
	//Default does nothing.
	//
	
	virtual void restart() {return;}
	//Methods for restore/save:
	virtual XmlNode* buildNode() { return 0;}
	virtual bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/) = 0;
	virtual bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/) = 0;
	
protected:
	
	//The Params is global if either the params object does not exist, OR if
	//it exists and local = false.
	//
	bool local;
	// The enabled flag is only used by renderer params
	//
	bool enabled;
	//Keep track of which window number corresp to this.  -1 for global parameters.
	//
	int vizNum;
	
	bool textChangedFlag;
	ParamType thisParamType;

	//Needed for renderer params:
	float* minColorEditBounds;
	float* maxColorEditBounds;
	float* minOpacEditBounds;
	float* maxOpacEditBounds;


};
};
#endif //PARAMS_H 
