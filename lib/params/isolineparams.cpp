
#include "isolineparams.h"
#include "datastatus.h"
#include "vapor/Version.h"
#include "glutil.h"
#include <string>
#include "transferfunction.h"

using namespace VetsUtil;
using namespace VAPoR;
const string IsolineParams::_shortName = "Contours";
const string IsolineParams::_isolineParamsTag = "IsolineParams";
const string IsolineParams::_IsoControlTag = "IsoControl";
const string IsolineParams::_panelBackgroundColorTag= "PanelBackgroundColor";
const string IsolineParams::_isolineExtentsTag = "IsolineExtents";
const string IsolineParams::_lineThicknessTag = "LineThickness";
const string IsolineParams::_panelLineThicknessTag = "PanelLineThickness";
const string IsolineParams::_textSizeTag = "TextSize";
const string IsolineParams::_textDensityTag = "TextDensity";
const string IsolineParams::_variableDimensionTag = "VariableDimension";
const string IsolineParams::_cursorCoordsTag = "CursorCoords";
const string IsolineParams::_2DBoxTag = "Box2D";
const string IsolineParams::_3DBoxTag = "Box3D";
const string IsolineParams::_editBoundsTag = "EditBounds";
const string IsolineParams::_histoScaleTag = "HistoScale";
const string IsolineParams::_histoBoundsTag = "HistoBounds";
const string IsolineParams::_numDigitsTag = "NumDigits";
const string IsolineParams::_textEnabledTag = "TextEnabled";
const string IsolineParams::_useSingleColorTag = "UseSingleColor";
const string IsolineParams::_singleColorTag = "SingleColor";

namespace {
	const string IsolineName = "IsolineParams";
};

IsolineParams::IsolineParams(
	XmlNode *parent, int winnum
) : RenderParams(parent, IsolineParams::_isolineParamsTag, winnum) {

	restart();
}


IsolineParams::~IsolineParams() {
}

