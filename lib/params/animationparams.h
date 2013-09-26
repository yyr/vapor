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

	//! Identify the current data timestep being used
	//! \retval long current time step
	long getCurrentTimestep() {
		return GetRootNode()->GetElementLong(_currentTimestepTag)[0];
	}
	//! Set the current data timestep being used
	//! \param long current time step
	void setCurrentTimestep(long ts) {
		GetRootNode()->SetElementLong(_currentTimestepTag,ts);
	}

	//! Identify the starting time step currently set in the UI.
	//! \retval int starting frame number.
	int getStartTimestep() {
		return GetRootNode()->GetElementLong(_startTimestepTag)[0];
	}
	//! set the starting time step
	//! \param int starting timestep
	void setStartTimestep(int val) {
		GetRootNode()->SetElementLong(_startTimestepTag,val);
	}

	//! Identify the ending time step used during playback
	//! \retval int ending timestep
	int getEndTimestep() {
		return GetRootNode()->GetElementLong(_endTimestepTag)[0];
	}
	//! set the ending time step 
	//! \param int ending timestep
	void setEndTimestep(int val) {
		GetRootNode()->SetElementLong(_endTimestepTag,val);
	}

	//! Identify the minimum time step (bound on setting start/end) 
	//! \retval int minimum frame number.
	int getMinTimestep() {
		return GetRootNode()->GetElementLong(_minTimestepTag)[0];
	}
	//! Set the minimum time step (bound on setting start/end) 
	//! \param int minimum timestep
	void setMinTimestep(int val) {
		GetRootNode()->SetElementLong(_minTimestepTag,val);
	}
	//! Identify the maximum time step
	//! \retval int maximum timestep
	int getMaxTimestep() {
		return GetRootNode()->GetElementLong(_maxTimestepTag)[0];
	}

	//! Set the maximum time step
	//! \param int maximum timestep
	void setMaxTimestep(int val) {
		GetRootNode()->SetElementLong(_maxTimestepTag,val);
	}

#ifndef DOXYGEN_SKIP_THIS

	//The rest is not part of the public API
	static ParamsBase* CreateDefaultInstance() {return new AnimationParams(-1);}
	const std::string& getShortName() {return _shortName;}
	virtual Params* deepCopy(ParamNode* n = 0);

	virtual void restart();
	static void setDefaultPrefs();
	virtual bool reinit(bool doOverride);

	bool isPlaying() {return (getPlayDirection() != 0);}

	int getMinTimeToRender() {return ((int)(1000.f/getMaxFrameRate()) );}
	
	int getPlayDirection() {
		return GetRootNode()->GetElementLong(_playDirectionTag)[0];
	}
	void setPlayDirection(int val) {
		GetRootNode()->SetElementLong(_playDirectionTag,val);
	}
	int getFrameStepSize() {
		return GetRootNode()->GetElementLong(_stepSizeTag)[0];
	}
	void setFrameStepSize(int val) {
		GetRootNode()->SetElementLong(_stepSizeTag,val);
	}
	double getMaxFrameRate() {
		return GetRootNode()->GetElementDouble(_maxRateTag)[0];
	}
	void setMaxFrameRate(double rate) {
		GetRootNode()->SetElementDouble(_maxRateTag,rate);
	}

	bool isRepeating() {
		return (GetRootNode()->GetElementLong(_repeatTag)[0] != 0);
	}
	void setRepeating(bool onOff){
		GetRootNode()->SetElementLong(_repeatTag,(long)onOff);
	}
	

	static double getDefaultMaxFPS() {return defaultMaxFPS;}
	
	static void setDefaultMaxFPS(double val) {defaultMaxFPS = val;}

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
	static const string _startTimestepTag;
	static const string _endTimestepTag;
	static const string _minTimestepTag;
	static const string _maxTimestepTag;
	static const string _playDirectionTag;
	static const string _currentTimestepTag;
	
	static double defaultMaxFPS;

	

#endif /* DOXYGEN_SKIP_THIS */
	
};

};

#endif //ANIMATIONPARAMS_H 
