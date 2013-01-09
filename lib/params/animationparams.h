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
#include "viewpointparams.h"


namespace VAPoR {
class ExpatParseMgr;
class Keyframe;
class XmlNode;
class ParamNode;
class animate;
//! \class AnimationParams
//! \brief A class that specifies parameters used in animation 
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//! When this class is local, it controls the time-steps in one visualizer.
//! The global (shared) AnimationParams controls the animation in any number of visualizers.
class PARAMS_API AnimationParams : public Params {
	
public: 
	//! Constructor
	//! \param[in] int winnum The number of the visualizer, or -1 for a global AnimationParams
	AnimationParams(int winnum);
	//! Destructor
	~AnimationParams();

	//! Identify the current frame being rendered
	//! \retval int current frame number
	int getCurrentFrameNumber() {
		if (keyframingEnabled()&&getLoadedViewpoints().size() > 0){
			return currentInterpolatedFrame;
		}
		return currentTimestep;
	}

	//! Identify the current data timestep being used
	//! \retval size_t current time step
	int getCurrentTimestep() {
		if (keyframingEnabled()&&getLoadedViewpoints().size() > 0){
			return loadedTimesteps[currentInterpolatedFrame];
		}
		return currentTimestep;
	}

	//! Identify the starting frame number currently set in the UI.
	//! \retval int starting frame number.
	int getStartFrameNumber() {
		if (keyframingEnabled()) return startKeyframeFrame;
		return startFrame;
	}

	//! Identify the ending frame number as currently set in the UI.
	//! \retval int ending frame number.
	int getEndFrameNumber() {
		if (keyframingEnabled()) return endKeyframeFrame;
		return endFrame;
	}

	//! Identify the minimum frame number 
	//! \retval int minimum frame number.
	int getMinTimestep() {return minTimestep;}

	//! Identify the maximum frame number 
	//! \retval int maximum frame number.
	int getMaxTimestep() {return maxTimestep;}

	//! Identify the minimum frame number 
	//! \retval int minimum frame number.
	int getMinFrame() {
		if (keyframingEnabled()) return 0;
		return minTimestep;
	}

	//! Identify the maximum frame number 
	//! \retval int maximum frame number.
	int getMaxFrame() {
		if (keyframingEnabled()) return (loadedViewpoints.size()-1);
		return maxTimestep;
	}


	//! Set the minimum frame number 
	//! \param[in] int minimum frame number.
	void setMinTimestep(int minF) {minTimestep = minF;}

	//! Identify the maximum frame number 
	//! \param[in] int maximum frame number.
	void setMaxTimestep(int maxF) {maxTimestep = maxF;}

	Keyframe* getKeyframe(int index) {return keyframes[index];}
	vector<Keyframe*>& getKeyframes() {return keyframes;}
	void deleteKeyframe(int index);
		
	void insertKeyframe(int index, Keyframe* keyframe);
	void insertViewpoint(int index, Viewpoint* vp);
		
	int getNumKeyframes() {return keyframes.size();}
	bool keyframingEnabled() {return useKeyframing;}
	void enableKeyframing( bool onoff);
		


#ifndef DOXYGEN_SKIP_THIS

	//Build vectors of viewpoints and timesteps by interpolating keyframes
	void buildViewsAndTimes();
	const vector<Viewpoint*> getLoadedViewpoints() {return loadedViewpoints;}
	void clearLoadedViewpoints(); 
	int getNumLoadedViewpoints(){return loadedViewpoints.size();}
	void addViewpoint(Viewpoint* vp){loadedViewpoints.push_back(vp);}
	void addTimestep(size_t ts){loadedTimesteps.push_back(ts);}
	const Viewpoint* getLoadedViewpoint(int framenum){
		return (loadedViewpoints[framenum%loadedViewpoints.size()]);
	}
	vector<size_t> getLoadedTimesteps() {return loadedTimesteps;}
	//The rest is not part of the public API
	static ParamsBase* CreateDefaultInstance() {return new AnimationParams(-1);}
	const std::string& getShortName() {return _shortName;}
	virtual Params* deepCopy(ParamNode* n = 0);

