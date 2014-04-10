
#include "isolineparams.h"
#include "datastatus.h"
#include "vapor/Version.h"
#include "glutil.h"
#include <string>

using namespace VetsUtil;
using namespace VAPoR;
const string IsolineParams::_shortName = "Isolines";
const string IsolineParams::_isolineParamsTag = "IsolineParams";
const string IsolineParams::_IsoControlTag = "IsoControl";
const string IsolineParams::_panelBackgroundColorTag= "PanelBackgroundColor";
const string IsolineParams::_isolineExtentsTag = "IsolineExtents";
const string IsolineParams::_lineThicknessTag = "LineThickness";
const string IsolineParams::_panelLineThicknessTag = "PanelLineThickness";
const string IsolineParams::_textSizeTag = "TextSize";
const string IsolineParams::_panelTextSizeTag = "PanelTextSize";
const string IsolineParams::_variableDimensionTag = "VariableDimension";
const string IsolineParams::_cursorCoordsTag = "CursorCoords";
const string IsolineParams::_2DBoxTag = "Box2D";
const string IsolineParams::_3DBoxTag = "Box3D";
const string IsolineParams::_editBoundsTag = "EditBounds";
const string IsolineParams::_histoScaleTag = "HistoScale";
const string IsolineParams::_histoBoundsTag = "HistoBounds";

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
	
	int totNumVariables = ds->getNumSessionVariables()+ ds->getNumSessionVariables2D();
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
	
	SetRefinementLevel(0);
	SetCompressionLevel(0);
	SetFidelityLevel(0);
	SetIgnoreFidelity(false);
	SetVisualizerNum(vizNum);
	SetVariableName("isovar");
	SetVariables3D(false);
	//Create the isocontrol node.  Just one here, but it contains the histo bounds and isovalues

	if (!GetRootNode()->HasChild(IsolineParams::_IsoControlTag)){
		IsoControl* iControl = (IsoControl*)IsoControl::CreateDefaultInstance();
		GetRootNode()->AddRegisteredNode(_IsoControlTag,iControl->GetRootNode(),iControl);
	}
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
	GetBox()->SetAngles(zeros);
	
}

float IsolineParams::getCameraDistance(ViewpointParams* vpp, RegionParams* , int ){
	//Determine the box that contains the isolines
	
	double dbexts[6];
	
	return RenderParams::getCameraDistance(vpp,dbexts);
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
void IsolineParams::spaceIsovals(float minval, float maxval){
	vector<double>newIsos;
	const vector<double>& isovals = GetIsovalues();
	if (isovals.size() == 1) newIsos.push_back(0.5*(minval+maxval));
	else {
		double minIso = 1.e30, maxIso = -1.e30;
	
		for (int i = 0; i<isovals.size(); i++){
			if (minIso > isovals[i]) minIso = isovals[i];
			if (maxIso < isovals[i]) maxIso = isovals[i]; 
		}
		for (int i = 0; i<isovals.size(); i++){
			double ival = minval + (maxval-minval)*(isovals[i]-minIso)/(maxIso-minIso);
			newIsos.push_back(ival);
		}
		SetIsovalues(newIsos);
	}
}