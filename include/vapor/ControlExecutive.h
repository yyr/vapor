
#ifndef ControlExec_h

#include <string>
#include <vector>
#include <map>
#include <vapor/ExpatParseMgr.h>
using namespace std;
namespace VAPoR {

#include "common.h"

class Visualizer;
class Params;
class ParamsBase;
class RenderParams;
class DataMgr;
class ErrorHandler;
class Command;
class ParamNode;



//! \class ControlExec
//!
//! \brief Provides API for VAPOR visualizer User Interfaces (UIs)
//

class RENDER_API ControlExec : public ParsedXml {
public:
	//! Initialize the control executive
	//!
	//! \note what, if any, arguments are needed?
	ControlExec();
	~ControlExec();

	//! Obtain the singleton ControlExec object
	static ControlExec* getInstance(){
		if (!controlExecutive) controlExecutive = new ControlExec();
		return controlExecutive;
	}

	//! Create a new visualizer
	//!
	//! This method creates a new visualizer. A visualizer is a drawable
	//! OpenGL object (e.g. window or pbuffer). The caller is responsible
	//! for establishing an OpenGL drawable object and making its context
	//! current before calling NewVisualizer(). 
	//!
	//! \param[in] viznum Specifies the proposed handle for the new visualizer.
	//! The default viznum is -1, meaning that NewVisualizer will assign an unused value.
	//!
	//! \return viz Upon success an integer identifier for the visualizer
	//! is returned. A negative int is returned on failure
	//!
	//! \note Need to establish what OpenGL state mgt, if any, is performed
	//! by UI. For example, who calls swapbuffers?
	//!
	//! \note Since the UI is responsible for setting up the graphics 
	//! contexts we may need a method that allows the ControlExec 
	//! to provide
	//! hints about what kind of graphics context is needed 
	//! (e.g. double buffering)
	//
	static int NewVisualizer(int vizNum = -1);

	//! Delete an existing visualizer
	//! \param[in] viz handle to existing visualizer returned by NewVisualizer
	//! \retval int 0 if successful;
	static int RemoveVisualizer(int viz);


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
	static void InitializeViz(int viz, int width, int height);

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
	static void ResizeViz(int viz, int width, int height);

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
	static int Paint(int viz, bool force=false);


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
	static int ActivateRender(int viz, std::string type, int instance, bool on);

	//! Get a pointer to the existing parameter state information 
	//!
	//! This method returns a pointer to a Params class object 
	//! that manages all of the state information for 
	//! the Params instance identified by \p (viz,type,instance). The object pointer returned
	//! is used to both query parameter information as well as change
	//! parameter information. 
	//!
	//! \param[in] viz A visualizer handle returned by NewVisualizer().  If this is -1, then
	//! the global or default params is returned.
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
	static Params* GetParams(int viz, string type, int instance);

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
	static int SetParams(int viz, string type, int instance, Params* p);

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
	static int RemoveParams(int viz, string type, int instance);

	//! Add a new Params instance to the set of instances for a particular
	//! visualizer and type.  
	//! \param[in] viz A valid visualizer handle
	//! \param[in] type The type of the Params instance
	//! \param[in] p The instance to be added.
	//! \retval zero if successful.
	static int AddParams(int viz, string type, Params* p);

	//! Determine the instance index associated with a particular RenderParams instance
	//! in a specified visualizer
	//! Returns -1 if it is not found
	//! \param[in] viz The visualizer index.
	//! \param[in] p The RenderParams whose index is sought
	//! \return instance index, or -1 if it is not found.
	static int FindInstanceIndex(int viz, RenderParams* p);

	//! Determine how many instances of a given renderer type are present
	//! in a visualizer.  Necessary for setting up a UI.
	//! \param[in] viz A visualizer handle returned by NewVisualizer()
	//! \param[in] type The type of the RenderParams or Renderer 
	//! \return number of instances 
	//!

	static int GetNumParamsInstances(int viz, string type);

	//! Determine how many different Params classes are available.
	//! These include the Params classes associated with tabs in the GUI
	//! as well as those that are just used for Undo/Redo
	//! \sa GetNumTabParamsClasses();
	//! \return number of classes
	//!

	static int GetNumParamsClasses();

	//! Determine how many different Params classes are available
	//! to be associated with tabs in the GUI.
	//! Does not include Params classes that are used just for Undo/Redo
	//! \sa GetNumParamsClasses();
	//! \return number of classes
	//!

	static int GetNumTabParamsClasses();

	//! Determine the short name associated with a Params type
	//! \param[in] tag of the params type
	//! \return short name, e.g. for tab
	static const std::string GetShortName(string& typetag);

	//! Specify that a particular instance of a Params is current
	//! \param[in] viz A valid visualizer handle
	//! \param[in] type The type of RenderParams
	//! \param[in] instance The instance index that will become current.
	//! \return zero if successful.
	//!
	static int SetCurrentParamsInstance(int viz, string type, int instance);

