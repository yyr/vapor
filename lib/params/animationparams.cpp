//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		animationparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		January 2005
//
//	Description:	Implements the AnimationParams class
//		This is derived from the Params class
//		It contains all the parameters required for animation

//

#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include "glutil.h"	// Must be included first!!!
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>


#include "datastatus.h" 
#include "params.h"
#include "animationparams.h"
#include "animate.h"
#include <vapor/ParamNode.h>


using namespace VAPoR;
const string AnimationParams::_shortName = "Animation";
const string AnimationParams::_repeatAttr = "RepeatPlay";
const string AnimationParams::_maxRateAttr = "MaxFrameRate";
const string AnimationParams::_stepSizeAttr = "FrameStepSize";
const string AnimationParams::_startFrameAttr = "StartFrame";
const string AnimationParams::_endFrameAttr = "EndFrame";
const string AnimationParams::_startKeyframeFrameAttr = "StartKeyframeFrame";
const string AnimationParams::_endKeyframeFrameAttr = "EndKeyframeFrame";
const string AnimationParams::_currentFrameAttr = "CurrentFrame";
const string AnimationParams::_currentInterpFrameAttr = "CurrentInterpolatedFrame";
const string AnimationParams::_maxWaitAttr = "MaxWait";
const string AnimationParams::_keyframesEnabledAttr = "KeyframesEnabled";
const string AnimationParams::_keyframeTag = "Keyframe";
const string AnimationParams::_keySpeedAttr = "KeySpeed";
const string AnimationParams::_keyTimestepAttr = "KeyTimestep";
const string AnimationParams::_keyNumFramesAttr = "KeyNumFrames";
const string AnimationParams::_keySynchToTimestepsAttr = "SynchToTimesteps";
const string AnimationParams::_keyTimestepsPerFrameAttr = "TimestepsPerFrame";
float AnimationParams::defaultMaxFPS = 10.f;


AnimationParams::AnimationParams(int winnum): Params( winnum, Params::_animationParamsTag){
	myAnimate = 0;
	restart();
}
AnimationParams::~AnimationParams(){
	for (int i = 0; i<keyframes.size(); i++){
		delete keyframes[i];
	}
	keyframes.clear();
	if( myAnimate) delete myAnimate;
	clearLoadedViewpoints();
}


//Currently nothing "deep" to copy:
Params* AnimationParams::deepCopy(ParamNode*){
	AnimationParams* newParams = new AnimationParams(*this);
	newParams->keyframes = keyframes;
	for (int i = 0; i<keyframes.size(); i++){
		newParams->keyframes[i] = new Keyframe(*keyframes[i]);
		newParams->keyframes[i]->viewpoint = new Viewpoint(*(keyframes[i]->viewpoint));
	}
	
	for (int i = 0; i<loadedViewpoints.size(); i++){
		newParams->loadedViewpoints[i] = new Viewpoint(*loadedViewpoints[i]);
	} 
	newParams->myAnimate = 0;
	return (Params*)newParams;
}

