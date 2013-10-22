//-- ControlExecutive.cpp ----------------------------------------------------------------
//
// Implementation of ControlExecutive methods
//----------------------------------------------------------------------------

#include "vapor/ControlExecutive.h"
#include "params.h"
#include "visualizer.h"
#include "renderer.h"
#include "arrowrenderer.h"
#include "arrowparams.h"
#include "animationparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "vapor/ExtensionClasses.h"
#include "vapor/DataMgrFactory.h"

using namespace VAPoR;
ControlExecutive* ControlExecutive::controlExecutive = 0;

ControlExecutive::ControlExecutive(){
	createAllDefaultParams();
}

	//! Create a new visualizer
	//!
	//! This method creates a new visualizer. A visualizer is a drawable
	//! OpenGL object (e.g. window or pbuffer). The caller is responsible
	//! for establishing an OpenGL drawable object and making its context
	//! current before calling NewVisualizer(). 
	//!
	//! \param[in] oglinfo Contains any information needed by the 
	//! control executive to start drawing in the drawable (perhaps no
	//! information is needed). TBD
	//!
	//! \return viz Upon success an integer identifier for the visualizer
	//! is returned. A negative int is returned on failure
	//!
	//! \note Need to establish what OpenGL state mgt, if any, is performed
	//! by UI. For example, who calls swapbuffers?
	//!
	//! \note Since the UI is responsible for setting up the graphics 
	//! contexts we may need a method that allows the ControlExecutive 
	//! to provide
	//! hints about what kind of graphics context is needed 
	//! (e.g. double buffering)
	//
int ControlExecutive::NewVisualizer(){
	int numviz = visualizers.size();
	Visualizer* viz = new Visualizer(numviz);
	visualizers.push_back(viz);
	return numviz;
}

	//! Perform OpenGL initialization of specified visualizer
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//! \param[in] width Width of visualizer
	//! \param[in] height Height of visualizer
	//!
	//! This method should be called by the UI once before any rendering is performed.
	//! The UI should make the OGL context associated with \p viz
	//! current prior to calling this method.
	//

void ControlExecutive::InitializeViz(int viz, int width, int height){
	Visualizer* v = visualizers[viz];
	v->initializeGL();
}

	//! Notify the control executive that a drawing object has
	//! changed size.
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//! \param[in] width Width of visualizer
	//! \param[in] height Height of visualizer
	//!
	//! This method should be called by the UI whenever the drawing
	//! object (e.g. window) associated with \p viz has changed size.
	//! The UI should make the OGL context associated with \p viz
	//! current prior to calling this method.
	//
void ControlExecutive::ResizeViz(int viz, int width, int height){
	Visualizer* v = visualizers[viz];
	v->resizeGL(width, height);
}

	//! Render the contents of a drawable
	//!
	//! Tells the control executive to invoke all active renderers associated
	//! with the visualizer \p viz. The control executive may elect to
	//! ignore this method if it believes the rendering state to be current,
	//! unless \p force is true.
	//!
	//! The UI should make the OGL context associated with \p viz
	//! current prior to calling this method.
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//!	\param[in] force If true all active renderers will rerender
	//! their scenes. If false, rendering will only be performed if
	//! the params objects associated with any of the active renderers 
	//! on this visualizer have changed state.
	//!
	//!
int ControlExecutive::Paint(int viz, bool force){
	Visualizer* v = visualizers[viz];
	if (!v) return -1;
	return v->paintEvent(force);
}

	//! Specify the current ModelViewMatrix
	//!
	//! Tells the control executive that the specified matrix will be used
	//! the next time Paint() is called on the specified visualizer.
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//!	\param[in] matrix Specifies a float array of 16 values representing the
	//! new ModelView matrix.
	//!
	//!
int ControlExecutive::SetModelViewMatrix(int viz, const double* mtx){
	Visualizer* v = visualizers[viz];
	v->setModelViewMatrix(mtx);
	return 0;
}

	//! Activate or Deactivate a renderer
	//!
	//!
	//! \return status A negative int is returned on failure, indicating that
	//! the renderer cannot be (de)activated
	//
