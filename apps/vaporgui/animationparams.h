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

	virtual Params* deepCopy();

	virtual void makeCurrent(Params* p, bool newWin);
	

	void setTab(AnimationTab* tab) {myAnimationTab = tab;}
	
	//Following are set by gui, result in save history state:
	void guiSetPlay(int direction);
	void guiGoToStart();
	void guiGoToEnd();
	void guiReleasePositionSlider(int position);
	void guiReleaseStepsizeSlider(int position);
	
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