	//! Identify the current instance of a RenderParams 
	//! \param[in] viz A valid visualizer handle
	//! \param[in] type The type of RenderParams
	//! \return The instance index that is current in this visualizer
	//!
	static int GetCurrentRenderParamsInstance(int viz, string type);

	//! Convert a ParamsBase typeId to a tag
	//! \param[in] typeId ParamsBaseTypeId to be converted.
	//! \retval tag XML tag associated with the ParamsBase Type
	//! \sa ParamsBase::ParamsBaseTypeId
	static const string GetTagFromType(int typeId);

	//! Convert a tag to a typeId
	//! \param[in] tag XML tag to be converted
	//! \retval typeId ParamsBaseTypeId associated with the tag
	//! \sa ParamsBase::ParamsBaseTypeId
	static int GetTypeFromTag(const std::string type);

	//! Obtain the Params instance that is currently active in the specified visualizer
	//! \param[in] viz A valid visualizer handle
	//! \param[in] type The type of Params
	//! \retval The current active Params, or NULL if invalid.
	static Params* GetCurrentParams(int viz, string type);

	//! Obtain the Params instance that is currently active in the active visualizer
	//! If no visualizer is active it returns the default params.
	//! \param[in] type The type of Params
	//! \retval The current active Params, or NULL if invalid.
	static Params* GetActiveParams(string type){
		return GetCurrentParams(GetActiveVizIndex(),type);
	}

	//! Method that returns the default Params instance of a particular type.
	//! With non-render params this is the global Params instance.
	//! \param[in] type tag associated with the params
	//! \retval Pointer to specified Params instance
	static Params* GetDefaultParams(string type);

	//! Determine how many visualizer windows are present
	//! \return number of visualizers 
	//!

	static int GetNumVisualizers(){
		return (int)visualizers.size();
	}
	//! obtain an existing visualizer
	//! \param[in] viz Handle of desired visualizer
	//! \return pointer to specified visualizer 
	//!

	static Visualizer* GetVisualizer(int viz){
		std::map<int, Visualizer*>::iterator it;
		it = visualizers.find(viz);
		if (it == visualizers.end()) return 0;
		return visualizers[viz];
	}

	//! Identify the active visualizer number
	//! When using a GUI this is the index of the selected visualizer
	//! and is established by calling SetActiveViz()
	//! \return index of the current active visualizer
	static int GetActiveVizIndex();

	//! Set the active visualizer
	//! GUI uses this method whenever user clicks on a window.
	//! \sa GetActiveVizIndex();
	//! \param[in] index of the current active visualizer
	//! \return 0 if successful
	static int SetActiveVizIndex(int index);

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
	static const DataMgr *LoadData(vector <string> files, bool deflt = true);

	//! Obtain a pointer to the current DataMgr
	//! Returns NULL if it does not exist.
	//! \retVal dataMgr
	static const DataMgr* GetDataMgr(){ return dataMgr;};

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
	static int DrawText(int viz, int x, int y, string font, int size, string text);

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

	string GetCommandText(int n);

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
	static int EnableCapture(string filename, int viz);

	//! Specify an error handler that the ControlExec will use
	//! to notify of asynchronous error conditions that arise.
	//!
	//! \note Not yet implemented
	//!
	//! The ErrorHandler class will have methods for
	//! Setting, clearing error state, which will support 
	//! A string description and a numerical error code.
	//! Only one ErrorHandler can be set.
	//! If none is set, no errors will be reported to the UI.
	static int SetErrorHandler(ErrorHandler* handler);

	//! Set the ControlExec to a default state:
	//! Remove all visualizers and Params instances
	//! Create default params
	//! restart the command queue
	//! Delete the DataMgr.
	static void SetToDefault();

	//! Verify that a Params instance is in a valid state
	//! Used to handle synchronous error checking,
	//! E.g. checking user input parameters.
	//!
	//! \note Not yet implemented
	//!
	//! \param[in] p pointer to Params instance being checked
	//! \return status nonzero indicates error
	static int ValidateParams(Params* p);

	private:
		static const string _VAPORVersionAttr;
		static const string _sessionTag;
		static const string _globalParamsTag;
		static const string _vizNumAttr;
		static const string _visualizerTag;
		static const string _visualizersTag;
		bool elementEndHandler(ExpatParseMgr* pm, int depth, std::string& tag);
		bool elementStartHandler(ExpatParseMgr* pm, int  depth, std::string& tag, const char **attrs);
		ParamNode* buildNode();
		//! At startup, create the initial Params instances:
		void createAllDefaultParams();
		//! when loading new data, reinitialize all the Params instances
		static void reinitializeParams(bool doOverride);
		//! delete all the Params instances and clean out the Undo/Redo queue e.g. before a session change
		void destroyParams();
		static std::map<int,Visualizer*> visualizers;
		static DataMgr* dataMgr;
		static ControlExec* controlExecutive;
		
		Params* tempParsedParams;
		int parsingVizNum;
		vector<int> parsingInstance;
};
};
#endif //ControlExec_h
