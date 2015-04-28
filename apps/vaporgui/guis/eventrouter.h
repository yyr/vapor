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
class QHBoxLayout;
class QGroupBox;
class QComboBox;
class QButtonGroup;

#define OUT_OF_BOUNDS 1.e30f
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100)
#endif

namespace VAPoR{
class Histo;
//!
//! \class EventRouter
//! \ingroup Public_GUI
//! \brief A pure virtual class specifying the common properties of all the parameter tabs in the VAPOR GUI
//! \author Alan Norton
//! \version 3.0
//! \date    May 2014
//!	The EventRouter class manages communication between
//! tabs in the GUI, visualizers, and params.
//! Implementers of new tabs in vaporgui must implement an EventRouter class that translates user actions in 
//! the tab to changes in the corresponding Params instance, and conversely, displays the tab based on the
//! most recent state of the params.
//! Programmers must also implement a Qt .ui file that describes the appearance of the tab and names the 
//! various widgets in the tab.  Each EventRouter class is derived from the _UI class that is based on the .ui widget. 
//! The widget is connected to the EventRouter by various signals emitted by the widget.  The connections must
//! be set up in the EventRouter::hookUpTab() method.  
//! In addition to implementing the various pure virtual methods on this class, additional virtual methods
//! must be implemented to support changes in rendering, Transfer Functions, etc., based on the functionality of the tab.
//! 
//
class EventRouter
{
	
public:
	EventRouter(){
		currentHistogram = 0;
		_isoShown = false;
		_colorMapShown=false;
		_opacityMapShown=false;
		_texShown=false;
		myParamsBaseType = 0;
		textChangedFlag=false;
		ignoreBoxSliderEvents=false;
	}
	//! Pure virtual method connects all the Qt signals and slots associated with the tab.
	//! Must include connections that send signals when any QTextEdit box is changed and when enter is pressed.
	virtual void hookUpTab() = 0;

	//! Pure virtual method to set the values of all the gui elements into the tab based on current params state.
	//! This is invoked whenever the user changes the tab or whenever the values in the tab need to be refreshed.
	virtual void updateTab() = 0;

	//! Pure virtual method to respond to changes in text in the tab.  The render argument
	//! is true if this requires the renderer to be updated.
	//! This method should called whenever user presses enter, or changes the state of
	//! a widget (other than a textEdit) in the tab.
	virtual void confirmText(bool /*render*/) = 0;

	//! Pure virtual method to set up the content in a tab based on a change in the DataMgr.
	//! Any widgets that are data-specific, such as variable selectors, must be
	//! initialized in this method.
	//! The doOverride argument indicates that the user has requested default settings,
	//! so any previous setup can be overridden.
	virtual void reinitTab(bool doOverride) = 0;

	//! Virtual method to
	//! update only tab state that is most urgent after an error message.
	//! In particular, do not update any GL widgets in this method.
	//! This is to deal with error messages coming from the rendering thread
	//! that are trapped by the gui thread.
	//! Default does nothing.
	virtual void updateUrgentTabState() {return;}


	//! Set theTextChanged flag.  The flag should be turned on whenever any textbox (affecting the 
	//! state of the Params) changes in the tab.  The change will not take effect until confirmText() is called.
	//! The flag will be turned off when confirmText() or updateTab() is called.
	//! \param[in] bool on : true indicates the flag is set.
	void guiSetTextChanged(bool on) {textChangedFlag = on;}

	
	//! Method for classes that capture mouse event events (i.e. have manipulators)
	//! This must be reimplemented to respond when the mouse is released.
	//! The mouse release event is received by the VizWin instance, which then calls
	//! captureMouseUp() for the EventRouter that is associated with the current mouse mode.
	virtual void captureMouseUp() {assert(0);}

