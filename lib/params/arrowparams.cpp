
#include "arrowparams.h"
#include "datastatus.h"


#include <string>

using namespace VetsUtil;
using namespace VAPoR;
const string ArrowParams::_shortName = "Barbs";
const string ArrowParams::_arrowParamsTag = "ArrowParams";
const string ArrowParams::_constantColorTag = "ConstantColor";
const string ArrowParams::_rakeGridTag = "GridDimensions";
const string ArrowParams::_lineThicknessTag = "LineThickness";
const string ArrowParams::_vectorScaleTag = "VectorScale";
const string ArrowParams::_terrainMapTag = "TerrainMap";
const string ArrowParams::_alignGridTag = "GridAlignedToData";
const string ArrowParams::_alignGridStridesTag = "GridAlignedStrides";
const string ArrowParams::_variableDimensionTag = "VariableDimension";
const string ArrowParams::_heightVariableNameTag = "HeightVariable";

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
void ArrowParams::
Validate(bool doOverride){

	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	int totNumVariables = ds->getNumVariables2DXY()+ds->getNumVariables3D();
	if (totNumVariables <= 0) return;
	bool is3D = VariablesAre3D();
	int numVariables;
	if (is3D) numVariables = ds->getNumVariables3D();
	else numVariables = ds->getNumVariables2DXY();
	
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
	
		if (dataMgr) {
			numCompressions = dataMgr->GetCRatios().size();
		}
		
		if (GetCompressionLevel() >= numCompressions){
			SetCompressionLevel(numCompressions-1);
		}
	}
	//Set up the variables. If doOverride is true, just make the first 3 variables the first 3 variables in the VDC.
	//Otherwise try to use the current variables 
	//In either case, if they don't exist replace them with 0.
	const vector<string>& vars = (is3D ? dataMgr->GetVariables3D() : dataMgr->GetVariables2DXY());
	if (doOverride){

		for (int i = 0; i<3; i++){
			string varname;
			if (i>=numVariables) {
				varname = "0";
			}
			else {
				varname = vars[i];
			}
			
			SetFieldVariableName(i,varname);
		}
	} 
	//Use HGT as the height variable name, if it's there. If not just use the first 2d variable.
	if (doOverride){
		string varname = "HGT";
		int indx = ds->getActiveVarNum2D(varname);
		if (indx < 0 && ds->getNumActiveVariables2D()>0) {
			varname = dataMgr->GetVariables2DXY()[0];
		}
		SetHeightVariableName(varname);
	} else {
		string varname = GetHeightVariableName();
		int indx = ds->getActiveVarNum2D(varname);
		if (indx < 0 && ds->getNumActiveVariables2D()>0) {
			varname = dataMgr->GetVariables2DXY()[0];
			SetHeightVariableName(varname);
		}
	}
	if (ds->getNumActiveVariables2D() <= 0) SetTerrainMapped(false);
	//Set the vector length so that the max arrow is 10% of the larger of the x or y size of scene
	//Check the rake extents.  If doOverride is true, set the extents to the bottom of the data domain. If not, 
	//shrink the extents to fit inside the domain.
	const float* extents = ds->getLocalExtents();
	vector<double>newExtents(3,0.);
	if (doOverride) {
		for (int i = 0; i<3; i++){
			newExtents.push_back((double)( extents[i+3]-extents[i]));
		}
		
	} else {
		double newExts[6];
		GetRakeLocalExtents(newExts);
		
		for (int i = 0; i<3; i++){
			if (i != 2){
				newExts[i] = Max(newExts[i], 0.);
				newExts[i+3] = Min(newExts[i+3], (double)(extents[i+3]-extents[i]));
			}
			if (newExts[i] > newExts[i+3]) newExts[i+3] = newExts[i];
		}
		newExtents.clear();
		for (int i = 0; i<6; i++) newExtents.push_back(newExts[i]);
	}
	SetRakeLocalExtents(newExtents);

	//If grid is mapped to data, 
	//Make the grid size default to 10x10x1.  If doOverride is false, make sure the grid dims
	//are at least 1, and no bigger than 10**5.
	int gridSize[3] = {10,10,1};
	vector<long> strides;
	strides.push_back(10);strides.push_back(10);
	if (doOverride) {
		SetRakeGrid(gridSize);
		AlignGridToData(false);
		SetGridAlignStrides(strides);
	} else {
		//Make sure grid is valid...
		const vector<long>& rakeGrid = GetRakeGrid();
		for (int i = 0; i<3; i++){
			gridSize[i] = rakeGrid[i];
			if (gridSize[i] < 1) gridSize[i] = 1;
			if (gridSize[i] > 100000) gridSize[i] = 100000;
		}
		SetRakeGrid(gridSize);
		if (!IsAlignedToData()){
			//set to default
			SetGridAlignStrides(strides);
		}

	}
	
	if (doOverride) {
		const double col[3] = {1.f, 0.f, 0.f};
		SetConstantColor(col);
		SetTerrainMapped(false);
		SetVectorScale(calcDefaultScale());
	}
	initializeBypassFlags();
	return;
}
//Set everything to default values
void ArrowParams::restart() {
	
	SetRefinementLevel(0);
	SetCompressionLevel(0);
	SetVizNum(0);
	SetFieldVariableName(0, "xvar");
	SetFieldVariableName(1, "yvar");
	SetFieldVariableName(2, "0");
	SetVariables3D(true);
	SetHeightVariableName("HGT");
	
	SetEnabled(false);
	const double default_color[3] = {1.0, 0.0, 0.0};
	SetConstantColor(default_color);
	SetVectorScale(1.0);
	SetTerrainMapped(false);
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
	SetRakeLocalExtents(exts);
	gridsize[2] = 1;
	SetRakeGrid(gridsize);
	vector<long> alignStrides;
	alignStrides.push_back(0);
	alignStrides.push_back(0);
	SetGridAlignStrides(alignStrides);
	AlignGridToData(false);
	
}
//calculate rake extents when aligned to data.
//Also determine the dimensions (size) of the grid-aligned rake
void ArrowParams::calcDataAlignment(double rakeExts[6], int rakeGrid[3],size_t timestep){
	//Find the first data point that fits in the rake extents:
	//Take the rake corner, convert it to voxels
	size_t corner[3],farCorner[3];
	double rakeExtents[6];
	double tempExtents[3];
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if(!dataMgr) {
		return;
	}
	GetRakeLocalExtents(rakeExtents);
	const vector<double>& usrExts = dataMgr->GetExtents(timestep);
	const vector<long> rGrid = GetRakeGrid();
	for (int i = 0; i<3; i++){
		rakeExts[i]=rakeExtents[i]+usrExts[i];
		rakeExts[i+3]=rakeExtents[i+3]+usrExts[i];
		rakeGrid[i] = rGrid[i];
	}
	size_t voxExts[6];
	DataStatus::getInstance()->mapBoxToVox(GetBox(), -1,GetCompressionLevel(),timestep, voxExts);
	for (int i = 0; i<3; i++) {
		corner[i] = voxExts[i];
		farCorner[i] = voxExts[i+3];
	}
	
	//Is the corner actually inside the rake?
	dataMgr->MapVoxToUser(timestep,corner, tempExtents, -1, GetCompressionLevel());
	if (tempExtents[0]<rakeExts[0]) corner[0]++;
	if (tempExtents[1]<rakeExts[1]) corner[1]++;
	//Now get the local (0-based) coords of the corner
	dataMgr->MapVoxToUser(timestep, corner, tempExtents,-1,GetCompressionLevel());
	rakeExts[0] = tempExtents[0];
	rakeExts[1] = tempExtents[1];
	
	//Now find how many voxels are in rake:
	
	vector<long>strides = GetGridAlignStrides();
	if (strides[0] <= 0) rakeGrid[0] = 1; 
	else rakeGrid[0] = 1+(farCorner[0]-corner[0])/strides[0];
	if (strides[1] <= 0) rakeGrid[1] = 1; 
	else rakeGrid[1] = 1+(farCorner[1]-corner[1])/strides[1];

	//Now adjust the rake extents to match the actual far corners:
	if(strides[0]>0)farCorner[0] = corner[0]+(rakeGrid[0]-1)*strides[0];
	else farCorner[0] = corner[0];
	if(strides[1]>0)farCorner[1] = corner[1]+(rakeGrid[1]-1)*strides[1];
	else farCorner[1] = corner[1];

	dataMgr->MapVoxToUser(timestep,farCorner, tempExtents, -1,GetCompressionLevel());
	rakeExts[3]=tempExtents[0];
	rakeExts[4]=tempExtents[1];
}