//Reset to initial state
//
void AnimationParams::
restart(){
	// set everything to default state:
	playDirection = 0;
	repeatPlay = false;
	maxFrameRate = defaultMaxFPS;
	frameStepSize = 1;
	startFrame = 1;
	endFrame = 100;
	startKeyframeFrame = 0;
	endKeyframeFrame = 0;
	endFrameIsDefault=true;
	maxTimestep = 100; 
	minTimestep = 1;
	currentInterpolatedFrame = 0;
	currentTimestep = 0;
	useTimestepSampleList = false;
	timestepList.clear();
	loadedViewpoints.clear();
	loadedTimesteps.clear();
	keyframes.clear();
	useKeyframing=false;
	//Insert a default keyframe:
	Viewpoint* vp = new Viewpoint();
	Keyframe* kf = new Keyframe(vp,0., 0, 1);
	keyframes.push_back(kf);
	stateChanged = true;
	if (myAnimate) delete myAnimate;
	myAnimate = new animate;
}
void AnimationParams::setDefaultPrefs(){
	defaultMaxFPS = 10.f;
}
//Respond to change in Metadata
//
bool AnimationParams::
reinit(bool doOverride){
	
	
	maxTimestep = DataStatus::getInstance()->getDataMgr()->GetNumTimeSteps()-1;
	minTimestep = 0;
	//Narrow the range to the actual data limits:
	//Find the first framenum with data:
	int mints = minTimestep;
	int maxts = maxTimestep;
	int i;
	for (i = minTimestep; i<= maxTimestep; i++){
		
		if(DataStatus::getInstance()->dataIsPresent(i)) break;

	}
	if(i <= maxts) mints = i;
	//Find the last framenum with data:
	for (i = maxts; i>= mints; i--){
		if(DataStatus::getInstance()->dataIsPresent(i)) break;
	}
	
	if(i >= mints) maxts = i;
	//force start & end to be consistent:
	if (doOverride){
		startFrame = mints;
		endFrame = maxts;
		startKeyframeFrame = 0;
		endKeyframeFrame = 0;
		endFrameIsDefault = true;
		currentTimestep = startFrame;
		currentInterpolatedFrame = 0;
		maxFrameRate = defaultMaxFPS;
		
	} else {
		
		if (startFrame > maxts) startFrame = maxts;
		if (startFrame < mints) startFrame = mints;
		if (endFrame < mints) endFrame = mints;
		if (endFrame > maxts) endFrame = maxts;
		
		if (currentTimestep < mints) currentTimestep = mints;
		if (currentTimestep > maxts) currentTimestep = maxts;
		if (currentInterpolatedFrame < startKeyframeFrame) currentInterpolatedFrame = startKeyframeFrame;
		if (currentInterpolatedFrame > endKeyframeFrame) currentInterpolatedFrame = endKeyframeFrame;
		if (endKeyframeFrame == loadedTimesteps.size()-1) endFrameIsDefault = true; else endFrameIsDefault = false;
	}
	
	// set pause state
	playDirection = 0;
	//Set the first frame for animation control:
	if (doOverride||keyframes.size()==0){
		ViewpointParams* vpParams = (ViewpointParams*)Params::GetParamsInstance(_viewpointParamsTag);
		Viewpoint* startingViewpoint = new Viewpoint(*vpParams->getHomeViewpoint());
		keyframes.clear();
		keyframes.push_back(new Keyframe(startingViewpoint, 0., 0, 1));
	} 
	buildViewsAndTimes();
	return true;
}