int ControlExecutive::ActivateRender(int viz, string type, int instance, bool on){
	int numInsts = Params::GetNumParamsInstances(type,viz);
	if (numInsts < instance) return -1;
	RenderParams* p = (RenderParams*)Params::GetParamsInstance(type,viz,instance);
	//p should already have its state set to enabled if we are activating it here
	if (!p->isEnabled()){ //we should deactivate it here
		if (on) return -2; //should not be activating it
		GetVisualizer(viz)->removeRenderer(p);
		return 0;
	}
	//OK, p is enabled, we should activate it
	if (on) { 
		Renderer* myArrow = new ArrowRenderer(GetVisualizer(viz), p);
		GetVisualizer(viz)->insertSortedRenderer(p,myArrow);
		return 0;
	}
	else return -1;//not on!
}

	//! Get a pointer to the existing parameter state information 
	//!
	//! This method returns a pointer to a Params class object 
	//! that manages all of the state information for 
	//! the Params instance identified by \p (viz,type,instance). The object pointer returned
	//! is used to both query parameter information as well as change
	//! parameter information. 
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer().  Use -1 for the current active visualizer. 
	//! \param[in] type The type of the Params (e.g. flow, probe)
	//! This is the same as the type of Renderer for a RenderParams.
	//! \param[in] instance Instance index, ignored for non-Render params.  Use -1 for the current active instance.
	//!
	//! \return ptr A pointer to the Params object of the specified type that is
	//! currently associated with the specified visualizer (and of the specified instance, if
	//! this Params is a RenderParams.)
	//!
	//! \note Currently the Params API includes a mechanism for a renderer to
	//! register its interest in a changed parameter, and to be notified when
	//! that parameter changes.  We may also add API to 
	//! the Params class to register
	//! interest in change of *any* parameter if that proves to be useful.
	//! 
	//
Params* ControlExecutive::GetParams(int viz, string type, int instance){
	return Params::GetParamsInstance(type,viz,instance);
}
//! Specify the Params instance for a particular visualizer, instance index, and Params type
	//! This can be used to replace the current Params instance using a new Params pointer.
	//! When used to install a Params instance the instance index must be equal to the number of instances.
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer().
	//! \param[in] type The type of the Params (e.g. flow, probe)
	//! \param[in] Params* The pointer to the Params instance being installed.
	//! This is the same as the type of Renderer for a RenderParams.
	//! \param[in] instance Instance index, ignored for non-Render params.  Use -1 for the current active instance.
	//!
	//! \return int is zero if successful
	//!
	//! 
	//
int SetParams(int viz, string type, int instance, Params* p){
	if (viz == -1) { //Global or default params.  Ignore instance.
		Params::SetDefaultParams(type,p);
		return 0;
	}
	if (instance == -1) { //Current instance
		instance = Params::GetCurrentParamsInstanceIndex(type,viz);
	}
	if (instance == Params::GetNumParamsInstances(type, viz)){
		Params::AppendParamsInstance(type, viz, p);
		return 0;
	}
	if (instance < 0 || instance >= Params::GetNumParamsInstances(type, viz) )
		return -1;
	vector<Params*>& paramsvec = Params::GetAllParamsInstances(type, viz);
	paramsvec[instance] = p;
	return 0;
}
	//! Determine how many instances of a given renderer type are present
	//! in a visualizer.  Necessary for setting up a UI.
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//! \param[in] type The type of the RenderParams or Renderer 
	//! \return number of instances 
	//!

int ControlExecutive::GetNumParamsInstances(int viz, string type){
	return Params::GetNumParamsInstances(type,viz);
}

	//! Save the current session state to a file
	//!
	//! This method saves all current session state information 
	//! to the file specified by path \p file. All session state information
	//! is stored in Params objects and their derivatives
	//!
	//! \param[in] file	Path to the output file
	//!
	//! \return status A negative int indicates failure
	//!
	//! \sa RestoreSession()
	//
int ControlExecutive::SaveSession(string file){return 0;}

	//!	Restore the session state from a session state file
	//!
	//! This method sets the session state based on the contents of the
	//! session file specified by \p file. It also has the side effect
	//! of deactivating all renderers and unloading any data set 
	//! previously loaded by LoadData(). If successful, the state of all 
	//! Params objects may have changed.
	//!
	//! \param[in] file	Path to the output file
	//!
	//! \return status A negative int indicates failure. If the method
	//! fails the session state remains unchanged (is it possible to
	//! guarantee this? )
	//! 
	//! \sa LoadData(), GetRenderParams(), etc.
	//! \sa SaveSession()
	//
int ControlExecutive::RestoreSession(string file){return 0;}

	//! Load a data set into the current session
	//!
	//! Loads a data set specified by the list of files in \p files
	//!
	//! \param[in] files A vector of data file paths. For data sets
	//! not containing explicit time information the ordering of 
	//! time varying data will be determined by the order of the files
	//! in \p files.  Initially this will just be a vdf file.
	//! \param[in] dflt boolean indicating whether data will loaded into 
	//! the default settings (versus into an existing session).
	//!
	//! \return datainfo Upon success a constant pointer to a DataInfo
	//! structure is returned. The DataInfo structure can be used to 
	//! query metadata information about the data set. A NULL pointer 
	//! is returned on failure. Initially this will just be a DataMgr
	//! Subsequent calls to LoadData() will 
	//! invalidate previously returned pointers. 
	//!
	//! \note The proposed DataMgr API doesn't provide methods to easily 
	//! query some of the information that will be required by the UI, for
	//! example, the coordinate extents of a variable. These methods
	//! should either be added to the DataMgr, or the current DataStatus
	//! class might be cleaned up. Hence, DataInfo might be an enhanced
	//! DataMgr, or a class object designed specifically for returning
	//! metadata to the UI.
	//! \note (AN) It would be much better to incorporate the DataStatus methods into
	//! the DataMgr class, rather than keeping them separate.
	//
