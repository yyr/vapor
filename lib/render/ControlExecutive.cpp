//-- ControlExec.cpp ----------------------------------------------------------------
//
// Implementation of ControlExec methods
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
#include "mousemodeparams.h"
#include "vizwinparams.h"
#include "instanceparams.h"
#include "vapor/ExtensionClasses.h"
#include "vapor/DataMgrFactory.h"
#include "vapor/ParamNode.h"
#include "vapor/Version.h"
#include "command.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "vizwinparams.h"

using namespace VAPoR;
ControlExec* ControlExec::controlExecutive = 0;
std::map <int,Visualizer*> ControlExec::visualizers;
DataMgr* ControlExec::dataMgr = 0;
const string ControlExec::_sessionTag = "VaporSession";
const string ControlExec::_VAPORVersionAttr = "VaporVersion";
const string ControlExec::_globalParamsTag = "GlobalParams";
const string ControlExec::_vizNumAttr = "VisualizerNum";
const string ControlExec::_visualizerTag = "Visualizer";
const string ControlExec::_visualizersTag = "Visualizers";

ControlExec::ControlExec(){
	createAllDefaultParams();
}
ControlExec::~ControlExec(){
	destroyParams();
	Command::resetCommandQueue();
}


int ControlExec::NewVisualizer(int viznum){
	std::map<int, Visualizer*>::iterator it;
	
	bool addVis = false;  //Indicate whether a new visualizer must be added to the VizWinParams
	//if viznum is nonnegative, see if the index is already used.
	if (viznum >= 0) {
		it = visualizers.find(viznum);
		if (it != visualizers.end()) viznum = -1;  //It's used; need to find another index!
		else { //viznum is OK
			Visualizer* viz = new Visualizer(viznum);
			visualizers[viznum] = viz;
		}
	}
	//if viznum < 0, Find the first unused index
	if (viznum < 0){
		int vizIndex = -1;
		for (int indx = 0; indx <= visualizers.size(); indx++){
			it = visualizers.find(indx);
			if (it == visualizers.end()){
				Visualizer* viz = new Visualizer(indx);
				visualizers[indx] = viz;
				vizIndex = indx;
				break;
			}
		}
		viznum = vizIndex;
	}
	if (viznum >= 0) addVis = true;
	//Save to VizWinParams with default settings
	if (addVis) VizWinParams::AddVizWin("Visualizer",viznum, 400,400);
	return viznum;
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

void ControlExec::InitializeViz(int viz, int width, int height){
	Visualizer* v = GetVisualizer(viz);
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
void ControlExec::ResizeViz(int viz, int width, int height){
	Visualizer* v = GetVisualizer(viz);
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
int ControlExec::Paint(int viz, bool force){
	Visualizer* v = GetVisualizer(viz);
	if (!v) return -1;
	return v->paintEvent(force);
}

	//! Activate or Deactivate a renderer
	//!
	//!
	//! \return status A negative int is returned on failure, indicating that
	//! the renderer cannot be (de)activated
	//
int ControlExec::ActivateRender(int viz, string type, int instance, bool on){
	int numInsts = Params::GetNumParamsInstances(type,viz);
	if (numInsts < instance) return -1;
	RenderParams* p = (RenderParams*)Params::GetParamsInstance(type,viz,instance);
	//p should already have its state set to enabled if we are activating it here
	if (!p->IsEnabled()){ //we should deactivate it here
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

Params* ControlExec::GetParams(int viz, string type, int instance){
	Params* p = Params::GetParamsInstance(type,viz,instance);
	if (instance >= 0) assert (p->GetInstanceIndex() == instance);
	return Params::GetParamsInstance(type,viz,instance);
}
int ControlExec::SetCurrentParamsInstance(int viz, string typetag, int instance){
	if (0 == GetVisualizer(viz)) return -1;
	if (instance < 0 || instance >= GetNumParamsInstances(viz,typetag)) return -1;
	int ptype = Params::GetTypeFromTag(typetag);
	if (ptype <= 0) return -1;
	int rc = 0;
	
	if (GetDefaultParams(typetag)->isRenderParams()){
		rc = InstanceParams::SetCurrentInstance(typetag,viz,instance);
	} else Params::SetCurrentParamsInstanceIndex(ptype,viz,instance);
	return rc;
}
int ControlExec::GetCurrentRenderParamsInstance(int viz, string typetag){
	if (0 == GetVisualizer(viz)) return -1;
	int ptype = Params::GetTypeFromTag(typetag);
	if (ptype <= 0) return -1;
	return Params::GetCurrentParamsInstanceIndex(ptype,viz);
}

Params* ControlExec::GetCurrentParams(int viz, string typetag){
	
	int ptype = Params::GetTypeFromTag(typetag);
	if (ptype <= 0) return 0;
	return Params::GetCurrentParamsInstance(ptype,viz);
}
int ControlExec::AddParams(int viz, string type, Params* p){
	if (0 == GetVisualizer(viz)) return -1;
	int ptype = Params::GetTypeFromTag(type);
	if (ptype <= 0) return -1;
	Params::AppendParamsInstance(ptype,viz,p);
	if (p->isRenderParams()) {
		int rc = InstanceParams::AddInstance(type, viz, p);
		return rc;
	}
	return 0;
}
int ControlExec::RemoveParams(int viz, string type, int instance){
	if (0 == GetVisualizer(viz)) return -1;
	int ptype = Params::GetTypeFromTag(type);
	if (ptype <= 0) return -1;
	//Must update the instance params before performing the actual removal...
	Params* p = ControlExec::GetDefaultParams(type);
	if (!p->isRenderParams()) return 0; //Instance params only exist for RenderParms
	int rc = InstanceParams::RemoveInstance(type, viz, instance);
	Params::RemoveParamsInstance(ptype,viz,instance);
	return rc;
}
int ControlExec::FindInstanceIndex(int viz, RenderParams* p){
	if (0 == GetVisualizer(viz)) return -1;
	ParamsBase::ParamsBaseType t = p->GetParamsBaseTypeId();
	for (int i = 0; i< Params::GetNumParamsInstances(t,viz); i++){
		if (p == Params::GetParamsInstance(t,viz,i)) return i;
	}
	return -1;
}
ParamsBase::ParamsBaseType ControlExec::GetTypeFromTag(const string tag){
	return ParamsBase::GetTypeFromTag(tag);
}
const std::string ControlExec::GetTagFromType(ParamsBase::ParamsBaseType t){
	return ParamsBase::GetTagFromType(t);
}
Params* ControlExec::GetDefaultParams(string type){
		return Params::GetDefaultParams(type);
}
int ControlExec::
SetParams(int viz, string type, int instance, Params* p){
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

int ControlExec::GetNumParamsInstances(int viz, string type){
	return Params::GetNumParamsInstances(type,viz);
}

	
const DataMgr *ControlExec::LoadData(vector <string> files, bool dflt){
	int cacheMB = 2000;
	dataMgr = DataMgrFactory::New(files, cacheMB);
	if (!dataMgr) return dataMgr;
	bool hasData = DataStatus::getInstance()->reset(dataMgr,cacheMB);
	if (!hasData) return 0;
	Command::blockCapture();
	reinitializeParams(dflt);
	Command::resetCommandQueue();
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
int ControlExec::DrawText(int viz, int x, int y, string font, int size, string text){return 0;}

	//! Identify the changes in the undo/Redo queue
	//! Returns the text associated with a change in the undo/redo queue.
	//! \param[in] int num indicates the position of the command relative to the current state of the queue
	//! The most recent entry added corresponds to num = 0
	//! Negative values of num correspond with entries that can be redone.
	//! The null string is returned if there is no entry corresponding to n. 
	//! \return descriptive text \p string associated with the specified command.
	//
string ControlExec::GetCommandText(int n){
	Command* cmd = Command::CurrentCommand(n);
	if (!cmd) return *(new string(""));
	return cmd->getDescription();
}

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
	int ControlExec::EnableCapture(string filename, int viz){return 0;}

	//! Specify an error handler that the ControlExec will use
	//! to notify of asynchronous error conditions that arise.
	//! The ErrorHandler class will have methods for
	//! Setting, clearing error state, which will support 
	//! A string description and a numerical error code.
	//! Only one ErrorHandler can be set.
	//! If none is set, no errors will be reported to the UI.
int ControlExec::SetErrorHandler(ErrorHandler* handler){return 0;}

	//! Verify that a Params instance is in a valid state
	//! Used to handle synchronous error checking,
	//! E.g. checking user input parameters.
	//! \param[in] p pointer to Params instance being checked
	//! \return status nonzero indicates error
int ControlExec::ValidateParams(Params* p){return 0;}

//Create the global params and the default renderer params:
void ControlExec::
createAllDefaultParams() {

	//Note that Basic Params must be registered after other params??
	ParamsBase::RegisterParamsBaseClass(MouseModeParams::_mouseModeParamsTag,MouseModeParams::CreateDefaultInstance, true);
	ParamsBase::RegisterParamsBaseClass(VizWinParams::_vizWinParamsTag,VizWinParams::CreateDefaultInstance, true);
	ParamsBase::RegisterParamsBaseClass(InstanceParams::_instanceParamsTag,InstanceParams::CreateDefaultInstance, true);
	//Install Extension Classes:
	InstallExtensions();

	ParamsBase::RegisterParamsBaseClass(Box::_boxTag, Box::CreateDefaultInstance, false);
	ParamsBase::RegisterParamsBaseClass(Viewpoint::_viewpointTag, Viewpoint::CreateDefaultInstance, false);

	//Animation Params comes first because others will refer to it.
	ParamsBase::RegisterParamsBaseClass(Params::_animationParamsTag, AnimationParams::CreateDefaultInstance, true);
	ParamsBase::RegisterParamsBaseClass(Params::_viewpointParamsTag, ViewpointParams::CreateDefaultInstance, true);
	ParamsBase::RegisterParamsBaseClass(Params::_regionParamsTag, RegionParams::CreateDefaultInstance, true);
	
	MouseModeParams::RegisterMouseModes();
	
}
void ControlExec::
reinitializeParams(bool doOverride){
	// Default render params should override; non render don't necessarily:
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		Params* p = Params::GetDefaultParams(i);
		bool rparams = p->isRenderParams();
		p->Validate(rparams||doOverride);
		ViewpointParams* vpp = dynamic_cast<ViewpointParams*>(p);
		if (vpp) vpp->SetChanged(true);
	}

	std::map<int,Visualizer*>::iterator it;
	for (it = visualizers.begin(); it != visualizers.end(); it++){

		int viz = it->first;
	
		//Reinitialize all the render params for each window
		for (int pType = 1; pType <= Params::GetNumParamsClasses(); pType++){
			for (int inst = 0; inst < Params::GetNumParamsInstances(pType, viz); inst++){
				Params* p = Params::GetParamsInstance(pType,viz,inst);
				p->Validate(doOverride);
				if (!p->isRenderParams()) break;
				RenderParams* rParams = (RenderParams*)p;
				rParams->SetEnabled(false);
			}
			//Set the active instances for renderParams
			Params* q = Params::GetDefaultParams(pType);
			if (!q->isRenderParams()) continue;
			int currentInstance = InstanceParams::GetCurrentInstance(GetTagFromType(pType),viz);
			Params::SetCurrentParamsInstanceIndex(pType, viz, currentInstance);
		}
		(it->second)->removeAllRenderers();
	}
}

const Params* ControlExec::Undo( ){
	Command::blockCapture();
	Params* p= Command::BackupQueue();
	Command::unblockCapture();
	return p;
}
const Params* ControlExec::Redo(){
	Command::blockCapture();
	Params* p= Command::AdvanceQueue();
	Command::unblockCapture();
	return p;
}

bool ControlExec::CommandExists(int offset) {
	return (Command::CurrentCommand(offset) != 0);
}
void ControlExec::destroyParams(){
	std::map<int,Visualizer*>::iterator it;
	for (it = visualizers.begin(); it != visualizers.end(); it++){
		int viz = it->first;
		for (int pType = 1; pType <= Params::GetNumParamsClasses(); pType++){
			for (int inst = 0; inst < Params::GetNumParamsInstances(pType, viz); inst++){
				Params* p = Params::GetParamsInstance(pType,viz,inst);
				delete p;
			}
		}
		(it->second)->removeAllRenderers();
	}
}
int ControlExec::GetNumBasicParamsClasses(){
	return ParamsBase::GetNumBasicParamsClasses();
}
int ControlExec::GetNumParamsClasses(){
	return ParamsBase::GetNumParamsClasses();
}
int ControlExec::GetNumTabParamsClasses(){
	return (ParamsBase::GetNumParamsClasses() - ParamsBase::GetNumBasicParamsClasses());
}
const std::string ControlExec::GetShortName(string& typetag){
	Params::ParamsBaseType ptype = Params::GetTypeFromTag(typetag);
	return Params::paramName(ptype);
}

int ControlExec::RestoreSession(string filename)
{

	
	if(filename.length() == 0) return -1;
		
	ifstream is;
	is.open((const char*)filename.c_str());
	if (!is){//Report error if you can't open the file
		return -1;
	}
	
	assert(MyBase::GetErrCode()==0);
	ExpatParseMgr* parseMgr = new ExpatParseMgr(this);
	parseMgr->parse(is);
	delete parseMgr;
	assert(MyBase::GetErrCode()==0);
	//Now create new visualizers all viz windows 
	
	int numViz = VizWinParams::GetNumVizWins();
	vector<long> sessionVizIndices = VizWinParams::GetVisualizerNums();
	
	for (int i = 0; i<numViz; i++){
		int newVizIndex = NewVisualizer(sessionVizIndices[i]);
		assert (newVizIndex == sessionVizIndices[i]);
	}

	return 0;
}


int ControlExec::SaveSession(string filename)
{
	
	ofstream fileout;
	string s;
	
	
	fileout.open(filename.c_str());
	if (! fileout) {
		return -1;
	}
	
	ParamNode* const rootNode = buildNode();
	fileout << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
	XmlNode::streamOut(fileout,(*rootNode));
	if (MyBase::GetErrCode() != 0) {
		
		MyBase::SetErrCode(0);
		delete rootNode;
		return -1;
	}
	delete rootNode;
	

	fileout.close();
	return 0;
}
bool ControlExec::
elementStartHandler(ExpatParseMgr* pm, int  depth, std::string& tag, const char **attrs){
	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	switch (depth){
	//Parse the global params, depth = 0
		case(0): 
		{
			if (StrCmpNoCase(tag, _sessionTag) == 0)
				return true;
			else return false;
		}
		case(1): 
			//Parse child tags (either global params or visualizers)
			if (StrCmpNoCase(tag, _globalParamsTag) == 0){
				return true;//The Params class will parse it at level 2 
			} else if (StrCmpNoCase(tag, _visualizersTag) == 0)
				return true;//The local Params class will parse it at level 3 

			else return false;

		case(2):
			//parse grandchild tags: global params or visualizers
			if (Params::IsParamsTag(tag)){
				tempParsedParams = Params::CreateDefaultParams(Params::GetTypeFromTag(tag));
				pm->pushClassStack(tempParsedParams);
				tempParsedParams->elementStartHandler(pm, depth, tag, attrs);
				return true;
			}
			else if (StrCmpNoCase(tag, _visualizerTag) == 0){
				parsingVizNum = -1;
				parsingInstance.clear();
				//Initialize the instances to -1
				for (int i = 0; i<= Params::GetNumParamsClasses(); i++)
					parsingInstance.push_back(-1);
				while (*attrs) {
					string attr = *attrs;
					attrs++;
					string value = *attrs;
					attrs++;
					istringstream ist(value);
					if (StrCmpNoCase(attr, _vizNumAttr) == 0) 
						ist >> parsingVizNum;
				}
				if (parsingVizNum < 0) return false;
				return true;//The local Params class will parse the children at level 3
			}  
			pm->skipElement(tag, depth);  //Ignore unrecognized params
			return true;
			
		case(3):
			{
				//parse local params
				//push the subsequent parsing to the params for current window 
				Params::ParamsBaseType typeId = Params::GetTypeFromTag(tag);
				if (typeId <= 0) {//Unrecognized, make a dummy params
					//error message....
					Params* dummyParams = Params::CreateDummyParams(tag);
					Params::addDummyParamsInstance(dummyParams);
					pm->pushClassStack(dummyParams);
					dummyParams->elementStartHandler(pm,depth,tag,attrs);
					return true;
				}
				parsingInstance[typeId]++;
				Params* parsingParams;
				
				parsingParams = Params::CreateDefaultParams(typeId);
				//only renderParams can have more than one instance
				assert(parsingParams->isRenderParams() || parsingInstance[typeId] == 0);
				Params::AppendParamsInstance(typeId,parsingVizNum, parsingParams);
				
				assert(Params::GetNumParamsInstances(typeId,parsingVizNum) == (parsingInstance[typeId] + 1));
			
				pm->pushClassStack(parsingParams);
				parsingParams->elementStartHandler(pm, depth, tag, attrs);
				tempParsedParams = parsingParams;
				return true;
			}
		default: 
			pm->skipElement(tag, depth);
			return true;
	}
}

bool ControlExec::
elementEndHandler(ExpatParseMgr* pm, int depth, std::string& tag){
	
	switch (depth){
		case (0):
			if(StrCmpNoCase(tag, _sessionTag) != 0) return false;
			return true;
		case (1):
			if (StrCmpNoCase(tag, _globalParamsTag) == 0) return true;
			else if (StrCmpNoCase(tag, _visualizersTag) == 0) return true;
			return false;
		case (2): // process visualizer tag or global params
			
			if (StrCmpNoCase(tag, _visualizerTag) == 0)
				return true;
			//Replace existing global params:
			if (Params::IsParamsTag(tag)){
				Params* oldP = GetDefaultParams(tag);
				if (oldP) delete oldP;
				SetParams(-1,tag,-1,tempParsedParams);
				return true;
			}
			return false;
		case(3):
			// finished with params instance parsing
			if (Params::IsParamsTag(tag)){
				return true;
			}
			return false;

		default: return false;
	}
}
//Construct an XML node from the session parameters
//
ParamNode* ControlExec::
buildNode() {
	//Construct the main node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	ostringstream oss;
	attrs[_VAPORVersionAttr] = Version::GetVersionString();
	
	ParamNode* mainNode = new ParamNode(_sessionTag, attrs, 2);
	attrs.clear();
	ParamNode* globalPanels = new ParamNode(_globalParamsTag, attrs, 5);
	mainNode->AddChild(globalPanels);
	//Have the global parameters populate this:
	for (int i = 0; i<GetNumParamsClasses(); i++){
		//Everything that is not a RenderParams goes here:
		Params* p = GetDefaultParams(GetTagFromType(i+1));
		if (p->isRenderParams()) continue;
		ParamNode* pNode = p->buildNode();
		if (pNode) globalPanels->AddChild(pNode);
	}
	//Create a child for all visualizers
	attrs.clear();
	ParamNode* allVisNode = new ParamNode(_visualizersTag,attrs,GetNumVisualizers() );
	mainNode->AddChild(allVisNode);
	std::map<int,Visualizer*>::iterator it;
	for (it = visualizers.begin(); it != visualizers.end(); it++){
		int viz = it->first;
		//Create a child for each visualizer:
		attrs.clear();
		oss.str(empty);
		oss << " " << viz;
		attrs[_vizNumAttr] = oss.str();
		ParamNode* vizNode = new ParamNode(_visualizerTag, attrs, 15);
		allVisNode->AddChild(vizNode);
		//In each visualizer node create a child paramnode for each local params instance
		for (int j = 0; j<GetNumParamsClasses(); j++){
			string tag = GetTagFromType(j+1);
			for (int inst = 0; inst< GetNumParamsInstances(viz,tag); inst++){
				Params* p = GetParams(viz,tag,inst);
				if (p->isBasicParams()) continue;
				ParamNode* pNode = p->buildNode();
				if (pNode) vizNode->AddChild(pNode);
			}
		}
	}
	return mainNode;
}
int ControlExec::GetActiveVizIndex(){
	return VizWinParams::GetCurrentVizWin();
}
int ControlExec::SetActiveVizIndex(int index){ 
	return VizWinParams::SetCurrentVizWin(index);
}
int ControlExec::RemoveVisualizer(int viz){
	std::map<int, Visualizer*>::iterator it;
	it = visualizers.find(viz);
	if (it == visualizers.end()) return -1;
	delete visualizers[viz];
	visualizers.erase(it);
	VizWinParams::RemoveVizWin(viz);
	int num = Params::DeleteVisualizer(viz);
	if (num == 0) return -1;
	return 0;
}

//! Set the ControlExec to a default state:

void ControlExec::SetToDefault(){
	//Delete the DataMgr
	if (dataMgr) delete dataMgr;
	dataMgr = 0;
	//! Remove all visualizers and Params instances
	std::map<int, Visualizer*>::iterator it = visualizers.begin();
	while (it != visualizers.end()){
		//Copy the one that will be removed:
		std::map<int, Visualizer*>::iterator it2 = it;
		++it;
		RemoveVisualizer(it2->first);
	}
	delete DataStatus::getInstance();
	//Set all the default params back to default state:
	for( int ptype = 1; ptype<= ParamsBase::GetNumParamsClasses(); ptype++){
		Params* p = Params::CreateDefaultParams(ptype);
		Params::SetDefaultParams(ptype, p);
	}
	Command::resetCommandQueue();
	
}