int ArrowParams::SetConstantColor(const double rgb[3]) {
	vector <double> valvec;
	int rc = 0;
	if (!NO_CHECK){
		for (int i=0; i<3; i++) {
			valvec.push_back(rgb[i]);
			if (valvec[i] < 0.) {valvec[i] = 0.; rc = -1;}
			if (valvec[i] > 1.) {valvec[i] = 1.; rc = -1;}
		}
		if (CHECK && rc) return rc;
	}
	int rc2 = CaptureSetDouble(_constantColorTag,"Change barb color",valvec);
	if(rc) return rc; else return rc2;
}

const double *ArrowParams::GetConstantColor() {
	const vector<double> white (3,1.);
	vector <double> valvec = GetRootNode()->GetElementDouble(_constantColorTag, white);
	for (int i=0; i<3; i++) _constcolorbuf[i] = valvec[i];
	return(_constcolorbuf);
}

int ArrowParams::SetFieldVariableName(int i, const string& varName){
	vector <string> svec;
	vector <string> defaultName(1,"0");
	GetRootNode()->GetElementStringVec(_VariableNamesTag, svec,defaultName);
	if(svec.size() <= i) 
		for (int j = svec.size(); j<=i; j++) svec.push_back("0");
	svec[i] = varName;
	//Capture the change to variable name and to scale
	Command* cmd = CaptureStart("set barb field name");
	int rc = GetRootNode()->SetElementStringVec(_VariableNamesTag,svec);
	if (!rc && svec.size() == 3) rc = GetRootNode()->SetElementDouble(_vectorScaleTag,calcDefaultScale());
	if (rc && cmd) {delete cmd; return rc;}
	if (cmd) CaptureEnd(cmd);
	setAllBypass(false);
	return rc;
}
int ArrowParams::SetHeightVariableName(const string& varName){
	return CaptureSetString(_heightVariableNameTag,"Set barb rake extents",varName);
}
const string& ArrowParams::GetHeightVariableName(){
	return GetRootNode()->GetElementString(_heightVariableNameTag, "HGT");
}
const string& ArrowParams::GetFieldVariableName(int i){
	vector <string> defaultName(3,"0");
	static string retval;
	vector <string> svec;
	GetRootNode()->GetElementStringVec(_VariableNamesTag, svec, defaultName);
	retval=svec[i];
	return retval;
}
double ArrowParams::calcDefaultScale(){
	string varname;
	double maxvarvals[3];
	DataStatus* ds = DataStatus::getInstance();
	const float* stretch = ds->getStretchFactors();
	for (int i = 0; i<3; i++){
		varname = GetFieldVariableName(i);
		if (varname == "0" || !ds->getDataMgr()) maxvarvals[i] = 0.;
		else {
			maxvarvals[i] = Max(abs(ds->getDefaultDataMax(varname)),abs(ds->getDefaultDataMin(varname)));
		}
	}
	for (int i = 0; i<3; i++) maxvarvals[i] *= stretch[i];
	const float* extents = DataStatus::getInstance()->getLocalExtents();
	double maxVecLength = (double)Max(extents[3]-extents[0],extents[4]-extents[1])*0.1;
	double maxVecVal = Max(maxvarvals[0],Max(maxvarvals[1],maxvarvals[2]));
	if (maxVecVal == 0.) return(maxVecLength);
	else return(maxVecLength/maxVecVal);
}
int ArrowParams::SetRakeLocalExtents(const vector<double>&exts){
	int rc = 0;
	vector<double> xexts;
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
		
	if (dataMgr && !NO_CHECK) {
		const vector<double>& fullExts = dataMgr->GetExtents();
		vector<double>sizes;
		for (int i = 0; i<3; i++) sizes.push_back(fullExts[i+3]-fullExts[i]);
		for (int i = 0; i<6; i++){
			xexts.push_back(exts[i]-fullExts[i%3]);
			if (xexts[i] < 0.){ xexts[i] = 0.; rc = -1;}
			if (xexts[i] > sizes[i%3]){ xexts[i] = sizes[i%3]; rc = -1;}
			if (i>=3 && xexts[i-3] >= xexts[i]){ xexts[i-3] = xexts[i]; rc = -1;}
		}
	}
	if (CHECK && rc) return rc;
	int rc2 = GetBox()->CaptureSetDouble(Box::_extentsTag,"Set Barb Rake extents", xexts, this);
	if (rc) return rc; else return rc2;
}