//Following method called when a frame has been rendered, to update the currentFrame, in
//preparation for next rendering.
//It should not be called if the UI has changed the frame number; in that case
//setDirty should be called, and AnimationController::paramsChanged() should be
//called, to force any ongoing animation to not increment the frame number
//This returns true if the caller needs to set the change bit, because 
//we have changed to "pause" status
bool AnimationParams::
advanceFrame(){
	assert(playDirection);
	int newFrame = getNextFrame(playDirection);
	if (newFrame == getCurrentFrameNumber() ) {
		if (repeatPlay) return false;
		setPlayDirection(0);
		return true;
	}
	//See if direction needs to change:

	if (((newFrame-getCurrentFrameNumber())*playDirection) > 0) {
		//No change in direction
		setCurrentFrameNumber(newFrame);
		return false;
	} else {
		setCurrentFrameNumber(newFrame);
		return true;
	}
}
bool AnimationParams::checkLastFrame(){
	int newFrame = getNextFrame(playDirection);
	if (newFrame == getCurrentFrameNumber() ) {
		setPlayDirection(0);
		return true;
	}
	else return false;
}
//Determine the next frame.  dir must be 1 or -1
//Returns currentTimestep if no change.
//If value returned is in opposite direction from dir, then there needs to
//be a direction change
int AnimationParams::
getNextFrame(int dir){
	//If we are using timestep list, find the current frame in that list:
	if(usingTimestepList() && timestepList.size() > 0){
		int prevIndex = -1; 
		int nextIndex = -1;
		int firstValidFrame = -1;
		int lastValidFrame = -1;
		
		for (int i = 0; i< timestepList.size(); i++){
			if (timestepList[i] >= startFrame && timestepList[i] <= endFrame){
				if (firstValidFrame < 0) firstValidFrame = timestepList[i];
				lastValidFrame = timestepList[i];
			} else continue;  //Ignore any timesteps outside the valid range.
			if (timestepList[i] < getCurrentFrameNumber()) prevIndex = i; //before
			else if (timestepList[i] == getCurrentFrameNumber()) continue;//at
			else if (nextIndex < 0) nextIndex = i;//beyond
		}
		if (lastValidFrame < 0) {
			//no valid frames
			return getCurrentFrameNumber();
		}
		//is currentFrame at or past the end/beginning?
		if (dir > 0 && (nextIndex < 0) ){ 
			if (repeatPlay)  return firstValidFrame;
			else return getCurrentFrameNumber();
		} 
		if (dir < 0 && prevIndex < 0)	{
			if (repeatPlay) return lastValidFrame;
			else return getCurrentFrameNumber();
		}
		//If we got this far, must be inside the valid range.
		if (dir > 0) return(timestepList[nextIndex]); 
		else return(timestepList[prevIndex]);
				
	} else {
		int i;
		//not using timestep sample list.
		// find next valid frame for which there exists data:
		int testFrame = getCurrentFrameNumber() + dir*frameStepSize;
		DataStatus* ds = DataStatus::getInstance();
		int firstFrame = startFrame;
		int lastFrame = endFrame;
		if (keyframingEnabled()){
			firstFrame = startKeyframeFrame;
			lastFrame = endKeyframeFrame;
		}
		for (i = 1; i<= (lastFrame - firstFrame + frameStepSize)/frameStepSize; i++){
			
			if (testFrame > lastFrame){ 
				if (repeatPlay) testFrame =  firstFrame;
				else testFrame = getCurrentFrameNumber();
			}
			if (testFrame < firstFrame){
				if (repeatPlay) testFrame = lastFrame;
				else testFrame = getCurrentFrameNumber();
			}

			if (keyframingEnabled()) break;

			if (ds->dataIsPresent(testFrame)) break;
			testFrame += dir*frameStepSize;
		}
		//It's OK, or we looped all the way around:
		if(i > ((lastFrame - firstFrame + frameStepSize)/frameStepSize))
			testFrame = getCurrentFrameNumber();
		
		return testFrame;
		
		
	}
}
void AnimationParams::deleteKeyframe(int index){
	delete keyframes[index];
	for (int i = index; i<keyframes.size()-1; i++){
		keyframes[i] = keyframes[i+1];
	}
	keyframes.pop_back();
}
void AnimationParams::insertKeyframe(int index, Keyframe* keyframe){
	if (index >= keyframes.size()-1){
		keyframes.push_back(keyframe);
		return;
	}
	vector<Keyframe*>::iterator it;
	it = keyframes.begin()+index+1;
	keyframes.insert(it,keyframe);
}
void AnimationParams::insertViewpoint(int index, Viewpoint* vp){
	if (index >= loadedViewpoints.size()-1){
		loadedViewpoints.push_back(vp);
		return;
	}
	vector<Viewpoint*>::iterator it;
	it = loadedViewpoints.begin()+index+1;
	loadedViewpoints.insert(it,vp);
}

