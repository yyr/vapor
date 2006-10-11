//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		animationeventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Defines the AnimationEventRouter class.
//		This class handles events for the Animation params
//
#ifndef ANIMATIONEVENTROUTER_H
#define ANIMATIONEVENTROUTER_H


#include <qobject.h>
#include "params.h"
#include "eventrouter.h"
#include "vapor/MyBase.h"
#include "animationtab.h"


using namespace VetsUtil;


namespace VAPoR {

class ViewpointParams;
class XmlNode;
class PanelCommand;
class AnimationEventRouter : public AnimationTab, public EventRouter {

	Q_OBJECT

public: 
	AnimationEventRouter(QWidget* parent = 0, const char* name = 0);
	~AnimationEventRouter();
	
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	virtual void makeCurrent(Params* prev, Params* next, bool newWin, int instance = -1);
	
	

	
	virtual void reinitTab(bool doOverride);
	
	void setSliders (AnimationParams* a);

	//Following are set by gui, result in save history state, 
	//plus notification to animation controller
	void guiSetPlay(int direction);
	void guiJumpToBegin();
	void guiJumpToEnd();
	void guiSetPosition(int sliderposition);
	void guiSetFrameStep(int sliderposition);
	void guiToggleReplay(bool replay);
	void guiSingleStep(bool forward);
	//Need to do more than Params::guiSetLocal():
	virtual void guiSetLocal(Params* p, bool lg);
	
protected slots:
	void setAtabTextChanged(const QString& qs);
	void animationReturnPressed();
	//Animation slots:
	void animationSetFrameStep();
	void animationSetPosition();
	void animationPauseClick();
	void animationPlayReverseClick();
	void animationPlayForwardClick();
	void animationReplayClick();
	void animationToBeginClick();
	void animationToEndClick();
	void animationStepForwardClick();
	void animationStepReverseClick();
	

};

};
#endif //ANIMATIONEVENTROUTER_H 
