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

#include <qwidget.h>
#include "params.h"

class AnimationTab;

namespace VAPoR {

class MainForm;
class AnimationParams : public Params {
	
public: 
	AnimationParams(MainForm* mainWin, int winnum);
	~AnimationParams();
	//Update the dialog with values from this:
	void updateDialog();
	//And vice-versa:
	virtual void updatePanelState();
	//Set the sliders consistent with values in tab panel
	void setSliders();
	virtual Params* deepCopy();

	virtual void makeCurrent(Params* p, bool newWin);

	virtual void reinit();

	//Need to do more than Params::guiSetLocal():
	virtual void guiSetLocal(bool lg);

	bool isPlaying() {return (playDirection != 0);}


	int getMinTimeToRender() {return ((int)(1000.f/maxFrameRate) );}
	int getCurrentFrameNumber() {return currentFrame;}

	//When values change that affect the frame to be used in the next rendering, 
	//call the following:
	
	void setDirty();   


	//When rendering is finished, renderer calls this:
	void advanceFrame();

	void setTab(AnimationTab* tab) {myAnimationTab = tab;}
	
	//Following are set by gui, result in save history state, 
	//plus notification to animation controller
	void guiSetPlay(int direction);
	void guiJumpToBegin();
	void guiJumpToEnd();
	void guiSetPosition(int sliderposition);
	void guiSetFrameStep(int sliderposition);
	void guiToggleReplay(bool replay);
	void guiSingleStep(bool forward);
	
protected:
	int playDirection; //-1, 0, or 1
	bool repeatPlay;
	float maxFrameRate;
	int frameStepSize;// always 1 or greater
	int startFrame;
	int endFrame;
	int maxFrame, minFrame;
	int currentFrame;
	AnimationTab* myAnimationTab;
	

};
};
#endif //ANIMATIONPARAMS_H 