//Initialize for new metadata.  Keep old transfer functions
//
bool IsolineParams::
reinit(bool doOverride){

	DataStatus* ds = DataStatus::getInstance();
	int numVariables3D = ds->getNumSessionVariables();
	int numVariables2D = ds->getNumSessionVariables2D();
	int totNumVariables = numVariables2D+numVariables3D;
	if (totNumVariables <= 0) return false;
	if (doOverride){//make it 3D if there are any 3D variables
		if (ds->getNumSessionVariables()>0) SetVariables3D(true);
		else SetVariables3D(false);
	}
	bool is3D = VariablesAre3D();
	int numVariables;
	if (is3D) numVariables = ds->getNumSessionVariables();
	else numVariables = ds->getNumSessionVariables2D();
	
	//Set up the numRefinements. 
	int maxNumRefinements = ds->getNumTransforms();
	int numrefs = GetRefinementLevel();
	if (doOverride) { 
		numrefs = 0;
		SetFidelityLevel(0);
		SetIgnoreFidelity(false);
		SetNumDigits(5);
	} else {  //Try to use existing values
		if (numrefs > maxNumRefinements) numrefs = maxNumRefinements;
		if (numrefs < 0) numrefs = 0;
		if (GetNumDigits() < 2) SetNumDigits(2);
		if (GetNumDigits() > 12) SetNumDigits(12);
	}
	SetRefinementLevel(numrefs);
	//Make sure fidelity is valid:
	int fidelity = GetFidelityLevel();
	DataMgr* dataMgr = ds->getDataMgr();
	if (dataMgr && fidelity > maxNumRefinements+dataMgr->GetCRatios().size()-1)
		SetFidelityLevel(maxNumRefinements+dataMgr->GetCRatios().size()-1);
	//Set up the compression level.  Whether or not override is true, make sure
	//That the compression level is valid.  If override is true set it to 0;
	if (doOverride) SetCompressionLevel(0);
	else {
		int numCompressions = 0;
	
		if (ds->getDataMgr()) {
			numCompressions = ds->getDataMgr()->GetCRatios().size();
		}
		
		if (GetCompressionLevel() >= numCompressions){
			SetCompressionLevel(numCompressions-1);
		}
	}
	//Create arrays of pointers to IsoControls
	assert(totNumVariables > 0);
	IsoControl** new2DIsoControls = new IsoControl*[numVariables2D];
	IsoControl** new3DIsoControls = new IsoControl*[numVariables3D];
	//If we are overriding previous values, delete the existing isocontrols, replace with new ones.
	//Set the histo bounds to the actual bounds in the data
	if (doOverride){
		for (int i = 0; i<numVariables3D; i++){
			
			//will need to set the iso value:
			float dataMin = ds->getDefaultDataMin3D(i);
			float dataMax = ds->getDefaultDataMax3D(i);
			if (dataMin == dataMax){
				dataMin -= 0.5f; dataMax += 0.5;
			}
			
			new3DIsoControls[i] = new IsoControl(this, 8);
			new3DIsoControls[i]->setVarNum(i);
			new3DIsoControls[i]->setMinHistoValue(dataMin);
			new3DIsoControls[i]->setMaxHistoValue(dataMax);
			vector<double> isovals(1,0.5*(dataMin+dataMax)); 
			new3DIsoControls[i]->setIsoValues(isovals);
		}
		for (int i = 0; i<numVariables2D; i++){
			
			//will need to set the iso value:
			float dataMin = ds->getDefaultDataMin2D(i);
			float dataMax = ds->getDefaultDataMax2D(i);
			if (dataMin == dataMax){
				dataMin -= 0.5f; dataMax += 0.5;
			}
			
			new2DIsoControls[i] = new IsoControl(this, 8);
			new2DIsoControls[i]->setVarNum(i);
			new2DIsoControls[i]->setMinHistoValue(dataMin);
			new2DIsoControls[i]->setMaxHistoValue(dataMax);
			vector<double> isovals(1,0.5*(dataMin+dataMax)); 
			new2DIsoControls[i]->setIsoValues(isovals);
		}
	} else {
		//attempt to make use of existing isocontrols.
		//delete any that are no longer referenced
		
		for (int i = 0; i<numVariables3D; i++){
			
			if(i<GetNumVariables3D()){ //make copy of existing ones, don't set their root nodes yet
				float dataMin = ds->getDefaultDataMin3D(i);
				float dataMax = ds->getDefaultDataMax3D(i);
				string varname = ds->getVariableName3D(i);
				if (GetIsoControl(varname,true)){	
					new3DIsoControls[i] = (IsoControl*)GetIsoControl(varname,true)->deepCopy(0);
				} else {
					new3DIsoControls[i] = new IsoControl(this, 8);
					new3DIsoControls[i]->setVarNum(i);
					new3DIsoControls[i]->setMinHistoValue(dataMin);
					new3DIsoControls[i]->setMaxHistoValue(dataMax);
					vector<double> isovals(1,0.5*(dataMin+dataMax)); 
					new3DIsoControls[i]->setIsoValues(isovals);
				}
				new3DIsoControls[i]->setParams(this);
				
			} else { //create new  isocontrols
				
				new3DIsoControls[i] = new IsoControl(this, 8);
				new3DIsoControls[i]->setMinHistoValue(ds->getDefaultDataMin3D(i));
				new3DIsoControls[i]->setMaxHistoValue(ds->getDefaultDataMax3D(i));
				vector<double> isovals(1,0.5*(ds->getDefaultDataMin3D(i)+ds->getDefaultDataMax3D(i))); 
				new3DIsoControls[i]->setIsoValues(isovals);
				new3DIsoControls[i]->setVarNum(i);
				new3DIsoControls[i]->setParams(this);
				
			}
			
		}
		for (int i = 0; i<numVariables2D; i++){
			
			if(i<GetNumVariables2D()){ //make copy of existing ones, don't set their root nodes yet
				float dataMin = ds->getDefaultDataMin2D(i);
				float dataMax = ds->getDefaultDataMax2D(i);
				string varname = ds->getVariableName2D(i);
				if (GetIsoControl(varname,false)){	
					new2DIsoControls[i] = (IsoControl*)GetIsoControl(varname,false)->deepCopy(0);
				} else {
					new2DIsoControls[i] = new IsoControl(this, 8);
					new2DIsoControls[i]->setVarNum(i);
					new2DIsoControls[i]->setMinHistoValue(dataMin);
					new2DIsoControls[i]->setMaxHistoValue(dataMax);
					vector<double> isovals(1,0.5*(dataMin+dataMax)); 
					new2DIsoControls[i]->setIsoValues(isovals);
				}
				new2DIsoControls[i]->setParams(this);
				
			} else { //create new  isocontrols
				
				new2DIsoControls[i] = new IsoControl(this, 8);
				new2DIsoControls[i]->setMinHistoValue(ds->getDefaultDataMin2D(i));
				new2DIsoControls[i]->setMaxHistoValue(ds->getDefaultDataMax2D(i));
				vector<double> isovals(1,0.5*(ds->getDefaultDataMin2D(i)+ds->getDefaultDataMax2D(i))); 
				new2DIsoControls[i]->setIsoValues(isovals);
				new2DIsoControls[i]->setVarNum(i);
				new2DIsoControls[i]->setParams(this);
				
			}
			
		}
	} //end if(doOverride)

	//Delete all existing variable nodes
	if (GetRootNode()->GetNode(_Variables3DTag)){
		GetRootNode()->GetNode(_Variables3DTag)->DeleteAll();
		assert(GetRootNode()->GetNode(_Variables3DTag)->GetNumChildren() == 0);
	}
	if (GetRootNode()->GetNode(_Variables2DTag)){
		GetRootNode()->GetNode(_Variables2DTag)->DeleteAll();
		assert(GetRootNode()->GetNode(_Variables2DTag)->GetNumChildren() == 0);
	}
	
	//Create new variable nodes, add them to the tree
	ParamNode* varsNode2 = GetRootNode()->GetNode(_Variables2DTag);
	if (!varsNode2 && numVariables2D > 0) { 
		varsNode2 = new ParamNode(_Variables2DTag, numVariables2D);
		GetRootNode()->AddNode(_Variables2DTag,varsNode2);
	}
	ParamNode* varsNode3 = GetRootNode()->GetNode(_Variables3DTag);
	if (!varsNode3 && numVariables3D > 0) { 
		varsNode3 = new ParamNode(_Variables3DTag, numVariables3D);
		GetRootNode()->AddNode(_Variables3DTag,varsNode3);
	}
	for (int i = 0; i<numVariables3D; i++){
		std::string& varname = ds->getVariableName3D(i);
		ParamNode* varNode = new ParamNode(varname, 1);
		varsNode3->AddChild(varNode);
		ParamNode* isoNode = new ParamNode(_IsoControlTag);
		varNode->AddRegisteredNode(_IsoControlTag,isoNode,new3DIsoControls[i]);
	}
	for (int i = 0; i<numVariables2D; i++){
		std::string& varname = ds->getVariableName2D(i);
		ParamNode* varNode = new ParamNode(varname, 1);
		varsNode2->AddChild(varNode);
		ParamNode* isoNode = new ParamNode(_IsoControlTag);
		varNode->AddRegisteredNode(_IsoControlTag,isoNode,new2DIsoControls[i]);
	}
	if (numVariables2D > 0)assert(GetRootNode()->GetNode(_Variables2DTag)->GetNumChildren() == numVariables2D);
	if (numVariables3D > 0)assert(GetRootNode()->GetNode(_Variables3DTag)->GetNumChildren() == numVariables3D);
	
	delete [] new2DIsoControls;
	delete [] new3DIsoControls;
	
	if (doOverride) {
		SetHistoStretch(1.0);
		float col[4] = {1.f, 1.f, 1.f, 1.f};
	}
	

	//Set up the current variable. If doOverride is true, just make the current variable the first variable in the VDC.
	//Otherwise try to use the current variable in the state
	
	if (doOverride){
		string varname;	
		if (is3D) {
			varname = ds->getVariableName3D(0);
		}
		else {
			varname = ds->getVariableName2D(0);
		}
			
		SetVariableName(varname);
	} else {
		
		string varname = GetVariableName();
		int indx = 0;
		if (is3D){
			indx = ds->getActiveVarNum3D(varname);
			if (indx < 0) varname = ds->getVariableName3D(0);
		}
		else {
			indx = ds->getActiveVarNum2D(varname);
			if (indx < 0) varname = ds->getVariableName2D(0);
		}

		if (indx < 0) {
			SetVariableName(varname);
		}
		
	}
	
	const float* extents = ds->getLocalExtents();
	//Must set (or correct) extents for both 2D and 3D boxes
	for (int dim = 2; dim<4; dim++){
		if (dim == 2) SetVariables3D(false);
		else SetVariables3D(true);
		vector<double>newExtents(3,0.);
	
		if (doOverride) {
			for (int i = 0; i<3; i++){
				newExtents.push_back((double)( extents[i+3]-extents[i]));
			}
			newExtents[2] = newExtents[5] = extents[5]*0.5;
		
		} else {
			double newExts[6];
			GetLocalExtents(newExts);
		
			for (int i = 0; i<3; i++){
				if (i != 2){
					newExts[i] = Max(newExts[i], 0.);
					newExts[i+3] = Min(newExts[i+3], (double)(extents[i+3]-extents[i]));
				}
				if (newExts[i] > newExts[i+3]) newExts[i+3] = newExts[i];
			}
			newExtents.clear();
			for (int i = 0; i<6; i++) newExtents.push_back(newExts[i]);
			newExtents[5] = newExtents[2];
		}
		SetLocalExtents(newExtents);
	}
	//Set dimensionality back to what it was before...
	SetVariables3D(is3D);

	if (doOverride) { //set default colors
		const float white_color[3] = {1.0, 1.0, 1.0};
		const float black_color[3] = {.0, .0, .0};
		SetPanelBackgroundColor(black_color);
	}
	initializeBypassFlags();
	
	if (doOverride){ SetLineThickness(1.0); SetPanelLineThickness(1.0);}
	else {
		if (GetLineThickness() < 1.0 || GetLineThickness() > 100.) SetLineThickness(1.0);
		if (GetPanelLineThickness() < 1.0 || GetPanelLineThickness() > 100.) SetPanelLineThickness(1.0);
	}
	initializeBypassFlags();
	return true;
}
//Set everything to default values
void IsolineParams::restart() {
	SetNumDigits(5);
	SetRefinementLevel(0);
	SetCompressionLevel(0);
	SetFidelityLevel(0);
	SetIgnoreFidelity(false);
	SetVisualizerNum(vizNum);
	
	SetVariables3D(true);
	double clr[3] = {1.,1.,1.};
	SetSingleColor(clr);
	SetUseSingleColor(true);
	//Create the isocontrol.  Just one placeholder, but it contains the histo bounds and isovalues and colors
	//The transferFunction and Isocontrol share the map/histo bounds and the edit bounds.
	vector<string>ctlPath;
	ctlPath.push_back(_Variables2DTag);
	ctlPath.push_back("isovar2d");
	ctlPath.push_back(_IsoControlTag);
	IsoControl* iControl = (IsoControl*)IsoControl::CreateDefaultInstance();
	iControl->setMinColorMapValue(0.);
	iControl->setMaxColorMapValue(1.);
	SetIsoControl("isovar2d",iControl,false);
	ctlPath.clear();
	ctlPath.push_back(_Variables3DTag);
	ctlPath.push_back("isovar3d");
	ctlPath.push_back(_IsoControlTag);
	iControl = (IsoControl*)IsoControl::CreateDefaultInstance();
	iControl->setMinColorMapValue(0.);
	iControl->setMaxColorMapValue(1.);
	SetIsoControl("isovar3d",iControl,true);
	SetVariableName("isovar3d");
	selectPoint[0]=selectPoint[1]=selectPoint[2]=0.f;
	vector<double>zeros;
	zeros.push_back(0.);
	SetIsovalues(zeros);
	zeros.push_back(0.);
	SetCursorCoords(zeros);
	float bnds[2] = {0.,1.};
	SetHistoBounds(bnds);
	SetHistoStretch(1.);
	zeros.push_back(0.);

	setMinEditBound(0.);
	setMaxEditBound(1.);
	
	setEnabled(false);
	const float white_color[3] = {1.0, 1.0, 1.0};
	const float black_color[3] = {.0, .0, .0};
	
	SetPanelBackgroundColor(black_color);

	SetLineThickness(1.0);
	
	vector<double> exts;
	exts.push_back(-1.);
	exts.push_back(-1.);
	exts.push_back(-1.);
	
	for (int i = 0; i<3; i++){
		exts.push_back(1.);
	}
	//Create box nodes if they don't already exist
	if (!GetRootNode()->HasChild(IsolineParams::_2DBoxTag)){
		Box* myBox2 = new Box();
		ParamNode* boxNode2 = myBox2->GetRootNode();
		ParamNode* child2D = new ParamNode(IsolineParams::_2DBoxTag,1);
		GetRootNode()->AddChild(child2D);
		child2D->AddRegisteredNode(Box::_boxTag,boxNode2,myBox2);
	}
	if (!GetRootNode()->HasChild(IsolineParams::_3DBoxTag)){
		Box* myBox3 = new Box();
		ParamNode* boxNode3 = myBox3->GetRootNode();
		ParamNode* child3D = new ParamNode(IsolineParams::_3DBoxTag,1);
		GetRootNode()->AddChild(child3D);
		child3D->AddRegisteredNode(Box::_boxTag,boxNode3,myBox3);
	}
	
	//Don't set the Box values until after it has been registered:
	SetLocalExtents(exts);
	SetVariables3D(true);
	SetLocalExtents(exts);
	SetTextSize(10.);
	SetTextDensity(0.25);
	SetTextEnabled(false);
	GetBox()->SetAngles(zeros);
	
}

