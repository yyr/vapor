
#include "isolineparams.h"
#include "datastatus.h"
#include "vapor/Version.h"
#include "glutil.h"
#include <string>

using namespace VetsUtil;
using namespace VAPoR;
const string IsolineParams::_shortName = "Isolines";
const string IsolineParams::_isolineParamsTag = "IsolineParams";

const string IsolineParams::_isolineColorTag= "IsolineColor";
const string IsolineParams::_textColorTag= "TextColor";
const string IsolineParams::_panelLineColorTag= "PanelLineColor";
const string IsolineParams::_panelBackgroundColorTag= "PanelBackgroundColor";
const string IsolineParams::_panelTextColorTag= "PanelTextColor";
const string IsolineParams::_isolineExtentsTag = "IsolineExtents";
const string IsolineParams::_lineThicknessTag = "LineThickness";
const string IsolineParams::_panelLineThicknessTag = "PanelLineThickness";
const string IsolineParams::_textSizeTag = "TextSize";
const string IsolineParams::_panelTextSizeTag = "PanelTextSize";
const string IsolineParams::_variableDimensionTag = "VariableDimension";
const string IsolineParams::_cursorCoordsTag = "CursorCoords";
const string IsolineParams::_isovaluesTag = "Isovalues";
const string IsolineParams::_2DBoxTag = "2DBox";
const string IsolineParams::_3DBoxTag = "3DBox";

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
// For each new variable in the metadata create a variable child node, and build the
// associated isoControl and transfer function nodes.
bool IsolineParams::
reinit(bool doOverride){

	DataStatus* ds = DataStatus::getInstance();
	
	int totNumVariables = ds->getNumSessionVariables()+ ds->getNumSessionVariables2D();
	if (totNumVariables <= 0) return false;
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
	} else {  //Try to use existing values
		
		if (numrefs > maxNumRefinements) numrefs = maxNumRefinements;
		if (numrefs < 0) numrefs = 0;
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
	//Set up the variables. If doOverride is true, just make the variable the first variable in the VDC.
	//Otherwise try to use the current variables 
	
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

	if (doOverride) { //set default colors
		const float white_color[3] = {1.0, 1.0, 1.0};
		const float black_color[3] = {.0, .0, .0};
		SetPanelLineColor(black_color);
		SetPanelTextColor(black_color);
		SetTextColor(white_color);
		SetPanelBackgroundColor(white_color);
		SetIsolineColor(white_color);
	}
	//Set up the isovalues
	if (doOverride || getNumIsovalues()<1){
		vector<double>ivals;
		ivals.push_back(0.);
		SetIsovalues(ivals);
	} else {
		const vector<double>& ivals = GetIsovalues();
		vector<double> newIvals;
		if (ivals[0] >= ivals[ivals.size()-1])
			newIvals.push_back(ivals[0]);
		else {
			
			for (int i = 0; i< ivals.size(); i++){
				if (i == 0 || i == ivals.size()-1) newIvals.push_back(ivals[i]);
				else newIvals.push_back(ivals[0] + ((float)i/(float)(ivals.size()-1))*(ivals[ivals.size()-1]-ivals[0]));
			}
		}
		SetIsovalues(newIvals);
	}
	if (doOverride) SetLineThickness(1.0);
	else if (GetLineThickness() < 1.0 || GetLineThickness() > 100.) SetLineThickness(1.0);

	initializeBypassFlags();
	return true;
}
//Set everything to default values
void IsolineParams::restart() {
	
	SetRefinementLevel(0);
	SetCompressionLevel(0);
	SetFidelityLevel(0);
	SetIgnoreFidelity(false);
	SetVisualizerNum(vizNum);
	SetVariableName("isovar");
	SetVariables3D(true);
	selectPoint[0]=selectPoint[1]=selectPoint[2]=0.f;
	vector<double>zeros;
	zeros.push_back(0.);
	SetIsovalues(zeros);
	zeros.push_back(0.);
	SetCursorCoords(zeros);
	zeros.push_back(0.);

	
	setEnabled(false);
	const float white_color[3] = {1.0, 1.0, 1.0};
	const float black_color[3] = {.0, .0, .0};
	SetPanelLineColor(black_color);
	SetPanelTextColor(black_color);
	SetTextColor(white_color);
	SetPanelBackgroundColor(white_color);
	SetIsolineColor(white_color);

	SetLineThickness(1.0);
	
	int gridsize[3];
	vector<double> exts;
	exts.push_back(-1.);
	exts.push_back(-1.);
	exts.push_back(-1.);
	
	for (int i = 0; i<3; i++){
		exts.push_back(1.);
		gridsize[i] = 10;
	}
	if (!GetRootNode()->HasChild(Box::_boxTag)){
		Box* myBox = new Box();
		ParamNode* boxNode = myBox->GetRootNode();
		GetRootNode()->AddRegisteredNode(Box::_boxTag,boxNode,myBox);
	}
	
	//Don't set the Box values until after it has been registered:
	SetLocalExtents(exts);
	GetBox()->SetAngles(zeros);
	
	
}



