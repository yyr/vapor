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
	AnimationParams(XmlNode* parent, int winnum);
	//! Destructor
	~AnimationParams();

	//! Identify the current data timestep being used
	//! \retval long current time step
	long getCurrentTimestep() {
		return GetRootNode()->GetElementLong(_currentTimestepTag)[0];
	}
	//! Set the current data timestep being used
	//! \param long current time step
	//! \retval int 0 if successful
	int setCurrentTimestep(long ts) {
		Command* cmd = 0;
		int rc = 0;
		if (ts < DataStatus::getInstance()->getMinTimestep()) {ts = DataStatus::getInstance()->getMinTimestep(); rc = -1;}
		if (ts > DataStatus::getInstance()->getMaxTimestep()) { ts = DataStatus::getInstance()->getMaxTimestep(); rc = -1;}
		if (Command::isRecording())
			cmd = Command::captureStart(this,"Change time step");
		GetRootNode()->SetElementLong(_currentTimestepTag,ts);
		if (cmd) Command::captureEnd(cmd,this);
		return rc;
	}

	//! Identify the starting time step currently set in the UI.
	//! \retval int starting frame number.
	int getStartTimestep() {
		return GetRootNode()->GetElementLong(_startTimestepTag)[0];
	}
	//! set the starting time step
	//! \param int starting timestep
	//! \retval int 0 if successful
	int setStartTimestep(int val) {
		Command* cmd = 0;
		int rc = 0;
		if (val < DataStatus::getInstance()->getMinTimestep()) {val = DataStatus::getInstance()->getMinTimestep(); rc = -1;}
		if (val > DataStatus::getInstance()->getMaxTimestep()) {val = DataStatus::getInstance()->getMaxTimestep(); rc = -1;}
		if (Command::isRecording())
			cmd = Command::captureStart(this,"Change start timestep");
		GetRootNode()->SetElementLong(_startTimestepTag,val);
		if (cmd) Command::captureEnd(cmd,this);
		return rc;
	}

	//! Identify the ending time step used during playback
	//! \retval int ending timestep
	int getEndTimestep() {
		return GetRootNode()->GetElementLong(_endTimestepTag)[0];
	}
	//! set the ending time step 
	//! \param int ending timestep
	//! \retval int 0 if success
	int setEndTimestep(int val) {
		int rc = 0;
		if (val > DataStatus::getInstance()->getMaxTimestep() || val < DataStatus::getInstance()->getMinTimestep()) {
			val = DataStatus::getInstance()->getMaxTimestep(); 
			rc = -1;
		}
		Command* cmd = 0;
		if (Command::isRecording())
			cmd = Command::captureStart(this,"Change end timestep");
		GetRootNode()->SetElementLong(_endTimestepTag,val);
		if (cmd) Command::captureEnd(cmd,this);
		return rc;
	}

	//! Identify the minimum time step (bound on setting start/end) 
	//! \retval int minimum frame number.
	int getMinTimestep() {
		return GetRootNode()->GetElementLong(_minTimestepTag)[0];
	}
	//! Set the minimum time step (bound on setting start/end) 
	//! \param int minimum timestep
	//! \retval int 0 if successful
	int setMinTimestep(int val) {
		int rc = 0;
		Command* cmd = 0;
		if (val < DataStatus::getInstance()->getMinTimestep() || val > DataStatus::getInstance()->getMaxTimestep()){
			val = DataStatus::getInstance()->getMinTimestep();
			rc = -1;
		}
		if (Command::isRecording())
			cmd = Command::captureStart(this,"Set min timestep");
		GetRootNode()->SetElementLong(_minTimestepTag,val);
		if (cmd) Command::captureEnd(cmd,this);
		return rc;
	}
	//! Identify the maximum time step
	//! \retval int maximum timestep
	int getMaxTimestep() {
		return GetRootNode()->GetElementLong(_maxTimestepTag)[0];
	}

	//! Set the maximum time step
	//! \param int maximum timestep
	//! \retval int 0 if successful
	int setMaxTimestep(int val) {
		int rc = 0;
		if (val < DataStatus::getInstance()->getMinTimestep() || val > DataStatus::getInstance()->getMaxTimestep()){
			DataStatus::getInstance()->getMaxTimestep();
			rc = -1;
		}
		Command* cmd = 0;
		if (Command::isRecording())
			cmd = Command::captureStart(this,"Set max timestep");
		GetRootNode()->SetElementLong(_maxTimestepTag,val);
		if (cmd) Command::captureEnd(cmd,this);
		return rc;
	}

	//! Get the current play direction (-1,0,1)
	//! \retval int 0 play direction
	int getPlayDirection() {
		return GetRootNode()->GetElementLong(_playDirectionTag)[0];
	}

	//! Set the play direction
	//! \param int play direction
	//! \retval int 0 if successful
	int setPlayDirection(int val) {
		Command* cmd = 0;
		int rc = 0;
		if (val < -1 ) {val = -1; rc = -1;}
		if (val > 1 ) {val = 1; rc = -1;}
		if (Command::isRecording())
			cmd = Command::captureStart(this,"Set play direction");
		GetRootNode()->SetElementLong(_playDirectionTag,val);
		if (cmd) Command::captureEnd(cmd,this);
		return rc;
	}
	//! Get the maximum frame step size
	//! \retval int current frame step size.
	int getFrameStepSize() {
		return GetRootNode()->GetElementLong(_stepSizeTag)[0];
	}
	//! Set the frame step size
	//! \param int val step size
	//! \retval int 0 if successful
	int setFrameStepSize(int val) {
		int numframes = DataStatus::getInstance()->getMaxTimestep()-DataStatus::getInstance()->getMinTimestep();
		int rc = 0;
		if (val <= 0 || val >= numframes ) {
			val = 1;
			rc = -1;
		}

		Command* cmd = 0;
		if (Command::isRecording())
			cmd = Command::captureStart(this,"Set frame stepsize");
		GetRootNode()->SetElementLong(_stepSizeTag,val);
		if (cmd) Command::captureEnd(cmd,this);
		return rc;
	}
	//! Determine max frames per second
	//! \retval double max frames per second
	double getMaxFrameRate() {
		return GetRootNode()->GetElementDouble(_maxRateTag)[0];
	}
	//! Set max frames per second
	//! \param double fps
	//! \retval int 0 if successful
	int setMaxFrameRate(double rate) {
		int rc = 0;
		if (rate <= 0. || rate > 1000.){ rate = 0.1; rc = -1;}
		Command* cmd = 0;
		if (Command::isRecording())
			cmd = Command::captureStart(this,"Set max frame rate");
		GetRootNode()->SetElementDouble(_maxRateTag,rate);
		if (cmd) Command::captureEnd(cmd,this);
		return rc;
	}
	//! Determine if repeat play is on
	//! \retval bool true if repeating
	bool isRepeating() {
		return (GetRootNode()->GetElementLong(_repeatTag)[0] != 0);
	}
	//! Set the whether repeat play is on
	//! \param bool repeat is on if true
	//! \retval int 0 if successful
	int setRepeating(bool onOff){
		Command* cmd = 0;
		if (Command::isRecording())
			cmd = Command::captureStart(this,"Enable repeat play");
		GetRootNode()->SetElementLong(_repeatTag,(long)onOff);
		if (cmd) Command::captureEnd(cmd,this);
		return 0;
	}
	
#ifndef DOXYGEN_SKIP_THIS

	//The rest is not part of the public API
	static ParamsBase* CreateDefaultInstance() {return new AnimationParams(0,-1);}
	const std::string& getShortName() {return _shortName;}

	virtual void restart();
	static void setDefaultPrefs();
	virtual bool reinit(bool doOverride);

	bool isPlaying() {return (getPlayDirection() != 0);}

	int getMinTimeToRender() {return ((int)(1000.f/getMaxFrameRate()) );}
	


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