	//! Method for classes that capture mouse event events (i.e. have manipulators)
	//! This must be reimplemented to respond appropriately when the mouse is pressed.
	//! The mouse press event is received by the VizWin instance, which then calls
	//! captureMouseDown() for the EventRouter that is associated with the current mouse mode.
	virtual void captureMouseDown(int mouseNum) {assert(0);}

	//! Indicate the ParamsBaseType associated with the Params of this EventRouter.
	//! \retval ParamsBase::ParamsBaseType
	Params::ParamsBaseType getParamsBaseType() {return myParamsBaseType;}
	
	//! Make the tab refresh after a scrolling operation.
	//! Useful on Windows only, and only some tabs
	virtual void refreshTab() {}

	//! Virtual method to enable or disable rendering when turned on or off by a gui tab.
	//! Only useful if the tab corresponds to a renderer.
	//! \param[in] rp RenderParams instance to be enabled/disabled
	//! \param[in] wasEnabled indicates if rendering was previously disabled
	//! \param[in] instance index being enabled
	//! \param[in] newVis indicates if the rendering is being enabled in a new visualizer.
	virtual void updateRenderer(RenderParams* rp, bool wasEnabled, int instance, bool newVis);
	
	//! Virtual method invoked when a renderer/visualizer is deleted
	//! To make sure the params cleanly detaches from gui, to
	//! handle possible connections from editors, frames, etc.
	//! Must be implemented if tab is associated with RenderParams.
	//! Default does nothing
	//! \param[in] Params associated with the tab.
	virtual void cleanParams(Params* rp ) {}
	
	//! Virtual method supports loading a transfer function from the session, for 
	//! tabs that have transfer functions.
	//! param[in] name indicates the name identifying the transfer function
	virtual void sessionLoadTF(string name ) {assert(0);}

	//! Virtual method should be invoked to respond to a change in the local/global setting in the tab
	//! Only used with non-render params.  Default simply ensures that the active
	//! params are based on correct setting of local/global
	//! \param[in] Params* p Current params associated with the tab
	//! \param[in] bool lg is true for local, false for global.
	virtual void guiSetLocal(Params* p, bool lg);
	
	//! Method that must be reimplemented in any EventRouter that is associated with a RenderParams.
	//! It is invoked whenever the user checks or un-checks the enable checkbox in the instance selector.
	//! \param[in] bool Turns on (true) or off the instance.
	//! \param[in] int Instance that is being enabled or disabled.
	virtual void guiSetEnabled(bool On, int instance ) {assert(0);}

	//! Method to indicate that a transfer function has changed
	//! so the tab display must be refreshed.  Only used
	//! with RenderParams that have a transfer function.
	//! If Params* argument is null, uses default params
	//! \param[in] RenderParams* is the Params that owns the Transfer Function
	virtual void setEditorDirty(RenderParams*){}
	//! Method used to indicate that the mapping bounds have changed,
	//! in the transfer function editor, requiring update of the display
	//! \param[in] RenderParams* owner of the Transfer Function
	virtual void updateMapBounds(RenderParams*) {assert (0);}

	//! Method supporting loading/saving transfer functions in tabs with transfer functions
	//! Launch a dialog to save the current transfer function to session or file.
	//! \param[in] rParams RenderParams instance associated with the transfer function
	void saveTF(RenderParams* rParams);
	//! Launch a dialog to save the current transfer function to file.
	//! \param[in] rParams RenderParams instance associated with the transfer function
	void fileSaveTF(RenderParams* rParams);
	//! Launch a dialog to enable user to load an installed transfer function.
	//! \param[in] rParams RenderParams instance associated with the transfer function
	//! \param[in] varname name of the variable associated with the TF.
	void loadInstalledTF(RenderParams* rParams, string varname);
	//! Launch a dialog to enable user to load a transfer function from session or file.
	//! \param[in] rParams RenderParams instance associated with the transfer function
	//! \param[in] varname name of the variable associated with the TF.
	void loadTF(RenderParams* rParams, string varname);
	//! Launch a dialog to enable user to load a transfer function from file.
	//! \param[in] rParams RenderParams instance associated with the transfer function
	//! \param[in] varname name of the variable associated with the TF.
	//! \param[in] startPath file path for the dialog to initially present to user
	//! \param[in] savePath indicates whether or not the resulting path should be saved to user preferences.
	void fileLoadTF(RenderParams* rParams, string varname, const char* startPath, bool savePath);

