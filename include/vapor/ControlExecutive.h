using namespace std;


Class Params

//! \class ControlExecutive
//!
//!	Provides API for VAPOR visualizer User Interface (UI)
//
class ControlExecutive {
public:
	//! Initialize the control executive
	//!
	//! \note what, if any, arguments are needed?
	ControlExecutive();

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
	int NewVisualizer(oglinfo_c oglinfo);

	//! Notify the control executive that a drawing object has
	//! changed size.
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//!
	//! This method should be called by the UI whenever the drawing
	//! object (e.g. window) associated with \p viz has changed size.
	//! The UI should make the OGL context associated with \p viz
	//! current prior to calling this method.
	//
	void ResizeViz(int viz);

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
	int Paint(int viz, bool force=false);

	//! Create a new renderer
	//!
	//! This method creates a new renderer, capable of rendering into 
	//! the visualizer associated with \p viz.
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//! \param[in] type The type of renderer to be created. Supported types
	//! include "dvr", "iso", "flow", etc.
	//! \param[in] p An instance of a RenderParams class, of same type, that is associated with the renderer
	//!
	//! \return instance The instance index of the new renderer
	//! is returned. A negative int is returned on failure
	//!
	//! \sa NewVisualizer()
	//
	int NewRenderer(int viz, string type, Params* p);

	//! Activate or Deactivate a renderer
	//!
	//! This method marks a renderer as active or inactive. An active 
	//! renderer will render when the Paint method is called. An inactive
	//! rendender will perform no rendering.
	//! 
	//! \param[in] viz The visualizer where the renderer is 
	//! \param[in] type The type of renderer.
	//! \param[in] instance The instance index to be (de)activated.
	//! \param[in] on A boolean indicating if the renderer is to be made
	//! active (true) or inactive (off)
	//!
	//! \return status A negative int is returned on failure, indicating that
	//! the renderer cannot be activated
	//
	int ActivateRender(int viz, string type, int instance, bool on);

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
	//! \param[in] instance Instance index, ignored for non-Render params
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

	//! Determine how many instances of a given renderer type are present
	//! in a visualizer.  Necessary for setting up a UI.
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//! \param[in] type The type of the RenderParams or Renderer 
	//! \return number of instances 
	//!

	int GetNumParamsInstances(int viz, string type);

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
	int SaveSession(string file);

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
	int RestoreSession(string file);

	//! Load a data set into the current session
	//!
	//! Loads a data set specified by the list of files in \p files
	//!
	//! \param[in] files A vector of data file paths. For data sets
	//! not containing explicit time information the ordering of 
	//! time varying data will be determined by the order of the files
	//! in \p files.
	//! \param[in] default boolean indicating whether data will loaded into 
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
	const DataInfo *LoadData(vector <string> files, bool default = true);

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
	int DrawText(int viz, int x, int y, string font, int size, string text);

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
	Params *NewParams(string name, int viz);

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
	Params* Undo(int* instance, int *viz, string& type);

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
	Params* Redo(int* instance, int *viz, string& type);

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
	void StartCommand(Params* p, string text);

	//! Complete a new entry in the Undo/Redo queue.  All state changes that occur in a Params instance
	//! between StartCommand() and EndCommand() result in a single entry in the Undo/Redo queue.
	//! \sa StartCommand
	//! \param[in] Params* p Pointer to the params that has changed
	//! \return int rc is zero if successful.  Otherwise state of p is returned to what is was
	//! when StartCommand was called
	//! \sa StartCommand()
	//
	int EndCommand(Params* p);

	//! Identify the changes in the undo/Redo queue
	//! Returns the text associated with a change in the undo/redo queue.
	//! \param[in] int num indicates the position of the command relative to the current state of the queue
	//! The most recent entry added corresponds to num = 0
	//! Negative values of num correspond with entries that can be redone.
	//! The null string is returned if there is no entry corresponding to n. 
	//! \return descriptive text \p string associated with the specified command.
	//
	string& GetCommandText(int n);

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
	int EnableCapture(string filename, int viz);

	//! Specify an error handler that the ControlExecutive will use
	//! to notify of asynchronous error conditions that arise.
	//! The ErrorHandler class will have methods for
	//! Setting, clearing error state, which will support 
	//! A string description and a numerical error code.
	//! Only one ErrorHandler can be set.
	//! If none is set, no errors will be reported to the UI.
	int SetErrorHandler(ErrorHandler* handler);

	//! Verify that a Params instance is in a valid state
	//! Used to handle synchronous error checking,
	//! E.g. checking user input parameters.
	//! \param[in] p pointer to Params instance being checked
	//! \return status nonzero indicates error
	int ValidateParams(Params* p);


	

}