float IsolineParams::getCameraDistance(ViewpointParams* vpp, RegionParams* , int ){
	//Determine the box that contains the isolines
	
	double dbexts[6];
	
	return RenderParams::getCameraDistance(vpp,dbexts);
}


void IsolineParams::SetIsolineColor(const float rgb[3]) {
	vector <double> valvec(3,0);
	for (int i=0; i<3; i++) {
		valvec[i] = rgb[i];
	}
	GetRootNode()->SetElementDouble(_isolineColorTag, valvec);
}
void SetIsolineColor(const float rgb[3]);
void IsolineParams::SetTextColor(const float rgb[3]){
	vector <double> valvec(3,0);
	for (int i=0; i<3; i++) {
		valvec[i] = rgb[i];
	}
	GetRootNode()->SetElementDouble(_textColorTag, valvec);
}
void IsolineParams::SetPanelLineColor(const float rgb[3]){
	vector <double> valvec(3,0);
	for (int i=0; i<3; i++) {
		valvec[i] = rgb[i];
	}
	GetRootNode()->SetElementDouble(_panelLineColorTag, valvec);
}
void IsolineParams::SetPanelBackgroundColor(const float rgb[3]){
	vector <double> valvec(3,0);
	for (int i=0; i<3; i++) {
		valvec[i] = rgb[i];
	}
	GetRootNode()->SetElementDouble(_panelBackgroundColorTag, valvec);
}
void IsolineParams::SetPanelTextColor(const float rgb[3]){
	vector <double> valvec(3,0);
	for (int i=0; i<3; i++) {
		valvec[i] = rgb[i];
	}
	GetRootNode()->SetElementDouble(_panelTextColorTag, valvec);
}

const vector<double>& IsolineParams::GetIsolineColor() {
	const vector<double> white (3,1.);
	const vector<double>& valvec = GetRootNode()->GetElementDouble(_isolineColorTag, white);
	return(valvec);
}
const vector<double>& IsolineParams::GetTextColor() {
	const vector<double> white (3,1.);
	const vector<double>& valvec = GetRootNode()->GetElementDouble(_textColorTag, white);
	return(valvec);
}
const vector<double>& IsolineParams::GetPanelLineColor() {
	const vector<double> white (3,1.);
	const vector<double>& valvec = GetRootNode()->GetElementDouble(_panelLineColorTag, white);
	return(valvec);
}
const vector<double>& IsolineParams::GetPanelBackgroundColor() {
	const vector<double> white (3,1.);
	const vector<double>& valvec = GetRootNode()->GetElementDouble(_panelBackgroundColorTag, white);
	return(valvec);
}
const vector<double>& IsolineParams::GetPanelTextColor() {
	const vector<double> white (3,1.);
	const vector<double>& valvec = GetRootNode()->GetElementDouble(_panelTextColorTag, white);
	return(valvec);
}
const string& IsolineParams::GetVariableName(){
	 return GetRootNode()->GetElementString(_VariableNameTag);
}
void IsolineParams::SetVariableName(const string& varname){
	 GetRootNode()->SetElementString(_VariableNameTag,varname);
}