const DataMgr *ControlExecutive::LoadData(vector <string> files, bool dflt){
	int cacheMB = 2000;
	dataMgr = DataMgrFactory::New(files, cacheMB);
	DataStatus::getInstance()->reset(dataMgr,cacheMB);
	reinitializeParams(dflt);
	return dataMgr;
}

	//! Draw 2D text on the screen
	//!
	//! This method provides a simple interface for rendering text on
	//! the screen. No text will actually be rendered until after Paint()
	//! is called. Text rendering will occur after all active renderers
	//! associated with \p viz have completed rendering.
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//! \param[in] x X coordinate in pixels coordinates of lower left
	//! corner of rectangle bounding the text.
	//! \param[in] y Y coordinate in pixels coordinates of lower left
	//! corner of rectangle bounding the text.
	//! \param[in] font A string specifying the font name
	//! \param[in] size Font size in points
	//! \param[in] text The text to render
	//
int ControlExecutive::DrawText(int viz, int x, int y, string font, int size, string text){return 0;}

	//! Make a new Params object
	//!
	//! Makes a new Params class object that the UI can use to manage
	//! UI-specific state information (e.g. the currently active tab). 
	//! Any parameter information stored in the created Params object
	//! will be saved or restored when SaveSession() and RestoreSession()
	//! are respectively called. The Undo and Redo methods will also 
	//! act upon the created Params object
	//!
	//! \param[in] name A unique string name for the new params object
	//! \param[in] viz A visualizer id, when the params is specific to a visualizer.
	//!
	//! \return ptr A pointer to a new Params object is returned on 
	//! success.  A NULL pointer is returned on failure. 
	//!
	//! \sa Undo(), Redo(), RestoreSession(), SaveSession()
	//
Params * ControlExecutive::NewParams(string name, int viz){return 0;}

	//! Undo the last session state change
	//!
	//! Restores the state of the session to what it was prior to the
	//! last change made via a Params object, or prior to the last call
	//! to Undo() or Redo(), whichever happened last. I.e. Undo() can
	//! be called repeatedly to undo multiple state changes. 
	//!
	//! State changes do not trigger rendering. It is the UI's responsibility
	//! to call Paint() after Undo(), and to make any UI internal changes
	//! necessary to reflect the new state. The Params object that was
	//! modified will have appropriate
	//! flags set to indicate the state that has changed.
	//! \param[out] instance specifies the instance index of the Params instance that is being undone
	//! \param[out] viz indicates the visualizer associated with the Undo
	//! \param[out] type indicates the type of the Params
    //!
	//! \return Params* ptr A pointer to the Params object that reflects the change.  Pointer is null if there is nothing to undo.
	//! \sa Redo()
	//!
Params* ControlExecutive::Undo(int* instance, int *viz, string& type){return 0;}

	//! Redo the next session state change
	//!
	//! Restores the state of the session to what it was before the
	//! last change made via Undo,Redo() can
	//! be called repeatedly to undo multiple state changes. 
	//!
	//! State changes do not trigger rendering. It is the UI's responsibility
	//! to call Paint() after Redo(), and to make any UI internal changes
	//! necessary to reflect the new state. The Params object that was
	//! modified will have appropriate
	//! flags set to indicate the state that has changed.
	//! \param[out] instance specifies the instance index of the Params instance that is being redone
	//! \param[out] viz indicates the visualizer associated with the Redo
	//! \param[out] type indicates the type of the Params
    //!
	//! \return Params* ptr A pointer to the Params object that reflects the change.  Pointer is null if there is nothing to Redo
	//! \sa UnDo()
	//
