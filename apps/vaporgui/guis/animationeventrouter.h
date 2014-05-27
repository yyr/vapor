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
#include <vapor/MyBase.h>
#include "animationtab.h"

class QTableWidget;


using namespace VetsUtil;
QT_USE_NAMESPACE


namespace VAPoR {

class ViewpointParams;
class XmlNode;
class ParamNode;

class AnimationEventRouter : public QWidget, public Ui_AnimationTab, public EventRouter {

	Q_OBJECT

public: 
	AnimationEventRouter(QWidget* parent = 0);
	~AnimationEventRouter();
	//Required method to create the tab:
	static EventRouter* CreateTab(){
		TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
		return (EventRouter*)(new AnimationEventRouter((QWidget*)tMgr));
	}
	//Connect signals and slots from tab
	virtual void hookUpTab();
	virtual void confirmText(bool /*render*/);
	virtual void updateTab();
	virtual void reinitTab(bool doOverride);
	//Following are set by gui, result in save history state, 
	//plus notification to animation controller
	void setPlay(int direction);
	virtual void guiSetLocal(Params* p, bool lg);
	void toggleReplay(bool replay);
	void setTimestep(int framenum);

public slots:
	void animationPauseClick();
	void animationPlayReverseClick();
	void animationPlayForwardClick();
	void animationStepForwardClick();
	void animationStepReverseClick();
	
protected:
	bool dontUpdate;
	
	void setSliders (AnimationParams* a);

	void refreshFrontTab();
	void jumpToBegin();
	void jumpToEnd();
	
	void singleStep(bool forward);
	//Need to do more than EventROuter::guiSetLocal():
	
	void updateTimestepList(const char* descr);

	void populateTimestepTable();
	//Point to actions on main window:
	QAction* mainPlayForwardAction;
	QAction* mainPlayBackwardAction;
	QAction* mainPauseAction;
	bool keyframeTextChanged;
	bool endFrameTextChanged;
	int currentKeyIndex;
	
protected slots:
	void setPosition(int sliderposition);
	void setFrameStep(int sliderposition);
	void setAtabTextChanged(const QString& qs);
	void setEndFrameTextChanged(const QString& qs);
	void animationReturnPressed();
	
	//Animation slots:
	void animationReplayClick();
	void animationToBeginClick();
	void animationToEndClick();
	

};

};
#endif //ANIMATIONEVENTROUTER_H 
