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

class XmlNode;
class ParamNode;

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
		
		return currentTimestep;
	}

	//! Identify the current data timestep being used
	//! \retval size_t current time step
	int getCurrentTimestep() {
		
		return currentTimestep;
	}

	//! Identify the starting frame number currently set in the UI.
	//! \retval int starting frame number.
	int getStartFrameNumber() {
		return startFrame;
	}

	//! Identify the ending frame number as currently set in the UI.
	//! \retval int ending frame number.
	int getEndFrameNumber() {
		
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
		
		return minTimestep;
	}

	//! Identify the maximum frame number 
	//! \retval int maximum frame number.
	int getMaxFrame() {
		
		return maxTimestep;
	}


	//! Set the minimum frame number 
	//! \param[in] int minimum frame number.
	void setMinTimestep(int minF) {minTimestep = minF;}

	//! Identify the maximum frame number 
	//! \param[in] int maximum frame number.
	void setMaxTimestep(int maxF) {maxTimestep = maxF;}

		

#ifndef DOXYGEN_SKIP_THIS

	
	
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
		
		currentTimestep = val;
	}
	void setStartFrameNumber(int val) {
		
		startFrame=val;
		
	}
	void setEndFrameNumber(int val) {
		
			endFrame=val;
		
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
	
	
protected:
	
	static const string _shortName;
	static const string _repeatTag;
	static const string _maxRateTag;
	static const string _stepSizeTag;
	static const string _startFrameTag;
	static const string _endFrameTag;
	
	static const string _currentFrameTag;
	

	int playDirection; //-1, 0, or 1
	bool repeatPlay;
	float maxFrameRate;
	
	int frameStepSize;// always 1 or greater
	int startFrame;
	int endFrame;
	
	int maxTimestep, minTimestep;
	int currentInterpolatedFrame;
	int currentTimestep;
	
	//If the animation state is changed, gui needs to update:
	bool stateChanged;
	
	static float defaultMaxFPS;

	

#endif /* DOXYGEN_SKIP_THIS */
	
};

};

#endif //ANIMATIONPARAMS_H 