	virtual Histo* getHistogram(RenderParams*, bool mustGet, bool isIsoWin = false);

	virtual void refreshHistogram(RenderParams* p){}
	virtual void refresh2DHistogram(RenderParams*);
	//Calculate a histogram of a slice of 3d variables
	void calcSliceHistogram(RenderParams*p, int ts, Histo* histo);

public slots:
	
	bool isoShown() {return _isoShown;}
	bool opacityMapShown() {return _opacityMapShown;}
	bool colorMapShown() {return _colorMapShown;}
	
protected:
	
	//! Method used by tabs which have box sliders.
	//! Avoids updating of the slider text and other widgets that
	//! could interfere with interactive updating.
	//! \param[in] doIgnore true to start ignoring, false to stop.
	void setIgnoreBoxSliderEvents(bool doIgnore) {ignoreBoxSliderEvents = doIgnore;}
	//Use fidelity setting and preferences to calculate LOD and Refinement
	//To support Fidelity, each eventRouter class must perform the following:
	//Implement fidelityBox and fidelityLayout as in dvr.ui
	// set fidelityButtons = 0 in constructor
	// implement slots guiSetFidelity(int) and guiSetFidelityDefault()
	// connect fidelityDefaultButton clicked() to guiSetFidelityDefault
	// in updateTab, check for fidelityUpdateChanged, if so, call setupFidelity
	//	 then connect fidelityButtons to guiSetFidelity, then call updateFidelity()
	// in reinitTab, call SetFidelityLevel, then connect fidelityButtons to guiSetFidelity
	// in guiSetCompRatios, call SetIgnoreFidelity(true)
	// in guiSetRefinement, call SetIgnoreFidelity(true)
	// in guiSetFidelityDefault, call setFidelityDefault

	//Build the vectors of reflevels and lods
	virtual int orderLODRefs(RenderParams* rParams, int dim);
	//Determine the default lod and ref level for a specified region size
	virtual void calcLODRefDefault(RenderParams* rParams, int dim, float regMBs, int* lod, int* reflevel);
	virtual void updateFidelity(QGroupBox*, RenderParams* rp, QComboBox* lodCombo, QComboBox* refinementCombo);

	void setupFidelity(int dim, QHBoxLayout* fidelityLayout,
		QGroupBox* fidelityBox, RenderParams* dParams, bool useDefault=false);
	void setFidelityDefault(int dim, RenderParams* dParams);
	//! Method indicates whether box slider events are being ignored.
	//! \retval true if the box slider events are being ignored.
	bool doIgnoreBoxSliderEvents() {return ignoreBoxSliderEvents;}
	
	vector<QAction*>* makeWebHelpActions(const char* text[], const char* urls[]);
	virtual QAction* getWebHelpAction(int n) {return (*myWebHelpActions)[n];}
	vector<QAction*>* myWebHelpActions;
	QButtonGroup* fidelityButtons;
	vector<int> fidelityRefinements;
	vector<int> fidelityLODs;
	bool fidelityDefaultChanged;
	Histo* currentHistogram;

#ifndef DOXYGEN_SKIP_THIS
	//! variables used to workaround Darwin bug, indicate whether or not
	//! various widgets in the tab have yet been displayed.
	bool _isoShown, _colorMapShown, _opacityMapShown, _texShown;
	Params::ParamsBaseType myParamsBaseType;
	bool textChangedFlag;
	bool ignoreBoxSliderEvents;
	
#endif //DOXYGEN_SKIP_THIS
};
};
#endif // EVENTROUTER_H

