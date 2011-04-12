
#include "arrowparams.h"
#include "datastatus.h"


#include <string>

using namespace VetsUtil;
using namespace VAPoR;
const string ArrowParams::_shortName = "Arrow";
const string ArrowParams::_arrowParamsTag = "ArrowParams";
const string ArrowParams::_constantColorTag = "ConstantColor";
const string ArrowParams::_rakeExtentsTag = "RakeExtents";
const string ArrowParams::_rakeGridTag = "GridDimensions";
const string ArrowParams::_lineThicknessTag = "LineThickness";
const string ArrowParams::_vectorScaleTag = "VectorScale";
const string ArrowParams::_terrainMapTag = "TerrainMap";


namespace {
	const string ArrowName = "ArrowParams";
};

ArrowParams::ArrowParams(
	XmlNode *parent, int winnum
) : RenderParams(parent, ArrowParams::_arrowParamsTag, winnum) {

	restart();
}


ArrowParams::~ArrowParams() {
}

//Initialize for new metadata.  Keep old transfer functions
//
// For each new variable in the metadata create a variable child node, and build the
// associated isoControl and transfer function nodes.
bool ArrowParams::
reinit(bool doOverride){

	DataStatus* ds = DataStatus::getInstance();
	
	int totNumVariables = ds->getNumSessionVariables();
	if (totNumVariables <= 0) return false;
	
	
	//Set up the numRefinements. 
	int maxNumRefinements = ds->getNumTransforms();
	int numrefs = GetRefinementLevel();
	if (doOverride) { 
		numrefs = 0;
	} else {  //Try to use existing values
		if (numrefs > maxNumRefinements) numrefs = maxNumRefinements;
		if (numrefs < 0) numrefs = 0;
	}
	SetRefinementLevel(numrefs);
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
	//Set up the variables. If doOverride is true, just make the first 3 variables the first 3 variables in the VDC.
	//Otherwise try to use the current variables 
	//In either case, if they don't exist replace them with 0.
	if (doOverride){
		for (int i = 0; i<3; i++){
			string varname;
			if (i>=totNumVariables) varname = "0";
			else varname = ds->getVariableName3D(i);
			SetFieldVariableName(i,varname);
		}
	} else {
		for (int i = 0; i<3; i++){
			string varname = GetFieldVariableName(i);
			int indx = ds->getActiveVarNum3D(varname);
			if (indx < 0) {
				SetFieldVariableName(i,"0");
			}
		}
	}

	//Check the rake extents.  If doOverride is true, set the extents to the bottom of the data domain. If not, 
	//shrink the extents to fit inside the domain.
	const float* extents = ds->getExtents();
	vector<double>newExtents;
	if (doOverride) {
		for (int i = 0; i<5; i++){
			newExtents.push_back((double) extents[i]);
		}
		newExtents.push_back((double) extents[2]);
		
	} else {
		double newExts[6];
		GetRakeExtents(newExts);
		for (int i = 0; i<3; i++){
			newExts[i] = Max(newExts[i], (double)extents[i]);
			newExts[i+3] = Min(newExts[i+3], (double)extents[i+3]);
			if (newExts[i] > newExts[i+3]) newExts[i+3] = newExts[i];
		}
		for (int i = 0; i<6; i++) newExtents.push_back(newExts[i]);
	}
	SetRakeExtents(newExtents);

	//Make the grid size default to 10x10x1.  If doOverride is false, make sure the grid dims
	//are at least 1, and no bigger than 10**5.
	int gridSize[3] = {10,10,1};
	if (doOverride) {
		SetRakeGrid(gridSize);
	} else {
		const vector<long>& rakeGrid = GetRakeGrid();
		for (int i = 0; i<3; i++){
			gridSize[i] = rakeGrid[i];
			if (gridSize[i] < 1) gridSize[i] = 1;
			if (gridSize[i] > 100000) gridSize[i] = 100000;
		}
		SetRakeGrid(gridSize);
	}
	
	if (doOverride) {
		const float col[3] = {1.f, 0.f, 0.f};
		SetConstantColor(col);
		SetTerrainMapped(false);
		SetVectorScale(1.);
	}
	initializeBypassFlags();
	
	return true;
}
//Set everything to default values
void ArrowParams::restart() {
	
	SetRefinementLevel(0);
	SetCompressionLevel(0);
	SetVisualizerNum(vizNum);
	SetFieldVariableName(0, "xvar");
	SetFieldVariableName(1, "yvar");
	SetFieldVariableName(2, "0");
	
	setEnabled(false);
	const float default_color[3] = {1.0, 0.0, 0.0};
	SetConstantColor(default_color);
	SetVectorScale(1.0);
	SetTerrainMapped(false);
	SetLineThickness(1.);
	
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
	SetRakeExtents(exts);
	gridsize[2] = 1;
	SetRakeGrid(gridsize);
	
}



float ArrowParams::getCameraDistance(ViewpointParams* vpp, RegionParams* , int ){
	//Determine the box that contains the arrows:
	
	double dbexts[6];
	GetRakeExtents(dbexts);
	return RenderParams::getCameraDistance(vpp,dbexts);
}


void ArrowParams::SetConstantColor(const float rgb[3]) {
	vector <double> valvec(3,0);
	for (int i=0; i<3; i++) {
		valvec[i] = rgb[i];
	}
	GetRootNode()->SetElementDouble(_constantColorTag, valvec);
}

const float *ArrowParams::GetConstantColor() {
	vector <double> valvec = GetRootNode()->GetElementDouble(_constantColorTag);
	for (int i=0; i<3; i++) _constcolorbuf[i] = valvec[i];
	return(_constcolorbuf);
}

int ArrowParams::GetCompressionLevel(){
	vector<long> valvec = GetRootNode()->GetElementLong(_CompressionLevelTag);
	return (int)valvec[0];
 }
void ArrowParams::SetCompressionLevel(int level){
	 vector<long> valvec(1,(long)level);
	 GetRootNode()->SetElementLong(_CompressionLevelTag,valvec);
	 setAllBypass(false);
 }

 void ArrowParams::SetVisualizerNum(int viznum){
	vector<long> valvec(1,(long)viznum);
	GetRootNode()->SetElementLong(_VisualizerNumTag,valvec);
	vizNum = viznum;
 }
 int ArrowParams::GetVisualizerNum(){
	vector<long> valvec = GetRootNode()->GetElementLong(_VisualizerNumTag);
	return (int)valvec[0];
 }
void ArrowParams::SetFieldVariableName(int i, const string& varName){
	vector <string> svec;
	GetRootNode()->GetElementStringVec(_VariableNamesTag, svec);
	if(svec.size() <= i) 
		for (int j = svec.size(); j<=i; j++) svec.push_back("0");
	svec[i] = varName;
	GetRootNode()->SetElementString(_VariableNamesTag, svec);
	setAllBypass(false);
}
const string& ArrowParams::GetFieldVariableName(int i){
	static string retval;
	vector <string> svec;
	GetRootNode()->GetElementStringVec(_VariableNamesTag, svec);
	retval=svec[i];
	return retval;
}