Params* ControlExecutive::Redo(int* instance, int *viz, string& type){return 0;}

	//! Initiate a new entry in the Undo/Redo queue.  The changes that occur
	//! between StartCommand() and EndCommand() result in an entry in the Undo/Redo queue.
	//! By default, single state changes in a Params object will insert entries in the queue.
	//! This method and the following  EndCommand()  enable the user to combine multiple state changes into a single queue entry.
	//! Single state changes do not get inserted into the command queue between a StartCommand() and an EndCommand().
	//! Note that these calls must be serialized:  The first EndCommand() issued after a
	//! StartCommand(), with the same Params* pointer p, will complete the entry.
	//! If another StartCommand() occurs, or if the EndCommand is for a different Params*,
	//! then the StartCommand() will have no effect.
	//! \note Changes to a Params instance that occur but *not* between StartCommand() and EndCommand() will automatically
	//! result in a new entry in the Command queue (unless they are in error).
	//! \param[in] Params* p Pointer to the params that is changing
	//! \param[in] string text describes what is changing.
	//! \sa EndCommand()
	//
void ControlExecutive::StartCommand(Params* p, string text){}

	//! Complete a new entry in the Undo/Redo queue.  All state changes that occur in a Params instance
	//! between StartCommand() and EndCommand() result in a single entry in the Undo/Redo queue.
	//! \sa StartCommand
	//! \param[in] Params* p Pointer to the params that has changed
	//! \return int rc is zero if successful.  Otherwise state of p is returned to what is was
	//! when StartCommand was called
	//! \sa StartCommand()
	//
int ControlExecutive::EndCommand(Params* p){return 0;}

	//! Identify the changes in the undo/Redo queue
	//! Returns the text associated with a change in the undo/redo queue.
	//! \param[in] int num indicates the position of the command relative to the current state of the queue
	//! The most recent entry added corresponds to num = 0
	//! Negative values of num correspond with entries that can be redone.
	//! The null string is returned if there is no entry corresponding to n. 
	//! \return descriptive text \p string associated with the specified command.
	//
string& ControlExecutive::GetCommandText(int n){return * (new string(""));}

	//! Capture the next rendered image to a file
	//!
	//! When this method is called, the next time Paint() is called for 
	//! the specified visualizer, the rendered image 
	//! will be written to the specified (.jpg, .tif, ...) file.
	//! The UI must call Paint(viz, true) after this method is called. 
	//! Only one image will be captured; i.e. a subsequent call to Paint()
	//! will not capture unless EnableCapture() is called again.
	//! If this is called concurrently with a call to Paint(), the
	//! image will not be captured until that rendering completes
	//! and another Paint() is initiated.
	int ControlExecutive::EnableCapture(string filename, int viz){return 0;}

	//! Specify an error handler that the ControlExecutive will use
	//! to notify of asynchronous error conditions that arise.
	//! The ErrorHandler class will have methods for
	//! Setting, clearing error state, which will support 
	//! A string description and a numerical error code.
	//! Only one ErrorHandler can be set.
	//! If none is set, no errors will be reported to the UI.
int ControlExecutive::SetErrorHandler(ErrorHandler* handler){return 0;}

	//! Verify that a Params instance is in a valid state
	//! Used to handle synchronous error checking,
	//! E.g. checking user input parameters.
	//! \param[in] p pointer to Params instance being checked
	//! \return status nonzero indicates error
int ControlExecutive::ValidateParams(Params* p){return 0;}

//Create the global params and the default renderer params:
void ControlExecutive::
createAllDefaultParams() {

	//Install Extension Classes:
	InstallExtensions();

	ParamsBase::RegisterParamsBaseClass(Box::_boxTag, Box::CreateDefaultInstance, false);
	ParamsBase::RegisterParamsBaseClass(Viewpoint::_viewpointTag, Viewpoint::CreateDefaultInstance, false);

	//Animation Params comes first because others will refer to it.
	ParamsBase::RegisterParamsBaseClass(Params::_animationParamsTag, AnimationParams::CreateDefaultInstance, true);
	ParamsBase::RegisterParamsBaseClass(Params::_viewpointParamsTag, ViewpointParams::CreateDefaultInstance, true);
	ParamsBase::RegisterParamsBaseClass(Params::_regionParamsTag, RegionParams::CreateDefaultInstance, true);
}
void ControlExecutive::
reinitializeParams(bool doOverride){
	// Default render params should override; non render don't necessarily:
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		Params* p = Params::GetDefaultParams(i);
		p->reinit(true);
	}

	
	for (int i = 0; i< GetNumVisualizers(); i++){
		if (!GetVisualizer(i)) continue;
	
		//Reinitialize all the render params for each window
		for (int pType = 1; pType <= Params::GetNumParamsClasses(); pType++){
			for (int inst = 0; inst < Params::GetNumParamsInstances(pType, i); inst++){
				Params* p = Params::GetParamsInstance(pType,i,inst);
				p->reinit(doOverride);
				if (!p->isRenderParams()) break;
				RenderParams* rParams = (RenderParams*)p;
				rParams->setEnabled(false);
			}
		}
		GetVisualizer(i)->removeAllRenderers();
	}
}