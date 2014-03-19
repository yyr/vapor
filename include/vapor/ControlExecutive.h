
#ifndef ControlExecutive_h

#include <string>
#include <vector>
using namespace std;
namespace VAPoR {

#include "common.h"


class Visualizer;
class Params;
class RenderParams;
class DataMgr;
class ErrorHandler;
class Command;

//! \class ControlExecutive
//!
//! \brief Provides API for VAPOR visualizer User Interfaces (UIs)
//

class RENDER_API ControlExecutive {
public:
	//! Initialize the control executive
	//!
	//! \note what, if any, arguments are needed?
	ControlExecutive();
	~ControlExecutive();

	//! Obtain the singleton ControlExecutive object
	static ControlExecutive* getInstance(){
		if (!controlExecutive) controlExecutive = new ControlExecutive();
		return controlExecutive;
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
	int NewVisualizer(/*oglinfo_c oglinfo*/);

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
	void InitializeViz(int viz, int width, int height);

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
	void ResizeViz(int viz, int width, int height);

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
	//! \return rc is 0 if actual painting occurred, -1 if not.
	//!
	//!
	int Paint(int viz, bool force=false);

	//! Specify the current ModelViewMatrix
	//!
	//! Tells the control executive that the specified matrix will be used
	//! at the next time Paint is called on the specified visualizer.
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//!	\param[in] matrix Specifies a float array of 16 values representing the
	//! new ModelView matrix.
	//!
	//!
	int SetModelViewMatrix(int viz, const double* mtx);

	//! Activate or Deactivate a renderer
	//!
	//! To activate a renderer, a new Renderer instance is created, associate
	//! with the RenderParams instance indicated by viz, type, and instance.
	//! This renderer instance is inserted in the queue of active renderers for the visualizer.
	//! To deactivate a renderer, the associated Renderer instance is removed
	//! from the queue of active renderers in the Visualizer, and then deleted.
	//! 
	//! \param[in] viz The visualizer where the renderer is 
	//! \param[in] type The type of renderer.
	//! \param[in] instance The instance index to be (de)activated.  Must be > 0.
	//! \param[in] on A boolean indicating if the renderer is to be made
	//! active (true) or inactive (off)
	//!
	//! \return status A negative int is returned on failure, indicating that
	//! the renderer cannot be activated
	//
	int ActivateRender(int viz, std::string type, int instance, bool on);

	//! Get a pointer to the existing parameter state information 
	//!
	//! This method returns a pointer to a Params class object 
	//! that manages all of the state information for 
	//! the Params instance identified by \p (viz,type,instance). The object pointer returned
	//! is used to both query parameter information as well as change
	//! parameter information. 
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer(). 
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
	Params* GetParams(int viz, string type, int instance);

	//! Specify the Params instance for a particular visualizer, instance index, and Params type
	//! This can be used to replace the current Params instance using a new Params pointer.
	//! This should not be used to install a Params instance where one did not exist already; Use
	//! NewParams for that purpose
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer().  Specify -1 if the Params is global. 
	//! \param[in] type The type of the Params (e.g. flow, probe)
	//! \param[in] Params* The pointer to the Params instance being installed.
	//! This is the same as the type of Renderer for a RenderParams.
	//! \param[in] instance Instance index, ignored for non-Render params.  Use -1 for the current active instance.
	//!
	//! \return int is zero if successful
	//!
	//
	int SetParams(int viz, string type, int instance, Params* p);

	//! Delete a RenderParams instance for a particular visualizer, instance index, and Params type
	//! The specified instance must previously have been de-activated.
	//! There must exist more than one instance or this will fail.
	//! All existing Params instances in the same visualizer and with larger instance index will have their
	//! instance index reduced by one (so as to avoid gaps in instance numbering).
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer(). 
	//! \param[in] type The type of the RenderParams (e.g. flow, probe)
	//! \param[in] instance Instance index to be removed.  Use -1 for the current active instance.
	//!
	//! \return int is zero if successful
	//!
	//! \sa ActivateRender
	//
	int RemoveParams(int viz, string type, int instance);

	//! Determine how many instances of a given renderer type are present
	//! in a visualizer.  Necessary for setting up a UI.
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//! \param[in] type The type of the RenderParams or Renderer 
	//! \return number of instances 
	//!

	int GetNumParamsInstances(int viz, string type);

	//! Determine how many visualizer windows are present
	//! \return number of visualizers 
	//!

	int GetNumVisualizers(){
		return (int)visualizers.size();
	}
	//! obtain an existing visualizer
	//! \param[in] viz Handle of desired visualizer
	//! \return pointer to specified visualizer 
	//!

	Visualizer* GetVisualizer(int viz){
		return visualizers[viz];
	}

	//! Save the current session state to a file
	//!
	//! \note Not yet implemented
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
	int SaveSession(string file);

	//!	Restore the session state from a session state file
	//!
	//! \note Not yet implemented
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
	int RestoreSession(string file);

	//! Load a data set into the current session
	//!
	//! Loads a data set specified by the list of files in \p files
	//!
	//! \param[in] files A vector of data file paths. For data sets
	//! not containing explicit time information the ordering of 
	//! time varying data will be determined by the order of the files
	//! in \p files.
	//! \param[in] deflt boolean indicating whether data will loaded into 
	//! the default settings (versus into an existing session).
	//!
	//! \return datainfo Upon success a constant pointer to a DataInfo
	//! structure is returned. The DataInfo structure can be used to 
	//! query metadata information about the data set. A NULL pointer 
	//! is returned on failure. Subsequent calls to LoadData() will 
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
	const DataMgr *LoadData(vector <string> files, bool deflt = true);

	//! Draw 2D text on the screen
	//!
	//! \note Not yet implemented
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
	int DrawText(int viz, int x, int y, string font, int size, string text);

	//! Undo the last session state change
	//! Restores the state of the session to what it was prior to the
	//! last change made via a Params object, or prior to the last call
	//! to Undo() or Redo(), whichever happened last. I.e. Undo() can
	//! be called repeatedly to undo multiple state changes. 
	//!
	//! State changes do not trigger rendering. It is the UI's responsibility
	//! to call Paint() after Undo(), and to make any UI internal changes
	//! necessary to reflect the new state. It is also the responsibility of
	//! the UI to set any change flags associated with the change in Params state.
	//! The returned const Params* pointer should be used by the GUI to determine which renderer(s)
	//! must be refreshed
    //!
	//! \return Params* ptr A pointer to the Params object that reflects the change.  Pointer is null if there is nothing to undo.
	//! \sa Redo()
	//!
	const Params* Undo();

	//! Redo the next session state change
	//! Restores the state of the session to what it was before the
	//! last change made via Undo,Redo() can
	//! be called repeatedly to undo multiple state changes. 
	//!
	//! State changes do not trigger rendering. It is the UI's responsibility
	//! to call Paint() after Redo(), and to make any UI internal changes
	//! necessary to reflect the new state.  It is also the responsibility of
	//! the UI to set any change flags associated with the change in Params state.
	//! The returned Params* pointer should be used by the GUI to determine which renderer(s)
	//! must be refreshed
    //!
	//! \return Params* ptr A pointer to the Params object that reflects the change.  Pointer is null if there is nothing to Redo
	//! \sa UnDo()
	//
	const Params* Redo();

	string& GetCommandText(int n);

	//! Indicates whether or not there exists a Command at the specified offset 
	//! from the latest Command which was issued, can be e.g. used to tell whether or not
	//! undo or redo is possible.
	//! Offset parameter indicates position in Command queue,
	//! Offset = 0 for last issued command, offset = -1 for next command to Redo
	//! \param [in] int offset from the end of the queue (last executed command)
	//! \return bool is true if there is a command.

	bool CommandExists(int offset); 

	//! Capture the next rendered image to a file
	//!
	//! \note Not yet implemented
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
	int EnableCapture(string filename, int viz);

	//! Specify an error handler that the ControlExecutive will use
	//! to notify of asynchronous error conditions that arise.
	//!
	//! \note Not yet implemented
	//!
	//! The ErrorHandler class will have methods for
	//! Setting, clearing error state, which will support 
	//! A string description and a numerical error code.
	//! Only one ErrorHandler can be set.
	//! If none is set, no errors will be reported to the UI.
	int SetErrorHandler(ErrorHandler* handler);

	//! Verify that a Params instance is in a valid state
	//! Used to handle synchronous error checking,
	//! E.g. checking user input parameters.
	//!
	//! \note Not yet implemented
	//!
	//! \param[in] p pointer to Params instance being checked
	//! \return status nonzero indicates error
	int ValidateParams(Params* p);

	private:
		//! At startup, create the initial Params instances:
		void createAllDefaultParams();
		//! when loading new data, reinitialize all the Params instances
		void reinitializeParams(bool doOverride);
		//! delete all the Params instances and clean out the Undo/Redo queue e.g. before a session change
		void destroyParams();
		vector<Visualizer*> visualizers;
		DataMgr* dataMgr;
		static ControlExecutive* controlExecutive;
};
};
#endif //ControlExecutive_h