int ArrowParams::SetRakeGrid(const int grid[3]){
	int rc = 0;
	vector<long> griddims;
	if (!NO_CHECK){
		for (int i = 0; i<3; i++){
			griddims.push_back((long)grid[i]);
			if (griddims[i] < 1) {griddims[i] = 1; rc= -1;}
			if (griddims[i] > 10000) {griddims[i] = 10000; rc= -1;}
		}
		if (CHECK && rc) return rc;
	}
	int rc2 = CaptureSetLong(_rakeGridTag,"Set barb grid", griddims);
	if (rc) return rc; else return rc2;
}
int ArrowParams::SetVectorScale(double val){
	int rc = 0;
	if (!NO_CHECK) {
		if (val <= 0. || val > 1.e30){
			val = 0.01; 
			rc = -1;
		}
		if (CHECK && rc) return rc;
	}
	int rc2 = CaptureSetDouble(_vectorScaleTag, "set barb scale", val);
	if(rc) return rc; else return rc2;
}
int ArrowParams::SetGridAlignStrides(const vector<long>& strides){
	int rc = 0;
	vector<long> xstrides;
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (dataMgr && !NO_CHECK){
		size_t dims[3];
		dataMgr->GetDim(dims, -1);
		for (int i = 0; i<2; i++){
			xstrides.push_back(strides[i]);
			if (xstrides[i] <= 0){
				xstrides[i] = 1;
				rc = -1;
			}
			if (xstrides[i] > dims[i]){xstrides[i] = dims[i]; rc = -1;}
		}
		if (CHECK && rc) return rc;
	}
	int rc2 = CaptureSetLong(_alignGridStridesTag, "Set barb grid strides", xstrides);
	if(rc) return rc; else return rc2;
}