bool AnimationParams::
elementStartHandler(ExpatParseMgr* pm, int depth, std::string& tag, const char ** attrs){
	if (StrCmpNoCase(tag, _animationParamsTag) == 0) {
		//If it's a Animation params tag, save 8 attributes (2 are from Params class)
		//Do this by repeatedly pulling off the attribute name and value
		useTimestepSampleList = false;
		timestepList.clear();
		keyframes.clear();
		currentInterpolatedFrame = 0;
		useKeyframing = false;
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _vizNumAttr) == 0) {
				ist >> vizNum;
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				if (value == "true") setLocal(true); else setLocal(false);
			}
			else if (StrCmpNoCase(attribName, _repeatAttr) == 0) {
				if (value == "true") repeatPlay = true; else repeatPlay = false;
			}
			else if (StrCmpNoCase(attribName, _stepSizeAttr) == 0) {
				ist >> frameStepSize;
			}
			else if (StrCmpNoCase(attribName, _maxRateAttr) == 0) {
				ist >> maxFrameRate;
				if (maxFrameRate <= 0.f) maxFrameRate = 0.001f;
			}
			else if (StrCmpNoCase(attribName, _maxWaitAttr) == 0) {
				// obsolete
			}
			else if (StrCmpNoCase(attribName, _startFrameAttr) == 0) {
				ist >> startFrame;
				if (startFrame < minTimestep) minTimestep = startFrame;
			}
			else if (StrCmpNoCase(attribName, _endFrameAttr) == 0) {
				ist >> endFrame;
				if (endFrame > maxTimestep) maxTimestep = endFrame;
			}
			else if (StrCmpNoCase(attribName, _startKeyframeFrameAttr) == 0) {
				ist >> startKeyframeFrame;
				if (startKeyframeFrame < minTimestep) minTimestep = startKeyframeFrame;
			}
			else if (StrCmpNoCase(attribName, _endKeyframeFrameAttr) == 0) {
				ist >> endKeyframeFrame;
				if (endKeyframeFrame > maxTimestep) maxTimestep = endKeyframeFrame;
			}
			else if (StrCmpNoCase(attribName, _currentFrameAttr) == 0) {
				ist >> currentTimestep;
			}
			else if (StrCmpNoCase(attribName, _currentInterpFrameAttr) == 0) {
				ist >> currentInterpolatedFrame;
			}
			else if (StrCmpNoCase(attribName, _useTimestepSampleListAttr) == 0){
				if (value == "true") useTimestepSampleList = true; else useTimestepSampleList = false;
			}
			else if (StrCmpNoCase(attribName, _timestepSampleListAttr) == 0){
				//The first value is the list size, followed by the entries in the list:
				int listLength, tstep;
				ist >> listLength;
				for (int i = 0; i<listLength; i++){
					ist >> tstep;
					timestepList.push_back(tstep);
				}
			}
			else if (StrCmpNoCase(attribName, _keyframesEnabledAttr) == 0) {
				if (value == "true") useKeyframing = true; else useKeyframing = false;
			}
			
		}
		return true;
	} else if (StrCmpNoCase(tag, _keyframeTag) == 0){
		_parseDepth++;
		//Peel off the attributes
		float speed = 0.1f;
		int frameNum = 1;
		int timestep = 0;
		bool synched = false;
		int timestepRate = 1;
		
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _keySpeedAttr) == 0) {
				ist >> speed;
			} else if (StrCmpNoCase(attribName, _keyTimestepAttr) == 0) {
				ist >> timestep;
			} else if (StrCmpNoCase(attribName, _keyNumFramesAttr) == 0) {
				ist >> frameNum;
			} 
			else if (StrCmpNoCase(attribName, _keySynchToTimestepsAttr) == 0) {
				if (value == "true") synched = true; else synched = false;
			} 
			else if (StrCmpNoCase(attribName, _keyTimestepsPerFrameAttr) == 0) {
				ist >> timestepRate;
			} 
		}
		
		//parse its viewpoint node
		Viewpoint* keyViewpoint = new Viewpoint();

		Keyframe* parsingKeyframe = new Keyframe(keyViewpoint,speed,timestep,frameNum);
		parsingKeyframe->synch = synched;
		parsingKeyframe->timestepsPerFrame = timestepRate;
		keyframes.push_back(parsingKeyframe);
		pm->pushClassStack(keyViewpoint);
		return true;
	}
	pm->skipElement(tag, depth);
	return true;
}
bool AnimationParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	//Check for the animation params tag
	if (StrCmpNoCase(tag, _animationParamsTag) == 0) {
		
		//pop the parse stack.  
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} else if (StrCmpNoCase(tag, _keyframeTag) == 0){
		return true;
	} else {
		pm->parseError("Unrecognized end tag in AnimationParams %s",tag.c_str());
		return false;  //Could there be other end tags that we ignore??
	}
	return true;  
}
ParamNode* AnimationParams::
buildNode(){
		//Construct the animation node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	
	ostringstream oss;

	oss.str(empty);
	oss << (long)vizNum;
	attrs[_vizNumAttr] = oss.str();

	oss.str(empty);
	if (local)
		oss << "true";
	else 
		oss << "false";
	attrs[_localAttr] = oss.str();

	oss.str(empty);
	oss << (double)maxFrameRate;
	attrs[_maxRateAttr] = oss.str();

	oss.str(empty);
	oss << (long)frameStepSize;
	attrs[_stepSizeAttr] = oss.str();

	oss.str(empty);
	oss << (long)startFrame;
	attrs[_startFrameAttr] = oss.str();

	oss.str(empty);
	oss << (long)endFrame;
	attrs[_endFrameAttr] = oss.str();

	oss.str(empty);
	oss << (long)startKeyframeFrame;
	attrs[_startKeyframeFrameAttr] = oss.str();

	oss.str(empty);
	oss << (long)endKeyframeFrame;
	attrs[_endKeyframeFrameAttr] = oss.str();

	oss.str(empty);
	oss << (long)currentTimestep;
	attrs[_currentFrameAttr] = oss.str();

	oss.str(empty);
	oss << (long)currentInterpolatedFrame;
	attrs[_currentInterpFrameAttr] = oss.str();

	oss.str(empty);
	if (repeatPlay)
		oss << "true";
	else 
		oss << "false";
	attrs[_repeatAttr] = oss.str();

	oss.str(empty);
	if (useTimestepSampleList){
		oss << "true";
	} else {
		oss << "false";
	}
	attrs[_useTimestepSampleListAttr] =  oss.str();

	if (timestepList.size()> 0){
		oss.str(empty);
		//First entry is the size of the list, followed by the values in the list
		oss << (long) timestepList.size();
		for (int i = 0; i<timestepList.size(); i++){
			oss<<" "<<(long)timestepList[i];
		}
		attrs[_timestepSampleListAttr] = oss.str();
	}
	oss.str(empty);
	if (useKeyframing){
		oss << "true";
	} else {
		oss << "false";
	}
	attrs[_keyframesEnabledAttr] = oss.str();

	ParamNode* animationNode = new ParamNode(_animationParamsTag, attrs, keyframes.size());

	for (int i = 0; i<keyframes.size(); i++){
		attrs.clear();
		oss.str(empty);
		oss << (long)keyframes[i]->timeStep;
		attrs[_keyTimestepAttr] = oss.str();
		oss.str(empty);
		oss << (long)keyframes[i]->numFrames;
		attrs[_keyNumFramesAttr] = oss.str();
		oss.str(empty);
		oss << (double)keyframes[i]->speed;
		attrs[_keySpeedAttr] = oss.str();
		oss.str(empty);
		oss << (long)keyframes[i]->timestepsPerFrame;
		attrs[_keyTimestepsPerFrameAttr] = oss.str();
		oss.str(empty);
		if (keyframes[i]->synch)
			oss << "true";
		else 
			oss << "false";
		attrs[_keySynchToTimestepsAttr] = oss.str();
		
		ParamNode* viewpointNode = keyframes[i]->viewpoint->buildNode();
		ParamNode* keyframeNode = new ParamNode(_keyframeTag, attrs,1);
		keyframeNode->AddChild(viewpointNode);
		animationNode->AddChild(keyframeNode);
	}

	return animationNode;
}
void AnimationParams::buildViewsAndTimes(){
	
	clearLoadedViewpoints();
	//modify distance based on scene stretch and max size.
	//Warp distance function by (1) dividing it by the largest stretched extent, and
	//(2) multiplying it by the scene stretch factors.
	const float* strExts = DataStatus::getInstance()->getStretchedExtents();
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	
	float stretchedSize;
	float maxStretchedSize = -1.e30;
	for (int i = 0; i<3; i++) {
		stretchedSize = strExts[i+3] - strExts[i];
		if (stretchedSize > maxStretchedSize){
			maxStretchedSize = stretchedSize;
		}
	}
	
	//Modify keyframes by stretching according to scene stretch factors.
	//Modify speeds by factor to make speed = 1 for crossing entire scene in one frame
	vector<Keyframe*> animKeyframes;
	
	for (int i = 0; i<keyframes.size(); i++){
		Viewpoint* vp = new Viewpoint();
		Keyframe* kFrame = new Keyframe(vp,keyframes[i]->speed * maxStretchedSize,keyframes[i]->timeStep,keyframes[i]->numFrames);
		kFrame->stationaryFlag = keyframes[i]->stationaryFlag;
		kFrame->synch = keyframes[i]->synch;
		kFrame->timestepsPerFrame = keyframes[i]->timestepsPerFrame;
		
		Viewpoint* vkey = keyframes[i]->viewpoint;
		for (int j = 0; j<3; j++){
			//The displacement between the camera and rotation center is stretched
			//The rotation center is not moved.
			float camdisp = stretch[j]*(vkey->getCameraPosLocal(j)-vkey->getRotationCenterLocal(j));
			vp->setCameraPosLocal(j,vkey->getRotationCenterLocal(j)+camdisp);
			vp->setRotationCenterLocal(j,vkey->getRotationCenterLocal(j));
			//force vdir to point from camera to rot center
			vp->setViewDir(j,vp->getRotationCenterLocal(j)-vp->getCameraPosLocal(j));
			vp->setUpVec(j,keyframes[i]->viewpoint->getUpVec(j));
		}
		vnormal(vp->getUpVec());
		vnormal(vp->getViewDir());
		kFrame->viewpoint = vp;
		animKeyframes.push_back(kFrame);
	}

	//Set stationary flags
	DataStatus* ds = DataStatus::getInstance();
	float sceneSize = vlength(ds->getFullSizes());
	for (int i = animKeyframes.size()-2; i>= 0; i--){
		float* campos1 = keyframes[i]->viewpoint->getCameraPosLocal();
		float* campos2 = keyframes[i+1]->viewpoint->getCameraPosLocal();
		float dist = vdist(campos1,campos2);
		if (dist < 1.e-6 * sceneSize){
			animKeyframes[i+1]->stationaryFlag = true;
		} else {
			animKeyframes[i+1]->stationaryFlag = false;
		}
	}
	

	if (!myAnimate) myAnimate = new animate();

	myAnimate->keyframeInterpolate(animKeyframes, loadedViewpoints);
	//copy back the frame counts, speeds (if synchronized)
	for (int i = 0; i<animKeyframes.size(); i++){
		keyframes[i]->numFrames = animKeyframes[i]->numFrames;
		if (keyframes[i]->synch) keyframes[i]->speed = animKeyframes[i]->speed/maxStretchedSize;
		keyframes[i]->stationaryFlag = animKeyframes[i]->stationaryFlag;
	}
	
	//Undo stretch:
	for (int i = 0; i<loadedViewpoints.size(); i++){
		Viewpoint* vp = loadedViewpoints[i];
		for (int j = 0; j<3; j++){
			float unstretch = 1./stretch[j];
			float camdisp = unstretch*(vp->getCameraPosLocal(j)-vp->getRotationCenterLocal(j));
			vp->setCameraPosLocal(j,vp->getRotationCenterLocal(j)+camdisp);
		}
		vnormal(vp->getUpVec());
		vnormal(vp->getViewDir());
	}

	//At this point, loadedViewpoints has all the correct viewpoints
	//Adjust time steps.  
	//Whenever there is a keyframe of nonzero numFrames,
	//interpolate the timesteps between current and previous keyframe
	
	for (int i = 0; i<animKeyframes.size(); i++){
		loadedTimesteps.push_back(animKeyframes[i]->timeStep);
		
		//interpolate timesteps between keyframe i and i+1:
		if (i == animKeyframes.size()-1) break;
		int firstTstep = animKeyframes[i]->timeStep;
		int nextTstep = animKeyframes[i+1]->timeStep;
		int numFrames = animKeyframes[i+1]->numFrames;
		if(animKeyframes[i+1]->synch) numFrames = abs(animKeyframes[i+1]->timeStep - animKeyframes[i]->timeStep)/animKeyframes[i+1]->timestepsPerFrame;
		for (int j = 1; j<numFrames; j++){
			float frameFraction = (float)j/(float)(numFrames);
			int tStep = (int)(0.4999+ (float)firstTstep + frameFraction*(nextTstep-firstTstep));
			loadedTimesteps.push_back(tStep);
		}
	}
	
	assert(loadedTimesteps.size() == loadedViewpoints.size());
	
	if ((endKeyframeFrame > (loadedViewpoints.size()-1)) || endFrameIsDefault) {
			endKeyframeFrame = loadedViewpoints.size()-1;
			endFrameIsDefault = true;
	}
	if (startKeyframeFrame > endKeyframeFrame) {
		startKeyframeFrame = 0;
	}
	
	if (getCurrentInterpolatedFrame() >= getNumLoadedViewpoints()){
		setCurrentInterpolatedFrame(getNumLoadedViewpoints()-1);
	}
	//Now we can clear it out:
	
	for (int i = 0; i<animKeyframes.size(); i++){
		delete animKeyframes[i]->viewpoint;
		delete animKeyframes[i];
	}
	animKeyframes.clear();
}
void AnimationParams::enableKeyframing( bool onoff){
	useKeyframing = onoff;
	if (useKeyframing){
		if (getCurrentFrameNumber() < startKeyframeFrame) setCurrentFrameNumber(startKeyframeFrame);
		if (getCurrentFrameNumber() > endKeyframeFrame) setCurrentFrameNumber(endKeyframeFrame);
	} else { // using timesteps
		DataStatus* ds = DataStatus::getInstance();
		setStartFrameNumber(ds->getMinTimestep());
		setEndFrameNumber(ds->getMaxTimestep());
		if (getCurrentFrameNumber() < startFrame) setCurrentFrameNumber(startFrame);
		if (getCurrentFrameNumber() > endFrame) setCurrentFrameNumber(endFrame);
		
	}
	
}
void AnimationParams::clearLoadedViewpoints() {
	
	for (int i = 0; i<loadedViewpoints.size(); i++){
		delete loadedViewpoints[i];
	}
	loadedViewpoints.clear();
	loadedTimesteps.clear();
	
}
int AnimationParams::getFrameIndex(int keyframeIndex){
	int totframes = 0;
	if (keyframeIndex >= keyframes.size()) keyframeIndex = keyframes.size()-1;
	for (int i = 0; i<=keyframeIndex; i++){
		totframes += (keyframes[i]->numFrames);
	}
	return totframes;
}
void AnimationParams::
rescaleKeyframes(const float ratio[3]){
	for (int j = 0; j< getNumKeyframes(); j++){
		Keyframe* kf = getKeyframe(j);
		Viewpoint* vp = kf->viewpoint;
		float* vps = vp->getCameraPosLocal();
		float* vctr = vp->getRotationCenterLocal();
		float vtemp[3],vtemp2[3];
		vsub(vps, vctr, vtemp);
		//move the camera in or out, based on scaling in directions orthogonal to view dir.
		for (int i = 0; i<3; i++){
			vtemp[i] /= ratio[i];
		}
		vadd(vtemp, vctr, vtemp2);
		vp->setCameraPosLocal(vtemp2);
	}
}
