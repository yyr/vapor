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
#include "command.h"


namespace VAPoR {
class ExpatParseMgr;

class XmlNode;
class ParamNode;

//! \class AnimationParams
//! \ingroup Public
//! \brief A class that specifies parameters used in animation 
//! \author Alan Norton
//! \version 3.0
//! \date    February 2014
//! When this class is local, it controls the time-steps in one visualizer.
//! The global (shared) AnimationParams controls the animation in any number of visualizers.
class PARAMS_API AnimationParams : public Params {
	
public: 

	//! Identify the current data timestep being used
	//! \retval long current time step
	long getCurrentTimestep() {
		return GetValueLong(_currentTimestepTag);
	}
	//! Set the current data timestep being used
	//! \param long current time step
	//! \retval int 0 if successful
	int setCurrentTimestep(long ts) {
		int rc = SetValueLong(_currentTimestepTag,"Set timestep",ts);
		return rc;
	}

	//! Identify the starting time step currently set in the UI.
	//! \retval int starting frame number.
	int getStartTimestep() {
		return GetValueLong(_startTimestepTag);
	}
	//! set the starting time step
	//! \param int starting timestep
	//! \retval int 0 if successful
	int setStartTimestep(int ts) {
		int rc= SetValueLong(_startTimestepTag,"Set start timestep",ts);
		return rc;
	}

	//! Identify the ending time step used during playback
	//! \retval int ending timestep
	int getEndTimestep() {
		return GetValueLong(_endTimestepTag);
	}
	//! set the ending time step 
	//! \param int ending timestep
	//! \retval int 0 if success
	int setEndTimestep(int val) {
		
		return SetValueLong(_endTimestepTag,"Set end timestep",val);
	}

	//! Identify the minimum time step (bound on setting start/end) 
	//! \retval int minimum frame number.
	int getMinTimestep() {
		return GetValueLong(_minTimestepTag);
	}
	//! Set the minimum time step (bound on setting start/end) 
	//! \param int minimum timestep
	//! \retval int 0 if successful
	int setMinTimestep(int val) {
		
		return SetValueLong(_minTimestepTag,"Set min timestep",val);
	}
	//! Identify the maximum time step
	//! \retval int maximum timestep
	int getMaxTimestep() {
		return GetValueLong(_maxTimestepTag);
	}

	//! Set the maximum time step
	//! \param int maximum timestep
	//! \retval int 0 if successful
	int setMaxTimestep(int val) {
		return SetValueLong(_maxTimestepTag,"Set max timestep",val);
	}

	//! Get the current play direction (-1,0,1)
	//! \retval int 0 play direction
	int getPlayDirection() {
		return GetValueLong(_playDirectionTag);
	}

	//! Set the play direction
	//! \param int play direction
	//! \retval int 0 if successful
	int setPlayDirection(int val) {
		return SetValueLong(_playDirectionTag,"Set play direction",val);
	}
	//! Get the maximum frame step size for skipping
	//! \retval int current frame step size.
	int getFrameStepSize() {
		return GetValueLong(_stepSizeTag);
	}
	//! Set the frame step size (for skipping)
	//! \param int val step size
	//! \retval int 0 if successful
	int setFrameStepSize(int val) {
		return SetValueLong(_stepSizeTag,"Set frame stepsize",val);
	}
	//! Determine max frames per second
	//! \retval double max frames per second
	double getMaxFrameRate() {
		return GetValueDouble(_maxRateTag);
	}
	//! Set max frames per second
	//! \param double fps
	//! \retval int 0 if successful
	int setMaxFrameRate(double rate) {
		return SetValueDouble(_maxRateTag,"Set max frame rate",rate);
	}
	//! Determine if repeat play is on
	//! \retval bool true if repeating
	bool isRepeating() {
		return (GetValueLong(_repeatTag) != 0);
	}
	//! Set the whether repeat play is on
	//! \param bool repeat is on if true
	//! \retval int 0 if successful
	int setRepeating(bool onOff){
		return SetValueLong(_repeatTag,"enable repeat play",(long)onOff);
	}
	
	//! Determine the shortest time (in milliseconds) for a render
	//! \retval int minimum render time.
	int getMinTimeToRender() {return ((int)(1000.f/getMaxFrameRate()) );}
//! @name Internal
//! Internal methods not intended for general use
//!
///@{

	//! Constructor
	//! \param[in] int winnum The number of the visualizer, or -1 for a global AnimationParams
	AnimationParams(XmlNode* parent, int winnum);
	//! Destructor
	~AnimationParams();

///}@

	//The following are not part of the public API
#ifndef DOXYGEN_SKIP_THIS
	//! Make state valid, either setting to defaults, or to values consistent with data
	//! \param in bool setDefault true if values are set to default.
	virtual void Validate(bool setdefault);
	//! Put a params instance into default state with no data.
	virtual void restart();
	//! Pure virtual method on Params. Provide a short name suitable for use in the GUI
	//! \retval string name
	const std::string getShortName() {return _shortName;}
	//! Required method to create params in default state with no data.
	//! \retval ParamsBase* created default instance
	static ParamsBase* CreateDefaultInstance() {return new AnimationParams(0,-1);}

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
	

#endif /* DOXYGEN_SKIP_THIS */
	
};

};

#endif //ANIMATIONPARAMS_H 
