//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		eventrouter.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:  Definition of EventRouter class	
//		This (pure virtual) class manages communication between
//		gui elements, visualizers, and params.

#ifndef EVENTROUTER_H
#define EVENTROUTER_H
#include "assert.h"
#include "params.h"

#include "vizwin.h"



#include <qobject.h>
class QWidget;

#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100)
#endif

namespace VAPoR{


//! \class EventRouter
//! \brief A pure virtual class specifying the common properties of all the parameter tabs in the VAPOR GUI
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!	The EventRouter class manages communication between
//! tabs in the GUI, visualizers, and params.
//! Includes support for some elements that are common in these tabs
//
class EventRouter
{
	
public:
	
	//! Pure virtual method where all the Qt signals and slots associated with the tab are connected.
	//! Must include connections that send signals when any QTextEdit box is changed and when enter is pressed.
	virtual void hookUpTab() = 0;

	//! Pure virtual method to set the values of all the gui elements in the tab based on current params state.
	//! This is invoked whenever the user changes the tab or whenever the values in the tab need to be refreshed.
	virtual void updateTab() = 0;

	//! Virtual method to
	//! update only tab state that is most urgent after an error message.
	//! In particular, don't update any GL widgets in this method!
	//! This is to deal with error messages coming from the rendering thread
	//! that are trapped by the gui thread.
	//! Default does nothing.
	virtual void updateUrgentTabState() {return;}

	
	//! Method to change the current instance index, and perform associated undo/redo capture.
	//! Child classes can use this to respond to instance selection in the instance selector.
	//! \param[in] int newCurrent specifies new current index
	//! \param[in] bool undoredo specifies whether or not the event will be put on the undo/redo queue.
	virtual void performGuiChangeInstance(int newCurrent, bool undoredo=true);

	//! Method to create a new instance, and perform associated undo/redo capture.
	//! Child classes can use this to respond to clicks on the "new" instance button.
	virtual void performGuiNewInstance();

	//! Method to delete the current instance, and perform associated undo/redo capture.
	//! Child classes can use this to respond to clicks on the "delete" instance selector.
	virtual void performGuiDeleteInstance();

	//! Method to make a copy of the current instance with the same visualizer, and perform associated undo/redo capture.
	//! Child classes can use this to respond to user requests to copy instance to current visualizer.
	virtual void performGuiCopyInstance();

	//! Method to make a copy of the current instance into another visualizer, and perform associated undo/redo capture.
	//! Child classes can use this to respond to user requests to copy instance to another visualizer.
	//! \param[in] int vizwin  Visualizer number of other visualizer
	virtual void performGuiCopyInstanceToViz(int vizwin);

	//! Set theTextChanged flag.  The flag should be turned on whenever any textbox (affecting the 
	//! state of the Params) changes in the tab.  The change will not take effect until confirmText() is called.
	//! The flag will be turned off when confirmText() or updateTab() is called.
	//! \param[in] bool on : true indicates the flag is set.
	void guiSetTextChanged(bool on) {textChangedFlag = on;}

	//! Pure virtual method to confirm any change in a text box.  The render argument
	//! is true if this requires updateRenderer().
	//! This method is called whenever user presses enter, or changes the state of
	//! a widget (other than a textEdit) in the tab.
	virtual void confirmText(bool /*render*/) = 0;

	//! Pure virtual method to set up the content in a tab based on a change in the DataMgr.
	//! Any widgets that are data-specific, such as variable selectors, must be
	//! initialized in this method.
	//! The doOverride argument indicates that the user has requested default settings,
	//! so any previous setup can be overridden.
	virtual void reinitTab(bool doOverride) = 0;

	//! Method that must be reimplemented in any EventRouter that is associated with a RenderParams.
	//! It is invoked whenever the user checks or un-checks the enable checkbox in the instance selector.
	//! \param[in] bool Turns on (true) or off the instance.
	//! \param[in] int Instance that is being enabled or disabled.
	virtual void guiSetEnabled(bool On, int instance, bool undoredo=false  ) {assert(0);}

	//! Method for classes that capture mouse event events (i.e. have manipulators)
	//! This must be reimplemented to respond when the mouse is released.
	virtual void captureMouseUp() {assert(0);}

	//! Method for classes that capture mouse event events (i.e. have manipulators)
	//! This must be reimplemented to respond when the mouse is pressed
	virtual void captureMouseDown(int mouseNum) {assert(0);}

#ifndef DOXYGEN_SKIP_THIS
	EventRouter() {
		textChangedFlag = 0;
		
		
		isoShown = colorMapShown = opacityMapShown = texShown = false;
	}
	virtual ~EventRouter() {
		
	}
	//Methods to change instances (for undo/redo).
	//Only used by renderer params
	//Insert an instance (where there was none).
	//if instPosition is at end, appends to current instances
	//void insertCurrentInstance(Params* newParams, int instPosition);
	//Remove specified instance, disable it if necessary.
	void removeRendererInstance(int winnum, int instance);
	void newRendererInstance(int winnum);
	void copyRendererInstance(int toWinnum, RenderParams* rParams);
	void changeRendererInstance(int winnum, int newInstance);
	
	Params::ParamsBaseType getParamsBaseType() {return myParamsBaseType;}
	
	//Make the tab refresh after a scrolling operation.
	//Helpful on windows only, and only some tabs
	virtual void refreshTab() {}

	//UpdateRenderer ignores renderParams argument and uses the
	//params associated with the instance if it is nonnegative
	virtual void updateRenderer(RenderParams* , bool /*wasEnabled*/, int /*instance*/, bool /*newWindow*/){assert(0);}
	
	//make sure the params cleanly detaches from gui, to
	//handle possible connections from editors, frames, etc.
	virtual void cleanParams(Params*) {}
	
	virtual void sessionLoadTF(QString* ) {assert(0);}
	//Clone the params and any other related classes.
	//Default just clones the params:
	virtual Params* deepCopy(Params* p) {return (p->deepCopy());}
	
	virtual void guiSetLocal(Params* p, bool lg);
	
	//Methods to support maintaining a list of histograms
	//in each router (at least those with a TFE)

	

	//For render params, setEditorDirty uses the current instance if Params
	//arg is null
	virtual void setEditorDirty(RenderParams*){}
	virtual void updateMapBounds(RenderParams*) {assert (0);}
	virtual void updateClut(RenderParams*){assert(0);}

	

//Methods for loading/saving transfer functions:
void saveTF(RenderParams* rParams);
void fileSaveTF(RenderParams* rParams);
void loadInstalledTF(RenderParams* rParams, int varnum);
void loadTF(RenderParams* rParams, int varnum);

void fileLoadTF(RenderParams* rParams, int varnum, const char* startPath, bool savePath);
	//Workaround Darwin/Qt bug.  Note these are public:
	bool isoShown, colorMapShown, opacityMapShown, texShown;

public slots:

	virtual void guiStartChangeMapFcn(QString) { assert(0); }
	virtual void guiEndChangeMapFcn() { assert(0); }
	
	
protected:
	//for subclasses with a box:
	void setIgnoreBoxSliderEvents(bool val) {ignoreBoxSliderEvents = val;}
	bool ignoreBoxSliderEvents;
	//for subclasses with a datarange:
	virtual void setDatarangeDirty(RenderParams* ) {assert(0);}
	//Routers with histograms keep an array, one for each variable,
	// or variable combination
	
	
	//There is one tabbed panel for each class of Params
	
	Params::ParamsBaseType myParamsBaseType;
	bool textChangedFlag;
	
#endif //DOXYGEN_SKIP_THIS
};
};
#endif // EVENTROUTER_H