	virtual void restart();
	static void setDefaultPrefs();
	virtual bool reinit(bool doOverride);

	bool isPlaying() {return (playDirection != 0);}


	int getMinTimeToRender() {return ((int)(1000.f/maxFrameRate) );}
	
	
	int getPlayDirection() {return playDirection;}
	int getFrameStepSize() {return frameStepSize;}
	float getMaxFrameRate() {return maxFrameRate;}
	void setCurrentInterpolatedFrame(int val){
		currentInterpolatedFrame=val;
	}
	int getCurrentInterpolatedFrame(){
		return currentInterpolatedFrame;
	}
	int getFrameIndex(int keyframeIndex);
	void setCurrentFrameNumber(int val) {
		if (keyframingEnabled())
			currentInterpolatedFrame = val;
		else
			currentTimestep = val;
	}
	void setStartFrameNumber(int val) {
		if (keyframingEnabled()){
			startKeyframeFrame=val;
		} else {
			startFrame=val;
		}
	}
	void setEndFrameNumber(int val) {
		if (keyframingEnabled()){
			endKeyframeFrame=val;
			if (val == getLoadedViewpoints().size()-1) endFrameIsDefault = true;
			else endFrameIsDefault=false;
		} else {
			endFrame=val;
		}
	}
	

	bool isRepeating() {return repeatPlay;}
	void setRepeating(bool onOff){repeatPlay = onOff;}
	void setFrameStepSize(int sz){ frameStepSize = sz;}
	void setMaxFrameRate(float val) {maxFrameRate = val;}
	void setPlayDirection(int val){stateChanged = true; playDirection = val;}
	bool isStateChanged() {return stateChanged;}
	//Used when the state changes during a render:
	void setStateChanged(bool state) {stateChanged = state;}

	static float getDefaultMaxFPS() {return defaultMaxFPS;}
	
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
	vector<Viewpoint*>loadedViewpoints;
	vector<size_t>loadedTimesteps;

	static const string _shortName;
	static const string _repeatAttr;
	static const string _maxRateAttr;
	static const string _stepSizeAttr;
	static const string _startFrameAttr;
	static const string _endFrameAttr;
	static const string _startKeyframeFrameAttr;
	static const string _endKeyframeFrameAttr;
	static const string _currentFrameAttr;
	static const string _currentInterpFrameAttr;
	static const string _maxWaitAttr;
	static const string _keyframesEnabledAttr;
	static const string _keyframeTag;
	static const string _keySpeedAttr;
	static const string _keyTimestepAttr;
	static const string _keyNumFramesAttr;
	static const string _keySynchToTimestepsAttr;
	static const string _keyTimestepsPerFrameAttr;


	int playDirection; //-1, 0, or 1
	bool repeatPlay;
	float maxFrameRate;
	
	int frameStepSize;// always 1 or greater
	int startFrame;
	int endFrame;
	int startKeyframeFrame;
	int endKeyframeFrame;
	bool endFrameIsDefault; //indicate if endKeyframeFrame is at default values.
	int maxTimestep, minTimestep;
	int currentInterpolatedFrame;
	int currentTimestep;
	bool useTimestepSampleList;
	std::vector<int> timestepList;
	//If the animation state is changed, gui needs to update:
	bool stateChanged;
	
	static float defaultMaxFPS;

	//Keyframing state:
	bool useKeyframing;
	std::vector<Keyframe*> keyframes;
	animate* myAnimate;  //used to perform keyframe interpolation

#endif /* DOXYGEN_SKIP_THIS */
	
};
class Keyframe{
public:
	Keyframe(Viewpoint* vp, float spd, int ts, int fnum){
		viewpoint = vp; speed = spd; timeStep = ts; numFrames = fnum; stationaryFlag = false;
		synch = false; timestepsPerFrame = 1;
	}
	Viewpoint* viewpoint;
	float speed;
	int timeStep;
	int numFrames;
	bool stationaryFlag; 
	bool synch;
	int timestepsPerFrame;
};
};

#endif //ANIMATIONPARAMS_H 
