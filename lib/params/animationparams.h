//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		animationparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		January 2005
//
//	Description:	Defines the AnimationParams class
//		This is derived from the Params class
//		It contains all the parameters required for animation

//
#ifndef ANIMATIONPARAMS_H
#define ANIMATIONPARAMS_H


#include "params.h"
#include <vapor/common.h>


namespace VAPoR {
class ExpatParseMgr;

class XmlNode;
class ParamNode;
class PARAMS_API AnimationParams : public Params {
	
public: 
	AnimationParams(int winnum);
	~AnimationParams();
	static ParamsBase* CreateDefaultInstance() {return new AnimationParams(-1);}
	const std::string& getShortName() {return _shortName;}
	virtual Params* deepCopy(ParamNode* n = 0);

	virtual void restart();
	static void setDefaultPrefs();
	virtual bool reinit(bool doOverride);

	

	bool isPlaying() {return (playDirection != 0);}


	int getMinTimeToRender() {return ((int)(1000.f/maxFrameRate) );}
	int getCurrentFrameNumber() {return currentFrame;}
	int getStartFrameNumber() {return startFrame;}
	int getEndFrameNumber() {return endFrame;}
	int getMinFrame() {return minFrame;}
	int getMaxFrame() {return maxFrame;}
	int getPlayDirection() {return playDirection;}
	int getFrameStepSize() {return frameStepSize;}
	float getMaxFrameRate() {return maxFrameRate;}
	void setCurrentFrameNumber(int val) {currentFrame = val;}
	void setStartFrameNumber(int val) {startFrame=val;}
	void setEndFrameNumber(int val) {endFrame=val;}
	float getMaxWait(){return maxWait;}
	bool isRepeating() {return repeatPlay;}
	void setRepeating(bool onOff){repeatPlay = onOff;}
	void setFrameStepSize(int sz){ frameStepSize = sz;}
	void setMaxFrameRate(float val) {maxFrameRate = val;}
	void setMaxWait(float wait) {maxWait = wait;}
	void setPlayDirection(int val){stateChanged = true; playDirection = val;}
	bool isStateChanged() {return stateChanged;}
	//Used when the state changes during a render:
	void setStateChanged(bool state) {stateChanged = state;}

	static float getDefaultMaxWait() {return defaultMaxWait;}
	static float getDefaultMaxFPS() {return defaultMaxFPS;}
	static void setDefaultMaxWait(float val) {defaultMaxWait = val;}
	static void setDefaultMaxFPS(float val) {defaultMaxFPS = val;}

	//When rendering is finished, renderer calls this.  Returns true no change (if the change bit
	//needs to be set. 
	//It advances the currentFrame to the next one
	bool advanceFrame();
	//set to pause if last frame is done, return true if done.
	bool checkLastFrame();
	int getNextFrame(int dir); //Determine the next frame in the specified direction
	bool usingTimestepList() {return useTimestepSampleList;}
	void setTimestepSampleList(bool on) {useTimestepSampleList = on;}
	std::vector<int>& getTimestepList() { return timestepList;}
	
	ParamNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	
protected:
	static const string _shortName;
	static const string _repeatAttr;
	static const string _maxRateAttr;
	static const string _stepSizeAttr;
	static const string _startFrameAttr;
	static const string _endFrameAttr;
	static const string _currentFrameAttr;
	static const string _maxWaitAttr;

	int playDirection; //-1, 0, or 1
	bool repeatPlay;
	float maxFrameRate;
	float maxWait;
	int frameStepSize;// always 1 or greater
	int startFrame;
	int endFrame;
	int maxFrame, minFrame;
	int currentFrame;
	bool useTimestepSampleList;
	std::vector<int> timestepList;
	//If the animation state is changed, gui needs to update:
	bool stateChanged;
	static float defaultMaxWait;
	static float defaultMaxFPS;
	
};
};
#endif //ANIMATIONPARAMS_H 