float IsolineParams::getCameraDistance(ViewpointParams* vpp, RegionParams* , int ){
	//Determine the box that contains the isolines
	
	double dbexts[6];
	
	return RenderParams::getCameraDistance(vpp,dbexts);
}
int IsolineParams::GetNumVariables2D() {
	if (!GetRootNode()->HasChild(_Variables2DTag)) return 0;
	return(GetRootNode()->GetNode(_Variables2DTag)->GetNumChildren());
}
int IsolineParams::GetNumVariables3D() {
	if (!GetRootNode()->HasChild(_Variables3DTag)) return 0;
	return(GetRootNode()->GetNode(_Variables3DTag)->GetNumChildren());
}
void IsolineParams::SetPanelBackgroundColor(const float rgb[3]){
	vector <double> valvec(3,0);
	for (int i=0; i<3; i++) {
		valvec[i] = rgb[i];
	}
	GetRootNode()->SetElementDouble(_panelBackgroundColorTag, valvec);
}

const vector<double>& IsolineParams::GetPanelBackgroundColor() {
	const vector<double> black (3,0.);
	const vector<double>& valvec = GetRootNode()->GetElementDouble(_panelBackgroundColorTag, black);
	return(valvec);
}
const string& IsolineParams::GetVariableName(){
	 return GetRootNode()->GetElementString(_VariableNameTag);
}
void IsolineParams::SetVariableName(const string& varname){
	 GetRootNode()->SetElementString(_VariableNameTag,varname);
}
int IsolineParams::getSessionVarNum(){
	DataStatus* ds = DataStatus::getInstance();
	if (VariablesAre3D())
		return ds->getSessionVariableNum3D(GetVariableName());
	else 
		return ds->getSessionVariableNum2D(GetVariableName());
}
void IsolineParams::spaceIsovals(float minval, float interval){
	vector<double>newIsos;
	const vector<double>& isovals = GetIsovalues();
	if (isovals.size() == 1) newIsos.push_back(minval);
	else {
		for (int i = 0; i<isovals.size(); i++){
			double ival = minval + i*interval;
			newIsos.push_back(ival);
		}
		SetIsovalues(newIsos);
	}
}
//Hook up the colors from a transfer function 
//
void IsolineParams::
hookupTF(TransferFunction* tf, int ){
	//We want to keep the existing IsoControl,
	//but replace its color control points.
	IsoControl *isoctl = GetIsoControl();
	VColormap* icmap = isoctl->getColormap();
	VColormap* vcm = tf->getColormap();
	icmap->clear();
	
	//Determine normalized values
	float minval = vcm->minValue();
	float maxval = vcm->maxValue();
	for (int i = 0; i< vcm->numControlPoints(); i++){
		float normval = (vcm->controlPointValue(i) - minval)/(maxval - minval);
		icmap->addNormControlPoint(normval,vcm->controlPointColor(i));
	}
}

MapperFunction* IsolineParams::GetMapperFunc(){
	//Convert it to a transfer function so it can be saved as such.
	//Start with a default TransferFunction, replace its colormap with
	//The one from the 
	TransferFunction* tf = new TransferFunction(this,8);
	
	//Set it opaque
	tf->setOpaque();
	IsoControl *isoctl = GetIsoControl();
	VColormap* icmap = isoctl->getColormap();
	//Remove the colors from the TF
	VColormap* tfcm = tf->getColormap();
	tfcm->clear();
	
	tf->setMinMapValue(isoctl->getMinHistoValue());
	tf->setMaxMapValue(isoctl->getMaxHistoValue());
	//Determine normalized values
	float minval = icmap->minValue();
	float maxval = icmap->maxValue();
	//Add the control points from the IsoControl
	for (int i = 0; i< icmap->numControlPoints(); i++){
		float normval = (icmap->controlPointValue(i) - minval)/(maxval - minval);
		tfcm->addNormControlPoint(normval,icmap->controlPointColor(i));
	}
	return (MapperFunction*) tf;
}
void IsolineParams::getLineColor(int isoNum, float lineColor[3]){
	if (UseSingleColor()){
		const vector<double>& clr = GetSingleColor();
		for (int i = 0; i<3; i++) lineColor[i] = clr[i];
		return;
	}
	float isoval = (float)GetIsovalues()[isoNum];
	VColormap* cmap = GetIsoControl()->getColormap();
	ColorMapBase::Color clr = cmap->color(isoval);
	clr.toRGB(lineColor);
}
int IsolineParams::
SetIsoControl(const string varname, IsoControl* ictl, bool is3D){
	vector<string>path;
	if (is3D)path.push_back(_Variables3DTag);
	else path.push_back(_Variables2DTag);
	path.push_back(varname);
	path.push_back(_IsoControlTag);
	//Make sure all child nodes exist
	ParamNode* varsNode, *varnameNode, *isoControlNode;
	if (!GetRootNode()->HasChild(path[0])){
		varsNode = new ParamNode(path[0]);
		GetRootNode()->AddNode(path[0],varsNode);
	} else varsNode = GetRootNode()->GetNode(path[0]);
	if (!GetRootNode()->HasChild(varname)){
		varnameNode = new ParamNode(varname);
		varsNode->AddNode(varname,varnameNode);
	} else varnameNode = varsNode->GetNode(varname);
	if (varnameNode->HasChild(_IsoControlTag)){
		//Remove, delete it:
		varnameNode->DeleteNode(_IsoControlTag);
	}
	isoControlNode = new ParamNode(_IsoControlTag);
	return varnameNode->AddRegisteredNode(_IsoControlTag, isoControlNode, ictl);
}
	